#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxOam.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/dma/dmaNitro.h>
#include <libtwl/rtos/rtosIrq.h>
#include "core/mini-printf.h"
#include "fat/File.h"
#include "Screenshot.h"

#define SCREEN_WIDTH        256
#define SCREEN_HEIGHT       192
#define CAPTURE_BYTES       (SCREEN_WIDTH * SCREEN_HEIGHT * 2)
#define CAPTURE_HALFWORDS   (CAPTURE_BYTES / 2)

// LCDC windows of the two blocks used as capture destinations.
#define VRAM_A_LCDC         ((vu16*)0x06800000)
#define VRAM_C_LCDC         ((vu16*)0x06840000)
#define VRAM_F_LCDC         ((vu16*)0x06890000)
#define VRAM_G_LCDC         ((vu16*)0x06894000)
#define VRAM_H_LCDC         ((vu16*)0x06898000)
// The top capture writes into the upper 96 KB of block A, which leaves the flat
// background texel the material theme keeps at offset 0 alone.
#define VRAM_A_CAPTURE_OFFSET   0x8000

// Extended background palettes are 8 KB per slot. Only slot 0 (the theme's
// background) and slot 3 (the cover) are ever filled, and only the first
// sub palette of each, so 512 bytes apiece is the whole job.
#define EXT_PLTT_SLOT_SIZE  0x2000
#define EXT_PLTT_COPY_BYTES 512
// The two saved blocks are cache line aligned so their invalidate cannot reach
// the members around them. That only holds while the copy is a whole number of
// lines and no bigger than the arrays it fills.
static_assert(EXT_PLTT_COPY_BYTES % 32 == 0, "ext palette copy must be whole cache lines");
#define PLTT_BYTES          512
#define PLTT_HALFWORDS      (PLTT_BYTES / 2)

#define OAM_ENTRIES         128
// Object tile numbers step in units of the boundary set in DISPCNT, which both
// engines run at 128 bytes here, so a copy that lands at a multiple of 128 can
// be pointed at by adding to the tile number.
#define OBJ_TILE_BOUNDARY   128

#define BMP_HEADER_SIZE     54
#define BMP_ROW_BYTES       (SCREEN_WIDTH * 3) // multiple of 4 already, no padding

#define SCREENSHOT_DIR      "/_pico/screenshots"
#define MAX_SCREENSHOTS     1000

// Capture the composited main engine output (BG + 3D + OBJ) at full size. Bit 31
// starts it and the hardware clears it when the frame is done.
#define DISPCAPCNT_BASE     ( 16            /* EVA: all of source A */        \
                            | (0u << 8)     /* EVB: none of source B */       \
                            | (3u << 20)    /* 256x192 */                     \
                            | (0u << 24)    /* source A: BG + 3D + OBJ */     \
                            | (0u << 29)    /* capture source A only */       \
                            | (1u << 31))   /* enable */
#define DISPCAPCNT_TO_C     (DISPCAPCNT_BASE | (2u << 16) | (0u << 18))
#define DISPCAPCNT_TO_A     (DISPCAPCNT_BASE | (0u << 16) | (1u << 18))
#define DISPCAPCNT_ENABLE   (1u << 31)

// Engine B's registers sit at the same offsets as engine A's, 0x1000 higher, so
// one list of offsets mirrors them. Deliberately not a block copy: offset 0x04
// is DISPSTAT and VCOUNT, and 0x60 to 0x68 are the 3D and capture registers, so
// copying the whole window would clear the capture enable bit we just set.
namespace
{
    // The write only registers this path dirties. They read back as zero, so
    // they cannot be saved and are put back by value.
    //
    // Zero is right for these because nobody else restates them each frame:
    // the launcher does not touch the main engine's affine matrices, windows,
    // mosaic or fade at all, and leaving the cover's matrix behind is not
    // harmless - background 3 is enabled on the main engine, and an offset
    // matrix makes it sample the dialog's map and draw bands over the browser.
    const u16 kWriteOnlyRegs[] =
    {
        0x10, 0x12, 0x14,       0x18, 0x1A, 0x1C, 0x1E, // BG0-3 scroll, bar 0x16
        0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E, // BG2 affine
        0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E, // BG3 affine
        0x40, 0x42, 0x44, 0x46,                         // window rectangles
        0x4C,                                           // mosaic
        0x54,                                           // blend fade
    };
    const u32 kWriteOnlyRegsCount = sizeof(kWriteOnlyRegs) / sizeof(kWriteOnlyRegs[0]);

    // Zeroed on the way in and left alone on the way out, because background 1
    // of this engine has an owner that does restate it: DialogPresenter writes
    // its vertical offset every vblank with the bottom sheet's position, parked
    // off screen when no sheet is up. Zeroing it in the restore scrolled the
    // sheet's own map to the top of the screen, and the rows that land there
    // are solid panel fill - so every screenshot painted a full screen slab
    // over the bottom screen until the next vblank took it away.
    //
    // The mirror still needs it zeroed: background 1 of the sub engine is on
    // during the splash, and the mirrored display control brings that bit with
    // it.
    const u16 kMirrorOnlyWriteOnlyRegs[] = { 0x16 }; // BG1's vertical offset
    const u32 kMirrorOnlyWriteOnlyRegsCount =
        sizeof(kMirrorOnlyWriteOnlyRegs) / sizeof(kMirrorOnlyWriteOnlyRegs[0]);

    /// Fade to black at full strength: mode 2 in bits 14-15, factor 16.
    #define REG_OFFSET_MASTER_BRIGHT    0x6C
    #define MASTER_BRIGHT_BLACK         ((2u << 14) | 16u)

    inline vu16& mainReg(u32 offset) { return *(vu16*)(0x04000000 + offset); }
    inline vu16& subReg(u32 offset) { return *(vu16*)(0x04001000 + offset); }
}

namespace
{
    void writeU32(u8* p, u32 value)
    {
        p[0] = (u8)value;
        p[1] = (u8)(value >> 8);
        p[2] = (u8)(value >> 16);
        p[3] = (u8)(value >> 24);
    }

    void writeU16(u8* p, u16 value)
    {
        p[0] = (u8)value;
        p[1] = (u8)(value >> 8);
    }

    void fillBmpHeader(u8* header)
    {
        const u32 imageSize = BMP_ROW_BYTES * SCREEN_HEIGHT;
        memset(header, 0, BMP_HEADER_SIZE);
        header[0] = 'B';
        header[1] = 'M';
        writeU32(header + 2, BMP_HEADER_SIZE + imageSize);
        writeU32(header + 10, BMP_HEADER_SIZE);
        writeU32(header + 14, 40); // BITMAPINFOHEADER
        writeU32(header + 18, SCREEN_WIDTH);
        writeU32(header + 22, SCREEN_HEIGHT);
        writeU16(header + 26, 1); // planes
        writeU16(header + 28, 24); // bits per pixel
        writeU32(header + 34, imageSize);
    }

    /// 5 bit channel to 8 bit, keeping white at 255.
    inline u8 expand5(u32 value)
    {
        return (u8)((value << 3) | (value >> 2));
    }

    /// One row of the capture as BMP pixels. DS colour is 0bbbbbgggggrrrrr and
    /// BMP wants blue, green, red per pixel.
    void convertRow(const u16* src, u8* dst)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            u16 colour = src[x];
            dst[x * 3 + 0] = expand5((colour >> 10) & 0x1F);
            dst[x * 3 + 1] = expand5((colour >> 5) & 0x1F);
            dst[x * 3 + 2] = expand5(colour & 0x1F);
        }
    }

    /// Everything the write needs that is too big for a 4KB worker stack: a FIL
    /// is about a kilobyte on its own.
    struct WriteScratch
    {
        File file;
        FILINFO fileInfo;
        u8 header[BMP_HEADER_SIZE];
        u8 row[BMP_ROW_BYTES];
    };

    /// The number the first half of a pair claimed, so the second half can be
    /// given the same one. Only ever touched from the io thread, which runs one
    /// task at a time in the order they were queued, so the bottom half has
    /// always finished before the top half reads this.
    int sPairIndex = -1;

    /// Lowest number neither screen has claimed, or -1 if the folder is full.
    ///
    /// One number per capture, not per screen. Deciding it per suffix would put
    /// shot007_bot and shot007_top minutes apart while reading as two halves of
    /// one moment, and deciding it twice for one gesture is worse: the bottom
    /// half claims a number, and the top half then rejects that number because
    /// the bottom file now exists, so a pair could never actually share one.
    int findFreeIndex(WriteScratch* scratch, char* pathOut, u32 pathSize)
    {
        for (int i = 0; i < MAX_SCREENSHOTS; i++)
        {
            mini_snprintf(pathOut, pathSize, SCREENSHOT_DIR "/shot%03d_bot.bmp", i);
            if (f_stat(pathOut, &scratch->fileInfo) == FR_OK)
                continue;

            mini_snprintf(pathOut, pathSize, SCREENSHOT_DIR "/shot%03d_top.bmp", i);
            if (f_stat(pathOut, &scratch->fileInfo) == FR_OK)
                continue;

            return i;
        }
        return -1;
    }

    /// Runs on the io thread. Writes row by row on purpose: one big f_write
    /// would hold the FatFs lock long enough for the music thread to starve.
    bool writeBmp(const u16* pixels, bool topScreen, bool secondHalfOfPair)
    {
        // A first half claims a number and a second half inherits it, so a
        // first half that gives up before claiming one must leave nothing to
        // inherit. Without this it left the number of the last gesture that did
        // claim, and files are opened with FA_CREATE_ALWAYS: reusing an old
        // number does not fail, it overwrites a screenshot already on the card.
        // The folder being full reaches this by itself, no fault required.
        if (!secondHalfOfPair)
            sPairIndex = -1;

        auto* scratch = new WriteScratch();
        if (!scratch)
            return false;

        FRESULT mkdirResult = f_mkdir(SCREENSHOT_DIR);
        if (mkdirResult != FR_OK && mkdirResult != FR_EXIST)
        {
            LOG_ERROR("Screenshot: couldn't create " SCREENSHOT_DIR " (%d)\n", mkdirResult);
            delete scratch;
            return false;
        }

        char path[64];
        int index = secondHalfOfPair && sPairIndex >= 0
            ? sPairIndex
            : findFreeIndex(scratch, path, sizeof(path));
        if (index < 0)
        {
            LOG_ERROR("Screenshot: " SCREENSHOT_DIR " is full\n");
            delete scratch;
            return false;
        }
        sPairIndex = index;
        mini_snprintf(path, sizeof(path), SCREENSHOT_DIR "/shot%03d_%s.bmp",
            index, topScreen ? "top" : "bot");

        if (scratch->file.Open(path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        {
            LOG_ERROR("Screenshot: couldn't open %s\n", path);
            delete scratch;
            return false;
        }

        fillBmpHeader(scratch->header);
        u32 written = 0;
        bool ok = scratch->file.Write(scratch->header, BMP_HEADER_SIZE, written) == FR_OK &&
            written == BMP_HEADER_SIZE;

        // BMP rows run bottom to top.
        for (int y = SCREEN_HEIGHT - 1; ok && y >= 0; y--)
        {
            convertRow(pixels + y * SCREEN_WIDTH, scratch->row);
            ok = scratch->file.Write(scratch->row, BMP_ROW_BYTES, written) == FR_OK &&
                written == BMP_ROW_BYTES;
        }

        // Close even when a write failed, and let its result count: a failure
        // here means the data never reached the card.
        if (scratch->file.Close() != FR_OK)
            ok = false;

        if (ok)
        {
            LOG_INFO("Screenshot: saved %s\n", path);
        }
        else
        {
            // The header already claims a full frame, so a half written file
            // would open as a broken screenshot and keep its number forever.
            f_unlink(path);
            LOG_ERROR("Screenshot: failed to write %s\n", path);
        }

        delete scratch;
        return ok;
    }

    /// How many of this class's write tasks have been handed to the io queue,
    /// and how many have come back. One writer each - the main thread bumps the
    /// first immediately before enqueueing, the io thread bumps the second as
    /// the last thing its task does - so comparing them needs no lock and no
    /// masked interrupts, only that a 32 bit aligned store is indivisible.
    ///
    /// File scope for the same reason as the result below: a queued task can
    /// outlive the object that queued it.
    volatile u32 sWritesQueued = 0;
    volatile u32 sWritesDone = 0;

    /// Records how a capture ended, for the one caller that polls it. Set from
    /// the io thread when a file is written, and from the main thread on every
    /// path that gives up before there is a file to write - without those, a
    /// failure that happens early is completely silent, which is the opposite of
    /// what publishing a result is for.
    // Written by the io thread and read by the main one. A file scope variable
    // rather than a member on purpose: a queued task that captured the object
    // could outlive it and write into a Screenshot that is already gone. Losing
    // a result to the read and clear race would cost one message, nothing more.
    volatile Screenshot::Result sResult = Screenshot::Result::None;
}

Screenshot::~Screenshot()
{
    // Whatever was borrowed goes back, so a teardown mid capture cannot leave
    // the engines pointed at each other's memory.
    if (_screen == Screen::Top)
    {
        if (_state == State::Capturing)
            RestoreMainEngine();
        else if (_state == State::Arming)
            mem_setVramAMapping(_savedVramAMapping);
    }
    else if (_state == State::Capturing)
    {
        mem_setVramCMapping(_savedVramCMapping);
    }
    delete[] _blockBackup;
    delete[] _pixels;
}

void Screenshot::VBlankBegin()
{
    // The bottom screen is what the main engine already draws, so it is armed
    // before this frame's sprites go up: those are the ones recorded.
    if (_state != State::Arming || _screen != Screen::Bottom)
        return;

    _savedVramCMapping = mem_getVramCMapping();
    mem_setVramCMapping(MEM_VRAM_C_LCDC);
    REG_DISPCAPCNT = DISPCAPCNT_TO_C;
    _captureWaitFrames = 0;
    _state = State::Capturing;
}

bool Screenshot::MirrorSubEngineIfPending()
{
    // The top screen is mirrored onto the main engine instead of captured, and
    // that needs the sub engine's sprites already uploaded, so it goes last in
    // the frame.
    if (_state != State::Arming || _screen != Screen::Top)
        return false;

    MirrorSubEngine();
    return true;
}

void Screenshot::ArmMirroredCapture()
{
    if (_state != State::Arming || _screen != Screen::Top)
        return;

    // Last, so the write only registers the views just wrote are in place and
    // this cannot be undone by anything else in the frame.
    REG_DISPCNT = REG_DISPCNT_SUB & ~(1u << 3);
    REG_DISPCAPCNT = DISPCAPCNT_TO_A;
    _captureWaitFrames = 0;
    _state = State::Capturing;
}

void Screenshot::PrepareTopCapture()
{
    // The sprite tiles have to end up somewhere the main engine can read, and
    // block B is written from the io thread at unpredictable moments, so nothing
    // already in it may be overwritten and put back - that would silently undo
    // an icon upload. The gap between the two allocators is free by
    // construction, and copied objects are pointed at it by adding to their
    // tile numbers.
    u32 spriteBytes = (_subObjVram->GetState() + 31) & ~31u;
    if (spriteBytes > 16 * 1024)
        spriteBytes = 16 * 1024;
    u32 gapStart = (_mainObjVram->GetState() + OBJ_TILE_BOUNDARY - 1) & ~(OBJ_TILE_BOUNDARY - 1u);
    u32 gapEnd = _mainObjDialogVram->GetState();
    if (spriteBytes != 0 && (gapEnd <= gapStart || gapEnd - gapStart < spriteBytes))
    {
        LOG_ERROR("Screenshot: no room in main obj vram for %d sprite bytes\n", spriteBytes);
        sResult = Result::Failed;
        _pendingTop = false;
        _topIsSecondHalf = false;
        _state = State::Idle;
        return;
    }
    _spriteTileOffset = gapStart;

    _blockBackup = new(cache_align) u16[CAPTURE_HALFWORDS];
    _pixels = new(cache_align) u16[CAPTURE_HALFWORDS];
    if (!_blockBackup || !_pixels)
    {
        LOG_ERROR("Screenshot: couldn't allocate %d bytes\n", CAPTURE_BYTES * 2);
        sResult = Result::Failed;
        _pendingTop = false;
        _topIsSecondHalf = false;
        delete[] _blockBackup;
        _blockBackup = nullptr;
        delete[] _pixels;
        _pixels = nullptr;
        _state = State::Idle;
        return;
    }

    // Block A receives the capture, so save the part being written to. The upper
    // 96 KB is used, which leaves the single flat texel the material theme keeps
    // at offset 0 alone.
    _savedVramAMapping = mem_getVramAMapping();
    mem_setVramAMapping(MEM_VRAM_AB_LCDC);
    DC_InvalidateRange(_blockBackup, CAPTURE_BYTES);
    dma_ntrCopy32(3, (const void*)(VRAM_A_LCDC + VRAM_A_CAPTURE_OFFSET / 2),
        _blockBackup, CAPTURE_BYTES);

    // Free space, so no backup and no race: nothing else is reading it.
    if (spriteBytes != 0)
    {
        dma_ntrCopy32(3, (const void*)GFX_OBJ_SUB,
            (void*)((vu8*)GFX_OBJ_MAIN + _spriteTileOffset), spriteBytes);
    }

    _state = State::Arming;
}

void Screenshot::MirrorSubEngine()
{
    _savedDispCnt = REG_DISPCNT;
    _savedMasterBright = mainReg(REG_OFFSET_MASTER_BRIGHT);
    for (u32 i = 0; i < kMirroredRegCount; i++)
        _savedRegs[i] = mainReg(kMirroredRegs[i]);
    for (u32 i = 0; i < PLTT_HALFWORDS; i++)
    {
        _savedBgPltt[i] = GFX_PLTT_BG_MAIN[i];
        _savedObjPltt[i] = GFX_PLTT_OBJ_MAIN[i];
    }
    _savedVramCMapping = mem_getVramCMapping();
    _savedVramFMapping = mem_getVramFMapping();
    _savedVramGMapping = mem_getVramGMapping();
    _savedVramHMapping = mem_getVramHMapping();

    // The backgrounds cost nothing to hand over: block C is the one block either
    // engine's backgrounds can read, and every base in the sub engine's
    // registers means the same byte to the main engine, because only the main
    // engine adds an offset of its own and that offset is zero here.
    mem_setVramCMapping(MEM_VRAM_C_MAIN_BG_00000);

    // F and G shadow the first 32 KB of that same window, so they have to move
    // anyway - and the main engine needs extended palettes, which live in block
    // H with no main engine mapping of their own. Only the first sub palette of
    // slot 0 (the theme background) and slot 3 (the cover) is ever used.
    mem_setVramFMapping(MEM_VRAM_FG_LCDC);
    mem_setVramGMapping(MEM_VRAM_FG_LCDC);
    // A palette slot has to sit at a fixed offset of its block, so these bytes
    // cannot be dodged - and they are the bottom sheet's background tiles, which
    // are uploaded once at startup and never again. Keep them.
    DC_InvalidateRange(_savedFBytes, EXT_PLTT_COPY_BYTES);
    DC_InvalidateRange(_savedGBytes, EXT_PLTT_COPY_BYTES);
    dma_ntrCopy32(3, (const void*)VRAM_F_LCDC, _savedFBytes, EXT_PLTT_COPY_BYTES);
    dma_ntrCopy32(3, (const void*)(VRAM_G_LCDC + EXT_PLTT_SLOT_SIZE / 2),
        _savedGBytes, EXT_PLTT_COPY_BYTES);

    mem_setVramHMapping(MEM_VRAM_H_LCDC);
    dma_ntrCopy32(3, (const void*)VRAM_H_LCDC, (void*)VRAM_F_LCDC, EXT_PLTT_COPY_BYTES);
    dma_ntrCopy32(3, (const void*)(VRAM_H_LCDC + (EXT_PLTT_SLOT_SIZE * 3) / 2),
        (void*)(VRAM_G_LCDC + EXT_PLTT_SLOT_SIZE / 2), EXT_PLTT_COPY_BYTES);
    mem_setVramFMapping(MEM_VRAM_FG_MAIN_BG_EXT_PLTT_SLOT_01);
    mem_setVramGMapping(MEM_VRAM_FG_MAIN_BG_EXT_PLTT_SLOT_23);
    mem_setVramHMapping(_savedVramHMapping);

    // Palette ram is per engine hardware with no mapping at all, so it is
    // copied. Entry 0 of the background palette carries the backdrop colour.
    for (u32 i = 0; i < PLTT_HALFWORDS; i++)
    {
        GFX_PLTT_BG_MAIN[i] = GFX_PLTT_BG_SUB[i];
        GFX_PLTT_OBJ_MAIN[i] = GFX_PLTT_OBJ_SUB[i];
    }

    // The gap was chosen a couple of frames ago and the io thread allocates
    // object vram without warning, so make sure it is still free before the
    // objects are pointed at it. Losing the sprites is better than drawing over
    // an icon that was uploaded in the meantime.
    bool spritesUsable = _spriteTileOffset >= _mainObjVram->GetState() &&
        _spriteTileOffset < _mainObjDialogVram->GetState();

    // The objects were uploaded to the sub engine's oam earlier this frame, so
    // they are copied across with their tile numbers shifted to where the tiles
    // were put. The main engine's own table comes back on its own next frame,
    // when App applies its shadow copy again.
    u16 tileShift = (u16)(_spriteTileOffset / OBJ_TILE_BOUNDARY);
    for (u32 i = 0; i < OAM_ENTRIES; i++)
    {
        const vu16* src = GFX_OAM_SUB + i * 4;
        vu16* dst = GFX_OAM_MAIN + i * 4;
        u16 attr0 = src[0];
        u16 attr2 = src[2];
        // 0x0200 in attr0 is the disable bit for a plain object, so an
        // unusable gap simply hides them all rather than drawing rubbish.
        dst[0] = spritesUsable ? attr0 : (u16)(attr0 | 0x0200);
        dst[1] = src[1];
        dst[2] = (u16)((attr2 & ~0x3FFu) | (((attr2 & 0x3FFu) + tileShift) & 0x3FFu));
        dst[3] = src[3];
    }

    for (u32 i = 0; i < kMirroredRegCount; i++)
        mainReg(kMirroredRegs[i]) = subReg(kMirroredRegs[i]);

    // The write only ones, by value. Start from zero so nothing is left over
    // from a previous capture, then set what the top screen needs: none of its
    // backgrounds scrolls, and the bitmap background a custom theme puts on
    // background 2 is drawn with an identity matrix, which is also what the
    // theme selector uses.
    for (u32 i = 0; i < kWriteOnlyRegsCount; i++)
        mainReg(kWriteOnlyRegs[i]) = 0;
    for (u32 i = 0; i < kMirrorOnlyWriteOnlyRegsCount; i++)
        mainReg(kMirrorOnlyWriteOnlyRegs[i]) = 0;
    REG_BG2PA = 0x100;
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 0x100;
    REG_BG2X = 0;
    REG_BG2Y = 0;
    REG_MOSAIC = 0;
    REG_BLDY = 0;
    // Background 3 and window 0 are the cover, whose numbers only the view that
    // draws it knows, so it is asked to write them itself - see App::VBlank.

    // The bottom screen belongs to this engine, and for the two frames the
    // capture holds it there is nothing honest to show on it: the registers,
    // the palettes and the oam all describe the other screen. That is what the
    // player sees as a flash, and in the icon grid as cells that lose their
    // fill and keep their outline - the grid's sprites drawn with the top
    // screen's palette.
    //
    // So turn it off instead of showing borrowed state. Master brightness is
    // the last thing in the display pipeline, after the capture unit reads the
    // engine, so the recorded image should not care - and if it does, the saved
    // bmp comes out black and says so plainly. Put back as the very last thing
    // the restore does, so the screen never lights up on borrowed state.
    mainReg(REG_OFFSET_MASTER_BRIGHT) = MASTER_BRIGHT_BLACK;

    // The palette manager rewrites object palette rows partway down the frame,
    // which would repaint the colours we just copied, inside the capture.
    rtos_disableIrqMask(RTOS_IRQ_VCOUNT);
    _vcountIrqSuppressed = true;
}

void Screenshot::RestoreMainEngine()
{
    for (u32 i = 0; i < kMirroredRegCount; i++)
        mainReg(kMirroredRegs[i]) = _savedRegs[i];
    for (u32 i = 0; i < kWriteOnlyRegsCount; i++)
        mainReg(kWriteOnlyRegs[i]) = 0;
    REG_DISPCNT = _savedDispCnt;
    for (u32 i = 0; i < PLTT_HALFWORDS; i++)
    {
        GFX_PLTT_BG_MAIN[i] = _savedBgPltt[i];
        GFX_PLTT_OBJ_MAIN[i] = _savedObjPltt[i];
    }
    mem_setVramCMapping(_savedVramCMapping);
    // Put the borrowed background tiles back before the blocks go back to being
    // background memory.
    mem_setVramFMapping(MEM_VRAM_FG_LCDC);
    mem_setVramGMapping(MEM_VRAM_FG_LCDC);
    DC_FlushRange(_savedFBytes, EXT_PLTT_COPY_BYTES);
    DC_FlushRange(_savedGBytes, EXT_PLTT_COPY_BYTES);
    dma_ntrCopy32(3, _savedFBytes, (void*)VRAM_F_LCDC, EXT_PLTT_COPY_BYTES);
    dma_ntrCopy32(3, _savedGBytes, (void*)(VRAM_G_LCDC + EXT_PLTT_SLOT_SIZE / 2),
        EXT_PLTT_COPY_BYTES);
    mem_setVramFMapping(_savedVramFMapping);
    mem_setVramGMapping(_savedVramGMapping);
    mem_setVramHMapping(_savedVramHMapping);
    mem_setVramAMapping(_savedVramAMapping);
    if (_vcountIrqSuppressed)
    {
        rtos_ackIrqMask(RTOS_IRQ_VCOUNT);
        rtos_enableIrqMask(RTOS_IRQ_VCOUNT);
        _vcountIrqSuppressed = false;
    }
    // Last, and on its own. This is what the player is looking at, so it comes
    // off only once every piece of borrowed state is back: while it was part of
    // the mirrored list it went back first, and the bottom screen then drew
    // several lines with the other screen's palettes, with two of its own
    // background blocks still lent out as palette slots, and for a moment with
    // no block mapped at its background memory at all.
    mainReg(REG_OFFSET_MASTER_BRIGHT) = _savedMasterBright;
}

void Screenshot::Update()
{
    if (_state == State::Requested)
    {
        if (_screen == Screen::Top)
        {
            PrepareTopCapture();
            return;
        }

        // Both buffers are taken before anything touches vram: if the heap
        // cannot spare them the capture is dropped and nothing is disturbed.
        _blockBackup = new(cache_align) u16[CAPTURE_HALFWORDS];
        _pixels = new(cache_align) u16[CAPTURE_HALFWORDS];
        if (!_blockBackup || !_pixels)
        {
            LOG_ERROR("Screenshot: couldn't allocate %d bytes\n", CAPTURE_BYTES * 2);
            sResult = Result::Failed;
            _pendingTop = false;
            delete[] _blockBackup;
            _blockBackup = nullptr;
            delete[] _pixels;
            _pixels = nullptr;
            _state = State::Idle;
            return;
        }

        // Block C is still the sub engine's background and the capture is about
        // to overwrite it. Nothing re-uploads that data, so keep a copy. Dma
        // does not go through the data cache, so drop the lines covering the
        // buffer first: otherwise a dirty one is written back over the copy.
        DC_InvalidateRange(_blockBackup, CAPTURE_BYTES);
        dma_ntrCopy32(3, (const void*)GFX_BG_SUB, _blockBackup, CAPTURE_BYTES);
        _state = State::Arming;
        return;
    }

    if (_state != State::Capturing)
        return;

    // The hardware clears the enable bit at the end of the captured frame.
    bool gaveUp = false;
    if (REG_DISPCAPCNT & DISPCAPCNT_ENABLE)
    {
        if (++_captureWaitFrames <= kCaptureWaitFrames)
            return;

        // Waiting forever is the one failure this class cannot come back from:
        // IsBusy() would never drop, so the top screen would stop uploading
        // covers and the vcount irq would stay masked, and the block would stay
        // borrowed - a launcher broken until it is rebooted. Stop the capture
        // unit by hand and put everything back, throwing the frame away.
        LOG_ERROR("Screenshot: capture did not finish, giving up\n");
        sResult = Result::Failed;
        _pendingTop = false;
        REG_DISPCAPCNT = 0;
        gaveUp = true;
    }

    vu16* destination = (_screen == Screen::Top)
        ? VRAM_A_LCDC + VRAM_A_CAPTURE_OFFSET / 2
        : VRAM_C_LCDC;

    if (!gaveUp)
    {
        // Same reason as the backup: the io thread reads these bytes with the cpu.
        DC_InvalidateRange(_pixels, CAPTURE_BYTES);
        dma_ntrCopy32(3, (const void*)destination, _pixels, CAPTURE_BYTES);
    }
    // Put back what was there while the block is still reachable through the
    // LCDC window, then hand the hardware back.
    dma_ntrCopy32(3, _blockBackup, (void*)destination, CAPTURE_BYTES);

    if (_screen == Screen::Top)
        RestoreMainEngine();
    else
        mem_setVramCMapping(_savedVramCMapping);

    delete[] _blockBackup;
    _blockBackup = nullptr;

    if (gaveUp)
    {
        // Nothing was read, so there is nothing worth writing.
        delete[] _pixels;
        _pixels = nullptr;
        // This returns before the line below that clears it, so it has to be
        // cleared here too: left set, the next gesture's first half would be
        // told it is somebody's second half.
        _topIsSecondHalf = false;
        _state = State::Idle;
        return;
    }

    // Only the pointer crosses to the io thread, which owns it from here.
    u16* pixels = _pixels;
    bool topScreen = _screen == Screen::Top;
    _pixels = nullptr;
    _state = State::Idle;

    // Read before the chaining below, which sets the flag for the capture that
    // comes NEXT. Reading it after meant this capture took the flag meant for
    // the other half, and the other half then ran without one - so the bottom
    // asked to reuse a number nobody had claimed yet, and the top went looking
    // for a fresh one.
    bool secondHalf = _topIsSecondHalf;
    _topIsSecondHalf = false;

    if (_pendingTop)
    {
        _pendingTop = false;
        _topIsSecondHalf = true;
        _screen = Screen::Top;
        _state = State::Requested;
    }

    sWritesQueued++;
    _ioTaskQueue->Enqueue([pixels, topScreen, secondHalf] (const vu8& cancelRequested)
    {
        // The delete stays outside the cancel check: this lambda is the only
        // owner of the pixels, so skipping it would leak 96 KB.
        if (!cancelRequested)
        {
            // Sticky on failure until it is taken: one gesture writes two
            // files, and a success arriving after a failure must not tell the
            // player everything is saved when half of it is not.
            bool ok = writeBmp(pixels, topScreen, secondHalf);
            if (!ok || sResult != Screenshot::Result::Failed)
                sResult = ok ? Screenshot::Result::Saved : Screenshot::Result::Failed;
        }
        delete[] pixels;
        // Last, and outside the cancel check: the slot this task holds is given
        // back either way, so the count has to move either way or the shortcut
        // would refuse for the rest of the session.
        sWritesDone++;
        return TaskResult<void>::Completed();
    });
}

bool Screenshot::IsWritePending()
{
    return sWritesQueued != sWritesDone;
}

void Screenshot::ReportBusy()
{
    if (sResult != Result::Failed)
        sResult = Result::Busy;
}

Screenshot::Result Screenshot::TakeResult()
{
    // Held back until the whole gesture has finished writing, which is what
    // makes the sticky failure above mean anything. One hold of the key writes
    // two files, and this is polled every frame while a write takes many, so
    // the first half's outcome was always collected before the second half had
    // one: a pair whose top half failed said "screenshot saved" and then
    // "couldn't save it", in that order, for the same shutter.
    //
    // Busy is not held. It answers a request that was refused *because* a write
    // is in flight, so waiting for that write to end would be waiting to answer
    // the one question whose answer is already known.
    if (sResult != Result::Busy && IsWritePending())
        return Result::None;

    Result result = sResult;
    sResult = Result::None;
    return result;
}

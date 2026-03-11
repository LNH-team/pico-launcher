#include "common.h"
#include <memory>
#include "fat/File.h"
#include "fat/Directory.h"
#include "core/mini-printf.h"
#include "LayoutService.h"

/*
 * * Header (5 bytes):
 * - magic:        u8[4]    "LYOT"
 * - version:      u8       1
 * * * Data Block (139 bytes - Packed Data):
 * - DateTime1:    (10b)    visible(u8), y(s16 LE), x(s16 LE), format(u8), separator(u8), font(u8), R,G,B(u8)
 * - DateTime2:    (10b)    visible(u8), y(s16 LE), x(s16 LE), format(u8), separator(u8), font(u8), R,G,B(u8)
 * - Prefix:       (12b)    visible(u8), y,x(s16 LE), font(u8), dash(u8), R,G,B(u8), gbaMode,ntrMode,twlMode(u8)
 * - Game ID:      (17b)    visible(u8), y,x(s16 LE), font(u8), dash(u8), R,G,B(u8), showLbl(u8), lblFont(u8), lblY,lblX(s16 LE), lblR,lblG,lblB(u8)
 * - Region:       (10b)    visible(u8), y,x(s16 LE), font(u8), dash(u8), R,G,B(u8)
 * - Version:      (9b)     visible(u8), y,x(s16 LE), font(u8), R,G,B(u8)
 * - CRC:          (9b)     visible(u8), y,x(s16 LE), font(u8), R,G,B(u8)
 * - Username:     (9b)     visible(u8), y,x(s16 LE), font(u8), R,G,B(u8)
 * - Box Art:      (5b)     visible(u8), y,x(s16 LE)
 * - Icon:         (5b)     visible(u8), y,x(s16 LE)
 * - ROM Row 1:    (9b)     visible(u8), y,x(s16 LE), font(u8), R,G,B(u8)
 * - ROM Row 2:    (9b)     visible(u8), y,x(s16 LE), font(u8), R,G,B(u8)
 * - ROM Row 3:    (9b)     visible(u8), y,x(s16 LE), font(u8), R,G,B(u8)
 * - File Name:    (11b)    visible(u8), y,x(s16 LE), font(u8), scroll(u8), speed(u8), R,G,B(u8)
 */

#define LAYOUT_HEADER_SIZE  5u   // magic(4) + version(1)
#define LAYOUT_PACKED_DATA_SIZE 139u
#define LAYOUT_MAX_DATA_SIZE LAYOUT_PACKED_DATA_SIZE
#define LAYOUT_FILE_SIZE    (LAYOUT_HEADER_SIZE + LAYOUT_MAX_DATA_SIZE)

static void PackLayoutData(const LayoutData& d, u8* buf)
{
    u32 i = 0;

    auto writeU8 = [&](u8 value) { buf[i++] = value; };
    auto writeS16 = [&](s16 value) {
        buf[i++] = (u8)(value & 0xFF);
        buf[i++] = (u8)((value >> 8) & 0xFF);
    };

    // DateTime1
    writeU8(d.dateTime1.visible);
    writeS16(d.dateTime1.y);
    writeS16(d.dateTime1.x);
    writeU8(d.dateTime1.format);
    writeU8(d.dateTime1.separator);
    writeU8(d.dateTime1.font);
    writeU8(d.dateTime1.colorR);
    writeU8(d.dateTime1.colorG);
    writeU8(d.dateTime1.colorB);

    // DateTime2
    writeU8(d.dateTime2.visible);
    writeS16(d.dateTime2.y);
    writeS16(d.dateTime2.x);
    writeU8(d.dateTime2.format);
    writeU8(d.dateTime2.separator);
    writeU8(d.dateTime2.font);
    writeU8(d.dateTime2.colorR);
    writeU8(d.dateTime2.colorG);
    writeU8(d.dateTime2.colorB);

    // Prefix
    writeU8(d.prefix.visible);
    writeS16(d.prefix.y);
    writeS16(d.prefix.x);
    writeU8(d.prefix.font);
    writeU8(d.prefix.trailingDash);
    writeU8(d.prefix.colorR);
    writeU8(d.prefix.colorG);
    writeU8(d.prefix.colorB);
    writeU8(d.prefix.gbaPrefixMode);
    writeU8(d.prefix.ntrPrefixMode);
    writeU8(d.prefix.twlPrefixMode);

    // Title ID
    writeU8(d.gameId.visible);
    writeS16(d.gameId.y);
    writeS16(d.gameId.x);
    writeU8(d.gameId.font);
    writeU8(d.gameId.trailingDash);
    writeU8(d.gameId.colorR);
    writeU8(d.gameId.colorG);
    writeU8(d.gameId.colorB);
    writeU8(d.gameId.showLabelText);
    writeU8(d.gameId.labelFont);
    writeS16(d.gameId.labelY);
    writeS16(d.gameId.labelX);
    writeU8(d.gameId.labelColorR);
    writeU8(d.gameId.labelColorG);
    writeU8(d.gameId.labelColorB);

    // Region
    writeU8(d.region.visible);
    writeS16(d.region.y);
    writeS16(d.region.x);
    writeU8(d.region.font);
    writeU8(d.region.trailingDash);
    writeU8(d.region.colorR);
    writeU8(d.region.colorG);
    writeU8(d.region.colorB);

    // Version
    writeU8(d.version.visible);
    writeS16(d.version.y);
    writeS16(d.version.x);
    writeU8(d.version.font);
    writeU8(d.version.colorR);
    writeU8(d.version.colorG);
    writeU8(d.version.colorB);

    // CRC
    writeU8(d.crc.visible);
    writeS16(d.crc.y);
    writeS16(d.crc.x);
    writeU8(d.crc.font);
    writeU8(d.crc.colorR);
    writeU8(d.crc.colorG);
    writeU8(d.crc.colorB);

    // Username
    writeU8(d.username.visible);
    writeS16(d.username.y);
    writeS16(d.username.x);
    writeU8(d.username.font);
    writeU8(d.username.colorR);
    writeU8(d.username.colorG);
    writeU8(d.username.colorB);

    // Box Art
    writeU8(d.boxArt.visible);
    writeS16(d.boxArt.y);
    writeS16(d.boxArt.x);

    // Icon
    writeU8(d.icon.visible);
    writeS16(d.icon.y);
    writeS16(d.icon.x);

    // ROM Name Row 1
    writeU8(d.romNameRow1.visible);
    writeS16(d.romNameRow1.y);
    writeS16(d.romNameRow1.x);
    writeU8(d.romNameRow1.font);
    writeU8(d.romNameRow1.colorR);
    writeU8(d.romNameRow1.colorG);
    writeU8(d.romNameRow1.colorB);

    // ROM Name Row 2
    writeU8(d.romNameRow2.visible);
    writeS16(d.romNameRow2.y);
    writeS16(d.romNameRow2.x);
    writeU8(d.romNameRow2.font);
    writeU8(d.romNameRow2.colorR);
    writeU8(d.romNameRow2.colorG);
    writeU8(d.romNameRow2.colorB);

    // ROM Name Row 3
    writeU8(d.romNameRow3.visible);
    writeS16(d.romNameRow3.y);
    writeS16(d.romNameRow3.x);
    writeU8(d.romNameRow3.font);
    writeU8(d.romNameRow3.colorR);
    writeU8(d.romNameRow3.colorG);
    writeU8(d.romNameRow3.colorB);

    // File Name
    writeU8(d.fileName.visible);
    writeS16(d.fileName.y);
    writeS16(d.fileName.x);
    writeU8(d.fileName.font);
    writeU8(d.fileName.scroll);
    writeU8(d.fileName.scrollSpeed);
    writeU8(d.fileName.colorR);
    writeU8(d.fileName.colorG);
    writeU8(d.fileName.colorB);
}

static void UnpackLayoutData(const u8* buf, u32 dataSize, LayoutData& d)
{
    if (dataSize == 0)
        return;

    u32 i = 0;

    auto readU8  = [&](u8& out) -> bool {
        if (i + 1 > dataSize)
            return false;
        out = buf[i++];
        return true;
    };
    auto readS16 = [&]() -> s16 {
        u8 lo = buf[i++];
        u8 hi = buf[i++];
        return (s16)(lo | (hi << 8));
    };

    auto readS16Safe = [&](s16& out) -> bool {
        if (i + 2 > dataSize)
            return false;
        out = readS16();
        return true;
    };

    auto readU8OrReturn = [&](u8& out) {
        if (!readU8(out))
            return false;
        return true;
    };

    auto readS16OrReturn = [&](s16& out) {
        if (!readS16Safe(out))
            return false;
        return true;
    };

    // DateTime1
    if (!readU8OrReturn(d.dateTime1.visible)) return;
    if (!readS16OrReturn(d.dateTime1.y)) return;
    if (!readS16OrReturn(d.dateTime1.x)) return;
    if (!readU8OrReturn(d.dateTime1.format)) return;
    if (!readU8OrReturn(d.dateTime1.separator)) return;
    if (!readU8OrReturn(d.dateTime1.font)) return;
    if (!readU8OrReturn(d.dateTime1.colorR)) return;
    if (!readU8OrReturn(d.dateTime1.colorG)) return;
    if (!readU8OrReturn(d.dateTime1.colorB)) return;

    // DateTime2
    if (!readU8OrReturn(d.dateTime2.visible)) return;
    if (!readS16OrReturn(d.dateTime2.y)) return;
    if (!readS16OrReturn(d.dateTime2.x)) return;
    if (!readU8OrReturn(d.dateTime2.format)) return;
    if (!readU8OrReturn(d.dateTime2.separator)) return;
    if (!readU8OrReturn(d.dateTime2.font)) return;
    if (!readU8OrReturn(d.dateTime2.colorR)) return;
    if (!readU8OrReturn(d.dateTime2.colorG)) return;
    if (!readU8OrReturn(d.dateTime2.colorB)) return;

    // PREFIX
    if (!readU8OrReturn(d.prefix.visible)) return;
    if (!readS16OrReturn(d.prefix.y)) return;
    if (!readS16OrReturn(d.prefix.x)) return;
    if (!readU8OrReturn(d.prefix.font)) return;
    if (!readU8OrReturn(d.prefix.trailingDash)) return;
    if (!readU8OrReturn(d.prefix.colorR)) return;
    if (!readU8OrReturn(d.prefix.colorG)) return;
    if (!readU8OrReturn(d.prefix.colorB)) return;
    if (!readU8OrReturn(d.prefix.gbaPrefixMode)) return;
    if (!readU8OrReturn(d.prefix.ntrPrefixMode)) return;
    if (!readU8OrReturn(d.prefix.twlPrefixMode)) return;

    // TITLE ID
    if (!readU8OrReturn(d.gameId.visible)) return;
    if (!readS16OrReturn(d.gameId.y)) return;
    if (!readS16OrReturn(d.gameId.x)) return;
    if (!readU8OrReturn(d.gameId.font)) return;
    if (!readU8OrReturn(d.gameId.trailingDash)) return;
    if (!readU8OrReturn(d.gameId.colorR)) return;
    if (!readU8OrReturn(d.gameId.colorG)) return;
    if (!readU8OrReturn(d.gameId.colorB)) return;
    if (!readU8OrReturn(d.gameId.showLabelText)) return;
    if (!readU8OrReturn(d.gameId.labelFont)) return;
    if (!readS16OrReturn(d.gameId.labelY)) return;
    if (!readS16OrReturn(d.gameId.labelX)) return;
    if (!readU8OrReturn(d.gameId.labelColorR)) return;
    if (!readU8OrReturn(d.gameId.labelColorG)) return;
    if (!readU8OrReturn(d.gameId.labelColorB)) return;

    // Region
    if (!readU8OrReturn(d.region.visible)) return;
    if (!readS16OrReturn(d.region.y)) return;
    if (!readS16OrReturn(d.region.x)) return;
    if (!readU8OrReturn(d.region.font)) return;
    if (!readU8OrReturn(d.region.trailingDash)) return;
    if (!readU8OrReturn(d.region.colorR)) return;
    if (!readU8OrReturn(d.region.colorG)) return;
    if (!readU8OrReturn(d.region.colorB)) return;

    // Version
    if (!readU8OrReturn(d.version.visible)) return;
    if (!readS16OrReturn(d.version.y)) return;
    if (!readS16OrReturn(d.version.x)) return;
    if (!readU8OrReturn(d.version.font)) return;
    if (!readU8OrReturn(d.version.colorR)) return;
    if (!readU8OrReturn(d.version.colorG)) return;
    if (!readU8OrReturn(d.version.colorB)) return;

    // CRC
    if (!readU8OrReturn(d.crc.visible)) return;
    if (!readS16OrReturn(d.crc.y)) return;
    if (!readS16OrReturn(d.crc.x)) return;
    if (!readU8OrReturn(d.crc.font)) return;
    if (!readU8OrReturn(d.crc.colorR)) return;
    if (!readU8OrReturn(d.crc.colorG)) return;
    if (!readU8OrReturn(d.crc.colorB)) return;

    // Username
    if (!readU8OrReturn(d.username.visible)) return;
    if (!readS16OrReturn(d.username.y)) return;
    if (!readS16OrReturn(d.username.x)) return;
    if (!readU8OrReturn(d.username.font)) return;
    if (!readU8OrReturn(d.username.colorR)) return;
    if (!readU8OrReturn(d.username.colorG)) return;
    if (!readU8OrReturn(d.username.colorB)) return;

    // Box Art
    if (!readU8OrReturn(d.boxArt.visible)) return;
    if (!readS16OrReturn(d.boxArt.y)) return;
    if (!readS16OrReturn(d.boxArt.x)) return;

    // Icon
    if (!readU8OrReturn(d.icon.visible)) return;
    if (!readS16OrReturn(d.icon.y)) return;
    if (!readS16OrReturn(d.icon.x)) return;

    // ROM Name Row 1
    if (!readU8OrReturn(d.romNameRow1.visible)) return;
    if (!readS16OrReturn(d.romNameRow1.y)) return;
    if (!readS16OrReturn(d.romNameRow1.x)) return;
    if (!readU8OrReturn(d.romNameRow1.font)) return;
    if (!readU8OrReturn(d.romNameRow1.colorR)) return;
    if (!readU8OrReturn(d.romNameRow1.colorG)) return;
    if (!readU8OrReturn(d.romNameRow1.colorB)) return;

    // ROM Name Row 2
    if (!readU8OrReturn(d.romNameRow2.visible)) return;
    if (!readS16OrReturn(d.romNameRow2.y)) return;
    if (!readS16OrReturn(d.romNameRow2.x)) return;
    if (!readU8OrReturn(d.romNameRow2.font)) return;
    if (!readU8OrReturn(d.romNameRow2.colorR)) return;
    if (!readU8OrReturn(d.romNameRow2.colorG)) return;
    if (!readU8OrReturn(d.romNameRow2.colorB)) return;

    // ROM Name Row 3
    if (!readU8OrReturn(d.romNameRow3.visible)) return;
    if (!readS16OrReturn(d.romNameRow3.y)) return;
    if (!readS16OrReturn(d.romNameRow3.x)) return;
    if (!readU8OrReturn(d.romNameRow3.font)) return;
    if (!readU8OrReturn(d.romNameRow3.colorR)) return;
    if (!readU8OrReturn(d.romNameRow3.colorG)) return;
    if (!readU8OrReturn(d.romNameRow3.colorB)) return;

    // File Name
    if (!readU8OrReturn(d.fileName.visible)) return;
    if (!readS16OrReturn(d.fileName.y)) return;
    if (!readS16OrReturn(d.fileName.x)) return;
    if (!readU8OrReturn(d.fileName.font)) return;
    if (!readU8OrReturn(d.fileName.scroll)) return;
    if (!readU8OrReturn(d.fileName.scrollSpeed)) return;
    if (!readU8OrReturn(d.fileName.colorR)) return;
    if (!readU8OrReturn(d.fileName.colorG)) return;
    if (!readU8OrReturn(d.fileName.colorB)) return;

    // Sanitize values
    d.dateTime1.format    %= LAYOUT_FORMAT_COUNT;
    d.dateTime1.separator %= LAYOUT_SEP_COUNT;
    d.dateTime1.font      %= LAYOUT_FONT_COUNT;
    d.dateTime2.format    %= LAYOUT_FORMAT_COUNT;
    d.dateTime2.separator %= LAYOUT_SEP_COUNT;
    d.dateTime2.font      %= LAYOUT_FONT_COUNT;
    d.prefix.font %= LAYOUT_FONT_COUNT;
    d.gameId.font %= LAYOUT_FONT_COUNT;
    d.region.font %= LAYOUT_FONT_COUNT;
    d.version.font %= LAYOUT_FONT_COUNT;
    d.username.font %= LAYOUT_FONT_COUNT;
    d.prefix.trailingDash = d.prefix.trailingDash ? 1u : 0u;
    d.gameId.trailingDash = d.gameId.trailingDash ? 1u : 0u;
    d.region.trailingDash = d.region.trailingDash ? 1u : 0u;
    d.prefix.gbaPrefixMode %= LAYOUT_PREFIX_GBA_COUNT;
    d.prefix.ntrPrefixMode %= LAYOUT_PREFIX_NTR_COUNT;
    d.prefix.twlPrefixMode %= LAYOUT_PREFIX_TWL_COUNT;
    d.gameId.showLabelText = d.gameId.showLabelText ? 1u : 0u;
    d.gameId.labelFont %= LAYOUT_FONT_COUNT;
    if (d.gameId.labelX < -30) d.gameId.labelX = -30;
    if (d.gameId.labelX > 260) d.gameId.labelX = 260;
    if (d.gameId.labelY < -20) d.gameId.labelY = -20;
    if (d.gameId.labelY > 200) d.gameId.labelY = 200;

    d.crc.font            %= LAYOUT_FONT_COUNT;
    d.romNameRow1.font    %= LAYOUT_FONT_COUNT;
    d.romNameRow2.font    %= LAYOUT_FONT_COUNT;
    d.romNameRow3.font    %= LAYOUT_FONT_COUNT;
    d.fileName.font       %= LAYOUT_FONT_COUNT;
    if (d.fileName.scrollSpeed < 1) d.fileName.scrollSpeed = 1;
    if (d.fileName.scrollSpeed > 20) d.fileName.scrollSpeed = 20;
}

void LayoutService::EnsureDirectory()
{
    Directory dir;
    if (dir.Open(LAYOUT_DIR_PATH) != FR_OK)
    {
        f_mkdir(LAYOUT_DIR_PATH);
    }
}

void LayoutService::ScanSlots()
{
    _slotCount = 0;
    for (u32 slot = 1; slot <= LAYOUT_MAX_SLOTS; slot++)
    {
        char path[64];
        mini_snprintf(path, sizeof(path), LAYOUT_FILE_PATH_FMT, slot);
        FILINFO fi;
        if (f_stat(path, &fi) == FR_OK && (fi.fattrib & AM_DIR) == 0)
        {
            _slotCount = slot;
        }
    }
    if (_slotCount == 0)
        _slotCount = 1;
}

bool LayoutService::LoadSlot(u32 slot)
{
    char path[64];
    mini_snprintf(path, sizeof(path), LAYOUT_FILE_PATH_FMT, slot);

    const auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    u32 fileSize = file->GetSize();
    if (fileSize < (LAYOUT_HEADER_SIZE))
        return false;

    u32 dataSize = fileSize - LAYOUT_HEADER_SIZE;
    if (dataSize > LAYOUT_MAX_DATA_SIZE)
        dataSize = LAYOUT_MAX_DATA_SIZE;

    u8 buf[LAYOUT_FILE_SIZE] = { 0 };
    u32 bytesRead = 0;
    if (file->Read(buf, LAYOUT_HEADER_SIZE + dataSize, bytesRead) != FR_OK
        || bytesRead < (LAYOUT_HEADER_SIZE))
        return false;

    // Verify magic
    if (buf[0] != 'L' || buf[1] != 'Y' || buf[2] != 'O' || buf[3] != 'T')
        return false;

    if (buf[4] != LAYOUT_FILE_VERSION)
        return false;

    _currentLayout = LayoutData_Default();
    UnpackLayoutData(buf + LAYOUT_HEADER_SIZE, dataSize, _currentLayout);
    return true;
}

bool LayoutService::SaveSlot(u32 slot, const LayoutData& data)
{
    EnsureDirectory();

    char path[64];
    mini_snprintf(path, sizeof(path), LAYOUT_FILE_PATH_FMT, slot);

    u8 buf[LAYOUT_FILE_SIZE] = { 0 };
    buf[0] = 'L'; buf[1] = 'Y'; buf[2] = 'O'; buf[3] = 'T';
    buf[4] = LAYOUT_FILE_VERSION;
    PackLayoutData(data, buf + LAYOUT_HEADER_SIZE);

    const auto file = std::make_unique<File>();
    if (file->Open(path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        return false;
    }

    u32 bytesWritten = 0;
    if (file->Write(buf, LAYOUT_FILE_SIZE, bytesWritten) != FR_OK
        || bytesWritten != LAYOUT_FILE_SIZE)
    {
        return false;
    }

    return true;
}

void LayoutService::Initialize(u32 slotIndex)
{
    EnsureDirectory();
    ScanSlots();

    // create a default layout1.bin
    {
        char path[64];
        mini_snprintf(path, sizeof(path), LAYOUT_FILE_PATH_FMT, 1u);
        FILINFO fi;
        if (f_stat(path, &fi) != FR_OK)
        {
            LayoutData def = LayoutData_Default();
            SaveSlot(1, def);
            _slotCount = 1;
        }
    }

    // Determine which slot to load
    _currentSlot = (slotIndex >= 1 && slotIndex <= _slotCount) ? slotIndex : 1;

    if (!LoadSlot(_currentSlot))
    {
        _currentLayout = LayoutData_Default();
        SaveSlot(_currentSlot, _currentLayout);
    }
}

void LayoutService::SetCurrentSlot(u32 slot)
{
    if (slot < 1) slot = 1;
    if (slot > LAYOUT_MAX_SLOTS) slot = LAYOUT_MAX_SLOTS;
    _currentSlot = slot;

    if (!LoadSlot(_currentSlot))
    {
        _currentLayout = LayoutData_Default();
        SaveSlot(_currentSlot, _currentLayout);
    }

    ScanSlots();
}

bool LayoutService::SaveCurrentSlot()
{
    bool ok = SaveSlot(_currentSlot, _currentLayout);
    if (ok)
    {
        if (_currentSlot > _slotCount)
            _slotCount = _currentSlot;
    }
    return ok;
}

void LayoutService::ResetCurrentSlot()
{
    _currentLayout = LayoutData_Default();
}

void LayoutService::ReloadCurrentSlot()
{
    if (!LoadSlot(_currentSlot))
        _currentLayout = LayoutData_Default();
}

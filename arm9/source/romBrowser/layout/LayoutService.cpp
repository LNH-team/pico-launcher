#include "common.h"
#include <memory>
#include "fat/File.h"
#include "fat/Directory.h"
#include "core/mini-printf.h"
#include "LayoutService.h"

// Binary file layout:
//   magic[4]   = "LYOT"
//   version[1] = 1

#define LAYOUT_HEADER_SIZE  5u   // magic(4) + version(1)
#define LAYOUT_DATA_SIZE    58u
#define LAYOUT_FILE_SIZE    (LAYOUT_HEADER_SIZE + LAYOUT_DATA_SIZE)

static void PackLayoutData(const LayoutData& d, u8* buf)
{
    u32 i = 0;

    auto writeU8  = [&](u8  v) { buf[i++] = v; };
    auto writeS16 = [&](s16 v) { buf[i++] = (u8)(v & 0xFF); buf[i++] = (u8)((v >> 8) & 0xFF); };

    // DateTime1
    writeU8(d.dateTime1.visible);
    writeS16(d.dateTime1.y);
    writeS16(d.dateTime1.x);
    writeU8(d.dateTime1.format);
    writeU8(d.dateTime1.separator);
    writeU8(d.dateTime1.font);

    // DateTime2
    writeU8(d.dateTime2.visible);
    writeS16(d.dateTime2.y);
    writeS16(d.dateTime2.x);
    writeU8(d.dateTime2.format);
    writeU8(d.dateTime2.separator);
    writeU8(d.dateTime2.font);

    // ROM IdCode
    writeU8(d.romIdCode.visible);
    writeS16(d.romIdCode.y);
    writeS16(d.romIdCode.x);
    writeU8(d.romIdCode.font);

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

    // ROM Name Row 2
    writeU8(d.romNameRow2.visible);
    writeS16(d.romNameRow2.y);
    writeS16(d.romNameRow2.x);
    writeU8(d.romNameRow2.font);

    // ROM Name Row 3
    writeU8(d.romNameRow3.visible);
    writeS16(d.romNameRow3.y);
    writeS16(d.romNameRow3.x);
    writeU8(d.romNameRow3.font);

    // File Name
    writeU8(d.fileName.visible);
    writeS16(d.fileName.y);
    writeS16(d.fileName.x);
    writeU8(d.fileName.font);
    writeU8(d.fileName.scroll);
    writeU8(d.fileName.scrollSpeed);
}

static void UnpackLayoutData(const u8* buf, u32 dataSize, LayoutData& d)
{
    u32 i = 0;

    auto readU8  = [&]() -> u8  { return buf[i++]; };
    auto readS16 = [&]() -> s16 {
        u8 lo = buf[i++];
        u8 hi = buf[i++];
        return (s16)(lo | (hi << 8));
    };

    // DateTime1
    d.dateTime1.visible   = readU8();
    d.dateTime1.y         = readS16();
    d.dateTime1.x         = readS16();
    d.dateTime1.format    = readU8();
    d.dateTime1.separator = readU8();
    d.dateTime1.font      = readU8();

    // DateTime2
    d.dateTime2.visible   = readU8();
    d.dateTime2.y         = readS16();
    d.dateTime2.x         = readS16();
    d.dateTime2.format    = readU8();
    d.dateTime2.separator = readU8();
    d.dateTime2.font      = readU8();

    // ROM IdCode
    d.romIdCode.visible = readU8();
    d.romIdCode.y       = readS16();
    d.romIdCode.x       = readS16();
    d.romIdCode.font    = readU8();

    // Box Art
    d.boxArt.visible = readU8();
    d.boxArt.y       = readS16();
    d.boxArt.x       = readS16();

    // Icon
    d.icon.visible = readU8();
    d.icon.y       = readS16();
    d.icon.x       = readS16();

    // ROM Name Row 1
    d.romNameRow1.visible = readU8();
    d.romNameRow1.y       = readS16();
    d.romNameRow1.x       = readS16();
    d.romNameRow1.font    = readU8();

    // ROM Name Row 2
    d.romNameRow2.visible = readU8();
    d.romNameRow2.y       = readS16();
    d.romNameRow2.x       = readS16();
    d.romNameRow2.font    = readU8();

    // ROM Name Row 3
    d.romNameRow3.visible = readU8();
    d.romNameRow3.y       = readS16();
    d.romNameRow3.x       = readS16();
    d.romNameRow3.font    = readU8();

    // File Name
    d.fileName.visible = readU8();
    d.fileName.y       = readS16();
    d.fileName.x       = readS16();
    d.fileName.font    = readU8();
    d.fileName.scroll  = readU8();
    if (dataSize >= 58u)
        d.fileName.scrollSpeed = readU8();
    if (dataSize >= 59u)
        readU8();
    if (dataSize >= 60u)
        readU8();

    d.dateTime1.format    %= LAYOUT_FORMAT_COUNT;
    d.dateTime1.separator %= LAYOUT_SEP_COUNT;
    d.dateTime1.font      %= LAYOUT_FONT_COUNT;
    d.dateTime2.format    %= LAYOUT_FORMAT_COUNT;
    d.dateTime2.separator %= LAYOUT_SEP_COUNT;
    d.dateTime2.font      %= LAYOUT_FONT_COUNT;
    d.romIdCode.font      %= LAYOUT_FONT_COUNT;
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
    if (fileSize < (LAYOUT_HEADER_SIZE + LAYOUT_LEGACY_DATA_SIZE))
        return false;

    u32 dataSize = fileSize - LAYOUT_HEADER_SIZE;
    if (dataSize > LAYOUT_DATA_SIZE)
        dataSize = LAYOUT_DATA_SIZE;

    u8 buf[LAYOUT_FILE_SIZE] = { 0 };
    u32 bytesRead = 0;
    if (file->Read(buf, LAYOUT_HEADER_SIZE + dataSize, bytesRead) != FR_OK
        || bytesRead < (LAYOUT_HEADER_SIZE + LAYOUT_LEGACY_DATA_SIZE))
        return false;

    // Verify magic
    if (buf[0] != 'L' || buf[1] != 'Y' || buf[2] != 'O' || buf[3] != 'T')
        return false;

    // Version check (accept version 1)
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

    u8 buf[LAYOUT_FILE_SIZE];
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

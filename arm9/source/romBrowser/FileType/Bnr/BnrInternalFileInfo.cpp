#include "common.h"
#include <string.h>
#include <memory>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "../Nds/NdsFileIcon.h"
#include "BnrInternalFileInfo.h"

void BnrInternalFileInfo::Init(const char* originalGameCode)
{
    memset(&_banner, 0, sizeof(_banner));
    memset(_gameCode, 0, sizeof(_gameCode));
    if (originalGameCode)
    {
        strncpy(_gameCode, originalGameCode, 4);
    }
}

void BnrInternalFileInfo::Load(File& file)
{
    u32 fileSize = file.GetSize();
    if (fileSize < 0x240) // Must have at least header + icon + palette (576 bytes)
        return;

    u32 toRead = fileSize < 0x840 ? fileSize : 0x840;
    if (!file.ReadExact(&_banner, toRead))
        return;

    // Read Version 2 Chinese Title override (0x100 bytes)
    if (_banner.header.version >= NDS_BANNER_VERSION_2 && fileSize >= 0x940)
    {
        if (!file.ReadExact(((u8*)&_banner) + 0x840, 0x100))
            return;
    }

    // Read Version 3 Korean Title override (0x100 bytes)
    if (_banner.header.version >= NDS_BANNER_VERSION_3 && fileSize >= 0xA40)
    {
        if (!file.ReadExact(((u8*)&_banner) + 0x940, 0x100))
            return;
    }

    // Read Version 103 (DSi) Animation Frames, Palettes, and Sequence (0x1980 bytes)
    if (_banner.header.version >= NDS_BANNER_VERSION_103 && fileSize >= 0x23C0)
    {
        if (!file.ReadExact(((u8*)&_banner) + 0xA40, 0x1980))
            return;
    }

    _hasBanner = true;
    DC_FlushRange(&_banner, sizeof(_banner));
}

BnrInternalFileInfo::BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* originalGameCode)
{
    Init(originalGameCode);

    const auto file = std::make_unique<File>();
    file->Open(bnrFileRef, FA_READ);
    Load(*file);
}

BnrInternalFileInfo::BnrInternalFileInfo(const TCHAR* path, const char* originalGameCode)
{
    Init(originalGameCode);

    const auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ) == FR_OK)
    {
        Load(*file);
    }
}

const char16_t* BnrInternalFileInfo::GetGameTitle() const
{
    if (!_hasBanner)
    {
        return nullptr;
    }

    // 1. Try English first
    const char16_t* title = _banner.title[NDS_BANNER_TITLE_LANGUAGE_ENGLISH];
    if (title && title[0] != 0)
    {
        return title;
    }

    // 2. Fall back to Japanese (index 0)
    title = _banner.title[NDS_BANNER_TITLE_LANGUAGE_JAPANESE];
    if (title && title[0] != 0)
    {
        return title;
    }

    // 3. Fall back to the first non-empty title found
    for (int i = 0; i < 8; ++i)
    {
        title = _banner.title[i];
        if (title && title[0] != 0)
        {
            return title;
        }
    }

    return nullptr;
}

std::unique_ptr<FileIcon> BnrInternalFileInfo::CreateGameIcon() const
{
    return _hasBanner
        ? std::make_unique<NdsFileIcon>(&_banner)
        : nullptr;
}

const char* BnrInternalFileInfo::GetGameCode() const
{
    return _gameCode[0] != 0 ? _gameCode : nullptr;
}

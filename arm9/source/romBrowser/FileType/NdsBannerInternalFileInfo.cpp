#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "Nds/NdsFileIcon.h"
#include "NdsBannerInternalFileInfo.h"

bool NdsBannerInternalFileInfo::ReadBannerChunks(File& file, u32 availableSize)
{
    memset(&_banner, 0, sizeof(_banner));

    if (availableSize < 0x240) // Must have at least header + icon + palette (576 bytes)
    {
        return false;
    }

    u32 toRead = availableSize < 0x840 ? availableSize : 0x840;
    if (!file.ReadExact(&_banner, toRead))
    {
        return false;
    }

    // Read Version 2 Chinese Title override (0x100 bytes)
    if (_banner.header.version >= NDS_BANNER_VERSION_2 && availableSize >= 0x940 &&
        !file.ReadExact(((u8*)&_banner) + 0x840, 0x100))
    {
        return false;
    }

    // Read Version 3 Korean Title override (0x100 bytes)
    if (_banner.header.version >= NDS_BANNER_VERSION_3 && availableSize >= 0xA40 &&
        !file.ReadExact(((u8*)&_banner) + 0x940, 0x100))
    {
        return false;
    }

    // Read Version 103 (DSi) Animation Frames, Palettes, and Sequence (0x1980 bytes)
    if (_banner.header.version >= NDS_BANNER_VERSION_103 && availableSize >= 0x23C0 &&
        !file.ReadExact(((u8*)&_banner) + 0xA40, 0x1980))
    {
        return false;
    }

    DC_FlushRange(&_banner, sizeof(_banner));
    return true;
}

std::unique_ptr<FileIcon> NdsBannerInternalFileInfo::CreateGameIcon() const
{
    return _hasBanner
        ? std::make_unique<NdsFileIcon>(&_banner)
        : nullptr;
}

const char* NdsBannerInternalFileInfo::GetGameCode() const
{
    return _gameCode[0] != 0 ? _gameCode : nullptr;
}

const char16_t* NdsBannerInternalFileInfo::GetGameTitle() const
{
    if (!_hasBanner)
    {
        return nullptr;
    }

    const char16_t* title = _banner.title[NDS_BANNER_TITLE_LANGUAGE_ENGLISH];
    if (title && title[0] != 0)
    {
        return title;
    }

    title = _banner.title[NDS_BANNER_TITLE_LANGUAGE_JAPANESE];
    if (title && title[0] != 0)
    {
        return title;
    }

    for (int i = NDS_BANNER_TITLE_LANGUAGE_FRENCH; i <= NDS_BANNER_TITLE_LANGUAGE_KOREAN; i++)
    {
        title = _banner.title[i];
        if (title && title[0] != 0)
        {
            return title;
        }
    }

    return nullptr;
}

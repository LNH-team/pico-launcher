#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "romBrowser/ICoverRepository.h"
#include "NdsInternalFileInfo.h"

NdsInternalFileInfo::NdsInternalFileInfo(const FastFileRef& fastFileRef)
{
    const auto file = std::make_unique<File>();
    _hasBanner = false;
    _unitCode = 0;
    _romVersion = 0;
    memset(_gameCode, 0, sizeof(_gameCode));

    file->Open(fastFileRef, FA_READ);

    if (file->Seek(0xC) != FR_OK)
        return;

    u32 bytesRead;
    if (file->Read(_gameCode, 4, bytesRead) != FR_OK || bytesRead < 4)
        return;

    if (file->Seek(0x12) != FR_OK)
        return;

    if (file->Read(&_unitCode, 1, bytesRead) != FR_OK || bytesRead < 1)
        return;

    if (file->Seek(0x1E) != FR_OK)
        return;

    if (file->Read(&_romVersion, 1, bytesRead) != FR_OK || bytesRead < 1)
        return;

    if (file->Seek(0x68) != FR_OK)
        return;

    u32 bannerOffset;
    if (file->Read(&bannerOffset, 4, bytesRead) != FR_OK)
        return;

    if (bannerOffset == 0)
        return;

    if (file->Seek(bannerOffset) != FR_OK)
        return;

    if (file->Read(&_banner, 0xA00, bytesRead) != FR_OK)
        return;

    if (_banner.header.version >= NDS_BANNER_VERSION_103)
    {
        if (file->Read(((u8*)&_banner) + 0xA00, 0x19C0, bytesRead) != FR_OK)
            return;
    }
    _hasBanner = true;
    DC_FlushRange(&_banner, sizeof(_banner));
}

const char16_t* NdsInternalFileInfo::GetGameTitle() const
{
    if (!_hasBanner)
        return nullptr;
    return _banner.title[NDS_BANNER_TITLE_LANGUAGE_ENGLISH];
}

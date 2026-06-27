#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "NdsInternalFileInfo.h"

NdsInternalFileInfo::NdsInternalFileInfo(const FastFileRef& fastFileRef)
{
    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);

    u32 bannerOffset;
    if (file->Seek(0xC) != FR_OK ||
        !file->ReadExact(_gameCode, 4) ||
        file->Seek(0x68) != FR_OK ||
        !file->ReadExact(&bannerOffset, 4) ||
        bannerOffset == 0 ||
        bannerOffset >= file->GetSize() ||
        file->Seek(bannerOffset) != FR_OK)
    {
        return;
    }

    _hasBanner = ReadBannerChunks(*file, file->GetSize() - bannerOffset);
}


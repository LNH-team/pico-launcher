#include "common.h"
#include <string.h>
#include <memory>
#include "fat/File.h"
#include "BnrInternalFileInfo.h"

BnrInternalFileInfo::BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* gameCode)
{
    auto file = std::make_unique<File>();
    file->Open(bnrFileRef, FA_READ);

    if (gameCode)
    {
        strncpy(_gameCode, gameCode, 4);
    }

    _hasBanner = ReadBannerChunks(*file, file->GetSize());
}


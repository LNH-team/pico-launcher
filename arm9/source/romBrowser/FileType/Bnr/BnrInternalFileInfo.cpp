#include "common.h"
#include <string.h>
#include <memory>
#include "fat/File.h"
#include "BnrInternalFileInfo.h"

void BnrInternalFileInfo::Load(File& file, const char* originalGameCode)
{
    if (originalGameCode)
    {
        strncpy(_gameCode, originalGameCode, 4);
    }

    _hasBanner = ReadBannerChunks(file, file.GetSize());
}

BnrInternalFileInfo::BnrInternalFileInfo(const FastFileRef& bnrFileRef, const char* originalGameCode)
{
    const auto file = std::make_unique<File>();
    file->Open(bnrFileRef, FA_READ);
    Load(*file, originalGameCode);
}

BnrInternalFileInfo::BnrInternalFileInfo(const TCHAR* path, const char* originalGameCode)
{
    const auto file = std::make_unique<File>();
    if (file->Open(path, FA_READ) == FR_OK)
    {
        Load(*file, originalGameCode);
    }
}


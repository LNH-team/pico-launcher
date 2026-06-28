#include "common.h"
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "GbaInternalFileInfo.h"

GbaInternalFileInfo::GbaInternalFileInfo(const FastFileRef& fastFileRef)
{
    memset(_gameCode, 0, sizeof(_gameCode));

    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);

    if (file->Seek(0xAC) != FR_OK)
        return;

    if (!file->ReadExact(_gameCode, 4))
    {
        memset(_gameCode, 0, sizeof(_gameCode));
    }
}

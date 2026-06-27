#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/Bnr/BnrInternalFileInfo.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "BannerRepository.h"
#include "fat/File.h"

void BannerRepository::Initialize()
{
    InitializeFolders("/_pico/banners/");
}

InternalFileInfo* BannerRepository::GetBannerForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[270];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for folder.bnr inside the folder (path relative to FatFs CWD = current browse dir)
        constexpr char suffix[] = "/folder.bnr";
        u32 len = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(),
            sizeof(nameBuffer) - sizeof(suffix));
        memcpy(nameBuffer + len, suffix, sizeof(suffix));

        FILINFO fi;
        if (f_stat(nameBuffer, &fi) == FR_OK && !(fi.fattrib & AM_DIR))
        {
            auto* bnr = new BnrInternalFileInfo(nameBuffer);
            if (bnr->HasBanner())
                return bnr;
            delete bnr;
        }

        return nullptr;
    }

    const FileInfo* bnrFile = nullptr;

    // Try to get a banner based on the filename in the user folder
    if (_userFolder)
    {
        // 1. Try with the full filename (e.g. game.gba.bnr)
        u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
        memcpy(nameBuffer + length, ".bnr", 5);
        bnrFile = _userFolder->BinarySearch(nameBuffer);

        // 2. Try stripping the ROM extension (e.g. game.bnr)
        if (!bnrFile)
        {
            const char* dot = strrchr(fileInfo.GetFileName(), '.');
            if (dot && dot != fileInfo.GetFileName())
            {
                u32 baseLen = dot - fileInfo.GetFileName();
                if (baseLen < sizeof(nameBuffer) - 5)
                {
                    u32 len = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), baseLen + 1);
                    memcpy(nameBuffer + len, ".bnr", 5);
                    bnrFile = _userFolder->BinarySearch(nameBuffer);
                }
            }
        }
    }

    // Try to get a banner based on an internal game code
    if (!bnrFile && internalFileInfo)
    {
        const auto* bannerFolder = GetSystemFolder(fileType->GetShortName());
        if (bannerFolder)
        {
            const char* gameCode = internalFileInfo->GetGameCode();
            if (gameCode)
            {
                u32 length = StringUtil::Copy(nameBuffer, gameCode, sizeof(nameBuffer) - 5);
                memcpy(nameBuffer + length, ".bnr", 5);
                bnrFile = bannerFolder->BinarySearch(nameBuffer);
            }
        }
    }

    if (bnrFile)
    {
        auto* bnr = new BnrInternalFileInfo(bnrFile->GetFastFileRef(), internalFileInfo ? internalFileInfo->GetGameCode() : nullptr);
        if (bnr->HasBanner())
            return bnr;
        delete bnr;
    }

    return nullptr;
}

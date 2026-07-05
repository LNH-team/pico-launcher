#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/Bnr/BnrInternalFileInfo.h"
#include "SdFolderFactory.h"
#include "fat/File.h"
#include "BannerRepository.h"

void BannerRepository::Initialize()
{
    InitializeFolders("/_pico/banners/");
}

InternalFileInfo* BannerRepository::GetBannerForFile(const FileInfo& fileInfo, const char* gameCode) const
{
    char nameBuffer[270];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for banner.bnr inside the folder (path relative to FatFs CWD = current browse dir).
        // Scan with the already-open directory handle so the match can be turned directly into a
        // FastFileRef, instead of stat'ing then re-opening the same path by name.
        Directory folderDir;
        if (folderDir.Open(fileInfo.GetFileName()) == FR_OK)
        {
            FILINFO folderFileInfo;
            while (folderDir.Read(&folderFileInfo) == FR_OK && folderFileInfo.fname[0] != 0)
            {
                if (!(folderFileInfo.fattrib & AM_DIR) && !strcasecmp(folderFileInfo.fname, "banner.bnr"))
                {
                    auto* bnr = new BnrInternalFileInfo(
                        FastFileRef(folderDir.GetFatFsDirectory(), &folderFileInfo), nullptr);
                    if (bnr->HasBanner())
                    {
                        return bnr;
                    }
                    delete bnr;
                    break;
                }
            }
        }

        return nullptr;
    }

    const FileInfo* bnrFile = nullptr;

    // Try to get a banner based on the filename in the user folder
    if (_userFolder)
    {
        mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bnr", fileInfo.GetFileName());
        bnrFile = _userFolder->BinarySearch(nameBuffer);
    }

    // Try to get a banner based on an internal game code
    if (!bnrFile && gameCode)
    {
        const auto* bannerFolder = GetFileTypeFolder(fileType->GetShortName());
        if (bannerFolder)
        {
            mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bnr", gameCode);
            bnrFile = bannerFolder->BinarySearch(nameBuffer);
        }
    }

    if (bnrFile)
    {
        auto* bnr = new BnrInternalFileInfo(bnrFile->GetFastFileRef(), gameCode);
        if (bnr->HasBanner())
        {
            return bnr;
        }
        delete bnr;
    }

    return nullptr;
}

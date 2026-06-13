#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/Bnr/BnrInternalFileInfo.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "BannerRepository.h"
#include "fat/File.h"

void BannerRepository::Initialize()
{
    NullFileTypeProvider fileTypeProvider;

    // Collect subdirectory names first so the enumeration dir is closed before opening each one.
    // Avoids nested simultaneous DIR objects which causes issues on DSi's SD IPC layer.
    char subfolderNames[MaxSystemBannerFolders][16] = {};
    int subfolderCount = 0;
    {
        Directory bannersDir;
        if (bannersDir.Open("/_pico/banners") == FR_OK)
        {
            auto sdFileInfo = std::make_unique<FILINFO>();
            while (bannersDir.Read(sdFileInfo.get()) == FR_OK && sdFileInfo->fname[0] != 0)
            {
                if (!(sdFileInfo->fattrib & AM_DIR))
                    continue;
                if (!strcmp(sdFileInfo->fname, "user") || !strcmp(sdFileInfo->fname, "sources"))
                    continue;
                if (subfolderCount >= MaxSystemBannerFolders)
                    break;
                StringUtil::Copy(subfolderNames[subfolderCount++], sdFileInfo->fname, 16);
            }
        }
    } // bannersDir closed before opening any subfolder

    for (int i = 0; i < subfolderCount; i++)
    {
        char path[270]; // "/_pico/banners/" (15) + max LFN (255) + null
        u32 len = StringUtil::Copy(path, "/_pico/banners/", sizeof(path));
        StringUtil::Copy(path + len, subfolderNames[i], sizeof(path) - len);

        auto folder = SdFolderFactory(&fileTypeProvider).CreateFromPath(path);
        if (folder)
        {
            folder->SortByNameInPlace();
            auto& entry = _systemBannerFolders[_systemBannerFolderCount++];
            StringUtil::Copy(entry.name, subfolderNames[i], sizeof(entry.name));
            entry.folder = std::move(folder);
        }
    }

    _userBannersFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/banners/user");
    if (_userBannersFolder)
        _userBannersFolder->SortByNameInPlace();
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
            return new BnrInternalFileInfo(nameBuffer);

        return nullptr;
    }

    const FileInfo* bnrFile = nullptr;

    // Try to get a banner based on the filename in the user folder
    if (_userBannersFolder)
    {
        // 1. Try with the full filename (e.g. game.gba.bnr)
        u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
        memcpy(nameBuffer + length, ".bnr", 5);
        bnrFile = _userBannersFolder->BinarySearch(nameBuffer);

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
                    bnrFile = _userBannersFolder->BinarySearch(nameBuffer);
                }
            }
        }
    }

    // Try to get a banner based on an internal game code
    if (!bnrFile && internalFileInfo)
    {
        const auto* bannerFolder = GetBannerFolder(fileType->GetShortName());
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
        return new BnrInternalFileInfo(bnrFile->GetFastFileRef(), internalFileInfo ? internalFileInfo->GetGameCode() : nullptr);

    return nullptr;
}

const SdFolder* BannerRepository::GetBannerFolder(const char* bannerFolderName) const
{
    for (int i = 0; i < _systemBannerFolderCount; i++)
    {
        if (!strcmp(_systemBannerFolders[i].name, bannerFolderName))
            return _systemBannerFolders[i].folder.get();
    }
    return nullptr;
}

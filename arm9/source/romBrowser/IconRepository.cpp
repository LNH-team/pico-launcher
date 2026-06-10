#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileIconData.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "IconRepository.h"

void IconRepository::Initialize()
{
    NullFileTypeProvider fileTypeProvider;

    // Collect subdirectory names first so the enumeration dir is closed before opening each one.
    // Avoids nested simultaneous DIR objects which causes issues on DSi's SD IPC layer.
    char subfolderNames[MaxSystemIconFolders][16] = {};
    int subfolderCount = 0;
    {
        Directory iconsDir;
        if (iconsDir.Open("/_pico/icons") == FR_OK)
        {
            auto sdFileInfo = std::make_unique<FILINFO>();
            while (iconsDir.Read(sdFileInfo.get()) == FR_OK && sdFileInfo->fname[0] != 0)
            {
                if (!(sdFileInfo->fattrib & AM_DIR))
                    continue;
                if (!strcmp(sdFileInfo->fname, "user") || !strcmp(sdFileInfo->fname, "sources"))
                    continue;
                if (subfolderCount >= MaxSystemIconFolders)
                    break;
                StringUtil::Copy(subfolderNames[subfolderCount++], sdFileInfo->fname, 16);
            }
        }
    } // iconsDir closed before opening any subfolder

    for (int i = 0; i < subfolderCount; i++)
    {
        char path[270]; // "/_pico/icons/" (13) + max LFN (255) + null
        u32 len = StringUtil::Copy(path, "/_pico/icons/", sizeof(path));
        StringUtil::Copy(path + len, subfolderNames[i], sizeof(path) - len);

        auto folder = SdFolderFactory(&fileTypeProvider).CreateFromPath(path);
        if (folder)
        {
            folder->SortByNameInPlace();
            auto& entry = _systemIconFolders[_systemIconFolderCount++];
            StringUtil::Copy(entry.name, subfolderNames[i], sizeof(entry.name));
            entry.folder = std::move(folder);
        }
    }

    _userIconsFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/icons/user");
    if (_userIconsFolder)
        _userIconsFolder->SortByNameInPlace();
}

SharedPtr<BmpFileIconData> IconRepository::LoadIconData(
    const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for folder.bmp inside the folder (path relative to FatFs CWD = current browse dir)
        constexpr char suffix[] = "/folder.bmp";
        u32 len = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(),
            sizeof(nameBuffer) - sizeof(suffix));
        memcpy(nameBuffer + len, suffix, sizeof(suffix));

        FILINFO fi;
        if (f_stat(nameBuffer, &fi) == FR_OK)
            return SharedPtr<BmpFileIconData>::MakeShared(nameBuffer);

        return nullptr;
    }

    const FileInfo* iconFile = nullptr;

    // Try to get an icon based on the filename in the user folder
    if (_userIconsFolder)
    {
        u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
        nameBuffer[length + 0] = '.';
        nameBuffer[length + 1] = 'b';
        nameBuffer[length + 2] = 'm';
        nameBuffer[length + 3] = 'p';
        nameBuffer[length + 4] = 0;
        iconFile = _userIconsFolder->BinarySearch(nameBuffer);
    }

    // Try to get an icon based on an internal game code
    if (!iconFile && internalFileInfo)
    {
        const auto* iconFolder = GetIconFolder(fileType->GetShortName());
        if (iconFolder)
        {
            const char* gameCode = internalFileInfo->GetGameCode();
            if (gameCode)
            {
                u32 length = StringUtil::Copy(nameBuffer, gameCode, sizeof(nameBuffer) - 5);
                nameBuffer[length + 0] = '.';
                nameBuffer[length + 1] = 'b';
                nameBuffer[length + 2] = 'm';
                nameBuffer[length + 3] = 'p';
                nameBuffer[length + 4] = 0;
                iconFile = iconFolder->BinarySearch(nameBuffer);
            }
        }
    }

    if (iconFile)
        return SharedPtr<BmpFileIconData>::MakeShared(iconFile->GetFastFileRef());

    return nullptr;
}

const SdFolder* IconRepository::GetIconFolder(const char* shortName) const
{
    for (int i = 0; i < _systemIconFolderCount; i++)
        if (!strcmp(_systemIconFolders[i].name, shortName))
            return _systemIconFolders[i].folder.get();
    return nullptr;
}

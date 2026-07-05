#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileIconData.h"
#include "SdFolderFactory.h"
#include "IconRepository.h"

void IconRepository::Initialize()
{
    InitializeFolders("/_pico/icons/");
}

SharedPtr<BmpFileIconData> IconRepository::GetIconForFile(const FileInfo& fileInfo, const char* gameCode) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for icon.bmp inside the folder (path relative to FatFs CWD = current browse dir).
        // Scan with the already-open directory handle so the match can be turned directly into a
        // FastFileRef, instead of stat'ing then re-opening the same path by name.
        Directory folderDir;
        if (folderDir.Open(fileInfo.GetFileName()) == FR_OK)
        {
            FILINFO folderFileInfo;
            while (folderDir.Read(&folderFileInfo) == FR_OK && folderFileInfo.fname[0] != 0)
            {
                if (!(folderFileInfo.fattrib & AM_DIR) && !strcasecmp(folderFileInfo.fname, "icon.bmp"))
                {
                    return SharedPtr<BmpFileIconData>::MakeShared(
                        FastFileRef(folderDir.GetFatFsDirectory(), &folderFileInfo));
                }
            }
        }

        return nullptr;
    }

    const FileInfo* iconFile = nullptr;

    // Try to get an icon based on the filename in the user folder
    if (_userFolder)
    {
        mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", fileInfo.GetFileName());
        iconFile = _userFolder->BinarySearch(nameBuffer);
    }

    // Try to get an icon based on an internal game code
    if (!iconFile && gameCode)
    {
        const auto* iconFolder = GetFileTypeFolder(fileType->GetShortName());
        if (iconFolder)
        {
            mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", gameCode);
            iconFile = iconFolder->BinarySearch(nameBuffer);
        }
    }

    return iconFile
        ? SharedPtr<BmpFileIconData>::MakeShared(iconFile->GetFastFileRef())
        : nullptr;
}

#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileIconData.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "IconRepository.h"

void IconRepository::Initialize()
{
    InitializeFolders("/_pico/icons/");
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
        if (f_stat(nameBuffer, &fi) == FR_OK && !(fi.fattrib & AM_DIR))
        {
            return SharedPtr<BmpFileIconData>::MakeShared(nameBuffer);
        }

        return nullptr;
    }

    const FileInfo* iconFile = nullptr;

    // Try to get an icon based on the filename in the user folder
    if (_userFolder)
    {
        u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
        memcpy(nameBuffer + length, ".bmp", 5);
        iconFile = _userFolder->BinarySearch(nameBuffer);
    }

    // Try to get an icon based on an internal game code
    if (!iconFile && internalFileInfo)
    {
        const auto* iconFolder = GetFileTypeFolder(fileType->GetShortName());
        if (iconFolder)
        {
            const char* gameCode = internalFileInfo->GetGameCode();
            if (gameCode)
            {
                u32 length = StringUtil::Copy(nameBuffer, gameCode, sizeof(nameBuffer) - 5);
                memcpy(nameBuffer + length, ".bmp", 5);
                iconFile = iconFolder->BinarySearch(nameBuffer);
            }
        }
    }

    return iconFile
        ? SharedPtr<BmpFileIconData>::MakeShared(iconFile->GetFastFileRef())
        : nullptr;
}

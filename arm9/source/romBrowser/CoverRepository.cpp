#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileCover.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "CoverRepository.h"

void CoverRepository::Initialize()
{
    InitializeFolders("/_pico/covers/");
}

FileCover* CoverRepository::GetCoverForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() != FileTypeClassification::Folder)
    {
        const FileInfo* coverFile = nullptr;

        // Try to get a cover based on the filename in the user folder
        if (_userFolder)
        {
            u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
            memcpy(nameBuffer + length, ".bmp", 5);
            coverFile = _userFolder->BinarySearch(nameBuffer);
        }

        // Try to get a cover based on an internal game code
        if (!coverFile && internalFileInfo)
        {
            const auto* coverFolder = GetSystemFolder(fileType->GetShortName());
            if (coverFolder)
            {
                const char* gameCode = internalFileInfo->GetGameCode();
                if (gameCode)
                {
                    u32 length = StringUtil::Copy(nameBuffer, gameCode, sizeof(nameBuffer) - 5);
                    memcpy(nameBuffer + length, ".bmp", 5);
                }

                coverFile = coverFolder->BinarySearch(nameBuffer);
            }
        }

        if (coverFile)
        {
            return new BmpFileCover(coverFile->GetFastFileRef());
        }

        if (!coverFile && internalFileInfo)
        {
            auto cover = internalFileInfo->CreateGameCover();
            if (cover)
            {
                return cover;
            }
        }
    }

    return fileType->CreateFileCover(fileInfo.GetFileName());
}

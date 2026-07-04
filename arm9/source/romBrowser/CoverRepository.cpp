#include "common.h"
#include "core/mini-printf.h"
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
            mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", fileInfo.GetFileName());
            coverFile = _userFolder->BinarySearch(nameBuffer);
        }

        // Try to get a cover based on an internal game code
        if (!coverFile && internalFileInfo)
        {
            const auto* coverFolder = GetFileTypeFolder(fileType->GetShortName());
            if (coverFolder)
            {
                const char* gameCode = internalFileInfo->GetGameCode();
                if (gameCode)
                {
                    mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", gameCode);
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

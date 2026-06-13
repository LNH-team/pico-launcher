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
    NullFileTypeProvider fileTypeProvider;
    _ndsCoversFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/covers/nds");
    if (_ndsCoversFolder)
    {
        _ndsCoversFolder->SortByNameInPlace();
    }
    _gbaCoversFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/covers/gba");
    if (_gbaCoversFolder)
    {
        _gbaCoversFolder->SortByNameInPlace();
    }
    _userCoversFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/covers/user");
    if (_userCoversFolder)
    {
        _userCoversFolder->SortByNameInPlace();
    }
}

FileCover* CoverRepository::GetCoverForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() != FileTypeClassification::Folder)
    {
        const FileInfo* coverFile = nullptr;

        // Try to get a cover based on the filename in the user folder
        if (_userCoversFolder)
        {
            u32 length = StringUtil::Copy(nameBuffer, fileInfo.GetFileName(), sizeof(nameBuffer) - 5);
            memcpy(nameBuffer + length, ".bmp", 5);
            coverFile = _userCoversFolder->BinarySearch(nameBuffer);
        }

        // Try to get a cover based on an internal game code
        if (!coverFile && internalFileInfo)
        {
            const auto* coverFolder = GetCoverFolder(fileType->GetShortName());
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

const SdFolder* CoverRepository::GetCoverFolder(const char* coverFolderName) const
{
    if (!strcmp(coverFolderName, "nds"))
    {
        return _ndsCoversFolder.get();
    }
    else if (!strcmp(coverFolderName, "gba"))
    {
        return _gbaCoversFolder.get();
    }
    else
    {
        return nullptr;
    }
}

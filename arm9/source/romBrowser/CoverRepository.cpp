#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileCover.h"
#include "FileType/RomIconCover.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "CoverRepository.h"

CoverRepository::CoverRepository(IconCoverMode iconCoverMode, u16 iconCoverBackgroundColor)
    : _iconCoverMode(iconCoverMode), _iconCoverBackgroundColor(iconCoverBackgroundColor) { }

void CoverRepository::Initialize()
{
    InitializeFolders("/_pico/covers/");
}

FileCover* CoverRepository::GetCoverForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for cover.bmp inside the folder (path relative to FatFs CWD = current browse dir).
        // Scan with the already-open directory handle so the match can be turned directly into a
        // FastFileRef, instead of stat'ing then re-opening the same path by name.
        auto folderDir = std::make_unique<Directory>();
        if (folderDir->Open(fileInfo.GetFileName()) == FR_OK)
        {
            FILINFO folderFileInfo;
            while (folderDir->Read(&folderFileInfo) == FR_OK && folderFileInfo.fname[0] != 0)
            {
                if (!(folderFileInfo.fattrib & AM_DIR) && !strcasecmp(folderFileInfo.fname, "cover.bmp"))
                {
                    return new BmpFileCover(FastFileRef(folderDir->GetFatFsDirectory(), &folderFileInfo));
                }
            }
        }

        // Fall back to icon-based cover if icon data exists (e.g. icon.bmp override)
        if (_iconCoverMode != IconCoverMode::Disabled && internalFileInfo && internalFileInfo->HasIconData())
        {
            return new RomIconCover(
                internalFileInfo->GetIconGfx(), internalFileInfo->GetIconPalette(),
                _iconCoverBackgroundColor, _iconCoverMode == IconCoverMode::Large);
        }

        return fileType->CreateFileCover(fileInfo.GetFileName());
    }

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
                coverFile = coverFolder->BinarySearch(nameBuffer);
            }
        }
    }

    if (coverFile)
    {
        return new BmpFileCover(coverFile->GetFastFileRef());
    }

    if (internalFileInfo)
    {
        auto cover = internalFileInfo->CreateGameCover();
        if (cover)
        {
            return cover;
        }
    }

    // Fall back to icon-based cover if icon data exists (for files)
    if (_iconCoverMode != IconCoverMode::Disabled && internalFileInfo && internalFileInfo->HasIconData())
    {
        return new RomIconCover(
            internalFileInfo->GetIconGfx(), internalFileInfo->GetIconPalette(),
            _iconCoverBackgroundColor, _iconCoverMode == IconCoverMode::Large);
    }

    return fileType->CreateFileCover(fileInfo.GetFileName());
}

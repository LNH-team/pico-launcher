#include "common.h"
#include <vector>
#include <string.h>
#include <strings.h>
#include "fat/Directory.h"
#include "FileInfo.h"
#include "FileType/Folder/FolderFileType.h"
#include "SdFolderFactory.h"

// Files and folders macOS leaves behind on FAT volumes: AppleDouble sidecars
// ("._<name>"), Finder/Spotlight/FSEvents bookkeeping and zip extraction leftovers.
static bool isMacOsMetadata(const TCHAR* name)
{
    if (name[0] == '.' && name[1] == '_')
        return true;

    static constexpr const char* sMetadataNames[] =
    {
        ".DS_Store",
        ".Spotlight-V100",
        ".fseventsd",
        ".Trashes",
        ".TemporaryItems",
        ".DocumentRevisions-V100",
        ".metadata_never_index",
        "__MACOSX"
    };
    for (const char* metadataName : sMetadataNames)
    {
        if (!strcasecmp(name, metadataName))
            return true;
    }

    return false;
}

std::unique_ptr<SdFolder> SdFolderFactory::CreateFromPath(const char* path) const
{
    Directory directory;
    if (directory.Open(path) != FR_OK)
        return nullptr;

    int count = 0;
    int bufferSize = 8;
    auto fileInfos = (FileInfo**)malloc(sizeof(FileInfo*) * bufferSize);
    auto sdFileInfo = std::make_unique<FILINFO>();
    while (true)
    {
        if (directory.Read(sdFileInfo.get()) != FR_OK)
            return nullptr;

        if (sdFileInfo->fname[0] == 0)
            break;

        if (isMacOsMetadata(sdFileInfo->fname))
            continue;

        if (count >= bufferSize)
        {
            bufferSize *= 2;
            fileInfos = (FileInfo**)realloc(fileInfos, sizeof(FileInfo*) * bufferSize);
        }
        auto fileType = sdFileInfo->fattrib & AM_DIR
            ? &FolderFileType::sInstance
            : _fileTypeProvider->GetFileType(sdFileInfo->fname);
        fileInfos[count++] = new FileInfo(sdFileInfo->fname, fileType,
            FastFileRef(directory.GetFatFsDirectory(), sdFileInfo.get()), sdFileInfo->fattrib);
    }

    return std::make_unique<SdFolder>(fileInfos, count);
}
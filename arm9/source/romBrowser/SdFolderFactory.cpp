#include "common.h"
#include <vector>
#include "fat/Directory.h"
#include "FileInfo.h"
#include "FileType/Folder/FolderFileType.h"
#include "SdFolderFactory.h"

static bool GlobMatch(const char* pat, const char* str)
{
    while (*pat && *str)
    {
        if (*pat == '*')
        {
            while (*pat == '*') pat++; // Skip consecutive wildcards
            if (!*pat) return true; // Trailing * matches everything
            while (*str)
            {
                if (GlobMatch(pat, str++)) return true;
            }
            return false;
        }
        if (*pat != '?' && tolower((unsigned char)*pat) != tolower((unsigned char)*str))
            return false;
        pat++; str++;
    }
    while (*pat == '*') pat++;
    return !*pat && !*str;
}

static bool ShouldHide(const char* name, const char* const* patterns, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (patterns[i] && GlobMatch(patterns[i], name)) return true;
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

        if (ShouldHide(sdFileInfo->fname, _hidePatterns, _hidePatternCount))
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
            FastFileRef(directory.GetFatFsDirectory(), sdFileInfo.get()));
    }

    return std::make_unique<SdFolder>(fileInfos, count);
}
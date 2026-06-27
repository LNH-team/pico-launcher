#include "common.h"
#include <string.h>
#include <cctype>
#include <algorithm>
#include "SdFolder.h"

namespace
{
    bool ContainsCaseInsensitive(const char* text, const char* query)
    {
        if (!query || query[0] == 0)
            return true;

        if (!text)
            return false;

        const char* queryStart = query;
        while (*queryStart != 0)
        {
            while (*queryStart != 0 && std::isspace(static_cast<unsigned char>(*queryStart)))
                queryStart++;

            if (*queryStart == 0)
                break;

            const char* queryEnd = queryStart;
            while (*queryEnd != 0 && !std::isspace(static_cast<unsigned char>(*queryEnd)))
                queryEnd++;

            const int tokenLen = queryEnd - queryStart;
            if (tokenLen <= 0)
            {
                queryStart = queryEnd;
                continue;
            }

            bool tokenMatched = false;
            for (const char* start = text; *start != 0; start++)
            {
                int i = 0;
                while (i < tokenLen && start[i] != 0)
                {
                    unsigned char textChar = static_cast<unsigned char>(start[i]);
                    unsigned char queryChar = static_cast<unsigned char>(queryStart[i]);
                    if (std::tolower(textChar) != std::tolower(queryChar))
                        break;
                    i++;
                }
                if (i == tokenLen)
                {
                    tokenMatched = true;
                    break;
                }
            }

            if (!tokenMatched)
                return false;

            queryStart = queryEnd;
        }

        return true;
    }
}

SdFolder::SdFolder(FileInfo** files, int fileCount)
    : _files(files), _fileCount(fileCount) { }

SdFolder::~SdFolder()
{
    for (int i = 0; i < _fileCount; i++)
        delete _files[i];
    free(_files);
}

std::unique_ptr<const FileInfo*[]> SdFolder::FilterAndSort(
    const SdFolderFilterSortParams& filterSortParams, int& resultCount) const
{
    auto sortedFilteredFiles = std::make_unique<const FileInfo*[]>(_fileCount);
    int filteredCount = 0;
    for (int i = 0; i < _fileCount; i++)
    {
        const FileInfo* file = _files[i];
        bool isHidden = file->GetFileName()[0] == '.' || file->IsHidden();
        auto classification = file->GetFileType()->GetClassification();
        bool nameMatches = ContainsCaseInsensitive(file->GetFileName(), filterSortParams.nameQuery);
        if (classification != FileTypeClassification::Unknown &&
            (!isHidden || filterSortParams.includeHiddenFiles) &&
            nameMatches)
        {
            sortedFilteredFiles[filteredCount++] = file;
        }
    }
    std::sort(sortedFilteredFiles.get(), sortedFilteredFiles.get() + filteredCount,
        [filterSortParams] (const FileInfo*& a, const FileInfo*& b)
        {
            bool result = true;
            if (CompareClassification(a, b, result))
                return result;

            auto sortType = filterSortParams.sortType;
            auto sortDirection = filterSortParams.sortDirection;
            if (a->GetFileType()->GetClassification() == FileTypeClassification::Folder &&
                b->GetFileType()->GetClassification() == FileTypeClassification::Folder)
            {
                if (sortType != SdFolderSortType::Name)
                {
                    sortType = SdFolderSortType::Name;
                    sortDirection = SdFolderSortDirection::Ascending;
                }
            }
            switch (sortType)
            {
                case SdFolderSortType::Name:
                default:
                {
                    result = CompareName(a, b);
                    break;
                }
            }
            return sortDirection == SdFolderSortDirection::Ascending ? result : !result;
        });
    resultCount = filteredCount;
    return sortedFilteredFiles;
}

bool SdFolder::CompareClassification(const FileInfo* a, const FileInfo* b, bool& result)
{
    auto aClassification = a->GetFileType()->GetClassification();
    auto bClassification = b->GetFileType()->GetClassification();
    if (aClassification == bClassification)
        return false;

    if (aClassification == FileTypeClassification::Folder)
    {
        result = true;
        return true;
    }
    else if (bClassification == FileTypeClassification::Folder)
    {
        result = false;
        return true;
    }

    return false;
}

bool SdFolder::CompareName(const FileInfo* a, const FileInfo* b)
{
    return strcasecmp(a->GetFileName(), b->GetFileName()) < 0;
}

void SdFolder::SortByNameInPlace()
{
    std::sort(_files, _files + _fileCount, CompareName);
}

const FileInfo* SdFolder::BinarySearch(const char* fileName) const
{
    if (_fileCount != 0)
    {
        const auto file = std::lower_bound(_files, _files + _fileCount, fileName,
            [] (const FileInfo* entry, const char* value)
            {
                return strcasecmp(entry->GetFileName(), value) < 0;
            });

        if (file != _files + _fileCount && !strcasecmp((*file)->GetFileName(), fileName))
        {
            return *file;
        }
    }

    return nullptr;
}

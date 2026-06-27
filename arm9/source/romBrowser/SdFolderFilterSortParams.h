#pragma once
#include "SdFolderSortType.h"
#include "SdFolderSortDirection.h"

class SdFolderFilterSortParams
{
public:
    SdFolderSortType sortType = SdFolderSortType::Name;
    SdFolderSortDirection sortDirection = SdFolderSortDirection::Ascending;
    bool includeHiddenFiles = false;
    const char* nameQuery = nullptr;

    SdFolderFilterSortParams() { }

    SdFolderFilterSortParams(SdFolderSortType sortType, SdFolderSortDirection sortDirection,
        bool includeHiddenFiles, const char* nameQuery = nullptr)
        : sortType(sortType), sortDirection(sortDirection)
        , includeHiddenFiles(includeHiddenFiles), nameQuery(nameQuery) { }
};

#include "common.h"
#include <algorithm>
#include "RomBrowserViewModel.h"

RomBrowserViewModel::RomBrowserViewModel(IRomBrowserController* romBrowserController, const char* initialSelectedFileName)
    : _romBrowserController(romBrowserController)
{
    SdFolderFilterSortParams filterSortParams;
    auto showHiddenFiles = romBrowserController->GetRomBrowserDisplaySettings().showHiddenFiles;
    switch (romBrowserController->GetRomBrowserDisplaySettings().sortMode)
    {
        case RomBrowserSortMode::NameAscending:
        default:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Ascending, showHiddenFiles);
            break;
        }
        case RomBrowserSortMode::NameDescending:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Descending, showHiddenFiles);
            break;
        }
        case RomBrowserSortMode::LastModified:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::LastModified, SdFolderSortDirection::Descending, showHiddenFiles);
            break;
        }
    }

    u64 startTick = gTickCounter.GetValue();
    const auto& sdFolder = romBrowserController->GetSdFolder();
    int filteredCount;
    auto sortedFilteredFiles = sdFolder.FilterAndSort(filterSortParams, filteredCount);
    u64 endTick = gTickCounter.GetValue();
    LOG_DEBUG("Filter + sort took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
    _fileInfoManager = std::make_unique<FileInfoManager>(std::move(sortedFilteredFiles), filteredCount, _romBrowserController->GetCoverRepository());
    _selectedItem = _fileInfoManager->GetItemIndex(initialSelectedFileName);
}

void RomBrowserViewModel::ItemActivated()
{
    const auto& item = _fileInfoManager->GetItem(_selectedItem);
    if (item.GetFileType()->GetClassification() == FileTypeClassification::Folder)
    {
        _romBrowserController->NavigateToPath(item.GetFileName());
    }
    else
    {
        _romBrowserController->LaunchFile(item);
    }
}

void RomBrowserViewModel::NavigateUp()
{
    _romBrowserController->NavigateUp();
}

void RomBrowserViewModel::ShowGameInfo()
{
    const auto& item = _fileInfoManager->GetItem(_selectedItem);
    if (item.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        return;
    _romBrowserController->ShowGameInfo();
}
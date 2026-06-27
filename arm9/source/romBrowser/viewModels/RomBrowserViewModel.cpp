#include "common.h"
#include <algorithm>
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserViewModel.h"

RomBrowserViewModel::RomBrowserViewModel(IRomBrowserController* romBrowserController, const char* initialSelectedFileName)
    : _romBrowserController(romBrowserController)
{
    BuildFileInfoManager(initialSelectedFileName);
}

void RomBrowserViewModel::Refresh(const char* initialSelectedFileName)
{
    BuildFileInfoManager(initialSelectedFileName);
}

void RomBrowserViewModel::BuildFileInfoManager(const char* initialSelectedFileName)
{
    SdFolderFilterSortParams filterSortParams;
    switch (_romBrowserController->GetRomBrowserDisplaySettings().sortMode)
    {
        case RomBrowserSortMode::NameAscending:
        default:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Ascending, false);
            break;
        }
        case RomBrowserSortMode::NameDescending:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::Name, SdFolderSortDirection::Descending, false);
            break;
        }
        case RomBrowserSortMode::LastModified:
        {
            filterSortParams = SdFolderFilterSortParams(
                SdFolderSortType::LastModified, SdFolderSortDirection::Descending, false);
            break;
        }
    }
    filterSortParams.nameQuery = _romBrowserController->GetSearchQuery();
    u64 startTick = gTickCounter.GetValue();
    const auto& sdFolder = _romBrowserController->GetSdFolder();
    int filteredCount;
    auto sortedFilteredFiles = sdFolder.FilterAndSort(filterSortParams, filteredCount);
    u64 endTick = gTickCounter.GetValue();
    LOG_DEBUG("Filter + sort took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
    const char* oldSelectedFileName = nullptr;
    if (_fileInfoManager && _selectedItem >= 0 && _selectedItem < _fileInfoManager->GetItemCount())
    {
        oldSelectedFileName = _fileInfoManager->GetItem(_selectedItem).GetFileName();
    }

    _fileInfoManager = std::make_unique<FileInfoManager>(
        std::move(sortedFilteredFiles), filteredCount, _romBrowserController->GetCoverRepository());

    const char* selectedFileName = initialSelectedFileName ? initialSelectedFileName : oldSelectedFileName;
    _selectedItem = _fileInfoManager->GetItemIndex(selectedFileName);
}

void RomBrowserViewModel::NavigateUp()
{
    _romBrowserController->NavigateUp();
}

void RomBrowserViewModel::ShowGameInfo()
{
    const auto& item = _fileInfoManager->GetItem(_selectedItem);
    if (item.GetFileType() == &NdsFileType::sInstance)
    {
        _romBrowserController->ShowGameInfo(item);
    }
}

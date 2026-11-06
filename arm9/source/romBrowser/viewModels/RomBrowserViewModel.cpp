#include "common.h"
#include <algorithm>
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserViewModel.h"

RomBrowserViewModel::RomBrowserViewModel(IRomBrowserController* romBrowserController, const char* initialSelectedFileName)
    : _romBrowserController(romBrowserController)
{
    SdFolderFilterSortParams filterSortParams;
    switch (romBrowserController->GetRomBrowserDisplaySettings().sortMode)
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
    filterSortParams.favoritesOnly = romBrowserController->IsFavoritesFilterEnabled();
    filterSortParams.completedOnly = romBrowserController->IsCompletedFilterEnabled();
    filterSortParams.hideEmptyFolders = romBrowserController->GetRomBrowserDisplaySettings().hideEmptyFolders;
    filterSortParams.hideSavesFolders = romBrowserController->GetSaveLocation() == SaveLocation::SavesFolder;
    // Hidden entries stay hidden in every layout, and there is no show-hidden
    // toggle to take away: this is the one place that decides it, so it stays
    // false.
    filterSortParams.includeHiddenFiles = false;
    filterSortParams.gameDataService = romBrowserController->GetGameDataService();
    u64 startTick = gTickCounter.GetValue();
    const auto& sdFolder = romBrowserController->GetSdFolder();
    u32 filteredCount;
    auto sortedFilteredFiles = sdFolder.FilterAndSort(filterSortParams, filteredCount);
    u64 endTick = gTickCounter.GetValue();
    LOG_DEBUG("Filter + sort took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
    _fileInfoManager = std::make_unique<FileInfoManager>(std::move(sortedFilteredFiles),
        filteredCount, _romBrowserController->GetCoverRepository(), _romBrowserController->GetIconRepository(),
        _romBrowserController->GetBannerRepository());
    _selectedItem = _fileInfoManager->GetItemIndex(initialSelectedFileName);
}

void RomBrowserViewModel::NavigateUp()
{
    _romBrowserController->NavigateUp();
}

#include "common.h"
#include <string.h>
#include "romBrowser/IRomBrowserController.h"
#include "RomBrowserViewModel.h"
#include "romBrowser/FileType/Nds/NdsFileType.h"
#include "RomBrowserItemViewModel.h"

void RomBrowserItemViewModel::Activate()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType()->GetClassification() == FileTypeClassification::Folder)
        {
            _romBrowserController->NavigateToPath(item.GetFileName());
        }
        else
        {
            _romBrowserController->LaunchFile(item);
        }
    }
}

void RomBrowserItemViewModel::ShowGameInfo()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (strcmp(item.GetFileType()->GetShortName(), "nds") == 0)
        {
            _romBrowserController->ShowGameInfo(item);
        }
    }
}

void RomBrowserItemViewModel::ToggleFavorite()
{
    if (_index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(_index);
        if (item.GetFileType()->GetClassification() != FileTypeClassification::Folder)
        {
            _romBrowserController->ToggleFavorite(item);
            _isFavorite.store(!_isFavorite.load(std::memory_order_relaxed), std::memory_order_release);
        }
    }
}

void RomBrowserItemViewModel::SetIndex(int index)
{
    _index = index;

    bool isFavorite = false;
    if (index >= 0)
    {
        const auto& item = _romBrowserController->GetRomBrowserViewModel()->GetFileInfoManager().GetItem(index);
        if (item.GetFileType()->GetClassification() != FileTypeClassification::Folder)
        {
            isFavorite = _romBrowserController->IsFavorite(item);
        }
    }
    _isFavorite.store(isFavorite, std::memory_order_release);
}

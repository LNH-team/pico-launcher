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
            bool inFavoritesView = strcmp(_romBrowserController->GetCurrentPath(), ":favorites") == 0;
            _romBrowserController->ToggleFavorite(item);
            if (inFavoritesView)
                _romBrowserController->NavigateToPath(":favorites");
            else
                _isFavorite = !_isFavorite;
        }
    }
}

bool RomBrowserItemViewModel::IsFavorite() const
{
    return _isFavorite;
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
            // Every item in the favorites view is a favorite by definition, so the badge would be
            // redundant noise. Checked here (on the IO thread, same as GetCurrentPath()'s writer)
            // rather than live in IsFavorite() on the render thread, which would otherwise flicker
            // true for a frame or two when _navigatePath gets overwritten mid-launch.
            bool inFavoritesView = strcmp(_romBrowserController->GetCurrentPath(), ":favorites") == 0;
            isFavorite = !inFavoritesView && _romBrowserController->IsFavorite(item);
        }
    }
    _isFavorite = isFavorite;
}

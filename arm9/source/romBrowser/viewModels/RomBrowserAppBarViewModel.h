#pragma once
#include <string.h>
#include "../IRomBrowserController.h"

/// @brief View model for the rom browser app bar
class RomBrowserAppBarViewModel
{
public:
    explicit RomBrowserAppBarViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void NavigateUp()
    {
        _romBrowserController->NavigateUp();
    }

    void ShowDisplaySettings()
    {
        _romBrowserController->ShowDisplaySettings();
    }

    void NavigateToPath(const TCHAR* name)
    {
        _romBrowserController->NavigateToPath(name);
    }

    bool IsAtRoot() const
    {
        return _romBrowserController->IsAtRoot();
    }

    bool IsFavoritesView() const
    {
        return strcmp(_romBrowserController->GetCurrentPath(), ":favorites") == 0;
    }

    void ToggleFavoritesView()
    {
        // While the favorites view is open the FatFs current directory still points at
        // the folder browsed before entering it, so navigating to "." returns there.
        _romBrowserController->NavigateToPath(IsFavoritesView() ? "." : ":favorites");
    }

    constexpr RomBrowserLayout GetRomBrowserLayout() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

private:
    IRomBrowserController* _romBrowserController;
};

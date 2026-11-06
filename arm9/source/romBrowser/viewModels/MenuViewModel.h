#pragma once
#include "../IRomBrowserController.h"

/// @brief View model for the menu the app bar's "more" button opens. Every
///        entry forwards to the controller: the panels replace the sheet with
///        their own, and a filter closes it through the display mode change
///        it fires (see the Menu state in RomBrowserStateMachine).
class MenuViewModel
{
public:
    explicit MenuViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void ShowRecents() { _romBrowserController->ShowRecents(); }
    void ShowFavorites() { _romBrowserController->ShowFavorites(); }
    void ShowStatistics() { _romBrowserController->ShowStatistics(); }
    bool CanDeleteSelected() const { return _romBrowserController->CanDeleteSelected(); }
    void RequestDeleteSelected() { _romBrowserController->RequestDeleteSelected(); }
    void ToggleFavoritesFilter() { _romBrowserController->ToggleFavoritesFilter(); }
    bool IsFavoritesFilterEnabled() const { return _romBrowserController->IsFavoritesFilterEnabled(); }
    void ToggleCompletedFilter() { _romBrowserController->ToggleCompletedFilter(); }
    bool IsCompletedFilterEnabled() const { return _romBrowserController->IsCompletedFilterEnabled(); }

    void ShowAbout() { _romBrowserController->ShowAbout(); }
    void ShowExitCombo() { _romBrowserController->ShowExitCombo(); }
    void ShowGameInfoEdit() { _romBrowserController->ShowGameInfoEdit(); }
    bool CanEditSelected() const { return _romBrowserController->CanEditSelected(); }

    void Close() { _romBrowserController->HideMenu(); }

private:
    IRomBrowserController* _romBrowserController;
};

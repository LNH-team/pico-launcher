#pragma once
#include "../IRomBrowserController.h"

/// @brief View model for the in-game exit combo sheet: the mask the loader
///        will use at the next launch, and where picking another one goes.
class ExitComboViewModel
{
public:
    explicit ExitComboViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    u16 GetCombo() const { return _romBrowserController->GetLoaderExitCombo(); }
    void SetCombo(u16 combo) { _romBrowserController->SetLoaderExitCombo(combo); }
    void Close() { _romBrowserController->HideExitCombo(); }

private:
    IRomBrowserController* _romBrowserController;
};

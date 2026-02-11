#pragma once
#include "core/SharedPtr.h"
#include "../RomBrowserController.h"
#include "RomBrowserViewModel.h"
#include "RomBrowserAppBarViewModel.h"

class ICoverRepository;

/// @brief View model for the rom browser bottom screen
class RomBrowserBottomScreenViewModel
{
public:
    RomBrowserBottomScreenViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _romBrowserAppBarViewModel(romBrowserController) { }

    bool IsRomBrowserVisible() const
    {
        auto state = _romBrowserController->GetStateMachine().GetCurrentState();
        return state == RomBrowserState::Browser
            || state == RomBrowserState::GameInfo
            || state == RomBrowserState::DisplaySettings
            || state == RomBrowserState::Launching;
    }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel()
    {
        return _romBrowserController->GetRomBrowserViewModel();
    }

    RomBrowserAppBarViewModel* GetRomBrowserAppBarViewModel()
    {
        return &_romBrowserAppBarViewModel;
    }

    void NavigateUp()
    {
        _romBrowserController->NavigateUp();
    }

    bool TryShowDisplaySettings()
    {
        auto state = _romBrowserController->GetStateMachine().GetCurrentState();
        if (state != RomBrowserState::Browser)
            return false;

        _romBrowserController->ShowDisplaySettings();
        return true;
    }

private:
    IRomBrowserController* _romBrowserController;
    RomBrowserAppBarViewModel _romBrowserAppBarViewModel;
};

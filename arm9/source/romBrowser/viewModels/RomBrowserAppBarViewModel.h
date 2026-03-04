#pragma once
#include "../IRomBrowserController.h"

static constexpr RomBrowserLayout sAppBarLayouts[] = {
    RomBrowserLayout::HorizontalIconGrid,
    RomBrowserLayout::VerticalIconGrid,
    RomBrowserLayout::BannerList,
    RomBrowserLayout::CoverFlow,
};

static constexpr RomBrowserSortMode sAppBarSortModes[] = {
    RomBrowserSortMode::NameAscending,
    RomBrowserSortMode::NameDescending,
};

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

    void ToggleFavoritesView()
    {
        _romBrowserController->ToggleFavoritesView();
    }

    void CycleLayout()
    {
        auto displaySettings = _romBrowserController->GetRomBrowserDisplaySettings();
        constexpr int layoutCount = sizeof(sAppBarLayouts) / sizeof(sAppBarLayouts[0]);
        int index = 0;
        for (int i = 0; i < layoutCount; i++)
        {
            if (sAppBarLayouts[i] == displaySettings.layout)
            {
                index = i;
                break;
            }
        }
        index = (index + 1) % layoutCount;
        displaySettings.layout = sAppBarLayouts[index];
        _romBrowserController->SetRomBrowserDisplaySettings(displaySettings);
    }

    void CycleSortMode()
    {
        auto displaySettings = _romBrowserController->GetRomBrowserDisplaySettings();
        constexpr int sortModeCount = sizeof(sAppBarSortModes) / sizeof(sAppBarSortModes[0]);
        int index = 0;
        for (int i = 0; i < sortModeCount; i++)
        {
            if (sAppBarSortModes[i] == displaySettings.sortMode)
            {
                index = i;
                break;
            }
        }
        index = (index + 1) % sortModeCount;
        displaySettings.sortMode = sAppBarSortModes[index];
        _romBrowserController->SetRomBrowserDisplaySettings(displaySettings);
    }

    bool IsFavoritesViewActive() const
    {
        return _romBrowserController->IsFavoritesViewActive();
    }

    constexpr RomBrowserLayout GetRomBrowserLayout() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

    constexpr RomBrowserSortMode GetRomBrowserSortMode() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().sortMode;
    }

private:
    IRomBrowserController* _romBrowserController;
};

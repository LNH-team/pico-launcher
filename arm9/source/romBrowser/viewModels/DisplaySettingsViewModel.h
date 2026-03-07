#pragma once
#include "../IRomBrowserController.h"
#include "services/settings/RomBrowserDisplaySettings.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    constexpr RomBrowserLayout GetRomBrowserDisplayMode() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

    void SetRomBrowserDisplayMode(RomBrowserLayout romBrowserDisplayMode)
    {
        auto romBrowserDisplaySettings = _romBrowserController->GetRomBrowserDisplaySettings();
        if (romBrowserDisplaySettings.layout != romBrowserDisplayMode)
        {
            romBrowserDisplaySettings.layout = romBrowserDisplayMode;
            _romBrowserController->SetRomBrowserDisplaySettings(romBrowserDisplaySettings);
        }
    }

    constexpr RomBrowserSortMode GetRomBrowserSortMode() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().sortMode;
    }

    void SetRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
    {
        auto romBrowserDisplaySettings = _romBrowserController->GetRomBrowserDisplaySettings();
        if (romBrowserDisplaySettings.sortMode != romBrowserSortMode)
        {
            romBrowserDisplaySettings.sortMode = romBrowserSortMode;
            _romBrowserController->SetRomBrowserDisplaySettings(romBrowserDisplaySettings);
        }
    }

    void Close()
    {
        _romBrowserController->HideDisplaySettings();
    }

    void MarkSettingsDirty()
    {
        _romBrowserController->MarkSettingsDirty();
    }

    void RequestThemeReload()
    {
        _romBrowserController->RequestThemeReload();
    }

    void ShowInfo()
    {
        _romBrowserController->ShowDisplayInfo();
    }

    void HideInfo()
    {
        _romBrowserController->HideDisplayInfo();
    }

private:
    IRomBrowserController* _romBrowserController;
};

#pragma once
#include "../IRomBrowserController.h"
#include "services/settings/RomBrowserDisplaySettings.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController, AppSettings* appSettings)
        : _romBrowserController(romBrowserController)
        , _appSettings(appSettings)
        , _romBrowserDisplaySettings(_romBrowserController->GetRomBrowserDisplaySettings()) { }

    constexpr RomBrowserLayout GetRomBrowserDisplayMode() const
    {
        return _romBrowserDisplaySettings.layout;
    }

    void SetRomBrowserDisplayMode(RomBrowserLayout romBrowserDisplayMode)
    {
        if (_romBrowserDisplaySettings.layout != romBrowserDisplayMode)
        {
            _romBrowserDisplaySettings.layout = romBrowserDisplayMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    constexpr RomBrowserSortMode GetRomBrowserSortMode() const
    {
        return _romBrowserDisplaySettings.sortMode;
    }

    void SetRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
    {
        if (_romBrowserDisplaySettings.sortMode != romBrowserSortMode)
        {
            _romBrowserDisplaySettings.sortMode = romBrowserSortMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    const char* GetTheme() const
    {
        return _appSettings->theme.GetString();
    }

    void SetTheme(const String<char, 64>& theme);
    void SelectPreviousTheme();
    void SelectNextTheme();

    void Close()
    {
        _romBrowserController->HideDisplaySettings();
    }

private:
    IRomBrowserController* _romBrowserController;
    AppSettings* _appSettings;
    RomBrowserDisplaySettings _romBrowserDisplaySettings;

    void SelectRelativeTheme(int offset);
};

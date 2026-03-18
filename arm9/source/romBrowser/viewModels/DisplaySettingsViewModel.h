#pragma once
#include "../IRomBrowserController.h"
#include "services/settings/RomBrowserDisplaySettings.h"
#include "services/settings/IAppSettingsService.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController), _appSettingsService(nullptr) { }

    void SetAppSettingsService(IAppSettingsService* appSettingsService)
    {
        _appSettingsService = appSettingsService;
    }

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

    void SaveSettingsNow()
    {
        _romBrowserController->SaveSettingsNow();
    }

    void RequestThemeReload()
    {
        _romBrowserController->RequestThemeReload();
    }

    void RequestChangeDisplayMode()
    {
        _romBrowserController->RequestChangeDisplayMode();
    }

    bool IsMaterialTheme() const
    {
        if (!_appSettingsService) return true;
        const auto& theme = _appSettingsService->GetAppSettings().theme;
        return theme == "material";
    }

    bool IsChinese() const
    {
        if (!_appSettingsService) return false;
        const auto& lang = _appSettingsService->GetAppSettings().language;
        return lang == "chinese";
    }

    bool GetDarkMode() const
    {
        if (!_appSettingsService) return false;
        return _appSettingsService->GetAppSettings().darkMode;
    }

    void ToggleDarkMode()
    {
        if (!_appSettingsService) return;
        auto& settings = _appSettingsService->GetAppSettings();
        settings.darkMode = !settings.darkMode;
        _romBrowserController->MarkSettingsDirty();
        _romBrowserController->RequestChangeDisplayMode();
    }

    void ToggleLanguage()
    {
        if (!_appSettingsService) return;
        auto& settings = _appSettingsService->GetAppSettings();
        if (settings.language == "chinese")
            settings.language = "english";
        else
            settings.language = "chinese";
        _romBrowserController->MarkSettingsDirty();
        _romBrowserController->SaveSettingsNow();
        _romBrowserController->RequestChangeDisplayMode();
    }

    void ToggleTheme()
    {
        if (!_appSettingsService) return;
        auto& settings = _appSettingsService->GetAppSettings();
        if (settings.theme == "material")
            settings.theme = "raspberry";
        else
            settings.theme = "material";
        _romBrowserController->MarkSettingsDirty();
        _romBrowserController->SaveSettingsNow();
        _romBrowserController->RequestChangeDisplayMode();
    }

private:
    IRomBrowserController* _romBrowserController;
    IAppSettingsService* _appSettingsService;
};

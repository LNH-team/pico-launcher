#pragma once
#include <string.h>
#include <memory>
#include <vector>
#include "../IRomBrowserController.h"
#include "services/settings/RomBrowserDisplaySettings.h"
#include "services/settings/IAppSettingsService.h"
#include "bgm/IBgmService.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/SdFolderFactory.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController), _appSettingsService(nullptr)
        , _bgmService(nullptr), _bgmIndex(-1) { }

    void SetAppSettingsService(IAppSettingsService* appSettingsService)
    {
        _appSettingsService = appSettingsService;
    }

    void SetBgmService(IBgmService* bgmService)
    {
        _bgmService = bgmService;
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
        return !strcasecmp(_appSettingsService->GetAppSettings().theme.GetString(), "material");
    }

    bool IsChinese() const
    {
        if (!_appSettingsService) return false;
        return !strcasecmp(_appSettingsService->GetAppSettings().language.GetString(), "chinese");
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
        if (IsChinese())
            settings.language = "English";
        else
            settings.language = "Chinese";
        _romBrowserController->MarkSettingsDirty();
        _romBrowserController->SaveSettingsNow();
    }

    void ToggleTheme()
    {
        if (!_appSettingsService) return;
        auto& settings = _appSettingsService->GetAppSettings();
        if (IsMaterialTheme())
            settings.theme = "raspberry";
        else
            settings.theme = "material";
        _romBrowserController->MarkSettingsDirty();
        _romBrowserController->SaveSettingsNow();
        _romBrowserController->RequestThemeReload();
        _romBrowserController->HideDisplaySettings();
    }

    /// @brief Scans /_pico/bgm/ and populates the BGM file list.
    void ScanBgmFiles()
    {
        _bgmFileNames.clear();
        _bgmIndex = -1; // -1 = Random

        NullFileTypeProvider fileTypeProvider;
        auto bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/bgm");
        if (!bgmFolder || bgmFolder->GetFileCount() == 0)
            return;

        for (int i = 0; i < bgmFolder->GetFileCount(); i++)
        {
            const char* name = bgmFolder->GetFiles()[i]->GetFileName();
            if (name)
                _bgmFileNames.push_back(std::string(name));
        }

        // Find current setting in list
        if (_appSettingsService)
        {
            const char* current = _appSettingsService->GetAppSettings().bgm.GetString();
            if (current && current[0] != '\0')
            {
                for (int i = 0; i < (int)_bgmFileNames.size(); i++)
                {
                    if (!strcasecmp(_bgmFileNames[i].c_str(), current))
                    {
                        _bgmIndex = i;
                        break;
                    }
                }
            }
        }
    }

    /// @brief Gets the display name for the current BGM selection.
    /// @param outBuf Buffer to write the display name into (UTF-16).
    /// @param bufLen Max number of char16_t units in outBuf.
    void GetBgmDisplayName(char16_t* outBuf, int bufLen) const
    {
        if (_bgmIndex < 0 || _bgmIndex >= (int)_bgmFileNames.size())
        {
            // "Random"
            const char16_t random[] = u"Random";
            int i = 0;
            for (; i < bufLen - 1 && random[i]; i++)
                outBuf[i] = random[i];
            outBuf[i] = 0;
            return;
        }

        const std::string& name = _bgmFileNames[_bgmIndex];
        // Strip .bcstm extension for display
        int nameLen = (int)name.size();
        int extPos = nameLen;
        for (int i = nameLen - 1; i >= 0; i--)
        {
            if (name[i] == '.')
            {
                extPos = i;
                break;
            }
        }
        int copyLen = extPos < (bufLen - 1) ? extPos : (bufLen - 1);
        for (int i = 0; i < copyLen; i++)
            outBuf[i] = (char16_t)(unsigned char)name[i];
        outBuf[copyLen] = 0;
    }

    /// @brief Cycles to the next BGM option.
    void NextBgm()
    {
        if (_bgmFileNames.empty())
            return;
        _bgmIndex++;
        if (_bgmIndex >= (int)_bgmFileNames.size())
            _bgmIndex = -1; // wrap to Random
        ApplyBgm();
    }

    /// @brief Cycles to the previous BGM option.
    void PrevBgm()
    {
        if (_bgmFileNames.empty())
            return;
        _bgmIndex--;
        if (_bgmIndex < -1)
            _bgmIndex = (int)_bgmFileNames.size() - 1;
        ApplyBgm();
    }

private:
    void ApplyBgm()
    {
        if (!_appSettingsService)
            return;

        auto& settings = _appSettingsService->GetAppSettings();
        if (_bgmIndex >= 0 && _bgmIndex < (int)_bgmFileNames.size())
            settings.bgm = _bgmFileNames[_bgmIndex].c_str();
        else
            settings.bgm = "";

        _romBrowserController->MarkSettingsDirty();

        // Immediately play the selected BGM
        if (_bgmService)
        {
            if (_bgmIndex >= 0 && _bgmIndex < (int)_bgmFileNames.size())
            {
                char path[256];
                // Build full path
                int len = 0;
                const char* prefix = "/_pico/bgm/";
                while (prefix[len]) { len++; }
                memcpy(path, prefix, len);
                const char* fname = _bgmFileNames[_bgmIndex].c_str();
                int flen = 0;
                while (fname[flen]) { flen++; }
                memcpy(path + len, fname, flen);
                path[len + flen] = '\0';
                _bgmService->StartBgm(path);
            }
            else
            {
                // Random: use StartBgmFromConfig
                _bgmService->StartBgmFromConfig("");
            }
        }
    }

    IRomBrowserController* _romBrowserController;
    IAppSettingsService* _appSettingsService;
    IBgmService* _bgmService;
    std::vector<std::string> _bgmFileNames;
    int _bgmIndex; // -1 = Random
};

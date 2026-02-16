#include "common.h"
#include <memory>
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "listIcon.h"
#include "sortNameAscendingIcon.h"
#include "sortNameDescendingIcon.h"
#include "recentIcon.h"
#include "gamesIcon.h"
#include "picturesIcon.h"
#include "musicIcon.h"
#include "moviesIcon.h"
#include "unknownIcon.h"
#include "coverflowIcon.h"
#include "../IRomBrowserController.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "fat/Directory.h"
#include "fat/File.h"
#include "json/ArduinoJson.h"
#include "core/StringUtil.h"
#include "core/mini-printf.h"
#include "DisplaySettingsBottomSheetView.h"
#include "services/localization/Localization.h"

#define TITLE_LABEL_X       15
#define TITLE_LABEL_Y       16

#define LAYOUT_LABEL_X      20
#define LAYOUT_LABEL_Y      46

#define SORTING_LABEL_X     20
#define SORTING_LABEL_Y     78

#define FILTERS_LABEL_X     20
#define FILTERS_LABEL_Y     112

#define THEME_LABEL_X       20
#define THEME_LABEL_Y       106
#define THEME_VALUE_X       100

#define LANGUAGE_LABEL_X    20
#define LANGUAGE_LABEL_Y    130
#define LANGUAGE_VALUE_X    100

static RomBrowserLayout sRomBrowserDisplayModes[4] =
{
    [0] = RomBrowserLayout::HorizontalIconGrid,
    [1] = RomBrowserLayout::VerticalIconGrid,
    [2] = RomBrowserLayout::BannerList,
    [3] = RomBrowserLayout::CoverFlow
};

static RomBrowserSortMode sRomBrowserSortModes[4] =
{
    [0] = RomBrowserSortMode::NameAscending,
    [1] = RomBrowserSortMode::NameDescending,
    [2] = RomBrowserSortMode::LastModified
};

DisplaySettingsBottomSheetView::DisplaySettingsBottomSheetView(
    DisplaySettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, IAppSettingsService* appSettingsService)
    : _viewModel(viewModel)
    , _appSettingsService(appSettingsService)
    , _titleLabel(128, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _layoutLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _sortingLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _themeLabel(80, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _themeValueLabel(120, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _languageLabel(80, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _languageValueLabel(120, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _materialColorScheme(materialColorScheme)
    // , _filtersLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))

{
    LoadLanguages();
    const char* currLang = _appSettingsService->GetAppSettings().language.GetString();
    _selectedLanguageIdx = 0;
    for (int i = 0; i < _languageCount; ++i) {
        if (strcasecmp(currLang, _languageEntries[i].fileName.GetString()) == 0) {
            _selectedLanguageIdx = i;
            break;
        }
    }

    LoadThemes();
    const char* currTheme = _appSettingsService->GetAppSettings().theme.GetString();
    _selectedThemeIdx = 0;
    for (int i = 0; i < _themeCount; ++i)
    {
        if (strcasecmp(currTheme, _themeNames[i].GetString()) == 0)
        {
            _selectedThemeIdx = i;
            break;
        }
    }

    Localization::Initialize(_appSettingsService);
    _titleLabel.SetText(Localization::Translate("display_settings"));
    AddChildTail(&_titleLabel);
    _layoutLabel.SetText(Localization::Translate("layout"));
    AddChildTail(&_layoutLabel);
    _sortingLabel.SetText(Localization::Translate("sorting"));
    AddChildTail(&_sortingLabel);
    _themeLabel.SetText(Localization::Translate("theme"));
    AddChildTail(&_themeLabel);
    AddChildTail(&_themeValueLabel);
    _languageLabel.SetText(Localization::Translate("language"));
    AddChildTail(&_languageLabel);
    AddChildTail(&_languageValueLabel);
    // _sortingLabel.SetText(Localization::Translate("filters"));
    // AddChildTail(&_filtersLabel);

    for (auto& layoutOption : _layoutOptions) {
        layoutOption = CreateLayoutOptionIconButton();
        AddChildTail(&layoutOption);
    }
    for (auto& sortOption : _sortOptions) {
        sortOption = CreateSortOptionIconButton();
        AddChildTail(&sortOption);
    }

    UpdateThemeUI();
    UpdateLanguageUI();
}

    // for (auto& filterOption : _filterOptions)
    // {
    //     filterOption = CreateFilterOptionIconButton();
    //     AddChildTail(&filterOption);
    // }

    // _filterOptions[0].SetState(IconButtonView::State::ToggleSelected);

void DisplaySettingsBottomSheetView::ChangeLanguage(int newIdx)
{
    _selectedLanguageIdx = newIdx;
    _appSettingsService->GetAppSettings().language = _languageEntries[_selectedLanguageIdx].fileName.GetString();
    _settingsDirty = true;
    Localization::Initialize(_appSettingsService);
    UpdateLanguageUI();
    _titleLabel.SetText(Localization::Translate("display_settings"));
    _layoutLabel.SetText(Localization::Translate("layout"));
    _sortingLabel.SetText(Localization::Translate("sorting"));
    _themeLabel.SetText(Localization::Translate("theme"));
    _languageLabel.SetText(Localization::Translate("language"));
}

void DisplaySettingsBottomSheetView::LoadThemes()
{
    _themeCount = 0;
    Directory directory;
    if (directory.Open("/_pico/themes") == FR_OK)
    {
        FILINFO fileInfo;
        
        while (true)
        {
            if (directory.Read(&fileInfo) != FR_OK)
                break;
            if (fileInfo.fname[0] == 0)
                break;
            if (fileInfo.fname[0] == '.')
                continue;
            if ((fileInfo.fattrib & AM_DIR) == 0)
                continue;
            if (_themeCount >= kMaxThemeCount)
                break;

            _themeNames[_themeCount++] = fileInfo.fname;
        }
    }

    if (_themeCount == 0)
    {
        _themeNames[0] = "NULL";
        _themeCount = 1;
    }
    else
    {
        _themeNames[_themeCount++] = "RANDOM";
    }
}

void DisplaySettingsBottomSheetView::LoadLanguages()
{
    _languageCount = 0;
    Directory directory;
    if (directory.Open("/_pico/extras/translations") != FR_OK)
        return;

    FILINFO fileInfo;
    while (true)
    {
        if (directory.Read(&fileInfo) != FR_OK)
            break;
        if (fileInfo.fname[0] == 0)
            break;
        if (fileInfo.fname[0] == '.')
            continue;
        if (fileInfo.fattrib & AM_DIR)
            continue;
        if (_languageCount >= kMaxLanguageCount)
            break;

        // Check if the file ends in .json
        const char* dot = strrchr(fileInfo.fname, '.');
        if (!dot || strcasecmp(dot, ".json") != 0)
            continue;

        // Extract filename without extension
        char baseName[64];
        size_t len = dot - fileInfo.fname;
        if (len >= sizeof(baseName))
            len = sizeof(baseName) - 1;
        memcpy(baseName, fileInfo.fname, len);
        baseName[len] = '\0';

        auto& entry = _languageEntries[_languageCount];
        entry.fileName = baseName;

        // Default display name: filename (ASCII to UTF-16)
        for (size_t i = 0; i < len && i < 63; i++)
            entry.displayName[i] = (char16_t)baseName[i];
        entry.displayName[len < 63 ? len : 63] = 0;

        // Try to read language_name from the JSON file
        char path[128];
        mini_snprintf(path, sizeof(path), "/_pico/extras/translations/%s", fileInfo.fname);
        {
            File file;
            if (file.Open(path, FA_READ | FA_OPEN_EXISTING) == FR_OK)
            {
                u32 fileSize = file.GetSize();
                if (fileSize > 0 && fileSize < 2048)
                {
                    auto buf = std::make_unique<u8[]>(fileSize);
                    u32 bytesRead = 0;
                    if (file.Read(buf.get(), fileSize, bytesRead) == FR_OK && bytesRead == fileSize)
                    {
                        // Skip UTF-8 BOM if present
                        const u8* jsonData = buf.get();
                        u32 jsonSize = fileSize;
                        if (jsonSize >= 3 && jsonData[0] == 0xEF && jsonData[1] == 0xBB && jsonData[2] == 0xBF)
                        {
                            jsonData += 3;
                            jsonSize -= 3;
                        }

                        DynamicJsonDocument json(2048);
                        if (deserializeJson(json, jsonData, jsonSize) == DeserializationError::Ok)
                        {
                            const char* langName = json["language_name"] | (const char*)nullptr;
                            if (langName && langName[0] != 0)
                            {
                                // Convert UTF-8 to UTF-16
                                u32 si = 0, di = 0;
                                while (langName[si] && di < 63)
                                {
                                    u8 c = (u8)langName[si];
                                    u32 cp;
                                    if (c < 0x80) { cp = c; si++; }
                                    else if ((c & 0xE0) == 0xC0) { cp = (c & 0x1F) << 6; cp |= ((u8)langName[si+1] & 0x3F); si += 2; }
                                    else if ((c & 0xF0) == 0xE0) { cp = (c & 0x0F) << 12; cp |= ((u8)langName[si+1] & 0x3F) << 6; cp |= ((u8)langName[si+2] & 0x3F); si += 3; }
                                    else { si += 4; continue; }
                                    if (cp <= 0xFFFF)
                                        entry.displayName[di++] = (char16_t)cp;
                                }
                                entry.displayName[di] = 0;
                            }
                        }
                    }
                }
            }
        }

        _languageCount++;
    }

    // If no languages found, add a default English entry
    if (_languageCount == 0)
    {
        _languageEntries[0].fileName = "english";
        StringUtil::Copy(_languageEntries[0].displayName, u"English", 64);
        _languageCount = 1;
    }
}

void DisplaySettingsBottomSheetView::ChangeTheme(int newIdx)
{
    _selectedThemeIdx = newIdx;
    UpdateThemeUI();
}

void DisplaySettingsBottomSheetView::ApplyTheme()
{
    _appSettingsService->GetAppSettings().theme = _themeNames[_selectedThemeIdx].GetString();
    _settingsDirty = true;
}

void DisplaySettingsBottomSheetView::UpdateThemeUI()
{
    _themeLabel.SetPosition(THEME_LABEL_X, _position.y + THEME_LABEL_Y);
    _themeValueLabel.SetPosition(THEME_VALUE_X, _position.y + THEME_LABEL_Y);
    _themeValueLabel.SetText(_themeNames[_selectedThemeIdx].GetString());
}

void DisplaySettingsBottomSheetView::UpdateLanguageUI()
{
    _languageLabel.SetPosition(LANGUAGE_LABEL_X, _position.y + LANGUAGE_LABEL_Y);
    _languageValueLabel.SetPosition(LANGUAGE_VALUE_X, _position.y + LANGUAGE_LABEL_Y);
    if (_languageCount > 0)
        _languageValueLabel.SetText(_languageEntries[_selectedLanguageIdx].displayName);
}

IconButton2DView DisplaySettingsBottomSheetView::CreateLayoutOptionIconButton()
{
    IconButton2DView layoutOption
    {
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    };
    layoutOption.SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        u32 idx = ((IconButton2DView*)sender) - &self->_layoutOptions[0];
        self->_viewModel->SetRomBrowserDisplayMode(sRomBrowserDisplayModes[idx]);
    }, this);
    return layoutOption;
}

IconButton2DView DisplaySettingsBottomSheetView::CreateSortOptionIconButton()
{
    IconButton2DView sortOption
    {
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    };
    sortOption.SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        u32 idx = ((IconButton2DView*)sender) - &self->_sortOptions[0];
        self->_viewModel->SetRomBrowserSortMode(sRomBrowserSortModes[idx]);
    }, this);
    return sortOption;
}

// IconButtonView DisplaySettingsBottomSheetView::CreateFilterOptionIconButton()
// {
//     IconButtonView filterOption
//     {
//         IconButtonView::Type::Tonal,
//         IconButtonView::State::ToggleUnselected,
//         md::sys::color::surfaceContainerLow,
//         _materialColorScheme
//     };
//     return filterOption;
// }

void DisplaySettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        // layout options
        _layoutOptions[0].SetIconVramOffset(LoadIcon(*objVramManager, hGridIconTiles, hGridIconTilesLen));
        _layoutOptions[1].SetIconVramOffset(LoadIcon(*objVramManager, vGridIconTiles, vGridIconTilesLen));
        _layoutOptions[2].SetIconVramOffset(LoadIcon(*objVramManager, bannerListIconTiles, bannerListIconTilesLen));
        _layoutOptions[3].SetIconVramOffset(LoadIcon(*objVramManager, coverflowIconTiles, coverflowIconTilesLen));

        // sort options
        _sortOptions[0].SetIconVramOffset(LoadIcon(*objVramManager, sortNameAscendingIconTiles, sortNameAscendingIconTilesLen));
        _sortOptions[1].SetIconVramOffset(LoadIcon(*objVramManager, sortNameDescendingIconTiles, sortNameDescendingIconTilesLen));
        // _sortOptions[2].SetIconVramOffset(LoadIcon(objVramManager, recentIconTiles, recentIconTilesLen));

        // filter options
        // _filterOptions[0].SetIconVramOffset(LoadIcon(objVramManager, gamesIconTiles, gamesIconTilesLen));
        // _filterOptions[1].SetIconVramOffset(LoadIcon(objVramManager, picturesIconTiles, picturesIconTilesLen));
        // _filterOptions[2].SetIconVramOffset(LoadIcon(objVramManager, musicIconTiles, musicIconTilesLen));
        // _filterOptions[3].SetIconVramOffset(LoadIcon(objVramManager, moviesIconTiles, moviesIconTilesLen));
        // _filterOptions[4].SetIconVramOffset(LoadIcon(objVramManager, unknownIconTiles, unknownIconTilesLen));
    }
}

void DisplaySettingsBottomSheetView::UpdateLabels()
{
    _titleLabel.SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _layoutLabel.SetPosition(LAYOUT_LABEL_X, _position.y + LAYOUT_LABEL_Y);
    _sortingLabel.SetPosition(SORTING_LABEL_X, _position.y + SORTING_LABEL_Y);
    // _filtersLabel.SetPosition(FILTERS_LABEL_X, _position.y + FILTERS_LABEL_Y);
    UpdateThemeUI();
    UpdateLanguageUI();
}

void DisplaySettingsBottomSheetView::Update()
{
    BottomSheetView::Update();
    UpdateLabels();
    auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();
    int x = 70;
    u32 idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption.SetPosition(x + 25, _position.y + 38);
        layoutOption.SetState(sRomBrowserDisplayModes[idx] == selectedDisplayMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        x += 32;
        idx++;
    }
    auto selectedSortMode = _viewModel->GetRomBrowserSortMode();
    x = 70;
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        sortOption.SetPosition(x + 25, _position.y + 70);
        sortOption.SetState(sRomBrowserSortModes[idx] == selectedSortMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        x += 32;
        idx++;
    }
    // x = 70;
    // for (auto& filterOption : _filterOptions)
    // {
    //     filterOption.SetPosition(x, _position.y + 102);
    //     x += 32;
    // }
}

void DisplaySettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel.SetForegroundColor(_materialColorScheme->onSurface);
        _layoutLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _layoutLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _sortingLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _sortingLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        // _filtersLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        // _filtersLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _themeLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _themeLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        bool themeFocused = _themeValueLabel.IsFocused();
        _themeValueLabel.SetBackgroundColor(themeFocused
            ? _materialColorScheme->GetColor(md::sys::color::secondaryContainer)
            : _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _themeValueLabel.SetForegroundColor(themeFocused
            ? _materialColorScheme->GetColor(md::sys::color::onSecondaryContainer)
            : _materialColorScheme->onSurfaceVariant);
        _languageLabel.SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _languageLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        bool langFocused = _languageValueLabel.IsFocused();
        _languageValueLabel.SetBackgroundColor(langFocused
            ? _materialColorScheme->GetColor(md::sys::color::secondaryContainer)
            : _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _languageValueLabel.SetForegroundColor(langFocused
            ? _materialColorScheme->GetColor(md::sys::color::onSecondaryContainer)
            : _materialColorScheme->onSurfaceVariant);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void DisplaySettingsBottomSheetView::SaveIfDirty()
{
    if (_settingsDirty)
    {
        _viewModel->MarkSettingsDirty();
        _settingsDirty = false;
    }
}

bool DisplaySettingsBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        ApplyTheme();
        SaveIfDirty();
        _viewModel->Close();
        return true;
    }
    return false;
}

View* DisplaySettingsBottomSheetView::MoveFocus(View* currentFocus,
    FocusMoveDirection direction, View* source)
{
    int idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        if (currentFocus == &layoutOption)
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _layoutOptions.size();
                return &_layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_layoutOptions.size())
                    idx = 0;
                return &_layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                return &_languageValueLabel;
            }
            else //if (direction == FocusMoveDirection::Down)
            {
                if (idx >= (int)_sortOptions.size())
                    idx = _sortOptions.size() - 1;
                return &_sortOptions[idx];
            }
        }
        idx++;
    }
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        if (currentFocus == &sortOption)
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _sortOptions.size();
                return &_sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_sortOptions.size())
                    idx = 0;
                return &_sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                if (idx >= (int)_layoutOptions.size())
                    idx = _layoutOptions.size() - 1;
                return &_layoutOptions[idx];
            }
            else //if (direction == FocusMoveDirection::Down)
            {
                return &_themeValueLabel;
            }
        }
        idx++;
    }
    idx = 0;
    if (currentFocus == &_themeValueLabel)
    {
        if (direction == FocusMoveDirection::Left)
        {
            int newIdx = (_selectedThemeIdx - 1 + _themeCount) % _themeCount;
            ChangeTheme(newIdx);
            return &_themeValueLabel;
        }
        if (direction == FocusMoveDirection::Right)
        {
            int newIdx = (_selectedThemeIdx + 1) % _themeCount;
            ChangeTheme(newIdx);
            return &_themeValueLabel;
        }
        if (direction == FocusMoveDirection::Up)
            return &_sortOptions[0];
        if (direction == FocusMoveDirection::Down)
            return &_languageValueLabel;
    }
    idx = 0;
    if (currentFocus == &_languageValueLabel)
    {
        if (direction == FocusMoveDirection::Left)
        {
            int newIdx = (_selectedLanguageIdx - 1 + _languageCount) % _languageCount;
            ChangeLanguage(newIdx);
            return &_languageValueLabel;
        }
        if (direction == FocusMoveDirection::Right)
        {
            int newIdx = (_selectedLanguageIdx + 1) % _languageCount;
            ChangeLanguage(newIdx);
            return &_languageValueLabel;
        }
        if (direction == FocusMoveDirection::Up)
            return &_themeValueLabel;
        if (direction == FocusMoveDirection::Down)
        {
            if (idx >= (int)_layoutOptions.size())
                idx = _layoutOptions.size() - 1;
            return &_layoutOptions[idx];
        }
    }
    return nullptr;
}

        // idx = 0;
    // for (auto& filterOption : _filterOptions)
    // {
    //     if (currentFocus == &filterOption)
    //     {
    //         if (direction == FocusMoveDirection::Left)
    //         {
    //             if (--idx < 0)
    //                 idx += _filterOptions.size();
    //             return &_filterOptions[idx];
    //         }
    //         else if (direction == FocusMoveDirection::Right)
    //         {
    //             if (++idx >= (int)_filterOptions.size())
    //                 idx = 0;
    //             return &_filterOptions[idx];
    //         }
    //         else if (direction == FocusMoveDirection::Up)
    //         {
    //             if (idx >= (int)_sortOptions.size())
    //                 idx = _sortOptions.size() - 1;
    //             return &_sortOptions[idx];
    //         }
    //         else //if (direction == FocusMoveDirection::Down)
    //         {
    //             if (idx >= (int)_layoutOptions.size())
    //                 idx = _layoutOptions.size() - 1;
    //             return &_layoutOptions[idx];
    //         }
    //     }
    //     idx++;
    // }

void DisplaySettingsBottomSheetView::SetGraphics(
    const IconButton2DView::VramToken& iconButtonVramToken)
{
    for (auto& layoutOption : _layoutOptions)
        layoutOption.SetGraphics(iconButtonVramToken);
    for (auto& sortOption : _sortOptions)
        sortOption.SetGraphics(iconButtonVramToken);
    // for (auto& filterOption : _filterOptions)
    // filterOption.SetGraphics(iconButtonVramToken);
}

u32 DisplaySettingsBottomSheetView::LoadIcon(IVramManager& vramManager,
    const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}

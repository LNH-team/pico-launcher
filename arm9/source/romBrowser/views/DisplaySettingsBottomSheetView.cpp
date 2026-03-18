#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/DescendingStackVramManager.h"
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
#include "gui/input/TouchEvent.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "services/Localization/Localization.h"
#include "core/mini-printf.h"
#include "romBrowser/SdFolder.h"
#include "romBrowser/SdFolderFactory.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "DisplaySettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LAYOUT_LABEL_X      20
#define LAYOUT_LABEL_Y      46

#define SORTING_LABEL_X     20
#define SORTING_LABEL_Y     78

#define FILTERS_LABEL_X     20
#define FILTERS_LABEL_Y     112

#define BGM_LIST_X          16
#define BGM_LIST_Y          36

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
    const IFontRepository* fontRepository,
    IAppSettingsService* appSettingsService, IBgmService* bgmService,
    const char* appliedThemeName)
    : _viewModel(viewModel)
    , _appSettingsService(appSettingsService)
    , _titleLabel(128, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _layoutLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _sortingLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _themeLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _languageLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _darkModeLabel(96, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _bgmLabel(64, 16, 25, fontRepository->GetFont(FontType::Regular10))
    , _themeChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _languageChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _darkModeChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _bgmChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _bgmService(bgmService)
    , _bgmSelectTitle(200, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _themeSelectTitle(200, 16, 25, fontRepository->GetFont(FontType::Medium11))
{
    _viewModel->SetAppSettingsService(appSettingsService);

    _titleLabel.SetText(Localization::Translate("display_settings"));
    AddChildTail(&_titleLabel);
    _layoutLabel.SetText(Localization::Translate("layout"));
    AddChildTail(&_layoutLabel);
    _sortingLabel.SetText(Localization::Translate("sorting"));
    AddChildTail(&_sortingLabel);
    _themeLabel.SetText(Localization::Translate("theme"));
    AddChildTail(&_themeLabel);
    _languageLabel.SetText(Localization::Translate("language"));
    AddChildTail(&_languageLabel);
    _darkModeLabel.SetText(Localization::Translate("dark_mode"));
    AddChildTail(&_darkModeLabel);
    _bgmLabel.SetText(Localization::Translate("bgm"));
    AddChildTail(&_bgmLabel);

    // Theme chip: show current theme name from settings
    {
        const char* themeName = appSettingsService->GetAppSettings().theme.GetString();
        char16_t tbuf[32];
        int tp = 0;
        while (themeName[tp] && tp < 30) { tbuf[tp] = (char16_t)(unsigned char)themeName[tp]; tp++; }
        tbuf[tp] = 0;
        _themeChip.SetText(tbuf);
    }
    _themeChip.SetSelected(true);
    AddChildTail(&_themeChip);

    _languageChip.SetText(_viewModel->IsChinese() ? u"\u4E2D\u6587" : u"English");
    _languageChip.SetSelected(true);
    AddChildTail(&_languageChip);

    _darkModeChip.SetText(_viewModel->GetDarkMode() ? u"On" : u"Off");
    _darkModeChip.SetSelected(_viewModel->GetDarkMode());
    AddChildTail(&_darkModeChip);

    // BGM chip
    ScanBgmFiles();
    UpdateBgmChipText();
    _bgmChip.SetSelected(true);
    AddChildTail(&_bgmChip);

    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption = CreateLayoutOptionIconButton();
        AddChildTail(&layoutOption);
    }

    for (auto& sortOption : _sortOptions)
    {
        sortOption = CreateSortOptionIconButton();
        AddChildTail(&sortOption);
    }

    // BGM select title (always child, positioned offscreen when not active)
    _bgmSelectTitle.SetText(Localization::Translate("select_bgm"));
    AddChildTail(&_bgmSelectTitle);

    // Theme select title
    _themeSelectTitle.SetText(Localization::Translate("select_theme"));
    AddChildTail(&_themeSelectTitle);
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

void DisplaySettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    _objVramManager = vramContext.GetObjVramManager();

    if (_usePreloadedIcons)
        return;

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
    }
}

void DisplaySettingsBottomSheetView::ScanBgmFiles()
{
    _bgmFileCount = 0;
    _bgmIndex = -1;

    NullFileTypeProvider fileTypeProvider;
    auto bgmFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath("/_pico/bgm");
    if (!bgmFolder) return;

    for (u32 i = 0; i < bgmFolder->GetFileCount() && _bgmFileCount < kMaxBgmFiles; i++)
    {
        const char* name = bgmFolder->GetFiles()[i]->GetFileName();
        if (name)
            _bgmFileNames[_bgmFileCount++] = name;
    }

    // Match current setting
    const char* current = _appSettingsService->GetAppSettings().bgm.GetString();
    if (current && current[0] != '\0')
    {
        for (int i = 0; i < _bgmFileCount; i++)
        {
            if (!strcasecmp(_bgmFileNames[i].GetString(), current))
            {
                _bgmIndex = i;
                break;
            }
        }
    }
}

void DisplaySettingsBottomSheetView::UpdateBgmChipText()
{
    if (_bgmIndex < 0 || _bgmIndex >= _bgmFileCount)
    {
        _bgmChip.SetText(u"Random");
        return;
    }
    // Strip prefix (3DS_ or DSi_) and .bcstm extension for display, replace '_' with ' '
    char16_t buf[32];
    const char* name = _bgmFileNames[_bgmIndex].GetString();
    int j = 0;
    // Skip 3DS_ or DSi_ prefix
    if ((name[0] == '3' && name[1] == 'D' && name[2] == 'S' && name[3] == '_') ||
        (name[0] == 'D' && name[1] == 'S' && name[2] == 'i' && name[3] == '_'))
        j = 4;
    int len = 0;
    while (name[j] && name[j] != '.' && len < 30)
    {
        buf[len] = (name[j] == '_') ? u' ' : (char16_t)(unsigned char)name[j];
        len++;
        j++;
    }
    buf[len] = 0;
    _bgmChip.SetText(buf);
}

void DisplaySettingsBottomSheetView::UpdateLabels()
{
    int s = _scrollOffset;
    _titleLabel.SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y - s);
    _layoutLabel.SetPosition(LAYOUT_LABEL_X, _position.y + LAYOUT_LABEL_Y - s);
    _sortingLabel.SetPosition(SORTING_LABEL_X, _position.y + SORTING_LABEL_Y - s);
    _themeLabel.SetPosition(20, _position.y + 110 - s);
    _languageLabel.SetPosition(20, _position.y + 142 - s);
    _darkModeLabel.SetPosition(20, _position.y + 174 - s);
    _bgmLabel.SetPosition(20, _position.y + 206 - s);
}

void DisplaySettingsBottomSheetView::Update()
{
    BottomSheetView::Update();

    if (_themeSelectMode || _bgmSelectMode)
    {
        // Hide all normal controls offscreen
        _titleLabel.SetPosition(-300, -300);
        _layoutLabel.SetPosition(-300, -300);
        _sortingLabel.SetPosition(-300, -300);
        _themeLabel.SetPosition(-300, -300);
        _languageLabel.SetPosition(-300, -300);
        _darkModeLabel.SetPosition(-300, -300);
        _bgmLabel.SetPosition(-300, -300);
        _themeChip.SetPosition(-300, -300);
        _languageChip.SetPosition(-300, -300);
        _darkModeChip.SetPosition(-300, -300);
        _bgmChip.SetPosition(-300, -300);
        for (auto& layoutOption : _layoutOptions)
            layoutOption.SetPosition(-300, -300);
        for (auto& sortOption : _sortOptions)
            sortOption.SetPosition(-300, -300);

        if (_themeSelectMode)
        {
            _themeSelectTitle.SetPosition(20, _position.y + 12);
            _bgmSelectTitle.SetPosition(-300, -300);
            if (_themeRecycler)
            {
                _themeRecycler->SetPosition(BGM_LIST_X, _position.y + BGM_LIST_Y);
                _themeRecycler->Update();
            }
            if (_bgmRecycler)
                _bgmRecycler->SetPosition(-300, -300);
        }
        else
        {
            _bgmSelectTitle.SetPosition(20, _position.y + 12);
            _themeSelectTitle.SetPosition(-300, -300);
            if (_bgmRecycler)
            {
                _bgmRecycler->SetPosition(BGM_LIST_X, _position.y + BGM_LIST_Y);
                _bgmRecycler->Update();
            }
            if (_themeRecycler)
                _themeRecycler->SetPosition(-300, -300);
        }
    }
    else
    {
        // Normal settings mode
        UpdateLabels();
        int s = _scrollOffset;
        auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();
        int x = 70;
        u32 idx = 0;
        for (auto& layoutOption : _layoutOptions)
        {
            layoutOption.SetPosition(x, _position.y + 38 - s);
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
            sortOption.SetPosition(x, _position.y + 70 - s);
            sortOption.SetState(sRomBrowserSortModes[idx] == selectedSortMode
                ? IconButtonView::State::ToggleSelected
                : IconButtonView::State::ToggleUnselected);
            x += 32;
            idx++;
        }

        _themeChip.SetPosition(70, _position.y + 106 - s);
        _languageChip.SetPosition(70, _position.y + 138 - s);
        _darkModeChip.SetPosition(70, _position.y + 170 - s);
        _bgmChip.SetPosition(70, _position.y + 202 - s);

        // Hide select titles offscreen
        _bgmSelectTitle.SetPosition(-300, -300);
        _themeSelectTitle.SetPosition(-300, -300);
        if (_themeRecycler)
            _themeRecycler->SetPosition(-300, -300);
    }
}

void DisplaySettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto bgColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        if (_themeSelectMode)
        {
            _themeSelectTitle.SetBackgroundColor(bgColor);
            _themeSelectTitle.SetForegroundColor(_materialColorScheme->onSurface);
            _themeSelectTitle.Draw(graphicsContext);

            if (_themeRecycler)
            {
                graphicsContext.SetClipArea(_themeRecycler->GetBounds());
                _themeRecycler->Draw(graphicsContext);
                graphicsContext.SetClipArea(GetBounds());
            }
        }
        else if (_bgmSelectMode)
        {
            _bgmSelectTitle.SetBackgroundColor(bgColor);
            _bgmSelectTitle.SetForegroundColor(_materialColorScheme->onSurface);
            _bgmSelectTitle.Draw(graphicsContext);

            if (_bgmRecycler)
            {
                graphicsContext.SetClipArea(_bgmRecycler->GetBounds());
                _bgmRecycler->Draw(graphicsContext);
                graphicsContext.SetClipArea(GetBounds());
            }
        }
        else
        {
            _titleLabel.SetBackgroundColor(bgColor);
            _titleLabel.SetForegroundColor(_materialColorScheme->onSurface);
            _layoutLabel.SetBackgroundColor(bgColor);
            _layoutLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _sortingLabel.SetBackgroundColor(bgColor);
            _sortingLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _themeLabel.SetBackgroundColor(bgColor);
            _themeLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _languageLabel.SetBackgroundColor(bgColor);
            _languageLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _darkModeLabel.SetBackgroundColor(bgColor);
            _darkModeLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _bgmLabel.SetBackgroundColor(bgColor);
            _bgmLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            BottomSheetView::Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void DisplaySettingsBottomSheetView::ScrollToFocus(View* target)
{
    int focusY = 0;
    if (target == &_themeChip) focusY = 106;
    else if (target == &_languageChip) focusY = 138;
    else if (target == &_darkModeChip) focusY = 170;
    else if (target == &_bgmChip) focusY = 202;
    else { _scrollOffset = 0; return; }

    int visibleHeight = 192 - _position.y;
    if (focusY + 28 - _scrollOffset > visibleHeight)
        _scrollOffset = focusY + 28 - visibleHeight;
    if (focusY - _scrollOffset < 38)
        _scrollOffset = focusY - 38;
    if (_scrollOffset < 0)
        _scrollOffset = 0;
}

void DisplaySettingsBottomSheetView::UpdateLanguageAndLabels()
{
    _viewModel->ToggleLanguage();

    // Re-initialize Localization for the new language
    Localization::Initialize(_appSettingsService);

    _languageChip.SetText(_viewModel->IsChinese() ? u"\u4E2D\u6587" : u"English");
    _titleLabel.SetText(Localization::Translate("display_settings"));
    _layoutLabel.SetText(Localization::Translate("layout"));
    _sortingLabel.SetText(Localization::Translate("sorting"));
    _themeLabel.SetText(Localization::Translate("theme"));
    _languageLabel.SetText(Localization::Translate("language"));
    _darkModeLabel.SetText(Localization::Translate("dark_mode"));
    _darkModeChip.SetText(_viewModel->GetDarkMode() ? u"On" : u"Off");
}

void DisplaySettingsBottomSheetView::ToggleDarkMode()
{
    _viewModel->ToggleDarkMode();
    bool dark = _viewModel->GetDarkMode();
    _darkModeChip.SetSelected(dark);
    _darkModeChip.SetText(dark ? u"On" : u"Off");
}

bool DisplaySettingsBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    _focusManager = &focusManager;

    if (_themeSelectMode)
    {
        if (inputProvider.Triggered(InputKey::B))
        {
            ExitThemeSelectMode(focusManager);
            return true;
        }
        if (inputProvider.Triggered(InputKey::A))
        {
            if (_themeRecycler && _themeAdapter)
            {
                int selectedIdx = _themeRecycler->GetSelectedItem();
                if (selectedIdx >= 0)
                    ApplyThemeSelection(selectedIdx);
            }
            return true;
        }
        return false;
    }

    if (_bgmSelectMode)
    {
        if (inputProvider.Triggered(InputKey::B))
        {
            ExitBgmSelectMode(focusManager);
            return true;
        }
        if (inputProvider.Triggered(InputKey::A))
        {
            if (_bgmRecycler && _bgmAdapter)
            {
                int selectedIdx = _bgmRecycler->GetSelectedItem();
                if (selectedIdx >= 0)
                {
                    if (_bgmAdapter->IsCategoryItem(selectedIdx))
                    {
                        _bgmAdapter->ToggleCategory(selectedIdx);
                        focusManager.Unfocus();
                        _bgmRecycler->SetAdapter(_bgmAdapter, selectedIdx);
                        if (_objVramManager)
                        {
                            ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);
                            _bgmRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
                        }
                        _bgmRecycler->Focus(focusManager);
                    }
                    else
                    {
                        int bgmFileIdx = _bgmAdapter->GetBgmFileIndex(selectedIdx);
                        ApplyBgmSelection(bgmFileIdx);
                    }
                }
            }
            return true;
        }
        // Let FocusManager handle up/down navigation automatically
        return false;
    }

    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    if (inputProvider.Triggered(InputKey::A))
    {
        auto focus = focusManager.GetCurrentFocus();
        if (focus == &_themeChip)
        {
            EnterThemeSelectMode(focusManager);
            return true;
        }
        else if (focus == &_languageChip)
        {
            UpdateLanguageAndLabels();
            return true;
        }
        else if (focus == &_darkModeChip)
        {
            ToggleDarkMode();
            return true;
        }
        else if (focus == &_bgmChip)
        {
            EnterBgmSelectMode(focusManager);
            return true;
        }
    }
    return false;
}

bool DisplaySettingsBottomSheetView::HandleTouch(
    const TouchEvent& event, FocusManager& focusManager)
{
    _focusManager = &focusManager;

    if (_themeSelectMode)
    {
        if (_themeRecycler)
            return _themeRecycler->HandleTouch(event, focusManager);
        return true;
    }

    if (_bgmSelectMode)
    {
        if (_bgmRecycler)
            return _bgmRecycler->HandleTouch(event, focusManager);
        return true;
    }

    // Touch drag scrolling
    if (event.type == TouchEventType::Move)
    {
        _scrollOffset -= event.deltaY;
        if (_scrollOffset < 0) _scrollOffset = 0;
        int maxScroll = 232 - (192 - _position.y);
        if (maxScroll < 0) maxScroll = 0;
        if (_scrollOffset > maxScroll) _scrollOffset = maxScroll;
        return true;
    }

    // Only handle taps (short touch-up)
    if (event.type != TouchEventType::Up || event.holdFrames > 24)
        return false;

    // Ignore if it was a drag gesture
    int totalDelta = event.totalDeltaX * event.totalDeltaX + event.totalDeltaY * event.totalDeltaY;
    if (totalDelta > 64)
        return false;

    // Check layout option taps
    for (u32 i = 0; i < _layoutOptions.size(); i++)
    {
        if (_layoutOptions[i].GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_layoutOptions[i]);
            _viewModel->SetRomBrowserDisplayMode(sRomBrowserDisplayModes[i]);
            return true;
        }
    }

    // Check sort option taps
    for (u32 i = 0; i < _sortOptions.size(); i++)
    {
        if (_sortOptions[i].GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_sortOptions[i]);
            _viewModel->SetRomBrowserSortMode(sRomBrowserSortModes[i]);
            return true;
        }
    }

    // Check chip taps
    if (_themeChip.GetBounds().Contains(event.position))
    {
        focusManager.Focus(&_themeChip);
        EnterThemeSelectMode(focusManager);
        return true;
    }
    if (_languageChip.GetBounds().Contains(event.position))
    {
        focusManager.Focus(&_languageChip);
        UpdateLanguageAndLabels();
        return true;
    }
    if (_darkModeChip.GetBounds().Contains(event.position))
    {
        focusManager.Focus(&_darkModeChip);
        ToggleDarkMode();
        return true;
    }
    if (_bgmChip.GetBounds().Contains(event.position))
    {
        focusManager.Focus(&_bgmChip);
        EnterBgmSelectMode(focusManager);
        return true;
    }

    return false;
}

void DisplaySettingsBottomSheetView::OnDismissed()
{
    _viewModel->SaveSettingsNow();
    _viewModel->Close();
}

View* DisplaySettingsBottomSheetView::MoveFocus(View* currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (_themeSelectMode && source == _themeRecycler.get())
        return nullptr;
    if (_bgmSelectMode && source == _bgmRecycler.get())
        return nullptr;

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
            else // Down
            {
                View* r = &_themeChip;
                ScrollToFocus(r);
                return r;
            }
        }
        idx++;
    }
    if (currentFocus == &_themeChip)
    {
        if (direction == FocusMoveDirection::Up)
        { _scrollOffset = 0; return &_sortOptions[0]; }
        View* r = &_languageChip;
        ScrollToFocus(r);
        return r;
    }
    if (currentFocus == &_languageChip)
    {
        if (direction == FocusMoveDirection::Up)
        { View* r = &_themeChip; ScrollToFocus(r); return r; }
        View* r = &_darkModeChip;
        ScrollToFocus(r);
        return r;
    }
    if (currentFocus == &_darkModeChip)
    {
        if (direction == FocusMoveDirection::Up)
        { View* r = &_languageChip; ScrollToFocus(r); return r; }
        View* r = &_bgmChip;
        ScrollToFocus(r);
        return r;
    }
    if (currentFocus == &_bgmChip)
    {
        if (direction == FocusMoveDirection::Up)
        { View* r = &_darkModeChip; ScrollToFocus(r); return r; }
        _scrollOffset = 0;
        return &_layoutOptions[0];
    }
    return nullptr;
}

void DisplaySettingsBottomSheetView::SetGraphics(
    const IconButton2DView::VramToken& iconButtonVramToken)
{
    for (auto& layoutOption : _layoutOptions)
        layoutOption.SetGraphics(iconButtonVramToken);
    for (auto& sortOption : _sortOptions)
        sortOption.SetGraphics(iconButtonVramToken);
}

void DisplaySettingsBottomSheetView::SetChipGraphics(
    const ChipView::VramToken& chipViewVramToken)
{
    _themeChip.SetGraphics(chipViewVramToken);
    _languageChip.SetGraphics(chipViewVramToken);
    _darkModeChip.SetGraphics(chipViewVramToken);
    _bgmChip.SetGraphics(chipViewVramToken);
}

void DisplaySettingsBottomSheetView::SetIconGraphics(const IconVramToken& iconVramToken)
{
    _usePreloadedIcons = true;
    for (int i = 0; i < 4; i++)
        _layoutOptions[i].SetIconVramOffset(iconVramToken.GetLayoutOffset(i));
    for (int i = 0; i < 2; i++)
        _sortOptions[i].SetIconVramOffset(iconVramToken.GetSortOffset(i));
}

DisplaySettingsBottomSheetView::IconVramToken
DisplaySettingsBottomSheetView::UploadIconGraphics(IVramManager& vramManager)
{
    auto load = [&](const unsigned int* tiles, u32 tilesLen)
    {
        u32 offset = vramManager.Alloc(tilesLen);
        dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(offset), tilesLen);
        return offset;
    };

    return IconVramToken(
        load(hGridIconTiles, hGridIconTilesLen),
        load(vGridIconTiles, vGridIconTilesLen),
        load(bannerListIconTiles, bannerListIconTilesLen),
        load(coverflowIconTiles, coverflowIconTilesLen),
        load(sortNameAscendingIconTiles, sortNameAscendingIconTilesLen),
        load(sortNameDescendingIconTiles, sortNameDescendingIconTilesLen)
    );
}

u32 DisplaySettingsBottomSheetView::LoadIcon(IVramManager& vramManager,
    const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}

void DisplaySettingsBottomSheetView::EnterBgmSelectMode(FocusManager& focusManager)
{
    _bgmSelectMode = true;
    _focusManager = &focusManager;

    // Compute list height: fill from BGM_LIST_Y to bottom of screen
    // Round down to multiple of 16 (item height) to avoid partial items
    int listHeight = 192 - _position.y - BGM_LIST_Y - 4;
    listHeight = (listHeight / 16) * 16;
    if (listHeight < 16) listHeight = 16;

    if (!_bgmRecycler)
    {
        // First time: create RecyclerView and add as child
        _bgmRecycler = std::make_unique<RecyclerView>(
            BGM_LIST_X, _position.y + BGM_LIST_Y, 224, listHeight, RecyclerView::Mode::VerticalList);
        _bgmRecycler->SetShoulderPagingEnabled(false);
        _bgmRecycler->SetTouchTapCallback([](int itemIdx, void* arg)
        {
            auto* self = static_cast<DisplaySettingsBottomSheetView*>(arg);
            if (self->_bgmAdapter && self->_bgmAdapter->IsCategoryItem(itemIdx))
            {
                self->_bgmAdapter->ToggleCategory(itemIdx);
                if (self->_focusManager)
                    self->_focusManager->Unfocus();
                self->_bgmRecycler->SetAdapter(self->_bgmAdapter, itemIdx);
                if (self->_objVramManager)
                {
                    ((DescendingStackVramManager*)self->_objVramManager)->SetState(self->_savedVramState);
                    self->_bgmRecycler->InitVram(VramContext(nullptr, self->_objVramManager, nullptr, nullptr));
                }
                if (self->_focusManager)
                    self->_bgmRecycler->Focus(*self->_focusManager);
            }
            else if (self->_bgmAdapter)
            {
                int bgmFileIdx = self->_bgmAdapter->GetBgmFileIndex(itemIdx);
                self->ApplyBgmSelection(bgmFileIdx);
            }
        }, this);
        AddChildTail(_bgmRecycler.get());
    }

    // Create adapter
    if (_bgmAdapter)
    {
        delete _bgmAdapter;
        _bgmAdapter = nullptr;
    }
    _bgmAdapter = new BgmAdapter(_bgmFileNames, _bgmFileCount, _materialColorScheme, _fontRepository, _bgmIndex);

    // Find the flat index for the currently selected BGM in the tree structure
    int initialIndex = 0; // default to Random
    if (_bgmIndex >= 0 && _bgmIndex < _bgmFileCount)
    {
        // Search through all flat indices to find the one matching _bgmIndex
        int count = (int)_bgmAdapter->GetItemCount();
        for (int i = 0; i < count; i++)
        {
            if (_bgmAdapter->GetBgmFileIndex(i) == _bgmIndex)
            {
                initialIndex = i;
                break;
            }
        }
    }

    focusManager.Unfocus();
    _bgmRecycler->SetAdapter(_bgmAdapter, initialIndex);

    if (_objVramManager)
    {
        _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();
        _bgmRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
    }
    _bgmRecycler->Focus(focusManager);
}

void DisplaySettingsBottomSheetView::ExitBgmSelectMode(FocusManager& focusManager)
{
    focusManager.Unfocus();

    if (_objVramManager)
    {
        ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);
    }

    // Hide recycler offscreen (can't remove from child list, so reuse it)
    if (_bgmRecycler)
        _bgmRecycler->SetPosition(-300, -300);

    _bgmSelectMode = false;
    focusManager.Focus(&_bgmChip);
    ScrollToFocus(&_bgmChip);
}

void DisplaySettingsBottomSheetView::ApplyBgmSelection(int index)
{
    // index: -1 = Random, 0+ = file index in _bgmFileNames
    if (index < 0)
        _bgmIndex = -1;
    else
        _bgmIndex = index;

    UpdateBgmChipText();

    auto& settings = _appSettingsService->GetAppSettings();
    if (_bgmIndex >= 0 && _bgmIndex < _bgmFileCount)
        settings.bgm = _bgmFileNames[_bgmIndex].GetString();
    else
        settings.bgm = "";
    _viewModel->SaveSettingsNow();
    _viewModel->RequestThemeReload();
    _viewModel->Close();
}

void DisplaySettingsBottomSheetView::EnterThemeSelectMode(FocusManager& focusManager)
{
    _themeSelectMode = true;
    _focusManager = &focusManager;

    int listHeight = 192 - _position.y - BGM_LIST_Y - 4;
    listHeight = (listHeight / 16) * 16;
    if (listHeight < 16) listHeight = 16;

    if (!_themeRecycler)
    {
        _themeRecycler = std::make_unique<RecyclerView>(
            BGM_LIST_X, _position.y + BGM_LIST_Y, 224, listHeight, RecyclerView::Mode::VerticalList);
        _themeRecycler->SetShoulderPagingEnabled(false);
        _themeRecycler->SetTouchTapCallback([](int itemIdx, void* arg)
        {
            auto* self = static_cast<DisplaySettingsBottomSheetView*>(arg);
            self->ApplyThemeSelection(itemIdx);
        }, this);
        AddChildTail(_themeRecycler.get());
    }

    if (_themeAdapter)
    {
        delete _themeAdapter;
        _themeAdapter = nullptr;
    }
    const char* currentTheme = _appSettingsService->GetAppSettings().theme.GetString();
    _themeAdapter = new ThemeAdapter(_materialColorScheme, _fontRepository, currentTheme);

    int initialIndex = _themeAdapter->GetCurrentIndex();
    if (initialIndex < 0) initialIndex = 0;

    focusManager.Unfocus();
    _themeRecycler->SetAdapter(_themeAdapter, initialIndex);

    if (_objVramManager)
    {
        _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();
        _themeRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
    }
    _themeRecycler->Focus(focusManager);
}

void DisplaySettingsBottomSheetView::ExitThemeSelectMode(FocusManager& focusManager)
{
    focusManager.Unfocus();

    if (_objVramManager)
        ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);

    if (_themeRecycler)
        _themeRecycler->SetPosition(-300, -300);

    _themeSelectMode = false;
    focusManager.Focus(&_themeChip);
    ScrollToFocus(&_themeChip);
}

void DisplaySettingsBottomSheetView::ApplyThemeSelection(int index)
{
    if (!_themeAdapter) return;
    auto& settings = _appSettingsService->GetAppSettings();
    settings.theme = _themeAdapter->GetThemeName(index);
    _viewModel->SaveSettingsNow();
    _viewModel->RequestThemeReload();
    _viewModel->Close();
}

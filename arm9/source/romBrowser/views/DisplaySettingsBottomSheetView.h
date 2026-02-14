#pragma once
#include <array>
#include "core/String.h"
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "IconButton2DView.h"
#include "../viewModels/DisplaySettingsViewModel.h"
#include "services/settings/IAppSettingsService.h"

class IRomBrowserController;
class MaterialColorScheme;
class IFontRepository;

class DisplaySettingsBottomSheetView : public BottomSheetView
{
public:
    DisplaySettingsBottomSheetView(DisplaySettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        IAppSettingsService* appSettingsService);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    View* MoveFocus(View* currentFocus,
        FocusMoveDirection direction, View* source) override;

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(&_layoutOptions[0]);
    }

private:
    DisplaySettingsViewModel* _viewModel;
    IAppSettingsService* _appSettingsService;

    Label2DView _titleLabel;
    Label2DView _layoutLabel;
    Label2DView _sortingLabel;
    Label2DView _themeLabel;
    Label2DView _themeValueLabel;
    Label2DView _languageLabel;
    Label2DView _languageValueLabel;
    // LabelView _filtersLabel;

    std::array<IconButton2DView, 4> _layoutOptions;
    std::array<IconButton2DView, /*3*/2> _sortOptions;
    // std::array<IconButton2DView, 5> _filterOptions;

    const MaterialColorScheme* _materialColorScheme;

    static constexpr int kMaxThemeCount = 16;
    std::array<String<char, 64>, kMaxThemeCount> _themeNames;
    int _themeCount = 0;
    int _selectedThemeIdx = 0;

    static constexpr int kLanguageCount = 6;
    static constexpr const char* sLanguages[kLanguageCount] = {"english", "italian", "spanish", "french", "german", "portuguese"};
    static constexpr const char16_t* sLanguageNames[kLanguageCount] = {u"English", u"Italiano", u"Español", u"Français", u"Deutsch", u"Português"};
    int _selectedLanguageIdx = 0;

    IconButton2DView CreateLayoutOptionIconButton();
    IconButton2DView CreateSortOptionIconButton();
    // IconButton2DView CreateFilterOptionIconButton();

    void UpdateLabels();
    void LoadThemes();
    void UpdateThemeUI();
    void ChangeTheme(int newIdx);
    void UpdateLanguageUI();
    void ChangeLanguage(int newIdx);
    void SaveIfDirty();
    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;

    bool _settingsDirty = false;
};

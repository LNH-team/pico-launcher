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
class IVramManager;
struct TouchEvent;

class DisplaySettingsBottomSheetView : public BottomSheetView
{
public:
    class IconVramToken
    {
        u32 _layoutOffsets[4];
        u32 _sortOffsets[2];
    public:
        IconVramToken()
            : _layoutOffsets { 0, 0, 0, 0 }
            , _sortOffsets { 0, 0 } { }

        IconVramToken(u32 layout0, u32 layout1, u32 layout2, u32 layout3,
            u32 sort0, u32 sort1)
            : _layoutOffsets { layout0, layout1, layout2, layout3 }
            , _sortOffsets { sort0, sort1 } { }

        constexpr u32 GetLayoutOffset(int idx) const { return _layoutOffsets[idx]; }
        constexpr u32 GetSortOffset(int idx) const { return _sortOffsets[idx]; }
    };

    /// @param appliedThemeName  The theme that is actually running

    DisplaySettingsBottomSheetView(DisplaySettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        IAppSettingsService* appSettingsService, const char* appliedThemeName);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;
    void OnDismissed() override;
    View* MoveFocus(View* currentFocus,
        FocusMoveDirection direction, View* source) override;

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);
    void SetIconGraphics(const IconVramToken& iconVramToken);

    static IconVramToken UploadIconGraphics(IVramManager& vramManager);

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

    String<char, 64> _appliedThemeName;

    static constexpr int kMaxThemeCount = 16;
    std::array<String<char, 64>, kMaxThemeCount> _themeNames;
    int _themeCount = 0;
    int _selectedThemeIdx = 0;
    bool _themesLoaded = false;
    String<char, 64> _pendingThemeName;

    static constexpr int kSettleFrames = 30;
    int _themeSettleCounter = 0;
    int _languageSettleCounter = 0;

    static constexpr int kMaxLanguageCount = 16;
    struct LanguageEntry
    {
        String<char, 64> fileName;
        char16_t displayName[64];
    };
    std::array<LanguageEntry, kMaxLanguageCount> _languageEntries;
    int _languageCount = 0;
    int _selectedLanguageIdx = 0;
    bool _languagesLoaded = false;
    String<char, 64> _pendingLanguageName;

    IconButton2DView CreateLayoutOptionIconButton();
    IconButton2DView CreateSortOptionIconButton();
    // IconButton2DView CreateFilterOptionIconButton();

    void UpdateLabels();
    void LoadThemes();
    void EnsureThemesLoaded();
    void UpdateThemeUI();
    void ChangeTheme(int newIdx);
    void ApplyTheme();
    void LoadLanguages();
    void EnsureLanguagesLoaded();
    void UpdateLanguageUI();
    void ChangeLanguage(int newIdx);
    void ReleaseLazyLists();
    void SaveIfDirty();
    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;

    bool _settingsDirty = false;
    bool _themeLongPressConsumed = false;
    bool _usePreloadedIcons = false;
};

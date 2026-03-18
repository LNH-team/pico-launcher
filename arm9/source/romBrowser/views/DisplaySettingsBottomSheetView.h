#pragma once
#include <array>
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "IconButton2DView.h"
#include "ChipView.h"
#include "../viewModels/DisplaySettingsViewModel.h"
#include "services/settings/IAppSettingsService.h"
#include "bgm/IBgmService.h"

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

    DisplaySettingsBottomSheetView(DisplaySettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        IAppSettingsService* appSettingsService, IBgmService* bgmService,
        const char* appliedThemeName);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;
    void OnDismissed() override;
    View* MoveFocus(View* currentFocus,
        FocusMoveDirection direction, View* source) override;

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);
    void SetChipGraphics(const ChipView::VramToken& chipViewVramToken);
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
    Label2DView _languageLabel;
    Label2DView _darkModeLabel;
    Label2DView _bgmLabel;

    std::array<IconButton2DView, 4> _layoutOptions;
    std::array<IconButton2DView, /*3*/2> _sortOptions;

    ChipView _themeChip;
    ChipView _languageChip;
    ChipView _darkModeChip;
    ChipView _bgmChip;

    int _scrollOffset = 0;

    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IBgmService* _bgmService;

    // BGM file list (fixed array, no heap allocation)
    static constexpr int kMaxBgmFiles = 64;
    String<char, 128> _bgmFileNames[kMaxBgmFiles];
    int _bgmFileCount = 0;
    int _bgmIndex = -1; // -1 = Random

    IconButton2DView CreateLayoutOptionIconButton();
    IconButton2DView CreateSortOptionIconButton();

    void UpdateLabels();
    void UpdateLanguageAndLabels();
    void ToggleDarkMode();
    void CycleBgm(bool forward);
    void UpdateBgmChipText();
    void ScanBgmFiles();
    void ScrollToFocus(View* target);

    bool _usePreloadedIcons = false;

    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};

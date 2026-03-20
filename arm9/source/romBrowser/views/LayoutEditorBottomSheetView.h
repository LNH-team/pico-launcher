#pragma once
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "services/Layout/LayoutService.h"
#include "themes/material/MaterialColorScheme.h"
#include "core/String.h"

class IRomBrowserController;
class IFontRepository;
class IAppSettingsService;
struct TouchEvent;

class LayoutEditorBottomSheetView : public BottomSheetView
{
public:
    /// @param appliedThemeName   The theme actually running
    /// @param initialColorR/G/B  Primary color from state.bin 
    /// @param initialDarkTheme   Dark-theme flag from state.bin
    LayoutEditorBottomSheetView(
        IRomBrowserController* controller,
        LayoutService* layoutService,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        IAppSettingsService* appSettingsService,
        const char* appliedThemeName,
        u8 initialColorR, u8 initialColorG, u8 initialColorB, bool initialDarkTheme);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override { return false; }
    void OnDismissed() override;
    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override { return nullptr; }
    void Focus(FocusManager& focusManager) override;

private:
    IRomBrowserController*     _controller;
    LayoutService*             _layoutService;
    IAppSettingsService*       _appSettingsService;
    /// The theme actually loaded 
    String<char, 64>           _appliedThemeName;

    mutable bool _themeColorLoaded = false;
    mutable bool _themeDarkMode = false;
    mutable int  _themeColorR = 0;
    mutable int  _themeColorG = 0;
    mutable int  _themeColorB = 0;
    bool         _themeColorDirty = false;
    bool         _themePreviewApplied = false;
    const MaterialColorScheme* _materialColorScheme;

    Label2DView _titleLabel;
    Label2DView _slotLabel;
    Label2DView _saveLabel;
    Label2DView _resetLabel;
    Label2DView _resetMenuLabel;
    Label2DView _subMenuLabel;
    Label2DView _itemName0;
    Label2DView _itemName1;
    Label2DView _itemName2;
    Label2DView _itemName3;
    Label2DView _itemName4;
    Label2DView _itemName5;
    Label2DView _itemName6;
    Label2DView _itemName7;
    Label2DView _itemName8;
    Label2DView _itemValue0;
    Label2DView _itemValue1;
    Label2DView _itemValue2;
    Label2DView _itemValue3;
    Label2DView _itemValue4;
    Label2DView _itemValue5;
    Label2DView _itemValue6;
    Label2DView _itemValue7;
    Label2DView _itemValue8;
    Label2DView _scrollHintLabel;
    MaterialColorScheme _originalMaterialColorScheme;

    static constexpr int kFocusSlot       = 0;
    static constexpr int kFocusSave       = 1;
    static constexpr int kFocusReset      = 2;
    static constexpr int kFocusResetMenu  = 3;
    static constexpr int kFocusSubMenu    = 4;
    static constexpr int kFocusItem0      = 5;
    static constexpr int kNumVisibleItems = 8;

    enum class ChoiceKind : u8
    {
        None,
        Slot,
        SubMenu,
        Format,
        Separator,
        Font,
        PrefixGbaMode,
        PrefixNtrMode,
        PrefixTwlMode,
    };

    int  _focusRow         = kFocusSlot;
    int  _topActionFocus   = kFocusSlot;
    int  _itemScrollOffset = 0;
    int  _currentSubMenu   = 0;
    int  _romNameSelectedLine = 0;

    bool      _isChoosing      = false;
    ChoiceKind _choiceKind      = ChoiceKind::None;
    int       _choiceSelected  = 0;
    int       _choiceScroll    = 0;
    int       _choiceCount     = 0;
    int       _choiceTargetIdx = -1;
    int       _focusBeforeChoice = kFocusSlot;

    u16  _holdLeft  = 0;
    u16  _holdRight = 0;
    u16  _holdUp    = 0;
    u16  _holdDown  = 0;

    Label2DView* GetItemNameLabel(int visRow);
    Label2DView* GetItemValueLabel(int visRow);

    int         GetSubMenuCount() const;
    const char* GetSubMenuName(int uiSubMenu) const;
    int         GetSubMenuItemCount(int subMenu) const;
    const char* GetItemName(int subMenu, int itemIdx) const;
    void        GetItemValueText(int subMenu, int itemIdx, char* buf, u32 bufLen) const;
    bool        IsNonFocusableItem(int subMenu, int itemIdx) const;
    int         GetNextFocusableItem(int subMenu, int itemIdx, int dir) const;
    int         GetChildIndentX(int subMenu, int itemIdx) const;
    void        ChangeItemValue(int subMenu, int itemIdx, int delta);
    bool        GetChoiceListForCurrentFocus(ChoiceKind& kind, int& count, int& selectedValue) const;
    const char* GetChoiceLabel(ChoiceKind kind, int choiceIdx) const;
    void        ApplyChoiceValue(ChoiceKind kind, int valueIdx);
    void        BeginChoice(ChoiceKind kind, int count, int selectedValue, int targetIdx);
    void        CancelChoice();
    void        ConfirmChoice();

    void        EnsureItemFocusVisible();
    void        EnsureThemeColorLoaded() const;
    void        ApplyThemeColorPreview();
    void        RestoreThemeColorPreview();
    void        RefreshThemeBackgroundPalettes();
    bool        SaveThemeColorToFile() const;
    static int  ClampInt(int value, int minValue, int maxValue);

    void UpdateAllLabels();
    void ClampItemScroll();
};

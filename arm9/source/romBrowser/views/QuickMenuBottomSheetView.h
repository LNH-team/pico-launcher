#pragma once
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"
#include "gui/views/Label2DView.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"

class IRomBrowserController;
struct TouchEvent;

class QuickMenuBottomSheetView : public BottomSheetView
{
public:
    QuickMenuBottomSheetView(
        IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        bool hasSelectedRom, bool isNdsRom);

    void SetGraphics(const ChipView::VramToken& chipVramToken);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;
    void OnDismissed() override;

    void Focus(FocusManager& focusManager) override;
    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;

    int GetScrimTargetBlend() const override { return 5; }

    Rectangle GetBounds() const override
    {
        return _panelBounds;
    }

    Rectangle GetFullyCoveredArea() const override { return Rectangle(0, 0, 0, 0); }

private:
    static constexpr int kPanelMarginLeft = 4;
    static constexpr int kPanelWidth = 180;
    static constexpr int kPanelPaddingX = 6;
    static constexpr int kPanelPaddingTop = 8;
    static constexpr int kPanelPaddingBottom = 6;
    static constexpr int kTitleHeight = 16;
    static constexpr int kTitleSpacing = 2;
    static constexpr int kRowSpacing = 2;
    static constexpr int kChipWidth = kPanelWidth - 2 * kPanelPaddingX;
    static constexpr int kTitleWidth = kChipWidth;

    enum class MenuItem
    {
        GameDetails,
        Cheats,
        DisplaySettings,
        LayoutEditor,
        Information,
        Count
    };

    static constexpr int kMaxChips = 5;

    IRomBrowserController* _romBrowserController;
    const MaterialColorScheme* _materialColorScheme;

    ChipView _chips[kMaxChips];
    MenuItem _chipMenuItem[kMaxChips];
    int _chipCount = 0;
    ChipView::VramToken _chipVramToken;
    u32 _iconVramOffsets[static_cast<int>(MenuItem::Count)] = {};
    Rectangle _panelBounds{0, 0, 180, 160};

    void ActivateChip(int index);
    int FindChipIndex(const View* view) const;
};

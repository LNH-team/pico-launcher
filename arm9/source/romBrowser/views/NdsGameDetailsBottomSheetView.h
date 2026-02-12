#pragma once
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"

class IRomBrowserController;
class IFontRepository;

class NdsGameDetailsBottomSheetView : public BottomSheetView
{
public:
    NdsGameDetailsBottomSheetView(
        IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);

    void SetGraphics(const ChipView::VramToken& chipVramToken)
    {
        _cheatsChip.SetGraphics(chipVramToken);
        _favoriteChip.SetGraphics(chipVramToken);
    }

    void InitVram(const VramContext& vramContext) override;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(&_cheatsChip);
    }

    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

private:
    IRomBrowserController* _romBrowserController;
    u32 _smallHeartIconVramOffset;
    u32 _smallHeartIconFilledVramOffset;
    ChipView _cheatsChip;
    ChipView _favoriteChip;
    bool _isFavorite = false;

    void UpdateFavoriteChipIcon()
    {
        _favoriteChip.SetSelected(_isFavorite);
        u32 iconOffset = _isFavorite ? _smallHeartIconFilledVramOffset : _smallHeartIconVramOffset;
        _favoriteChip.SetIcon(true, iconOffset);
    }
};
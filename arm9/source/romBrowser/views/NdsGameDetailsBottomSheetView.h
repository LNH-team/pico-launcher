#pragma once
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"

class IRomBrowserController;
class IFontRepository;

class NdsGameDetailsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(NdsGameDetailsBottomSheetView)

public:
    void SetGraphics(const ChipView::VramToken& chipVramToken)
    {
        _cheatsChip->SetGraphics(chipVramToken);
        _favoriteChip->SetGraphics(chipVramToken);
    }

    void InitVram(const VramContext& vramContext) override;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_cheatsChip);
    }

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    IRomBrowserController* _romBrowserController;
    u32 _smallHeartIconVramOffset;
    u32 _smallHeartIconFilledVramOffset;
    SharedPtr<ChipView> _cheatsChip;
    SharedPtr<ChipView> _favoriteChip;

    NdsGameDetailsBottomSheetView(
        IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);
};
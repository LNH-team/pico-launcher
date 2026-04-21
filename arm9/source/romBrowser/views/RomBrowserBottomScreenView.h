#pragma once
#include "AppBarView.h"
#include "gui/views/View.h"
#include "gui/views/RecyclerView.h"
#include "../FileRecyclerAdapter.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "RomBrowserView.h"
#include "RomBrowserAppBarView.h"
#include "../viewModels/RomBrowserBottomScreenViewModel.h"

class IRomBrowserViewFactory;
class VBlankTextureLoader;

class RomBrowserBottomScreenView : public View
{
    SHARED_ONLY(RomBrowserBottomScreenView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    void Focus(FocusManager& focusManager)
    {
        if (!_romBrowserView || !_romBrowserView->Focus(focusManager))
        {
            _romBrowserAppBarView->Focus(focusManager);
        }
    }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    void RomBrowserViewModelInvalidated(const VramContext& vramContext);

    bool IsAppBarFocused(const FocusManager& focusManager) const
    {
        return focusManager.IsFocusInside(_romBrowserAppBarView.GetPointer());
    }

private:
    RomBrowserBottomScreenViewModel* _viewModel;
    const IRomBrowserViewFactory* _romBrowserViewFactory;

    const RomBrowserDisplayMode* _romBrowserDisplayMode;
    const IThemeFileIconFactory* _themeFileIconFactory;
    SharedPtr<RomBrowserAppBarView> _romBrowserAppBarView;
    SharedPtr<RomBrowserView> _romBrowserView;
    VBlankTextureLoader* _vblankTextureLoader;

    RomBrowserBottomScreenView(
        RomBrowserBottomScreenViewModel* viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        VBlankTextureLoader* vblankTextureLoader);
};

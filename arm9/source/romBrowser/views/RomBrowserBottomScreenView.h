#pragma once
#include "AppBarView.h"
#include "gui/views/View.h"
#include "gui/views/RecyclerView.h"
#include "../FileRecyclerAdapter.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "RomBrowserView.h"
#include "RomBrowserAppBarView.h"
#include "core/SharedPtr.h"
#include "../viewModels/RomBrowserBottomScreenViewModel.h"

class IRomBrowserViewFactory;
class VBlankTextureLoader;

class RomBrowserBottomScreenView : public View
{
public:
    RomBrowserBottomScreenView(
        RomBrowserBottomScreenViewModel* viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        VBlankTextureLoader* vblankTextureLoader);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;

    void Focus(FocusManager& focusManager)
    {
        if (!_romBrowserView || !_romBrowserView->Focus(focusManager))
            _romBrowserAppBarView.Focus(focusManager);
    }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;

    void FocusAppBar(FocusManager& focusManager, RomBrowserAppBarView::AppBarButton button)
    {
        _romBrowserAppBarView.Focus(focusManager, button);
    }

    RomBrowserAppBarView::AppBarButton GetFocusedAppBarButton(const FocusManager& focusManager) const
    {
        return _romBrowserAppBarView.GetFocusedButton(focusManager);
    }

    bool IsViewInsideRomBrowser(const View* view) const;

    void RomBrowserViewModelInvalidated(const VramContext& vramContext);

    bool IsAppBarFocused(const FocusManager& focusManager) const
    {
        return focusManager.IsFocusInside(&_romBrowserAppBarView);
    }

private:
    RomBrowserBottomScreenViewModel* _viewModel;
    const IRomBrowserViewFactory* _romBrowserViewFactory;

    const RomBrowserDisplayMode* _romBrowserDisplayMode;
    const IThemeFileIconFactory* _themeFileIconFactory;
    RomBrowserAppBarView _romBrowserAppBarView;
    std::unique_ptr<RomBrowserView> _romBrowserView;
    VBlankTextureLoader* _vblankTextureLoader;

    View* _touchCaptureChild = nullptr;
};

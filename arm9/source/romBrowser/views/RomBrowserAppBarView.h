#pragma once
#include "gui/views/View.h"
#include "AppBarView.h"
#include "../viewModels/RomBrowserAppBarViewModel.h"

class RomBrowserDisplayMode;
class IRomBrowserViewFactory;

class RomBrowserAppBarView : public View
{
public:
    RomBrowserAppBarView(
        RomBrowserAppBarViewModel* viewModel, const RomBrowserDisplayMode& displayMode,
        const IRomBrowserViewFactory* romBrowserViewFactory);

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

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
        _appBarView->Focus(focusManager, 0);
    }


public:
    enum AppBarButton
    {
        APP_BAR_BUTTON_BACK = 0,
        APP_BAR_BUTTON_FAVORITES,
        APP_BAR_BUTTON_SORT_MODE,
        APP_BAR_BUTTON_LAYOUT,
        APP_BAR_BUTTON_DISPLAY_SETTINGS
    };

    void Focus(FocusManager& focusManager, AppBarButton button)
    {
        _appBarView->Focus(focusManager, button);
    }

    AppBarButton GetFocusedButton(const FocusManager& focusManager) const
    {
        const auto* focusedView = focusManager.GetCurrentFocus();
        if (!focusedView)
            return APP_BAR_BUTTON_BACK;

        int buttonIndex = _appBarView->GetButtonIndex(focusedView);
        if (buttonIndex < APP_BAR_BUTTON_BACK || buttonIndex > APP_BAR_BUTTON_DISPLAY_SETTINGS)
            return APP_BAR_BUTTON_BACK;

        return static_cast<AppBarButton>(buttonIndex);
    }

    RomBrowserAppBarViewModel* _viewModel;
    std::unique_ptr<AppBarView> _appBarView;

    u32 _layoutIconHorizontalGridVramOffset = 0;
    u32 _layoutIconVerticalGridVramOffset = 0;
    u32 _layoutIconBannerListVramOffset = 0;
    u32 _layoutIconCoverFlowVramOffset = 0;
    u32 _sortIconNameAscendingVramOffset = 0;
    u32 _sortIconNameDescendingVramOffset = 0;
};

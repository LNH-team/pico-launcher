#pragma once
#include "gui/views/ViewContainer.h"
#include "AppBarView.h"
#include "../viewModels/RomBrowserAppBarViewModel.h"

class RomBrowserDisplayMode;
class IRomBrowserViewFactory;

class RomBrowserAppBarView : public ViewContainer
{
    SHARED_ONLY(RomBrowserAppBarView)

public:
    void InitVram(const VramContext& vramContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    void Focus(FocusManager& focusManager)
    {
        _appBarView->Focus(focusManager, 0);
    }

private:
    enum AppBarButton
    {
        APP_BAR_BUTTON_BACK = 0,

        APP_BAR_BUTTON_DISPLAY_SETTINGS,
        // APP_BAR_BUTTON_RECENT,
        // APP_BAR_BUTTON_FAVORITE,
        // APP_BAR_BUTTON_SETTINGS
    };

    RomBrowserAppBarViewModel* _viewModel;
    SharedPtr<AppBarView> _appBarView;

    RomBrowserAppBarView(
        RomBrowserAppBarViewModel* viewModel, const RomBrowserDisplayMode& displayMode,
        const IRomBrowserViewFactory* romBrowserViewFactory);
};

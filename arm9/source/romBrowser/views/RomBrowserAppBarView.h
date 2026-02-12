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
        // APP_BAR_BUTTON_RECENT,
        // APP_BAR_BUTTON_SETTINGS,
        APP_BAR_BUTTON_FAVORITES,
        APP_BAR_BUTTON_DISPLAY_SETTINGS
    };

    void Focus(FocusManager& focusManager, AppBarButton button)
    {
        _appBarView->Focus(focusManager, button);
    }

    RomBrowserAppBarViewModel* _viewModel;
    std::unique_ptr<AppBarView> _appBarView;
};

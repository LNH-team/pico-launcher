#include "common.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../views/IconGridItemView.h"
#include "gui/GraphicsContext.h"
#include "backIcon.h"
#include "settingsIcon.h"
#include "heartIcon.h"
#include "recentIcon.h"
#include "listIcon.h"
#include "gui/IVramManager.h"
#include "gui/input/InputProvider.h"
#include "RomBrowserBottomScreenView.h"

RomBrowserBottomScreenView::RomBrowserBottomScreenView(
    RomBrowserBottomScreenViewModel* viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    VBlankTextureLoader* vblankTextureLoader)
    : _viewModel(viewModel)
    , _romBrowserViewFactory(romBrowserViewFactory)
    , _romBrowserDisplayMode(displayMode)
    , _themeFileIconFactory(themeFileIconFactory)
    , _romBrowserAppBarView(_viewModel->GetRomBrowserAppBarViewModel(),
        *displayMode, romBrowserViewFactory)
    , _vblankTextureLoader(vblankTextureLoader)
{
    _romBrowserAppBarView.SetParent(this);
}

void RomBrowserBottomScreenView::InitVram(const VramContext& vramContext)
{
    _romBrowserAppBarView.InitVram(vramContext);
}

void RomBrowserBottomScreenView::Update()
{
    _romBrowserAppBarView.Update();
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
        _romBrowserView->Update();
}

void RomBrowserBottomScreenView::Draw(GraphicsContext& graphicsContext)
{
    _romBrowserAppBarView.Draw(graphicsContext);
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
        _romBrowserView->Draw(graphicsContext);
}

void RomBrowserBottomScreenView::VBlank()
{
    _romBrowserAppBarView.VBlank();
    if (_romBrowserView && _viewModel->IsRomBrowserVisible())
        _romBrowserView->VBlank();
}

View* RomBrowserBottomScreenView::MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (currentFocus == nullptr)
    {
        return nullptr;
    }
    if (source == &_romBrowserAppBarView)
    {
        if (_romBrowserDisplayMode->IsVertical())
        {
            if (direction == FocusMoveDirection::Right)
                return _romBrowserView->MoveFocus(currentFocus, direction, this);
        }
        else
        {
            if (direction == FocusMoveDirection::Down)
                return _romBrowserView->MoveFocus(currentFocus, direction, this);
        }
        return nullptr;
    }
    else if (source == _romBrowserView.get())
    {
        if (_romBrowserDisplayMode->IsVertical())
        {
            if (direction == FocusMoveDirection::Left)
                return _romBrowserAppBarView.MoveFocus(currentFocus, direction, this);
        }
        else
        {
            if (direction == FocusMoveDirection::Up)
                return _romBrowserAppBarView.MoveFocus(currentFocus, direction, this);
        }
        return nullptr;
    }
    return nullptr;
}

bool RomBrowserBottomScreenView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::Select))
    {
            if (_viewModel->TryShowDisplaySettings()) {
            return true;
        }
    }
    if (inputProvider.Triggered(InputKey::B)
        && _viewModel->GetRomBrowserAppBarViewModel()->IsFavoritesViewActive())
    {
        _romBrowserAppBarView.HandleInput(inputProvider, focusManager);
        return true;
    }
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->NavigateUp();
        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}

void RomBrowserBottomScreenView::RomBrowserViewModelInvalidated(const VramContext& vramContext)
{
    if (_viewModel->GetRomBrowserViewModel().IsValid())
    {
        _romBrowserView = std::make_unique<RomBrowserView>(
            _viewModel->GetRomBrowserViewModel(), *_romBrowserDisplayMode,
            _themeFileIconFactory, _romBrowserViewFactory, _vblankTextureLoader);
        _romBrowserView->SetParent(this);
        _romBrowserView->InitVram(vramContext);
    }
    else
    {
        _romBrowserView.reset();
    }
}
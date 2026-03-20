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
#include "gui/input/TouchEvent.h"
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

    if (!_romBrowserView)
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
        if (_viewModel->TryShowQuickMenu()) {
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

bool RomBrowserBottomScreenView::IsViewInsideRomBrowser(const View* view) const
{
    if (!_romBrowserView || !view)
        return false;

    for (auto current = view; current; current = current->GetParent())
    {
        if (current == _romBrowserView.get())
            return true;
        if (current == this || current == &_romBrowserAppBarView)
            return false;
    }

    return false;
}

void RomBrowserBottomScreenView::RomBrowserViewModelInvalidated(const VramContext& vramContext)
{
    _touchCaptureChild = nullptr;

    if (_viewModel->GetRomBrowserViewModel().IsValid())
    {
        if (_romBrowserView)
        {
            // Fast path: reuse existing view (avoids heap fragmentation)
            _romBrowserView->UpdateViewModel(
                _viewModel->GetRomBrowserViewModel(), *_romBrowserDisplayMode,
                _themeFileIconFactory, _romBrowserViewFactory, _vblankTextureLoader,
                vramContext);
        }
        else
        {
            // First time: create new view
            _romBrowserView = std::make_unique<RomBrowserView>(
                _viewModel->GetRomBrowserViewModel(), *_romBrowserDisplayMode,
                _themeFileIconFactory, _romBrowserViewFactory, _vblankTextureLoader);
            _romBrowserView->SetParent(this);
            _romBrowserView->InitVram(vramContext);
        }
    }
    else
    {
        _romBrowserView.reset();
    }
}

bool RomBrowserBottomScreenView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (event.type == TouchEventType::Down)
    {
        _touchCaptureChild = nullptr;

        static constexpr int RECYCLER_START = 42;
        bool inAppBarZone = _romBrowserDisplayMode->IsVertical()
            ? (event.position.x < RECYCLER_START)
            : (event.position.y < RECYCLER_START);

        if (inAppBarZone)
        {
            _romBrowserAppBarView.HandleTouch(event, focusManager);
            _touchCaptureChild = &_romBrowserAppBarView;
            return true;
        }

        if (_romBrowserAppBarView.HandleTouch(event, focusManager))
        {
            _touchCaptureChild = &_romBrowserAppBarView;
            return true;
        }

        if (_romBrowserView && _viewModel->IsRomBrowserVisible())
        {
            if (_romBrowserView->HandleTouch(event, focusManager))
            {
                _touchCaptureChild = _romBrowserView.get();
                return true;
            }
        }

        return false;
    }
    else
    {
        if (_touchCaptureChild)
        {
            _touchCaptureChild->HandleTouch(event, focusManager);
            if (event.type == TouchEventType::Up)
                _touchCaptureChild = nullptr;
            return true;
        }

        return false;
    }
}

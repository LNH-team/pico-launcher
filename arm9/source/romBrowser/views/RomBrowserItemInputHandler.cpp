#include "common.h"
#include "gui/FocusManager.h"
#include "gui/input/InputProvider.h"
#include "gui/views/View.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "RomBrowserItemInputHandler.h"

#define LONG_PRESS_FRAMES   30

bool RomBrowserItemInputHandler::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        _viewModel->Activate();
        return true;
    }
    else if (inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->ShowGameInfo();
        return true;
    }
    else
    {
        return false;
    }
}

void RomBrowserItemInputHandler::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (_view->GetBounds().Contains(touchPoint))
    {
        _penDown = true;
        _penDownFrames = 0;
    }
}

void RomBrowserItemInputHandler::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (_penDown && _view->GetBounds().Contains(touchPoint))
    {
        if (++_penDownFrames == LONG_PRESS_FRAMES)
        {
            // Long press
            if (focusManager.GetCurrentFocus().GetPointer() != _view)
            {
                focusManager.Focus(_view->SharedFromThis());
            }

            _viewModel->ShowGameInfo();

            _penDown = false; // pen action is complete
        }
    }
    else
    {
        _penDown = false;
    }
}

void RomBrowserItemInputHandler::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && _view->GetBounds().Contains(lastTouchPoint))
    {
        // Short tap
        if (focusManager.GetCurrentFocus().GetPointer() == _view)
        {
            _viewModel->Activate();
        }
        else
        {
            focusManager.Focus(_view->SharedFromThis());
        }
    }

    _penDown = false;
}

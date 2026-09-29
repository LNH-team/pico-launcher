#pragma once
#include "core/math/Point.h"

class InputProvider;
class FocusManager;
class IRomBrowserItemViewModel;
class View;

class RomBrowserItemInputHandler
{
public:
    RomBrowserItemInputHandler(View* view, IRomBrowserItemViewModel* viewModel)
        : _view(view), _viewModel(viewModel) { }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager);
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager);
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager);
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager);

    void CancelPen() { _penDown = false; }

    bool IsPenDown() const { return _penDown; }

private:
    View* _view;
    IRomBrowserItemViewModel* _viewModel;
    bool _penDown = false;
    int _penDownFrames = 0;
};

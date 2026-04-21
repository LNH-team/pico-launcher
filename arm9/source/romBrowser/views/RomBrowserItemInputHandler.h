#pragma once
#include "core/math/Point.h"

class InputProvider;
class FocusManager;
class RomBrowserItemViewModel;
class View;

class RomBrowserItemInputHandler
{
public:
    RomBrowserItemInputHandler(View* view, RomBrowserItemViewModel* viewModel)
        : _view(view), _viewModel(viewModel) { }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager);
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager);
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager);
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager);

    bool IsPenDown() const { return _penDown; }

private:
    View* _view;
    RomBrowserItemViewModel* _viewModel;
    bool _penDown = false;
    int _penDownFrames = 0;
};

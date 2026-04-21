#include "common.h"
#include "BottomSheetView.h"

void BottomSheetView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    DialogView::HandlePenDown(touchPoint, focusManager);

    if (!GetBounds().Contains(touchPoint))
    {
        _oobPenDown = true;
    }
}

void BottomSheetView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    DialogView::HandlePenMove(touchPoint, focusManager);

    if (GetBounds().Contains(touchPoint))
    {
        _oobPenDown = false;
    }
}

void BottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    DialogView::HandlePenUp(lastTouchPoint, focusManager);

    if (_oobPenDown && !GetBounds().Contains(lastTouchPoint))
    {
        Close();
    }

    _oobPenDown = false;
}

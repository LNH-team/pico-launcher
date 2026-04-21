#include "common.h"
#include "AppBarView.h"

#define BUTTON_SIZE     32

AppBarView::AppBarView(int x, int y, Orientation orientation,
    int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme)
    : _orientation(orientation)
    , _buttons(std::make_unique<SharedPtr<IconButtonView>[]>(startButtonCount + endButtonCount))
    , _startButtonCount(startButtonCount), _endButtonCount(endButtonCount)
{
    SetPosition(x, y);
}

Rectangle AppBarView::GetBounds() const
{
    if (_orientation == Orientation::Horizontal)
        return Rectangle(0, _position.y, 256, BUTTON_SIZE);
    else
        return Rectangle(_position.x, 0, BUTTON_SIZE, 192);
}

void AppBarView::Update()
{
    if (_orientation == Orientation::Horizontal)
        UpdateButtonPositionsHorizontal();
    else
        UpdateButtonPositionsVertical();

    ViewContainer::Update();
}

SharedPtr<View> AppBarView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    int idx = FindButtonIndex(currentFocus.GetPointer());
    if (idx >= 0)
    {
        if (_orientation == Orientation::Horizontal)
        {
            if (direction == FocusMoveDirection::Left)
                idx--;
            else if (direction == FocusMoveDirection::Right)
                idx++;
            else
                return View::MoveFocus(currentFocus, direction, this);
        }
        else
        {
            if (direction == FocusMoveDirection::Up)
                idx--;
            else if (direction == FocusMoveDirection::Down)
                idx++;
            else
                return View::MoveFocus(currentFocus, direction, this);
        }

        if (idx < 0 || idx >= _startButtonCount + _endButtonCount)
            return nullptr;

        return _buttons[idx];
    }
    else if ((_orientation == Orientation::Horizontal && (direction == FocusMoveDirection::Up || direction == FocusMoveDirection::Down)) ||
             (_orientation == Orientation::Vertical && (direction == FocusMoveDirection::Left || direction == FocusMoveDirection::Right)))
    {
        if (!currentFocus)
            return _buttons[0];
        Point curFocusPoint = currentFocus->GetBounds().GetCenter();
        s64 bestDistance = std::numeric_limits<s64>::max();
        int nearestButton = -1;
        for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
        {
            s64 distance = curFocusPoint.DistanceSquaredTo(_buttons[i]->GetBounds().GetCenter());
            if (distance < bestDistance)
            {
                bestDistance = distance;
                nearestButton = i;
            }
        }
        if (nearestButton >= 0)
        {
            return _buttons[nearestButton];
        }
        else
        {
            return nullptr;
        }
    }
    else
        return View::MoveFocus(currentFocus, direction, this);
}

void AppBarView::Focus(FocusManager& focusManager, int button)
{
    focusManager.Focus(_buttons[button]);
}

void AppBarView::UpdateButtonPositionsHorizontal()
{
    for (int i = 0; i < _startButtonCount; i++)
        _buttons[i]->SetPosition(i * BUTTON_SIZE, _position.y);

    int x = 256;
    for (int i = _startButtonCount + _endButtonCount - 1; i >= _startButtonCount; i--)
    {
        x -= BUTTON_SIZE;
        _buttons[i]->SetPosition(x, _position.y);
    }
}

void AppBarView::UpdateButtonPositionsVertical()
{
    for (int i = 0; i < _startButtonCount; i++)
        _buttons[i]->SetPosition(_position.x, i * BUTTON_SIZE);

    int y = 192;
    for (int i = _startButtonCount + _endButtonCount - 1; i >= _startButtonCount; i--)
    {
        y -= BUTTON_SIZE;
        _buttons[i]->SetPosition(_position.x, y);
    }
}

int AppBarView::FindButtonIndex(const View* view)
{
    for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
    {
        if (_buttons[i].GetPointer() == view)
        {
            return i;
        }
    }

    return -1;
}

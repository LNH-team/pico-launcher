#include "common.h"
#include "gui/input/TouchEvent.h"
#include "AppBarView.h"

#define BUTTON_SIZE     32
#define TOUCH_EXPAND    8

AppBarView::AppBarView(int x, int y, Orientation orientation,
    int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme)
    : _orientation(orientation)
    , _buttons(std::make_unique<IconButtonView*[]>(startButtonCount + endButtonCount))
    , _startButtonCount(startButtonCount), _endButtonCount(endButtonCount)
{
    SetPosition(x, y);
}

AppBarView::~AppBarView()
{
    for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
    {
        delete _buttons[i];
        _buttons[i] = nullptr;
    }
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

View* AppBarView::MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source)
{
    int idx = FindButtonIndex(currentFocus);
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
        if (currentFocus == nullptr)
            return _buttons[0];
        Point curFocusPoint = currentFocus->GetBounds().GetCenter();
        s64 bestDistance = std::numeric_limits<s64>::max();
        View* nearestButton = nullptr;
        for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
        {
            s64 distance = curFocusPoint.DistanceSquaredTo(_buttons[i]->GetBounds().GetCenter());
            if (distance < bestDistance)
            {
                bestDistance = distance;
                nearestButton = _buttons[i];
            }
        }
        return nearestButton;
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

int AppBarView::FindButtonIndex(const View* view) const
{
    for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
    {
        if (_buttons[i] == view)
        {
            return i;
        }
    }

    return -1;
}

bool AppBarView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (event.type == TouchEventType::Down)
    {
        _touchPressedButton = -1;
        for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
        {
            Rectangle hitBounds(
                _buttons[i]->GetBounds().GetX() - TOUCH_EXPAND,
                _buttons[i]->GetBounds().GetY() - TOUCH_EXPAND,
                _buttons[i]->GetBounds().GetWidth() + TOUCH_EXPAND * 2,
                _buttons[i]->GetBounds().GetHeight() + TOUCH_EXPAND * 2);
            if (hitBounds.Contains(event.position))
            {
                _touchPressedButton = i;
                focusManager.Focus(_buttons[i]);
                return true;
            }
        }
    }
    else if (event.type == TouchEventType::Up)
    {
        int dx = event.position.x - event.startPosition.x;
        int dy = event.position.y - event.startPosition.y;
        int absDx = dx < 0 ? -dx : dx;
        int absDy = dy < 0 ? -dy : dy;

        bool quickTap = event.holdFrames <= TOUCH_TAP_MAX_FRAMES
                     && absDx <= TOUCH_TAP_MAX_DRIFT && absDy <= TOUCH_TAP_MAX_DRIFT;

        if (quickTap)
        {
            if (_touchPressedButton >= 0)
            {
                focusManager.Focus(_buttons[_touchPressedButton]);
                _buttons[_touchPressedButton]->ActivateAction();
                _touchPressedButton = -1;
                return true;
            }

            for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
            {
                Rectangle hitBounds(
                    _buttons[i]->GetBounds().GetX() - TOUCH_EXPAND,
                    _buttons[i]->GetBounds().GetY() - TOUCH_EXPAND,
                    _buttons[i]->GetBounds().GetWidth() + TOUCH_EXPAND * 2,
                    _buttons[i]->GetBounds().GetHeight() + TOUCH_EXPAND * 2);
                if (hitBounds.Contains(event.startPosition) || hitBounds.Contains(event.position))
                {
                    focusManager.Focus(_buttons[i]);
                    _buttons[i]->ActivateAction();
                    _touchPressedButton = -1;
                    return true;
                }
            }
        }
        _touchPressedButton = -1;
    }
    else if (event.type == TouchEventType::Move)
    {
        for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
        {
            Rectangle hitBounds(
                _buttons[i]->GetBounds().GetX() - TOUCH_EXPAND,
                _buttons[i]->GetBounds().GetY() - TOUCH_EXPAND,
                _buttons[i]->GetBounds().GetWidth() + TOUCH_EXPAND * 2,
                _buttons[i]->GetBounds().GetHeight() + TOUCH_EXPAND * 2);
            if (hitBounds.Contains(event.position))
            {
                focusManager.Focus(_buttons[i]);
                return true;
            }
        }

        if (_touchPressedButton >= 0)
            return true;
    }
    return false;
}

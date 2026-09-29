#include "common.h"
#include "gui/input/InputProvider.h"
#include "IconButtonView.h"

bool IconButtonView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        if (_action && !_disabled)
        {
            _action(this, _actionArg);
        }

        return true;
    }
    return View::HandleInput(inputProvider, focusManager);
}

void IconButtonView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (!_disabled && GetBounds().Contains(touchPoint))
    {
        _penDown = true;
    }
}

void IconButtonView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!GetBounds().Contains(touchPoint))
    {
        _penDown = false;
    }
}

void IconButtonView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && GetBounds().Contains(lastTouchPoint))
    {
        focusManager.Focus(SharedFromThis());

        if (_action)
        {
            _action(this, _actionArg);
        }
    }

    _penDown = false;
}

bool IconButtonView::IsCircleBackgroundVisible() const
{
    switch (_type)
    {
        case Type::Standard:
        {
            // Standard never shows a circle for NoToggle/ToggleSelected (back/settings stay
            // plain), but the favorite button's ToggleActive state gets one so "favorited"
            // reads as a filled, selected capsule instead of just a tinted icon - matching
            // the filled look Tonal already has on the custom theme.
            return _state == State::ToggleActive;
        }
        case Type::Filled:
        case Type::Tonal:
        {
            return true;
        }
        default:
        {
            // shouldn't happen
            return false;
        }
    }
}

md::sys::color IconButtonView::GetCircleBackgroundColor() const
{
    switch (_type)
    {
        case Type::Standard:
        {
            // Only reached for the circle itself when ToggleActive (see
            // IsCircleBackgroundVisible()); back/settings (NoToggle) never call this for their
            // circle, but the focus/press highlight below blends off this color unconditionally,
            // so the non-active case still needs its original fallback.
            return _state == State::ToggleActive ? md::sys::color::primary : _backgroundColor;
        }
        case Type::Filled:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::surfaceContainerHighest;
            else
                return md::sys::color::primary;
        }
        case Type::Tonal:
        {
            // ToggleActive is only ever set by the favorite button - give it the same
            // prominent look as Filled's "on" state rather than falling into the plain
            // secondaryContainer capsule shared by NoToggle/ToggleSelected, so favoriting
            // reads as active rather than just "not muted".
            if (_state == State::ToggleUnselected)
                return md::sys::color::surfaceContainerHighest;
            else if (_state == State::ToggleActive)
                return md::sys::color::primary;
            else
                return md::sys::color::secondaryContainer;
        }
        default:
        {
            // shouldn't happen
            return md::sys::color::onSurfaceVariant;
        }
    }
}

md::sys::color IconButtonView::GetForegroundColor() const
{
    if (_disabled)
    {
        return md::sys::color::outline;
    }

    switch (_type)
    {
        case Type::Standard:
        {
            // ToggleActive now gets a filled primary circle (see IsCircleBackgroundVisible()),
            // so its icon needs onPrimary for contrast, same pairing Filled/Tonal use for their
            // "on" state. ToggleSelected keeps the old tinted-icon-only look (no circle) - it's
            // currently unused, kept only so a future caller isn't left with no distinction.
            if (_state == State::ToggleActive)
                return md::sys::color::onPrimary;
            else if (_state == State::ToggleSelected)
                return md::sys::color::primary;
            else
                return md::sys::color::onSurfaceVariant;
        }
        case Type::Filled:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::primary;
            else
                return md::sys::color::onPrimary;
        }
        case Type::Tonal:
        {
            if (_state == State::ToggleUnselected)
                return md::sys::color::onSurfaceVariant;
            else if (_state == State::ToggleActive)
                return md::sys::color::onPrimary;
            else
                return md::sys::color::onSecondaryContainer;
        }
        default:
        {
            // shouldn't happen
            return md::sys::color::onSurfaceVariant;
        }
    }
}

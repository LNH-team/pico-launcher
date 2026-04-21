#include "common.h"
#include "gui/views/View.h"
#include "input/InputProvider.h"
#include "FocusManager.h"

void FocusManager::Focus(const SharedPtr<View>& newFocus)
{
    if (!newFocus)
    {
        Unfocus();
        return;
    }

    if (auto currentFocus = _currentFocus.Lock())
    {
        currentFocus->SetFocused(false);
    }
    newFocus->SetFocused(true);
    _currentFocus = newFocus;
}

void FocusManager::Unfocus()
{
    if (auto currentFocus = _currentFocus.Lock())
    {
        currentFocus->SetFocused(false);
    }
    _currentFocus.Reset();
}

void FocusManager::Update(const InputProvider& inputProvider)
{
    auto currentFocus = _currentFocus.Lock();
    if (!currentFocus || !currentFocus->GetParent())
        return; // todo

    SharedPtr<View> newFocus;
    if (inputProvider.Triggered(InputKey::DpadUp))
    {
        newFocus = currentFocus->GetParent()->MoveFocus(currentFocus, FocusMoveDirection::Up, currentFocus.GetPointer());
    }
    else if (inputProvider.Triggered(InputKey::DpadDown))
    {
        newFocus = currentFocus->GetParent()->MoveFocus(currentFocus, FocusMoveDirection::Down, currentFocus.GetPointer());
    }
    else if (inputProvider.Triggered(InputKey::DpadLeft))
    {
        newFocus = currentFocus->GetParent()->MoveFocus(currentFocus, FocusMoveDirection::Left, currentFocus.GetPointer());
    }
    else if (inputProvider.Triggered(InputKey::DpadRight))
    {
        newFocus = currentFocus->GetParent()->MoveFocus(currentFocus, FocusMoveDirection::Right, currentFocus.GetPointer());
    }
    else
    {
        currentFocus->HandleInput(inputProvider, *this);
    }

    if (newFocus)
    {
        Focus(newFocus);
    }
}

bool FocusManager::IsFocusInside(const View* view) const
{
    if (auto currentFocus = _currentFocus.Lock())
    {
        auto focusView = currentFocus.GetPointer();
        while (focusView)
        {
            if (view == focusView)
            {
                return true;
            }

            focusView = focusView->GetParent();
        }
    }
    return false;
}

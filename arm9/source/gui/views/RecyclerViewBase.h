#pragma once
#include "View.h"
#include "RecyclerAdapter.h"
#include "gui/FocusManager.h"

/// @brief Abstract base class for a recycler view that displays a possibly large collection of items
///        provided by an adapter in an efficient way.
class RecyclerViewBase : public View
{
public:
    /// @brief Callback invoked when an item is tapped via touch.
    typedef void (*touch_tap_callback_t)(int itemIdx, void* arg);
    /// @brief Callback invoked when an item is long-pressed via touch.
    typedef void (*touch_long_press_callback_t)(int itemIdx, void* arg);

    virtual void SetAdapter(const RecyclerAdapter* adapter, int initialSelectedIndex = 0) = 0;
    virtual void Focus(FocusManager& focusManager) = 0;
    virtual int GetSelectedItem() const = 0;

    /// @brief Sets a callback for touch tap on an item. Override in subclasses.
    virtual void SetTouchTapCallback(touch_tap_callback_t callback, void* arg) { (void)callback; (void)arg; }
    /// @brief Sets a callback for long press on an item. Override in subclasses.
    virtual void SetTouchLongPressCallback(touch_long_press_callback_t callback, void* arg) { (void)callback; (void)arg; }
    /// @brief If enabled, the first tap only focuses/selects an item; tapping the already selected item activates it.
    virtual void SetTouchTapRequiresSelected(bool enabled) { (void)enabled; }

protected:
    const RecyclerAdapter* _adapter = nullptr;
};

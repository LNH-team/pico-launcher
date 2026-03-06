#pragma once
#include <array>
#include "gui/views/RecyclerViewBase.h"
#include "core/math/Point.h"

struct TouchEvent;

class CoverFlowRecyclerViewBase : public RecyclerViewBase
{
public:
    void InitVram(const VramContext& vramContext) override;
    void SetAdapter(const RecyclerAdapter* adapter, int initialSelectedIndex = 0) override;
    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;

    void SetTouchTapCallback(touch_tap_callback_t callback, void* arg) override
    {
        _touchTapCallback = callback;
        _touchTapCallbackArg = arg;
    }

    void SetTouchLongPressCallback(touch_long_press_callback_t callback, void* arg) override
    {
        _touchLongPressCallback = callback;
        _touchLongPressCallbackArg = arg;
    }

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 256, 160);
    }

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_selectedItem ? _selectedItem->view : this);
    }

    int GetSelectedItem() const override
    {
        return _selectedItem ? _selectedItem->itemIdx : -1;
    }

protected:
    struct ViewPoolEntry
    {
        View* view;
        int itemIdx = -1;
    };

    std::array<ViewPoolEntry, 10> _viewPool;
    u32 _viewPoolFreeCount;
    u32 _itemCount;
    ViewPoolEntry* _selectedItem = nullptr;
    int _curRangeStart = 0;
    int _curRangeLength = 0;

    ViewPoolEntry* GetViewPoolEntryByItemIndex(int itemIdx);
    ViewPoolEntry* BindViewPoolEntry(int itemIdx);
    void BindRange(int start, int end);
    void ReleaseViewPoolEntry(int itemIdx);
    void ReleaseRange(int start, int end);
    void SetSelectedItem(int itemIdx, bool initial);
    virtual void UpdateItemPosition(int viewPoolIndex, bool initial) = 0;

    /// @brief Returns the screen-space tap bounds for the selected item.
    virtual Rectangle GetSelectedItemTapBounds() const
    {
        return _selectedItem ? _selectedItem->view->GetBounds() : Rectangle(0, 0, 0, 0);
    }

    virtual void SwapViewPoolEntry(int indexA, int indexB)
    {
        std::swap(_viewPool[indexA], _viewPool[indexB]);
    }

private:
    bool _touchDragging = false;
    bool _touchLongPressFired = false;
    Point _touchStartPos;
    int _touchStartSelectedItem = 0;

    int _touchHoldSide = 0;
    int _touchHoldRepeat = 0;
    bool _touchHoldSideCalculated = false;

    touch_tap_callback_t _touchTapCallback = nullptr;
    void* _touchTapCallbackArg = nullptr;
    touch_long_press_callback_t _touchLongPressCallback = nullptr;
    void* _touchLongPressCallbackArg = nullptr;

    /// @brief Horizontal drag threshold before swiping starts.
    static constexpr int TOUCH_DRAG_THRESHOLD = 12;
    /// @brief Pixels of horizontal drag to advance one item.
    static constexpr int TOUCH_ITEM_STEP = 32;
    /// @brief Maximum tap duration in frames.
    static constexpr int TOUCH_TAP_MAX_FRAMES = 24;
    /// @brief Long press threshold in frames.
    static constexpr int TOUCH_LONG_PRESS_FRAMES = 30;
    /// @brief Minimum fling velocity (4.4 fp) to advance extra items.
    static constexpr int TOUCH_MIN_FLING_VELOCITY = 10;
    /// @brief Frames to hold before auto-scroll begins (~330ms at 60fps).
    static constexpr int TOUCH_HOLD_START_DELAY = 20;
    /// @brief Frames between auto-scroll steps while holding.
    static constexpr int TOUCH_HOLD_REPEAT_DELAY = 8;
};

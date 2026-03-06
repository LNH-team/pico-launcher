#include "common.h"
#include <algorithm>
#include "gui/input/TouchEvent.h"
#include "CoverFlowRecyclerViewBase.h"

void CoverFlowRecyclerViewBase::InitVram(const VramContext& vramContext)
{
    for (u32 i = 0; i < _viewPool.size(); i++)
    {
        _viewPool[i].view->InitVram(vramContext);
    }
}

void CoverFlowRecyclerViewBase::SetAdapter(const RecyclerAdapter* adapter, int initialSelectedIndex)
{
    if (_adapter)
    {
        _selectedItem = nullptr;
        for (u32 i = 0; i < _viewPool.size(); i++)
        {
            _adapter->DestroyView(_viewPool[i].view);
        }
    }
    _adapter = adapter;
    // _adapter->GetViewSize(_itemWidth, _itemHeight);
    _itemCount = _adapter->GetItemCount();

    for (u32 i = 0; i < _viewPool.size(); i++)
    {
        _viewPool[i].view = _adapter->CreateView();
        _viewPool[i].view->SetParent(this);
        _viewPool[i].itemIdx = -1;
    }
    _viewPoolFreeCount = _viewPool.size();

    if (initialSelectedIndex < 0 || (u32)initialSelectedIndex >= _itemCount)
    {
        initialSelectedIndex = 0;
    }
    if (_itemCount > 0)
    {
        SetSelectedItem(initialSelectedIndex, true);
    }
}

View* CoverFlowRecyclerViewBase::MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (!_selectedItem || currentFocus != _selectedItem->view)
    {
        // incoming focus
        if (direction == FocusMoveDirection::Down)
        {
            return _selectedItem ? _selectedItem->view : this;
        }
        else
        {
            return nullptr;
        }
    }

    if (direction == FocusMoveDirection::Left)
    {
        int idx = _selectedItem->itemIdx;
        if (idx - 1 >= 0)
        {
            idx -= 1;
        }

        SetSelectedItem(idx, false);
    }
    else if (direction == FocusMoveDirection::Right)
    {
        int idx = _selectedItem->itemIdx + 1;
        idx = std::min(idx, (int)_itemCount - 1);

        SetSelectedItem(idx, false);
    }
    else if (direction == FocusMoveDirection::Up || direction == FocusMoveDirection::Down)
    {
        return View::MoveFocus(currentFocus, direction, this);
    }

    return _selectedItem ? _selectedItem->view : this;
}

CoverFlowRecyclerViewBase::ViewPoolEntry* CoverFlowRecyclerViewBase::GetViewPoolEntryByItemIndex(int itemIdx)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        if (_viewPool[i].itemIdx == (int)itemIdx)
            return &_viewPool[i];
    }

    return nullptr;
}

CoverFlowRecyclerViewBase::ViewPoolEntry* CoverFlowRecyclerViewBase::BindViewPoolEntry(int itemIdx)
{
    if (_viewPoolFreeCount == 0)
    {
        LOG_FATAL("No free view pool entries left\n");
        while (true);
        return nullptr;
    }

    int viewPoolIndex = _viewPoolFreeCount - 1;
    auto& entry = _viewPool[viewPoolIndex];
    _viewPoolFreeCount--;
    entry.itemIdx = itemIdx;
    _adapter->BindView(entry.view, itemIdx);
    UpdateItemPosition(viewPoolIndex, true);
    return &entry;
}

void CoverFlowRecyclerViewBase::BindRange(int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if (_selectedItem && _selectedItem->itemIdx == i)
            continue;
        if (_curRangeLength != 0 && _curRangeStart <= i && i < _curRangeStart + _curRangeLength)
            continue;
        BindViewPoolEntry(i);
    }
}

void CoverFlowRecyclerViewBase::ReleaseViewPoolEntry(int itemIdx)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        if (_viewPool[i].itemIdx == (int)itemIdx)
        {
            _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
            _viewPool[i].itemIdx = -1;
            SwapViewPoolEntry(i, _viewPoolFreeCount);
            if (_selectedItem == &_viewPool[_viewPoolFreeCount])
                _selectedItem = &_viewPool[i];
            _viewPoolFreeCount++;
            break;
        }
    }
}

void CoverFlowRecyclerViewBase::ReleaseRange(int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if (_selectedItem && _selectedItem->itemIdx == i)
            continue;
        ReleaseViewPoolEntry(i);
    }
}

void CoverFlowRecyclerViewBase::SetSelectedItem(int itemIdx, bool initial)
{
    if (_selectedItem)
    {
        if (_selectedItem->itemIdx == itemIdx)
            return;

        if (_selectedItem->itemIdx < _curRangeStart ||
            _selectedItem->itemIdx >= _curRangeStart + _curRangeLength)
        {
            ReleaseViewPoolEntry(_selectedItem->itemIdx);
        }
        _selectedItem = nullptr;
    }

    if (itemIdx < 0 || itemIdx >= (int)_itemCount)
        return;

    if (itemIdx >= _curRangeStart &&
        itemIdx < _curRangeStart + _curRangeLength)
    {
        _selectedItem = GetViewPoolEntryByItemIndex(itemIdx);
    }
    else
    {
        _selectedItem = BindViewPoolEntry(itemIdx);
    }

    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        UpdateItemPosition(i, initial);
    }
}

bool CoverFlowRecyclerViewBase::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (_itemCount == 0)
        return false;

    switch (event.type)
    {
        case TouchEventType::Down:
        {
            _touchDragging = false;
            _touchLongPressFired = false;
            _touchStartPos = event.position;
            _touchStartSelectedItem = GetSelectedItem();
            _touchHoldSide = 0;
            _touchHoldRepeat = 0;
            _touchHoldSideCalculated = false;
            return true;
        }
        case TouchEventType::Move:
        {
            int deltaX = event.position.x - _touchStartPos.x;
            int absDelta = deltaX > 0 ? deltaX : -deltaX;

            if (!_touchDragging && absDelta > TOUCH_DRAG_THRESHOLD)
            {
                _touchDragging = true;
            }

            if (_touchDragging)
            {
                int itemDelta = -deltaX / TOUCH_ITEM_STEP;
                int newIdx = std::clamp(_touchStartSelectedItem + itemDelta,
                                        0, (int)_itemCount - 1);
                if (newIdx != GetSelectedItem())
                {
                    SetSelectedItem(newIdx, false);
                    if (_selectedItem)
                        focusManager.Focus(_selectedItem->view);
                }
            }

            if (!_touchHoldSideCalculated && _selectedItem)
            {
                _touchHoldSideCalculated = true;
                const Rectangle selectedBounds = GetSelectedItemTapBounds();
                if (_touchStartPos.x < selectedBounds.GetLeft())
                    _touchHoldSide = -1;
                else if (_touchStartPos.x >= selectedBounds.GetRight())
                    _touchHoldSide = 1;
                else
                    _touchHoldSide = 0;
            }

            if (!_touchDragging && !_touchLongPressFired &&
                _touchHoldSide == 0 &&
                event.holdFrames >= TOUCH_LONG_PRESS_FRAMES)
            {
                _touchLongPressFired = true;
                if (_touchLongPressCallback && GetSelectedItem() >= 0)
                {
                    _touchLongPressCallback(GetSelectedItem(), _touchLongPressCallbackArg);
                }
            }

            if (!_touchDragging && _touchHoldSide != 0 && _selectedItem)
            {
                if (event.holdFrames >= TOUCH_HOLD_START_DELAY)
                {
                    _touchHoldRepeat++;
                    if (_touchHoldRepeat >= TOUCH_HOLD_REPEAT_DELAY)
                    {
                        _touchHoldRepeat = 0;
                        int newIdx = std::clamp(GetSelectedItem() + _touchHoldSide,
                                                0, (int)_itemCount - 1);
                        if (newIdx != GetSelectedItem())
                        {
                            SetSelectedItem(newIdx, false);
                            if (_selectedItem)
                                focusManager.Focus(_selectedItem->view);
                            _touchLongPressFired = true; 
                        }
                    }
                }
            }
            return true;
        }
        case TouchEventType::Up:
        {
            if (!_touchDragging && !_touchLongPressFired &&
                event.holdFrames <= TOUCH_TAP_MAX_FRAMES)
            {
                if (!_selectedItem)
                {
                    return true;
                }

                Rectangle selectedBounds = GetSelectedItemTapBounds();
                if (selectedBounds.Contains(event.position) || selectedBounds.Contains(event.startPosition))
                {
                    if (_touchTapCallback && GetSelectedItem() >= 0)
                    {
                        _touchTapCallback(GetSelectedItem(), _touchTapCallbackArg);
                    }
                }
                else
                {
                    int direction = event.position.x < selectedBounds.GetLeft() ? -1 : 1;
                    int newIdx = std::clamp(GetSelectedItem() + direction, 0, (int)_itemCount - 1);
                    if (newIdx != GetSelectedItem())
                    {
                        SetSelectedItem(newIdx, false);
                        if (_selectedItem)
                            focusManager.Focus(_selectedItem->view);
                    }
                }
            }
            else if (_touchDragging)
            {
                int velocityX = event.velocityX;
                int absVelocity = velocityX > 0 ? velocityX : -velocityX;
                if (absVelocity > TOUCH_MIN_FLING_VELOCITY)
                {
                    int extraItems = std::clamp(absVelocity / 18, 1, 3);
                    int direction = velocityX > 0 ? -1 : 1; 
                    int newIdx = std::clamp(GetSelectedItem() + direction * extraItems,
                                            0, (int)_itemCount - 1);
                    SetSelectedItem(newIdx, false);
                    if (_selectedItem)
                        focusManager.Focus(_selectedItem->view);
                }
            }
            _touchDragging = false;
            return true;
        }
    }
    return false;
}

#include "common.h"
#include "CoverFlowRecyclerViewBase.h"

CoverFlowRecyclerViewBase::~CoverFlowRecyclerViewBase()
{
    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
    }
}

void CoverFlowRecyclerViewBase::InitVram(const VramContext& vramContext)
{
    for (u32 i = 0; i < _viewPool.size(); i++)
    {
        _viewPool[i].view->InitVram(vramContext);
    }
}

void CoverFlowRecyclerViewBase::SetAdapter(SharedPtr<const RecyclerAdapter> adapter, int initialSelectedIndex)
{
    if (_adapter)
    {
        _selectedItem = nullptr;
        for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
        {
            _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
            _viewPool[i].view.Reset();
        }
    }
    _adapter = std::move(adapter);
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

SharedPtr<View> CoverFlowRecyclerViewBase::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!_selectedItem || currentFocus.GetPointer() != _selectedItem->view.GetPointer())
    {
        // incoming focus
        if (direction == FocusMoveDirection::Down)
        {
            return _selectedItem ? _selectedItem->view : SharedFromThis();
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

    return _selectedItem ? _selectedItem->view : SharedFromThis();
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
}

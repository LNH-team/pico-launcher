#include "common.h"
#include <algorithm>
#include "gui/materialDesign.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "RecyclerView.h"

RecyclerView::RecyclerView(int x, int y, int width, int height, Mode mode)
    : _width(width), _height(height), _mode(mode), _rows(0), _columns(0)
    , _viewPoolFreeCount(0), _viewPoolTotalCount(0)
    , _xOffset(0), _yOffset(0), _xPadding(0), _yPadding(0)
    , _xSpacing(0), _ySpacing(0), _itemWidth(0), _itemHeight(0)
    , _itemCount(0), _selectedItem(nullptr)
    , _curRangeStart(0), _curRangeLength(0)
{
    _position.x = x;
    _position.y = y;
}

RecyclerView::~RecyclerView()
{
    if (_adapter)
    {
        for (u32 i = 0; i < _viewPoolTotalCount; i++)
        {
            _adapter->DestroyView(_viewPool[i].view);
        }
    }
}

void RecyclerView::SetAdapter(const RecyclerAdapter* adapter, int initialSelectedIndex)
{
    if (_adapter)
    {
        // Check if we can reuse the existing view pool
        int newItemWidth, newItemHeight;
        adapter->GetViewSize(newItemWidth, newItemHeight);

        if (newItemWidth == _itemWidth && newItemHeight == _itemHeight && _viewPool)
        {
            // Fast path: reuse view pool (same view dimensions = same pool size)
            // Release all currently bound items with the old adapter
            _selectedItem = nullptr;
            for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
            {
                if (_viewPool[i].itemIdx >= 0)
                {
                    _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
                    _viewPool[i].itemIdx = -1;
                }
            }
            _viewPoolFreeCount = _viewPoolTotalCount;
            _xOffset = 0;
            _yOffset = 0;
            _curRangeStart = 0;
            _curRangeLength = 0;

            _adapter = adapter;
            _itemCount = _adapter->GetItemCount();

            if (initialSelectedIndex < 0 || initialSelectedIndex >= (int)_itemCount)
                initialSelectedIndex = 0;
            EnsureVisible(initialSelectedIndex, false);
            if (_itemCount > 0)
                SetSelectedItem(initialSelectedIndex);
            return;
        }

        // Slow path: view dimensions changed, must recreate pool
        _selectedItem = nullptr;
        for (u32 i = 0; i < _viewPoolTotalCount; i++)
        {
            _adapter->DestroyView(_viewPool[i].view);
        }
        _viewPool.reset();
        _viewPoolFreeCount = 0;
        _viewPoolTotalCount = 0;
        _xOffset = 0;
        _yOffset = 0;
        _curRangeStart = 0;
        _curRangeLength = 0;
    }
    _adapter = adapter;
    _adapter->GetViewSize(_itemWidth, _itemHeight);
    _itemCount = _adapter->GetItemCount();
    if (_mode == Mode::HorizontalList || _mode == Mode::HorizontalGrid)
    {
        if (_mode == Mode::HorizontalList)
        {
            _rows = 1;
        }
        else
        {
            _rows = std::max(1, _height / _itemHeight);
        }
        _columns = (_width + _xSpacing + _itemWidth - 1) / (_xSpacing + _itemWidth) + 1;
        _viewPoolTotalCount = _rows * (_columns + 1) + 1;
    }
    else
    {
        if (_mode == Mode::VerticalList)
        {
            _columns = 1;
        }
        else
        {
            _columns = std::max(1, _width / _itemWidth);
        }
        _rows = (_height + _ySpacing + _itemHeight - 1) / (_ySpacing + _itemHeight) + 1;
        _viewPoolTotalCount = (_rows + 1) * _columns + 1;
    }
    LOG_DEBUG("_rows: %d\n", _rows);
    LOG_DEBUG("_columns: %d\n", _columns);
    LOG_DEBUG("_viewPoolTotalCount: %d\n", _viewPoolTotalCount);
    _viewPool = std::unique_ptr<ViewPoolEntry[]>(new ViewPoolEntry[_viewPoolTotalCount]);
    for (u32 i = 0; i < _viewPoolTotalCount; i++)
    {
        _viewPool[i].view = _adapter->CreateView();
        _viewPool[i].view->SetParent(this);
        _viewPool[i].itemIdx = -1;
    }
    _viewPoolFreeCount = _viewPoolTotalCount;

    if (initialSelectedIndex < 0 || initialSelectedIndex >= (int)_itemCount)
    {
        initialSelectedIndex = 0;
    }
    EnsureVisible(initialSelectedIndex, false);

    if (_itemCount > 0)
    {
        SetSelectedItem(initialSelectedIndex);
    }
}

void RecyclerView::InitVram(const VramContext& vramContext)
{
    for (u32 i = 0; i < _viewPoolTotalCount; i++)
    {
        _viewPool[i].view->InitVram(vramContext);
    }
}

void RecyclerView::Update()
{
    if (_itemCount == 0)
    {
        return;
    }
    if (!_scrollOffsetAnimator.IsFinished())
    {
        _scrollOffsetAnimator.Update();
    }

    int rangeStartIndex = 0;
    int rangeEndIndex = 0;
    if (_mode == Mode::HorizontalList || _mode == Mode::HorizontalGrid)
    {
        _xOffset = _scrollOffsetAnimator.GetValue();
        rangeStartIndex = ((-_xOffset - _xPadding) / (_xSpacing + _itemWidth) - 1) * _rows;
        rangeEndIndex = rangeStartIndex + (_columns + 1) * _rows;
    }
    else
    {
        _yOffset = _scrollOffsetAnimator.GetValue();
        rangeStartIndex = ((-_yOffset - _yPadding) / (_ySpacing + _itemHeight) - 1) * _columns;
        rangeEndIndex = rangeStartIndex + (_rows + 1) * _columns;
    }

    rangeStartIndex = std::clamp(rangeStartIndex, 0, (int)_itemCount - 1);
    rangeEndIndex = std::clamp(rangeEndIndex, 0, (int)_itemCount);

    if (_curRangeStart != rangeStartIndex || _curRangeLength != rangeEndIndex - rangeStartIndex)
    {
        LOG_DEBUG("range: %d - %d\n", rangeStartIndex, rangeEndIndex - 1);
        if (_curRangeLength != 0)
        {
            if (_curRangeStart < rangeStartIndex)
            {
                ReleaseRange(_curRangeStart, rangeStartIndex);
            }
            if (rangeEndIndex < _curRangeStart + _curRangeLength)
            {
                ReleaseRange(rangeEndIndex, _curRangeStart + _curRangeLength);
            }
        }

        BindRange(rangeStartIndex, rangeEndIndex);

        _curRangeStart = rangeStartIndex;
        _curRangeLength = rangeEndIndex - rangeStartIndex;
    }

    for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
    {
        UpdatePosition(_viewPool[i]);
        _viewPool[i].view->Update();
    }
}

void RecyclerView::Draw(GraphicsContext& graphicsContext)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
    {
        _viewPool[i].view->Draw(graphicsContext);
    }
}

void RecyclerView::VBlank()
{
    for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
    {
        _viewPool[i].view->VBlank();
    }
}

View* RecyclerView::MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (_itemCount == 0)
    {
        return View::MoveFocus(currentFocus, direction, this);
    }

    if (_mode == Mode::HorizontalList || _mode == Mode::HorizontalGrid)
    {
        return MoveFocusHorizontal(currentFocus, direction, source);
    }
    else
    {
        return MoveFocusVertical(currentFocus, direction, source);
    }
}

View* RecyclerView::MoveFocusHorizontal(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (!_selectedItem || currentFocus != _selectedItem->view)
    {
        // incoming focus
        if (direction != FocusMoveDirection::Down)
        {
            return nullptr;
        }

        int idx = (-_xOffset + currentFocus->GetPosition().x - _xPadding + ((_xSpacing + _itemWidth) >> 1)) / (_xSpacing + _itemWidth) * _rows;
        SetSelectedItem(std::clamp(idx, 0, ((int)_itemCount - 1) / _rows * _rows));
        return _selectedItem != nullptr ? _selectedItem->view : this;
    }

    int row = _selectedItem->itemIdx % _rows;

    if ((row == 0 && direction == FocusMoveDirection::Up) ||
        (row == _rows - 1 && direction == FocusMoveDirection::Down) ||
        (_selectedItem->itemIdx < _rows && direction == FocusMoveDirection::Left) ||
        (_selectedItem->itemIdx / _rows >= (int)(_itemCount - 1) / _rows && direction == FocusMoveDirection::Right))
    {
        return View::MoveFocus(currentFocus, direction, this);
    }

    if (direction == FocusMoveDirection::Left)
    {
        int idx = _selectedItem->itemIdx;
        if (idx - _rows >= 0)
        {
            idx -= _rows;
        }

        SetSelectedItem(idx);
    }
    else if (direction == FocusMoveDirection::Right)
    {
        int idx = _selectedItem->itemIdx + _rows;
        idx = std::min(idx, (int)_itemCount - 1);

        SetSelectedItem(idx);
    }
    else if (direction == FocusMoveDirection::Up)
    {
        int idx = (_selectedItem->itemIdx / _rows * _rows) + std::clamp((_selectedItem->itemIdx % _rows) - 1, 0, _rows - 1);
        SetSelectedItem(std::clamp(idx, 0, (int)_itemCount - 1));
    }
    else if (direction == FocusMoveDirection::Down)
    {
        int idx = (_selectedItem->itemIdx / _rows * _rows) + std::clamp((_selectedItem->itemIdx % _rows) + 1, 0, _rows - 1);
        SetSelectedItem(std::clamp(idx, 0, (int)_itemCount - 1));
    }

    return _selectedItem != nullptr ? _selectedItem->view : this;
}

View* RecyclerView::MoveFocusVertical(View* currentFocus, FocusMoveDirection direction, View* source)
{
    if (!_selectedItem || currentFocus != _selectedItem->view)
    {
        // incoming focus
        if (direction != FocusMoveDirection::Right)
        {
            return nullptr;
        }

        int idx = (-_yOffset + currentFocus->GetPosition().y - _yPadding + ((_ySpacing + _itemHeight) >> 1)) / (_ySpacing + _itemHeight) * _columns;
        SetSelectedItem(std::clamp(idx, 0, ((int)_itemCount - 1) / _columns * _columns));
        return _selectedItem != nullptr ? _selectedItem->view : this;
    }

    int column = _selectedItem->itemIdx % _columns;

    if ((column == 0 && direction == FocusMoveDirection::Left) ||
        (column == _columns - 1 && direction == FocusMoveDirection::Right) ||
        (_selectedItem->itemIdx < _columns && direction == FocusMoveDirection::Up) ||
        (_selectedItem->itemIdx / _columns >= (int)(_itemCount - 1) / _columns && direction == FocusMoveDirection::Down))
    {
        return View::MoveFocus(currentFocus, direction, this);
    }

    if (direction == FocusMoveDirection::Up)
    {
        int idx = _selectedItem->itemIdx;
        if (idx - _columns >= 0)
        {
            idx -= _columns;
        }

        SetSelectedItem(idx);
    }
    else if (direction == FocusMoveDirection::Down)
    {
        int idx = _selectedItem->itemIdx + _columns;
        idx = std::min(idx, (int)_itemCount - 1);

        SetSelectedItem(idx);
    }
    else if (direction == FocusMoveDirection::Left)
    {
        int idx = (_selectedItem->itemIdx / _columns * _columns) + std::clamp((_selectedItem->itemIdx % _columns) - 1, 0, _columns - 1);
        SetSelectedItem(std::clamp(idx, 0, (int)_itemCount - 1));
    }
    else if (direction == FocusMoveDirection::Right)
    {
        int idx = (_selectedItem->itemIdx / _columns * _columns) + std::clamp((_selectedItem->itemIdx % _columns) + 1, 0, _columns - 1);
        SetSelectedItem(std::clamp(idx, 0, (int)_itemCount - 1));
    }

    return _selectedItem != nullptr ? _selectedItem->view : this;
}

bool RecyclerView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_shoulderPagingEnabled && inputProvider.Triggered(InputKey::L | InputKey::R))
    {
        PageByShoulderButtons(inputProvider.Triggered(InputKey::L), focusManager);
        return true;
    }

    return View::HandleInput(inputProvider, focusManager);
}

void RecyclerView::PageByShoulderButtons(bool useLBehavior, FocusManager& focusManager)
{
    if (_selectedItem == nullptr || _itemCount == 0)
    {
        return;
    }

    int direction = useLBehavior ? 1 : -1;
    int selected = _selectedItem->itemIdx;
    if (_mode == Mode::HorizontalList || _mode == Mode::HorizontalGrid)
    {
        int visibleColumns = _width / (_itemWidth + _xSpacing);
        SetScrollOffset(_scrollOffsetAnimator.GetTargetValue() + direction * visibleColumns * (_itemWidth + _xSpacing), true);
        int row = selected % _rows;
        selected = std::clamp(selected - direction * visibleColumns * _rows, 0, (int)_itemCount - 1);
        selected = selected / _rows * _rows + row; // try to stay in the same row
        selected = std::clamp(selected, 0, (int)_itemCount - 1); // but clamp to the last item
    }
    else
    {
        int visibleRows = _height / (_itemHeight + _ySpacing);
        SetScrollOffset(_scrollOffsetAnimator.GetTargetValue() + direction * visibleRows * (_itemHeight + _ySpacing), true);
        int column = selected % _columns;
        selected = std::clamp(selected - direction * visibleRows * _columns, 0, (int)_itemCount - 1);
        selected = selected / _columns * _columns + column; // try to stay in the same row
        selected = std::clamp(selected, 0, (int)_itemCount - 1); // but clamp to the last item
    }

    focusManager.Unfocus();
    SetSelectedItem(selected);
    if (_selectedItem != nullptr)
    {
        focusManager.Focus(_selectedItem->view);
    }
    else
    {
        focusManager.Focus(this);
    }
}

Point RecyclerView::GetItemPosition(int itemIdx)
{
    int x = 0;
    int y = 0;
    switch (_mode)
    {
        case Mode::HorizontalList:
        {
            x = _xPadding + itemIdx * (_xSpacing + _itemWidth);
            y = _yPadding;
            break;
        }
        case Mode::HorizontalGrid:
        {
            x = _xPadding + (itemIdx / _rows) * (_xSpacing + _itemWidth);
            y = _yPadding + (itemIdx % _rows) * (_ySpacing + _itemHeight);
            break;
        }
        case Mode::VerticalList:
        {
            x = _xPadding;
            y = _yPadding + itemIdx * (_ySpacing + _itemHeight);
            break;
        }
        case Mode::VerticalGrid:
        {
            x = _xPadding + (itemIdx % _columns) * (_xSpacing + _itemWidth);
            y = _yPadding + (itemIdx / _columns) * (_ySpacing + _itemHeight);
            break;
        }
    }
    return Point(x, y);
}

void RecyclerView::UpdatePosition(ViewPoolEntry& viewPoolEntry)
{
    auto itemPosition = GetItemPosition(viewPoolEntry.itemIdx);
    viewPoolEntry.view->SetPosition(
        _position.x + _xOffset + itemPosition.x,
        _position.y + _yOffset + itemPosition.y);
}

RecyclerView::ViewPoolEntry* RecyclerView::GetViewPoolEntryByItemIndex(int itemIdx)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
    {
        if (_viewPool[i].itemIdx == (int)itemIdx)
        {
            return &_viewPool[i];
        }
    }

    return nullptr;
}

RecyclerView::ViewPoolEntry* RecyclerView::BindViewPoolEntry(int itemIdx)
{
    if (_viewPoolFreeCount == 0)
    {
        LOG_FATAL("No free view pool entries left\n");
        while (true);
        return nullptr;
    }

    auto& entry = _viewPool[_viewPoolFreeCount - 1];
    _viewPoolFreeCount--;
    entry.itemIdx = itemIdx;
    _adapter->BindView(entry.view, itemIdx);
    return &entry;
}

void RecyclerView::BindRange(int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if ((_selectedItem && _selectedItem->itemIdx == i) ||
            (_curRangeLength != 0 && _curRangeStart <= i && i < _curRangeStart + _curRangeLength))
        {
            continue;
        }
        BindViewPoolEntry(i);
    }
}

void RecyclerView::ReleaseViewPoolEntry(int itemIdx)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
    {
        if (_viewPool[i].itemIdx == (int)itemIdx)
        {
            _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
            _viewPool[i].itemIdx = -1;
            std::swap(_viewPool[i], _viewPool[_viewPoolFreeCount]);
            if (_selectedItem == &_viewPool[_viewPoolFreeCount])
            {
                _selectedItem = &_viewPool[i];
            }
            _viewPoolFreeCount++;
            return;
        }
    }
}

void RecyclerView::ReleaseRange(int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if (_selectedItem && _selectedItem->itemIdx == i)
        {
            continue;
        }
        ReleaseViewPoolEntry(i);
    }
}

void RecyclerView::SetSelectedItem(int itemIdx)
{
    if (_selectedItem)
    {
        if (_selectedItem->itemIdx == itemIdx)
        {
            return;
        }

        if (_selectedItem->itemIdx < _curRangeStart ||
            _selectedItem->itemIdx >= _curRangeStart + _curRangeLength)
        {
            ReleaseViewPoolEntry(_selectedItem->itemIdx);
        }
        _selectedItem = nullptr;
    }

    if (itemIdx < 0 || itemIdx >= (int)_itemCount)
    {
        return;
    }

    if (itemIdx >= _curRangeStart &&
        itemIdx < _curRangeStart + _curRangeLength)
    {
        _selectedItem = GetViewPoolEntryByItemIndex(itemIdx);
    }
    else
    {
        _selectedItem = BindViewPoolEntry(itemIdx);
    }

    EnsureVisible(itemIdx, true);
}

int RecyclerView::GetMaxScrollOffset()
{
    if (_mode == Mode::HorizontalGrid || _mode == Mode::HorizontalList)
    {
        int totalColumns = (_itemCount + _rows - 1) / _rows;
        int contentWidth = totalColumns * _itemWidth + (totalColumns - 1) * _xSpacing + _xPadding * 2;
        return std::min(0, _width - contentWidth);
    }
    else
    {
        int totalRows = ((_itemCount + _columns - 1) / _columns);
        int contentHeight = totalRows * _itemHeight + (totalRows - 1) * _ySpacing + _yPadding * 2;
        return std::min(0, _height - contentHeight);
    }
}

void RecyclerView::SetScrollOffset(int offset, bool animate)
{
    offset = std::clamp(offset, GetMaxScrollOffset(), 0);

    if (!animate)
    {
        _scrollOffsetAnimator = Animator<int>(offset);
    }
    else
    {
        if (std::abs(offset - _scrollOffsetAnimator.GetTargetValue()) <= 128)
        {
            _scrollOffsetAnimator.Goto(offset,
                md::sys::motion::duration::medium1, &md::sys::motion::easing::emphasized);
        }
        else
        {
            _scrollOffsetAnimator.Goto(offset,
                md::sys::motion::duration::long2, &md::sys::motion::easing::standard);
        }
    }
}

void RecyclerView::EnsureVisible(int itemIdx, bool animate)
{
    const auto itemPosition = GetItemPosition(itemIdx);
    int minItemScrollOffset;
    int maxItemScollOffset;
    if (_mode == Mode::HorizontalGrid || _mode == Mode::HorizontalList)
    {
        minItemScrollOffset = -itemPosition.x + _xPadding;
        maxItemScollOffset = -itemPosition.x + _width - _itemWidth - _xPadding;
    }
    else
    {
        minItemScrollOffset = -itemPosition.y + _yPadding;
        maxItemScollOffset = -itemPosition.y + _height - _itemHeight - _yPadding;
    }
    int targetScrollOffset = std::clamp(_scrollOffsetAnimator.GetTargetValue(), minItemScrollOffset, maxItemScollOffset);
    if (targetScrollOffset != _scrollOffsetAnimator.GetTargetValue())
    {
        SetScrollOffset(targetScrollOffset, animate);
    }
}

int RecyclerView::FindItemAtScreenPosition(int screenX, int screenY) const
{
    int localX = screenX - _position.x - (IsHorizontalMode() ? _xOffset : 0);
    int localY = screenY - _position.y - (IsHorizontalMode() ? 0 : _yOffset);

    int itemIdx = -1;
    switch (_mode)
    {
        case Mode::HorizontalList:
        {
            int cellX = localX - _xPadding;
            int cellStep = _xSpacing + _itemWidth;
            int col = cellX / cellStep;
            int maxCols = (_itemCount + _rows - 1) / _rows;
            if (col >= 0 && col < maxCols)
            {
                itemIdx = col;
            }
            break;
        }
        case Mode::HorizontalGrid:
        {
            int cellX = localX - _xPadding;
            int cellY = localY - _yPadding;
            int cellStepX = _xSpacing + _itemWidth;
            int cellStepY = _ySpacing + _itemHeight;
            int col = cellX / cellStepX;
            int row = cellY / cellStepY;
            int maxCols = (_itemCount + _rows - 1) / _rows;
            if (col >= 0 && col < maxCols && row >= 0 && row < _rows)
            {
                itemIdx = col * _rows + row;
            }
            break;
        }
        case Mode::VerticalList:
        {
            int cellY = localY - _yPadding;
            int cellStep = _ySpacing + _itemHeight;
            int row = cellY / cellStep;
            if (row >= 0 && row < (int)_itemCount)
            {
                itemIdx = row;
            }
            break;
        }
        case Mode::VerticalGrid:
        {
            int cellX = localX - _xPadding;
            int cellY = localY - _yPadding;
            int cellStepX = _xSpacing + _itemWidth;
            int cellStepY = _ySpacing + _itemHeight;
            int col = cellX / cellStepX;
            int row = cellY / cellStepY;
            if (col >= 0 && col < _columns && row >= 0)
            {
                itemIdx = row * _columns + col;
            }
            break;
        }
    }

    if (itemIdx >= 0 && itemIdx < (int)_itemCount)
        return itemIdx;
    return -1;
}

bool RecyclerView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (_itemCount == 0)
        return false;

    switch (event.type)
    {
        case TouchEventType::Down:
        {
            _scrollOffsetAnimator = Animator<int>(_scrollOffsetAnimator.GetValue());
            _touchStartScrollOffset = _scrollOffsetAnimator.GetValue();
            _touchDragging = false;
            _touchLongPressFired = false;
            _touchStartPos = event.position;
            _touchSelectedItemOnDown = GetSelectedItem();

            int itemIdx = FindItemAtScreenPosition(event.position.x, event.position.y);
            if (itemIdx >= 0)
            {
                SetSelectedItem(itemIdx);
                if (_selectedItem)
                    focusManager.Focus(_selectedItem->view);
            }
            return true;
        }
        case TouchEventType::Move:
        {
            int primaryDelta = IsHorizontalMode()
                ? (event.position.x - _touchStartPos.x)
                : (event.position.y - _touchStartPos.y);

            int crossDelta = IsHorizontalMode()
                ? (event.position.y - _touchStartPos.y)
                : (event.position.x - _touchStartPos.x);

            int totalMovement = (primaryDelta > 0 ? primaryDelta : -primaryDelta)
                              + (crossDelta > 0 ? crossDelta : -crossDelta);

            if (!_touchDragging && totalMovement > TOUCH_DRAG_THRESHOLD)
            {
                _touchDragging = true;
            }

            if (_touchDragging)
            {
                int newOffset = _touchStartScrollOffset + primaryDelta;
                int maxScroll = GetMaxScrollOffset();
                if (newOffset > 0)
                {
                    newOffset = newOffset / 3;
                }
                else if (newOffset < maxScroll)
                {
                    int overshoot = maxScroll - newOffset;
                    newOffset = maxScroll - overshoot / 3;
                }
                _scrollOffsetAnimator = Animator<int>(newOffset);
            }

            if (!_touchDragging && !_touchLongPressFired &&
                event.holdFrames >= TOUCH_LONG_PRESS_FRAMES)
            {
                _touchLongPressFired = true;
                int itemIdx = FindItemAtScreenPosition(event.startPosition.x, event.startPosition.y);
                if (itemIdx >= 0 && _touchLongPressCallback)
                {
                    SetSelectedItem(itemIdx);
                    if (_selectedItem)
                        focusManager.Focus(_selectedItem->view);
                    _touchLongPressCallback(itemIdx, _touchLongPressCallbackArg);
                }
            }
            return true;
        }
        case TouchEventType::Up:
        {
            if (!_touchDragging && !_touchLongPressFired &&
                event.holdFrames <= TOUCH_TAP_MAX_FRAMES)
            {
                int itemIdx = FindItemAtScreenPosition(event.startPosition.x, event.startPosition.y);
                if (itemIdx >= 0)
                {
                    SetSelectedItem(itemIdx);
                    if (_selectedItem)
                        focusManager.Focus(_selectedItem->view);
                    if (_touchTapCallback && (!_touchTapRequiresSelected || itemIdx == _touchSelectedItemOnDown))
                        _touchTapCallback(itemIdx, _touchTapCallbackArg);
                }
            }
            else if (_touchDragging)
            {
                int curOffset = _scrollOffsetAnimator.GetValue();
                int maxScroll = GetMaxScrollOffset();
                if (curOffset > 0 || curOffset < maxScroll)
                {
                    int target = std::clamp(curOffset, maxScroll, 0);
                    _scrollOffsetAnimator.Goto(target,
                        md::sys::motion::duration::medium2, &md::sys::motion::easing::emphasizedDecelerate);
                }
                else
                {
                    int velocity = IsHorizontalMode() ? event.velocityX : event.velocityY;
                    int absVelocity = velocity > 0 ? velocity : -velocity;
                    if (absVelocity > TOUCH_MIN_FLING_VELOCITY)
                    {
                        int momentumDistance = (velocity * TOUCH_MOMENTUM_FRAMES) >> 4;
                        int targetOffset = curOffset + momentumDistance;
                        targetOffset = std::clamp(targetOffset, maxScroll, 0);
                        _scrollOffsetAnimator.Goto(targetOffset,
                            TOUCH_MOMENTUM_FRAMES, &md::sys::motion::easing::emphasizedDecelerate);
                    }
                }

                if (IsHorizontalMode())
                {
                    int centerX = _position.x + _width / 2;
                    int itemIdx = FindItemAtScreenPosition(centerX, _position.y + _height / 2);
                    if (itemIdx >= 0)
                    {
                        SetSelectedItem(itemIdx);
                        if (_selectedItem)
                            focusManager.Focus(_selectedItem->view);
                    }
                }
                else
                {
                    int centerY = _position.y + _height / 2;
                    int itemIdx = FindItemAtScreenPosition(_position.x + _width / 2, centerY);
                    if (itemIdx >= 0)
                    {
                        SetSelectedItem(itemIdx);
                        if (_selectedItem)
                            focusManager.Focus(_selectedItem->view);
                    }
                }
            }
            _touchDragging = false;
            _touchSelectedItemOnDown = -1;
            return true;
        }
    }
    return false;
}
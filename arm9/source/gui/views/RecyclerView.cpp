#include "common.h"
#include <algorithm>
#include "gui/materialDesign.h"
#include "gui/input/InputProvider.h"
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
        for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
        {
            _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
        }
    }
}

void RecyclerView::SetAdapter(SharedPtr<const RecyclerAdapter> adapter, int initialSelectedIndex)
{
    if (_adapter)
    {
        _selectedItem = nullptr;
        for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
        {
            _adapter->ReleaseView(_viewPool[i].view, _viewPool[i].itemIdx);
        }
        _viewPool.reset();
        _viewPoolFreeCount = 0;
        _viewPoolTotalCount = 0;
        _xOffset = 0;
        _yOffset = 0;
        _curRangeStart = 0;
        _curRangeLength = 0;
    }
    _adapter = std::move(adapter);
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

SharedPtr<View> RecyclerView::MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
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

SharedPtr<View> RecyclerView::MoveFocusHorizontal(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!_selectedItem || currentFocus.GetPointer() != _selectedItem->view.GetPointer())
    {
        // incoming focus
        if (direction != FocusMoveDirection::Down)
        {
            return nullptr;
        }

        int idx = (-_xOffset + currentFocus->GetPosition().x - _xPadding + ((_xSpacing + _itemWidth) >> 1)) / (_xSpacing + _itemWidth) * _rows;
        SetSelectedItem(std::clamp(idx, 0, ((int)_itemCount - 1) / _rows * _rows));
        return _selectedItem != nullptr ? _selectedItem->view : SharedFromThis();
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

    return _selectedItem != nullptr ? _selectedItem->view : SharedFromThis();
}

SharedPtr<View> RecyclerView::MoveFocusVertical(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!_selectedItem || currentFocus.GetPointer() != _selectedItem->view.GetPointer())
    {
        // incoming focus
        if (direction == FocusMoveDirection::Right)
        {
            int idx = (-_yOffset + currentFocus->GetPosition().y - _yPadding + ((_ySpacing + _itemHeight) >> 1)) / (_ySpacing + _itemHeight) * _columns;
            SetSelectedItem(std::clamp(idx, 0, ((int)_itemCount - 1) / _columns * _columns));
            return _selectedItem != nullptr ? _selectedItem->view : SharedFromThis();
        }
        else if (direction == FocusMoveDirection::Down)
        {
            int idx = (-_xOffset + currentFocus->GetPosition().x - _xPadding + ((_xSpacing + _itemWidth) >> 1)) / (_xSpacing + _itemWidth);
            SetSelectedItem(std::clamp(idx, 0, _columns - 1));
            return _selectedItem != nullptr ? _selectedItem->view : SharedFromThis();
        }

        return nullptr;
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

    return _selectedItem != nullptr ? _selectedItem->view : SharedFromThis();
}

bool RecyclerView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_itemCount != 0 && inputProvider.Triggered(InputKey::L | InputKey::R))
    {
        int direction = inputProvider.Triggered(InputKey::L) ? 1 : -1;
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
            selected = selected / _columns * _columns + column; // try to stay in the same column
            selected = std::clamp(selected, 0, (int)_itemCount - 1); // but clamp to the last item
        }

        focusManager.Unfocus();
        SetSelectedItem(selected);
        focusManager.Focus(_selectedItem->view);
        return true;
    }

    return View::HandleInput(inputProvider, focusManager);
}

void RecyclerView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
    {
        _penDown = true;
        _penDownPosition = touchPoint;
        _hasScrollStarted = false;
        _penDownScrollOffset = _scrollOffsetAnimator.GetValue();

        for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
        {
            _viewPool[i].view->HandlePenDown(touchPoint, focusManager);
        }
    }
}

void RecyclerView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!_penDown)
    {
        return;
    }

    if (!_hasScrollStarted)
    {
        for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
        {
            _viewPool[i].view->HandlePenMove(touchPoint, focusManager);
            if (focusManager.GetCurrentFocus().GetPointer() == _viewPool[i].view.GetPointer())
            {
                SetSelectedItem(_viewPool[i].itemIdx);
            }
        }

        int dx = touchPoint.x - _penDownPosition.x;
        int dy = touchPoint.y - _penDownPosition.y;
        if (dx * dx + dy * dy > 7 * 7)
        {
            bool shouldScrollStart = (_mode == Mode::HorizontalGrid || _mode == Mode::HorizontalList)
                ? (std::abs(touchPoint.x - _penDownPosition.x) > std::abs(touchPoint.y - _penDownPosition.y))
                : (std::abs(touchPoint.x - _penDownPosition.x) < std::abs(touchPoint.y - _penDownPosition.y));

            if (shouldScrollStart)
            {
                _hasScrollStarted = true;
            }
            else
            {
                _penDown = false; //wrong direction drag, so cancel it
            }

            for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
            {
                _viewPool[i].view->HandlePenUp(Point(-1, -1), focusManager);
            }
        }
    }
    else
    {
        int newScrollOffset;
        if (_mode == Mode::HorizontalGrid || _mode == Mode::HorizontalList)
        {
            newScrollOffset = _penDownScrollOffset + touchPoint.x - _penDownPosition.x;
            if (-newScrollOffset < 0)
            {
                newScrollOffset = 0;
                _penDownScrollOffset = 0;
                _penDownPosition.x = touchPoint.x;
            }
            else if (newScrollOffset < GetMaxScrollOffset())
            {
                newScrollOffset = GetMaxScrollOffset();
                _penDownScrollOffset = newScrollOffset;
                _penDownPosition.x = touchPoint.x;
            }
        }
        else
        {
            newScrollOffset = _penDownScrollOffset + touchPoint.y - _penDownPosition.y;
            if (-newScrollOffset < 0)
            {
                newScrollOffset = 0;
                _penDownScrollOffset = 0;
                _penDownPosition.y = touchPoint.y;
            }
            else if (newScrollOffset < GetMaxScrollOffset())
            {
                newScrollOffset = GetMaxScrollOffset();
                _penDownScrollOffset = newScrollOffset;
                _penDownPosition.y = touchPoint.y;
            }
        }

        SetScrollOffset(newScrollOffset, false);
    }
}

void RecyclerView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPoolTotalCount; i++)
    {
        _viewPool[i].view->HandlePenUp(lastTouchPoint, focusManager);
        if (focusManager.GetCurrentFocus().GetPointer() == _viewPool[i].view.GetPointer())
        {
            SetSelectedItem(_viewPool[i].itemIdx);
        }
    }

    _penDown = false;
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
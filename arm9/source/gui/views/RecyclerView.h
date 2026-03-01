#pragma once
#include <memory>
#include "View.h"
#include "RecyclerAdapter.h"
#include "gui/FocusManager.h"
#include "animation/Animator.h"
#include "RecyclerViewBase.h"

class RecyclerView : public RecyclerViewBase
{
public:
    enum class Mode
    {
        /// @brief A single row that grows horizontally.
        HorizontalList,
        /// @brief A multi-row grid that grows horizontally.
        HorizontalGrid,
        /// @brief A single column that grows vertically.
        VerticalList,
        /// @brief A multi-column grid that grows vertically.
        VerticalGrid
    };

    RecyclerView(int x, int y, int width, int height, Mode mode);
    ~RecyclerView();

    void SetAdapter(const RecyclerAdapter* adapter, int initialSelectedIndex = 0) override;
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, _width, _height);
    }

    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void PageByShoulderButtons(bool useLBehavior, FocusManager& focusManager);

    void SetShoulderPagingEnabled(bool enabled)
    {
        _shoulderPagingEnabled = enabled;
    }

    void Focus(FocusManager& focusManager) override
    {
        if (_selectedItem)
            focusManager.Focus(_selectedItem->view);
        else
            focusManager.Focus(this);
    }

    int GetSelectedItem() const override
    {
        return _selectedItem ? _selectedItem->itemIdx : -1;
    }

    constexpr Mode GetMode() const { return _mode; }

    void SetPadding(int x, int y)
    {
        _xPadding = x;
        _yPadding = y;
    }

    void SetItemSpacing(int x, int y)
    {
        _xSpacing = x;
        _ySpacing = y;
    }

private:
    struct ViewPoolEntry
    {
        View* view;
        int itemIdx;
    };

    int _width;
    int _height;
    Mode _mode;
    int _rows;
    int _columns;
    std::unique_ptr<ViewPoolEntry[]> _viewPool;
    u32 _viewPoolFreeCount;
    u32 _viewPoolTotalCount;
    int _xOffset;
    int _yOffset;
    int _xPadding;
    int _yPadding;
    int _xSpacing;
    int _ySpacing;
    int _itemWidth;
    int _itemHeight;
    u32 _itemCount;
    ViewPoolEntry* _selectedItem;
    int _curRangeStart;
    int _curRangeLength;
    Animator<int> _scrollOffsetAnimator;
    bool _shoulderPagingEnabled = true;

    void UpdatePosition(ViewPoolEntry& viewPoolEntry);
    ViewPoolEntry* GetViewPoolEntryByItemIndex(int itemIdx);
    void BindRange(int start, int end);
    ViewPoolEntry* BindViewPoolEntry(int itemIdx);
    void ReleaseRange(int start, int end);
    void ReleaseViewPoolEntry(int itemIdx);
    void SetSelectedItem(int itemIdx);
    int GetMaxScrollOffset();
    void SetScrollOffset(int offset, bool animate);
    void EnsureVisible(int itemIdx, bool animate);
    Point GetItemPosition(int itemIdx);

    View* MoveFocusHorizontal(View* currentFocus, FocusMoveDirection direction, View* source);
    View* MoveFocusVertical(View* currentFocus, FocusMoveDirection direction, View* source);
};

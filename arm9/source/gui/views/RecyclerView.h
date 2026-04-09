#pragma once
#include <memory>
#include "core/EnableSharedFromThis.h"
#include "View.h"
#include "RecyclerAdapter.h"
#include "gui/FocusManager.h"
#include "animation/Animator.h"
#include "RecyclerViewBase.h"

class RecyclerView : public RecyclerViewBase, public EnableSharedFromThis<RecyclerView>
{
    struct Private { explicit Private() = default; };

public:
    struct State
    {
        int selectedItem = -1;
        int scrollOffset = 0;
    };

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

    RecyclerView(Private, int x, int y, int width, int height, Mode mode);

    static SharedPtr<RecyclerView> CreateShared(int x, int y, int width, int height, Mode mode)
    {
        return SharedPtr<RecyclerView>::MakeShared(Private(), x, y, width, height, mode);
    }

    ~RecyclerView();

    void SetAdapter(SharedPtr<const RecyclerAdapter> adapter, int initialSelectedIndex = 0) override;
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, _width, _height);
    }

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override
    {
        if (_selectedItem)
        {
            focusManager.Focus(_selectedItem->view);
        }
        else
        {
            focusManager.Focus(SharedFromThis());
        }
    }

    int GetSelectedItem() const override
    {
        return _selectedItem ? _selectedItem->itemIdx : -1;
    }

    // Returns the current selection and scroll position
    State GetState() const
    {
        return { GetSelectedItem(), _scrollOffsetAnimator.GetValue() };
    }

    // Restores the selection and scroll position
    void RestoreState(const State& state)
    {
        if (_itemCount == 0)
        {
            _scrollOffsetAnimator = Animator<int>(0);
            return;
        }

        int selectedItem = state.selectedItem;
        if (selectedItem < 0 || selectedItem >= (int)_itemCount)
        {
            selectedItem = 0;
        }

        SetSelectedItem(selectedItem);
        SetScrollOffset(state.scrollOffset, false);
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
        SharedPtr<View> view;
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

    SharedPtr<View> MoveFocusHorizontal(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source);
    SharedPtr<View> MoveFocusVertical(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source);
};

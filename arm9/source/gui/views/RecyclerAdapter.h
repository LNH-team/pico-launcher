#pragma once
class View;

/// @brief Adapter class providing the content to show in a \see RecyclerView.
///        All items in the adapter use an identical view that can be reused to
///        show another item of this adapter when it goes off screen.
class RecyclerAdapter
{
public:
    virtual ~RecyclerAdapter() { }

    /// @brief Returns the total number of items in this adapter.
    /// @return The total number of items in this adapter.
    virtual u32 GetItemCount() const = 0;

    /// @brief Returns the size of each view of this adapter.
    /// @param width Returns the with of the view.
    /// @param height Returns the height of the view.
    virtual void GetViewSize(int& width, int& height) const = 0;

    /// @brief Creates and returns a view for this adapter.
    /// @return The created view.
    virtual SharedPtr<View> CreateView() const = 0;

    /// @brief Binds the given \p view to the item at the given \p index.
    /// @param view The view to bind.
    /// @param index The item index to bind to.
    virtual void BindView(SharedPtr<View> view, int index) const = 0;

    /// @brief Releases a \p view that was previously bound with BindView, such that it can be reused.
    /// @param view The view to release.
    /// @param index The item index that was bound to the view.
    virtual void ReleaseView(SharedPtr<View> view, int index) const = 0;
};

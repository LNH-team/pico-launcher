#pragma once
#include <core/SharedPtr.h>
#include <core/WeakPtr.h>

class View;
class InputProvider;

/// @brief Class for managing focus.
class FocusManager
{
public:
    /// @brief Focuses the given view.
    /// @param newFocus The view to focus.
    void Focus(const SharedPtr<View>& newFocus);

    /// @brief Clears the current focus.
    void Unfocus();

    /// @brief Updates the focus using the given input provider.
    /// @param inputProvider The input provider to use.
    void Update(const InputProvider& inputProvider);

    /// @brief Gets the currently focused view.
    /// @return A pointer to the view that is currently focused, or null if none.
    constexpr SharedPtr<View> GetCurrentFocus() const { return _currentFocus.Lock(); }

    /// @brief Checks whether the current focus lies inside the given view.
    /// @param view The view to check for.
    /// @return True if the current focus lies inside the given view, or false otherwise.
    bool IsFocusInside(const View* view) const;

private:
    WeakPtr<View> _currentFocus;
};

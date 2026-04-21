#pragma once
#include "core/LinkedListLink.h"
#include "core/math/Point.h"
#include "core/math/Rectangle.h"
#include "core/SharedPtr.h"
#include "core/EnableSharedFromThis.h"
#include "../FocusManager.h"
#include "../FocusMoveDirection.h"

class GraphicsContext;
class VramContext;
class InputProvider;

/// @brief Base class for views.
class View : public EnableSharedFromThis<View>
{
public:
    /// @brief Link used for views that contain other views.
    LinkedListLink listLink;

    virtual ~View() { }

    /// @brief Initializes the vram for the view.
    /// @param vramContext The vram context to use.
    virtual void InitVram(const VramContext& vramContext) { }

    /// @brief Updates the view.
    virtual void Update() { }

    /// @brief Draws the view.
    /// @param graphicsContext The graphics context to use.
    virtual void Draw(GraphicsContext& graphicsContext) = 0;

    /// @brief Performs vblank processes for the view.
    virtual void VBlank() { }

    /// @brief Moves the focus from the currentFocus view into the given direction.
    /// @param currentFocus The currently focused view.
    /// @param direction The direction to move the focus in.
    /// @param source The view that requested this view to move focus.
    /// @return The newly focused view, or null if the focus didn't change.
    virtual SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
    {
        if (_parent && _parent != source)
            return _parent->MoveFocus(currentFocus, direction, this);

        return nullptr;
    }

    /// @brief Handles input for the view.
    /// @param inputProvider The input provider.
    /// @param focusManager The focus manager.
    /// @return True if input was handled, or false otherwise.
    virtual bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
    {
        if (_parent)
            return _parent->HandleInput(inputProvider, focusManager);

        return false;
    }

    /// @brief Handles a pen down event.
    /// @param touchPoint The touch point.
    /// @param focusManager The focus manager.
    virtual void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) { }

    /// @brief Handles a pen move event.
    /// @param touchPoint The touch point.
    /// @param focusManager The focus manager.
    virtual void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) { }

    /// @brief Handles a pen up event.
    /// @param lastTouchPoint The last touch point.
    /// @param focusManager The focus manager.
    virtual void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) { }

    /// @brief Gets the bounds of the view.
    /// @return The bounds of the view.
    virtual Rectangle GetBounds() const = 0;

    /// @brief Sets the position of the view.
    /// @param position The position to set.
    void SetPosition(const Point& position) { _position = position; }

    /// @brief Sets the position of the view.
    /// @param x The x position to set.
    /// @param y The y position to set.
    void SetPosition(int x, int y)
    {
        _position.x = x;
        _position.y = y;
    }

    /// @brief Gets the position of the view.
    /// @return The position of the view.
    const Point& GetPosition() const { return _position; }

    /// @brief Sets the parent of the view.
    /// @param parent The parent to set.
    void SetParent(View* parent) { _parent = parent; }

    /// @brief Gets the parent of the view.
    /// @return The parent of the view, or null if none.
    View* GetParent() const { return _parent; }

    /// @brief Sets whether the view is currently focused.
    /// @param focused True if the view is currently focused, or false otherwise.
    void SetFocused(bool focused) { _isFocused = focused; }

    /// @brief Gets whether the view is currently focused.
    /// @return True if the view is currently focused, or false otherwise.
    bool IsFocused() const { return _isFocused; }

protected:
    Point _position;
    bool _isFocused;

    View()
        : _position(0, 0), _isFocused(false), _parent(nullptr) { }

private:
    View* _parent;
};
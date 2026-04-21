#pragma once
#include "View.h"
#include "core/LinkedList.h"

/// @brief Base class for views that contain other views.
class ViewContainer : public View
{
public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

protected:
    /// @brief Adds a child to the head of the list.
    /// @param view The child to add.
    void AddChildHead(View* view)
    {
        _children.InsertHead(view);
        view->SetParent(this);
    }

    /// @brief Adds a child to the tail of the list.
    /// @param view The child to add.
    void AddChildTail(View* view)
    {
        _children.InsertTail(view);
        view->SetParent(this);
    }

    /// @brief Removes a child from the list.
    /// @param view The child to remove.
    void RemoveChild(View* view)
    {
        _children.Remove(view);
        view->SetParent(nullptr);
    }

private:
    LinkedList<View, &View::listLink> _children;
};

#include "common.h"
#include "ViewContainer.h"

void ViewContainer::InitVram(const VramContext& vramContext)
{
    for (auto& view : _children)
    {
        view.InitVram(vramContext);
    }
}

void ViewContainer::Update()
{
    for (auto& view : _children)
    {
        view.Update();
    }
}

void ViewContainer::Draw(GraphicsContext& graphicsContext)
{
    for (auto& view : _children)
    {
        view.Draw(graphicsContext);
    }
}

void ViewContainer::VBlank()
{
    for (auto& view : _children)
    {
        view.VBlank();
    }
}

void ViewContainer::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    for (auto& view : _children)
    {
        view.HandlePenDown(touchPoint, focusManager);
    }
}

void ViewContainer::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    for (auto& view : _children)
    {
        view.HandlePenMove(touchPoint, focusManager);
    }
}

void ViewContainer::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    for (auto& view : _children)
    {
        view.HandlePenUp(lastTouchPoint, focusManager);
    }
}

#include "common.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/input/InputProvider.h"
#include "IconGridItemView.h"

void IconGridItemView::InitVram(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _iconVramOffset = objVramManager->Alloc(FILE_ICON_VRAM_SIZE);
        _iconVram = objVramManager->GetVramAddress(_iconVramOffset);
    }
}

void IconGridItemView::Update()
{
    _viewModel->DisposeQueueTaskWhenComplete();

    if (_icon)
    {
        _icon->Update();
    }
}

bool IconGridItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    return _inputHandler.HandleInput(inputProvider, focusManager)
        || View::HandleInput(inputProvider, focusManager);
}

void IconGridItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenDown(touchPoint, focusManager);
}

void IconGridItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenMove(touchPoint, focusManager);
}

void IconGridItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenUp(lastTouchPoint, focusManager);
}

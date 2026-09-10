#include "common.h"
#include "gui/IVramManager.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "BannerListItemView.h"

BannerListItemView::BannerListItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
    SharedPtr<LabelView> firstLine, SharedPtr<LabelView> secondLine, SharedPtr<LabelView> thirdLine)
    : _viewModel(std::move(viewModel))
    , _firstLine(std::move(firstLine))
    , _secondLine(std::move(secondLine))
    , _thirdLine(std::move(thirdLine))
    , _inputHandler(this, _viewModel.get())
{
    AddChildTail(_firstLine.GetPointer());
    AddChildTail(_secondLine.GetPointer());
    AddChildTail(_thirdLine.GetPointer());
}

void BannerListItemView::Update()
{
    _viewModel->DisposeQueueTaskWhenComplete();

    // While this row's background load task is still pending, its completion
    // will call SetGameTitle()/SetFileName() - which render straight into
    // _firstLine/_secondLine/_thirdLine's text buffers - from the IO thread.
    // Marquee scrolling (below, via ViewContainer::Update() -> LabelView::
    // Update()) re-renders those same buffers every frame from the *main*
    // thread with no locking, so letting it run while the load is still in
    // flight races the IO thread and produces blank/garbled or wrong text -
    // most visibly on the focused row, since that's the one marquee applies
    // to. Skip it for this frame; once the load task completes it'll pick up
    // normally on the next one.
    if (_viewModel->IsQueueTaskPending())
    {
        if (_icon)
        {
            _icon->Update();
        }
        return;
    }

    if (IsFocused())
    {
        _firstLine->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
        _secondLine->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
        _thirdLine->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    }
    else
    {
        _firstLine->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
        _secondLine->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
        _thirdLine->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    }

    ViewContainer::Update();

    if (_icon)
    {
        _icon->Update();
    }
}

bool BannerListItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    return _inputHandler.HandleInput(inputProvider, focusManager)
        || View::HandleInput(inputProvider, focusManager);
}

void BannerListItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenDown(touchPoint, focusManager);
}

void BannerListItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenMove(touchPoint, focusManager);
}

void BannerListItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenUp(lastTouchPoint, focusManager);
}

#include "common.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/PaletteManager.h"
#include "gui/palette/GradientPalette.h"
#include "core/math/Rgb.h"
#include "smallHeartIconFilled.h"
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

FavoriteBadgeVramToken BannerListItemView::UploadFavoriteBadgeGraphics(const VramContext& vramContext)
{
    return FavoriteBadgeRenderer::UploadGraphics(vramContext);
}

void BannerListItemView::DrawFavoriteBadge(GraphicsContext& graphicsContext) const
{
    _favoriteBadgeRenderer.Draw(graphicsContext, GetBounds(), _viewModel->IsFavorite());
}

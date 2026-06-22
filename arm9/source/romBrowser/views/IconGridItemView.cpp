#include "common.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/input/InputProvider.h"
#include "gui/GraphicsContext.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/PaletteManager.h"
#include "gui/palette/GradientPalette.h"
#include "core/math/Rgb.h"
#include "smallHeartIconFilled.h"
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

IconGridItemView::FavoriteBadgeVramToken IconGridItemView::UploadFavoriteBadgeGraphics(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    u32 vramOffset = 0;
    if (objVramManager)
    {
        vramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
        dma_ntrCopy32(3, smallHeartIconFilledTiles, objVramManager->GetVramAddress(vramOffset), smallHeartIconFilledTilesLen);
    }
    return IconGridItemView::FavoriteBadgeVramToken(vramOffset);
}

void IconGridItemView::DrawFavoriteBadge(GraphicsContext& graphicsContext) const
{
    if (!_viewModel->IsFavorite())
    {
        return;
    }

    Rectangle bounds = GetBounds();
    int badgeX = bounds.GetRight() - 16;
    int badgeY = bounds.GetBottom() - 16;

    u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(Rgb<8, 8, 8>(255, 255, 255), Rgb<8, 8, 8>(229, 57, 53)),
        badgeY, badgeY + 16);

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<16, 16>(badgeX, badgeY, _favoriteBadgeVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);
}

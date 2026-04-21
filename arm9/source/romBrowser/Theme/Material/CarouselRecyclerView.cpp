#include "common.h"
#include <string.h>
#include <algorithm>
#include <libtwl/mem/memVram.h>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/materialDesign.h"
#include "gui/VramContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "romBrowser/FileType/FileCover.h"
#include "core/math/SinTable.h"
#include "carouselMask.h"
#include "animation/Interpolator.h"
#include "gui/input/InputProvider.h"
#include "CarouselRecyclerView.h"

#define COVER_SPACING           4
#define Y_OFFSET                56
#define CORNER_RADIUS           18
#define SELECTED_COVER_WIDTH    COVER_WIDTH
#define SELECTED_COVER_X        46
#define NEXT_COVER_WIDTH        54
#define NEXT_COVER_X            (SELECTED_COVER_X + SELECTED_COVER_WIDTH + COVER_SPACING)
#define SMALL_COVER_WIDTH       36
#define HORIZONTAL_PADDING      6

u32 CarouselRecyclerView::sMaskTextureVramOffset;

void CarouselRecyclerView::UploadGraphics(const VramContext& vramContext)
{
    const auto textureVramManager = vramContext.GetTexVramManager();
    if (textureVramManager)
    {
        sMaskTextureVramOffset = textureVramManager->Alloc(carouselMaskBitmapLen);
        dma_ntrCopy32(3, carouselMaskBitmap, textureVramManager->GetVramAddress(sMaskTextureVramOffset), carouselMaskBitmapLen);
    }
}

void CarouselRecyclerView::Update()
{
    if (_itemCount == 0)
    {
        return;
    }

    _scrollAnimator.Update();

    int rangeStartIndex = _scrollAnimator.GetValue().Int() - 4;
    int rangeEndIndex = _scrollAnimator.GetValue().Int() + 1 + 4;
    rangeStartIndex = std::clamp(rangeStartIndex, 0, (int)_itemCount - 1);
    rangeEndIndex = std::clamp(rangeEndIndex, 0, (int)_itemCount);

    if (_curRangeStart != rangeStartIndex || _curRangeLength != rangeEndIndex - rangeStartIndex)
    {
        LOG_DEBUG("range: %d - %d\n", rangeStartIndex, rangeEndIndex - 1);
        if (_curRangeLength != 0)
        {
            if (_curRangeStart < rangeStartIndex)
                ReleaseRange(_curRangeStart, rangeStartIndex);

            if (rangeEndIndex < _curRangeStart + _curRangeLength)
                ReleaseRange(rangeEndIndex, _curRangeStart + _curRangeLength);
        }

        BindRange(rangeStartIndex, rangeEndIndex);

        _curRangeStart = rangeStartIndex;
        _curRangeLength = rangeEndIndex - rangeStartIndex;
    }

    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        UpdateItemPosition(i);
        _viewPool[i].view->Update();
    }
}

void CarouselRecyclerView::Draw(GraphicsContext& graphicsContext)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        fix32<12> x = (_viewPoolEx[i].xPosition + 0.5).Int();
        fix32<12> width = (_viewPoolEx[i].width + 0.5).Int();
        fix32<12> left = x;
        if (left < HORIZONTAL_PADDING)
        {
            left = HORIZONTAL_PADDING;
        }
        fix32<12> right = x + width;
        if (right > (256 - HORIZONTAL_PADDING))
        {
            right = 256 - HORIZONTAL_PADDING;
        }
        if (left != right)
        {
            graphicsContext.SetPolygonId(i);
            Gx::MtxIdentity();
            Gx::MtxTranslate(0, 0, fix32<12>(-5 - std::abs(GetSelectedItem() - _viewPool[i].itemIdx)) / 64);
            RenderCoverMask(graphicsContext, left, right);

            // Render cover
            _viewPool[i].view->SetPosition((x - ((COVER_WIDTH - width) / 2) + 0.5).Int(), Y_OFFSET);
            _viewPool[i].view->Draw(graphicsContext);

            RenderRoundedCorners(graphicsContext, x, width);
        }
    }

    Gx::MtxIdentity();
}

void CarouselRecyclerView::RenderCoverMask(GraphicsContext& graphicsContext, fix32<12> left, fix32<12> right) const
{
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);
    Gx::TexImageParam((128 * 1024) >> 3, false, false, false, false, GX_TEXSIZE_8,
        GX_TEXSIZE_8, GX_TEXFMT_PLTT16, false, GX_TEXGEN_NONE);
    graphicsContext.GetRgb6Palette()->ApplyColor(Rgb<6, 6, 6>(_materialColorScheme->surfaceBright));
    Gx::TexCoord(0, 0);
    Gx::Begin(GX_PRIMITIVE_QUAD);
    {
        REG_GX_VTX_16 = GX_VTX_PACK((left / 64).GetRawValue(), Y_OFFSET << 3);
        REG_GX_VTX_16 = 0;
        REG_GX_VTX_XY = GX_VTX_PACK((left / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT) << 3);
        REG_GX_VTX_XY = GX_VTX_PACK((right / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT) << 3);
        REG_GX_VTX_XY = GX_VTX_PACK((right / 64).GetRawValue(), Y_OFFSET << 3);
    }
    Gx::End();
}

void CarouselRecyclerView::RenderRoundedCorners(GraphicsContext& graphicsContext, fix32<12> x, fix32<12> width) const
{
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_EQUAL, false, 31, 0);
    Gx::TexImageParam(sMaskTextureVramOffset >> 3, true, true, true, true, GX_TEXSIZE_32,
        GX_TEXSIZE_32, GX_TEXFMT_A5I3, false, GX_TEXGEN_NONE);

    graphicsContext.GetRgb6Palette()->ApplyColor(Rgb<6, 6, 6>(_materialColorScheme->inverseOnSurface));
    Gx::Begin(GX_PRIMITIVE_QUAD);
    // top-left
    Gx::TexCoord(0, 0);
    REG_GX_VTX_16 = GX_VTX_PACK((x / 64).GetRawValue(), Y_OFFSET << 3);
    REG_GX_VTX_16 = 0;
    Gx::TexCoord(0, CORNER_RADIUS);
    REG_GX_VTX_XY = GX_VTX_PACK((x / 64).GetRawValue(), (Y_OFFSET + CORNER_RADIUS) << 3);
    Gx::TexCoord(CORNER_RADIUS, CORNER_RADIUS);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + CORNER_RADIUS) / 64).GetRawValue(), (Y_OFFSET + CORNER_RADIUS) << 3);
    Gx::TexCoord(CORNER_RADIUS, 0);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + CORNER_RADIUS) / 64).GetRawValue(), Y_OFFSET << 3);

    // top-right
    Gx::TexCoord(-CORNER_RADIUS, 0);
    REG_GX_VTX_16 = GX_VTX_PACK(((x + width - CORNER_RADIUS) / 64).GetRawValue(), Y_OFFSET << 3);
    REG_GX_VTX_16 = 0;
    Gx::TexCoord(-CORNER_RADIUS, CORNER_RADIUS);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + width - CORNER_RADIUS) / 64).GetRawValue(), (Y_OFFSET + CORNER_RADIUS) << 3);
    Gx::TexCoord(0, CORNER_RADIUS);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + width) / 64).GetRawValue(), (Y_OFFSET + CORNER_RADIUS) << 3);
    Gx::TexCoord(0, 0);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + width) / 64).GetRawValue(), Y_OFFSET << 3);

    // bottom-left
    Gx::TexCoord(0, -CORNER_RADIUS);
    REG_GX_VTX_16 = GX_VTX_PACK((x / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT - CORNER_RADIUS) << 3);
    REG_GX_VTX_16 = 0;
    Gx::TexCoord(0, 0);
    REG_GX_VTX_XY = GX_VTX_PACK((x / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT) << 3);
    Gx::TexCoord(CORNER_RADIUS, 0);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + CORNER_RADIUS) / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT) << 3);
    Gx::TexCoord(CORNER_RADIUS, -CORNER_RADIUS);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + CORNER_RADIUS) / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT - CORNER_RADIUS) << 3);

    // bottom-right
    Gx::TexCoord(-CORNER_RADIUS, -CORNER_RADIUS);
    REG_GX_VTX_16 = GX_VTX_PACK(((x + width - CORNER_RADIUS) / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT - CORNER_RADIUS) << 3);
    REG_GX_VTX_16 = 0;
    Gx::TexCoord(-CORNER_RADIUS, 0);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + width - CORNER_RADIUS) / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT) << 3);
    Gx::TexCoord(0, 0);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + width) / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT) << 3);
    Gx::TexCoord(0, -CORNER_RADIUS);
    REG_GX_VTX_XY = GX_VTX_PACK(((x + width) / 64).GetRawValue(), (Y_OFFSET + COVER_HEIGHT - CORNER_RADIUS) << 3);
    Gx::End();
}

bool CarouselRecyclerView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_itemCount != 0 && inputProvider.Triggered(InputKey::L | InputKey::R))
    {
        int direction = inputProvider.Triggered(InputKey::L) ? -1 : 1;
        int selected = std::clamp(_selectedItem->itemIdx + 10 * direction, 0, (int)_itemCount - 1);

        focusManager.Unfocus();
        SetSelectedItem(selected, false);
        focusManager.Focus(_selectedItem->view);
        return true;
    }

    return View::HandleInput(inputProvider, focusManager);
}

void CarouselRecyclerView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
    {
        _penDown = true;
        _penDownPosition = touchPoint;
        _hasScrollStarted = false;
        _penDownScrollOffset = _scrollAnimator.GetValue();

        if (_itemCount > 0)
        {
            _selectedItem->view->HandlePenDown(touchPoint, focusManager);
        }
        for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
        {
            auto bounds = _viewPool[i].view->GetBounds();
            bounds = Rectangle(_viewPoolEx[i].xPosition.Int(), bounds.GetTop(), _viewPoolEx[i].width.Int(), bounds.GetHeight());
            if (bounds.Contains(touchPoint))
            {
                _viewPool[i].view->HandlePenDown(touchPoint, focusManager);
            }
        }
    }
}

void CarouselRecyclerView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!_penDown)
    {
        return;
    }

    if (!_hasScrollStarted)
    {
        for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
        {
            _viewPool[i].view->HandlePenMove(touchPoint, focusManager);
            if (focusManager.GetCurrentFocus().GetPointer() == _viewPool[i].view.GetPointer())
            {
                SetSelectedItem(_viewPool[i].itemIdx, false);
            }
        }

        int dx = touchPoint.x - _penDownPosition.x;
        int dy = touchPoint.y - _penDownPosition.y;
        if (dx * dx + dy * dy > 7 * 7)
        {
            bool shouldScrollStart = std::abs(touchPoint.x - _penDownPosition.x) > std::abs(touchPoint.y - _penDownPosition.y);
            if (shouldScrollStart)
            {
                _hasScrollStarted = true;
            }
            else
            {
                _penDown = false; //wrong direction drag, so cancel it
            }

            for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
            {
                _viewPool[i].view->HandlePenUp(Point(-1, -1), focusManager);
            }
        }
    }
    else
    {
        fix32<12> newScrollOffset = _penDownScrollOffset + fix32<12>(_penDownPosition.x - touchPoint.x) * (2.5 / COVER_WIDTH);
        if (newScrollOffset < 0)
        {
            newScrollOffset = 0;
            _penDownScrollOffset = 0;
            _penDownPosition.x = touchPoint.x;
        }
        else if (newScrollOffset > (int)_itemCount - 1)
        {
            newScrollOffset = (int)_itemCount - 1;
            _penDownScrollOffset = newScrollOffset;
            _penDownPosition.x = touchPoint.x;
        }

        _scrollAnimator = Animator<fix32<12>>(newScrollOffset);
    }
}

void CarouselRecyclerView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_hasScrollStarted)
    {
        SetSelectedItem((_scrollAnimator.GetValue() + 0.5).Int(), false);
        focusManager.Focus(_selectedItem->view);
    }

    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        _viewPool[i].view->HandlePenUp(lastTouchPoint, focusManager);
        if (focusManager.GetCurrentFocus().GetPointer() == _viewPool[i].view.GetPointer())
        {
            SetSelectedItem(_viewPool[i].itemIdx, false);
        }
    }

    _penDown = false;
}

void CarouselRecyclerView::SetSelectedItem(int itemIdx, bool initial)
{
    CoverFlowRecyclerViewBase::SetSelectedItem(itemIdx, initial);

    if (itemIdx < 0 || itemIdx >= (int)_itemCount)
    {
        return;
    }

    if (initial)
    {
        _scrollAnimator = Animator<fix32<12>>(itemIdx);
    }
    else
    {
        _scrollAnimator.Goto(itemIdx, md::sys::motion::duration::medium4, &md::sys::motion::easing::standard);
    }
}

void CarouselRecyclerView::UpdateItemPosition(int viewPoolIndex)
{
    auto& item = _viewPool[viewPoolIndex];
    auto& itemEx = _viewPoolEx[viewPoolIndex];
    int itemIndex = item.itemIdx;
    fix32<12> absOffsetFromCenter = (itemIndex - _scrollAnimator.GetValue()).Abs();
    fix32<12> initialOffsetFromCenter = absOffsetFromCenter.Clamp(0, 1);
    fix32<12> initialOffsetFromNext = (absOffsetFromCenter - 1).Clamp(0, 1);

    fix32<12> x;
    fix32<12> width;
    if (itemIndex < _scrollAnimator.GetValue())
    {
        // everything before selected
        x = SELECTED_COVER_X - (SMALL_COVER_WIDTH + COVER_SPACING) * absOffsetFromCenter;
        width = Interpolator::InterpolateLinear<fix32<12>>(SELECTED_COVER_WIDTH, SMALL_COVER_WIDTH, initialOffsetFromCenter);
    }
    else if (initialOffsetFromCenter < 1)
    {
        // from selected to next
        x = Interpolator::InterpolateLinear<fix32<12>>(SELECTED_COVER_X, NEXT_COVER_X, initialOffsetFromCenter);
        width = Interpolator::InterpolateLinear<fix32<12>>(SELECTED_COVER_WIDTH, NEXT_COVER_WIDTH, initialOffsetFromCenter);
    }
    else
    {
        // everything after next
        x = NEXT_COVER_X
            + Interpolator::InterpolateLinear<fix32<12>>(0, NEXT_COVER_WIDTH + COVER_SPACING, initialOffsetFromNext);
        if (absOffsetFromCenter - 2 > 0)
        {
            x = x + (SMALL_COVER_WIDTH + COVER_SPACING) * (absOffsetFromCenter - 2);
        }
        width = Interpolator::InterpolateLinear<fix32<12>>(NEXT_COVER_WIDTH, SMALL_COVER_WIDTH, initialOffsetFromNext);
    }

    itemEx.xPosition = x;
    itemEx.width = width;
}

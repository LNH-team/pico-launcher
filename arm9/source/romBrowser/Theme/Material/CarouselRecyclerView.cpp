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

    int rangeStartIndex = GetSelectedItem() - 4;
    int rangeEndIndex = GetSelectedItem() + 1 + 4;
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
        _viewPoolEx[i].xPositionAnimator.Update();
        _viewPoolEx[i].widthAnimator.Update();
        _viewPool[i].view->Update();
    }
}

void CarouselRecyclerView::Draw(GraphicsContext& graphicsContext)
{
    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        fix32<12> x = (_viewPoolEx[i].xPositionAnimator.GetValue() + 0.5).Int();
        fix32<12> width = (_viewPoolEx[i].widthAnimator.GetValue() + 0.5).Int();
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

Rectangle CarouselRecyclerView::GetSelectedItemTapBounds() const
{
    if (!_selectedItem)
        return Rectangle(0, 0, 0, 0);
    for (u32 i = _viewPoolFreeCount; i < _viewPool.size(); i++)
    {
        if (&_viewPool[i] == _selectedItem)
        {
            int x = (_viewPoolEx[i].xPositionAnimator.GetValue() + 0.5).Int();
            int w = (_viewPoolEx[i].widthAnimator.GetValue() + 0.5).Int();
            return Rectangle(x, Y_OFFSET, w, COVER_HEIGHT);
        }
    }
    return Rectangle(0, 0, 0, 0);
}

void CarouselRecyclerView::UpdateItemPosition(int viewPoolIndex, bool initial)
{
    ViewPoolEntry* item = &_viewPool[viewPoolIndex];
    ViewPoolEntryEx* itemEx = &_viewPoolEx[viewPoolIndex];
    int selectedIndex = GetSelectedItem();
    if (selectedIndex == -1)
    {
        selectedIndex = 0;
    }
    int itemIndex = item->itemIdx;
    fix32<12> x;
    fix32<12> width;
    if (itemIndex < selectedIndex)
    {
        x = SELECTED_COVER_X + (SMALL_COVER_WIDTH + COVER_SPACING) * (itemIndex - selectedIndex);
        width = SMALL_COVER_WIDTH;
    }
    else if (itemIndex == selectedIndex + 1)
    {
        x = NEXT_COVER_X;
        width = NEXT_COVER_WIDTH;
    }
    else if (itemIndex > selectedIndex + 1)
    {
        x = NEXT_COVER_X + NEXT_COVER_WIDTH + COVER_SPACING
            + (SMALL_COVER_WIDTH + COVER_SPACING) * (itemIndex - selectedIndex - 2);
        width = SMALL_COVER_WIDTH;
    }
    else
    {
        x = SELECTED_COVER_X;
        width = SELECTED_COVER_WIDTH;
    }

    if (initial)
    {
        itemEx->xPositionAnimator = Animator(x);
        itemEx->widthAnimator = Animator(width);
    }
    else
    {
        itemEx->xPositionAnimator.Goto(x, md::sys::motion::duration::medium4, &md::sys::motion::easing::standard);
        itemEx->widthAnimator.Goto(width, md::sys::motion::duration::medium4, &md::sys::motion::easing::standard);
    }
}

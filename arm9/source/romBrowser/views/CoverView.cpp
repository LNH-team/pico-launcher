#include "common.h"
#include "core/math/SinTable.h"
#include "gui/VramContext.h"
#include "gui/Gx.h"
#include "gui/materialDesign.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "CoverView.h"

void CoverView::InitVram(const VramContext& vramContext)
{
    const auto texVramManager = vramContext.GetTexVramManager();
    const auto texPlttVramManager = vramContext.GetTexPlttVramManager();
    if (texVramManager && texPlttVramManager)
    {
        _texVramOffset = texVramManager->Alloc(128 * 96);
        _plttVramOffset = texPlttVramManager->Alloc(256 * 2);
    }
}

void CoverView::Update()
{
    _viewModel->DisposeQueueTaskWhenComplete();
}

void CoverView::Draw(GraphicsContext& graphicsContext)
{
    if (_cover.Lock() && _textureLoadRequest.GetState() == VBlankTextureLoadRequestState::LoadComplete)
    {
        Gx::TexImageParam(_texVramOffset >> 3, false, true, false, true, GX_TEXSIZE_128,
            GX_TEXSIZE_128, GX_TEXFMT_PLTT256, false, GX_TEXGEN_NONE);
        Gx::TexPlttBase(_plttVramOffset >> 4);

        u32 polygonId = graphicsContext.GetPolygonId();
        Gx::PolygonAttr(GX_LIGHTMASK_0, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
            false, false, false, GX_DEPTH_FUNC_LESS, false, 31, polygonId);

        auto halfWidth = fix16<12>::FromRawValue(COVER_WIDTH << 8);
        auto halfHeight = fix16<12>::FromRawValue((COVER_HEIGHT / 6) << 8);
        Gx::Begin(GX_PRIMITIVE_QUAD);
        {
            Gx::Normal(0, 0, 0x1FF);
            Gx::TexCoord(0 + 0.25, -COVER_HEIGHT + 0.25);
            Gx::Vtx16(-halfWidth, -halfHeight, 0);
            Gx::TexCoord(0 + 0.25, 0 + 0.25);
            Gx::VtxXY(-halfWidth, halfHeight);
            Gx::TexCoord(COVER_WIDTH + 0.25, 0 + 0.25);
            Gx::VtxXY(halfWidth, halfHeight);
            Gx::TexCoord(COVER_WIDTH + 0.25, -COVER_HEIGHT + 0.25);
            Gx::VtxXY(halfWidth, -halfHeight);
        }
        Gx::End();

        Gx::PolygonAttr(GX_LIGHTMASK_0, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
            false, false, false, GX_DEPTH_FUNC_EQUAL, false, 16, polygonId);
        Gx::Begin(GX_PRIMITIVE_QUAD);
        {
            Gx::Normal(0, 0, 0x1FF);
            Gx::TexCoord(0 + 0.75, -COVER_HEIGHT + 0.75);
            Gx::Vtx16(-halfWidth, -halfHeight, 0);
            Gx::TexCoord(0 + 0.75, 0 + 0.75);
            Gx::VtxXY(-halfWidth, halfHeight);
            Gx::TexCoord(COVER_WIDTH + 0.75, 0 + 0.75);
            Gx::VtxXY(halfWidth, halfHeight);
            Gx::TexCoord(COVER_WIDTH + 0.75, -COVER_HEIGHT + 0.75);
            Gx::VtxXY(halfWidth, -halfHeight);
        }
        Gx::End();

        for (int i = 0; i < 20; ++i)
        {
            u32 alpha = 20 - i;
            Gx::PolygonAttr(GX_LIGHTMASK_0, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
                false, false, false, GX_DEPTH_FUNC_LESS, false, alpha, polygonId);
            Gx::Begin(GX_PRIMITIVE_QUAD);
            {
                Gx::Normal(0, 0, 0x1FF);
                Gx::TexCoord(0 + 0.5, fix16<4>(i) + 0.5);
                Gx::Vtx16(-halfWidth, fix16<12>::FromRawValue(((COVER_HEIGHT + i) << 9) / 6) - halfHeight, 0);
                Gx::TexCoord(0 + 0.5, fix16<4>(i) + 1 + 0.5);
                Gx::VtxXY(-halfWidth, fix16<12>::FromRawValue(((COVER_HEIGHT + i + 1) << 9) / 6) - halfHeight);
                Gx::TexCoord(COVER_WIDTH + 0.5, fix16<4>(i) + 1 + 0.5);
                Gx::VtxXY(halfWidth, fix16<12>::FromRawValue(((COVER_HEIGHT + i + 1) << 9) / 6) - halfHeight);
                Gx::TexCoord(COVER_WIDTH + 0.5, fix16<4>(i) + 0.5);
                Gx::VtxXY(halfWidth, fix16<12>::FromRawValue(((COVER_HEIGHT + i) << 9) / 6) - halfHeight);
            }
            Gx::End();
        }
    }
}

void CoverView::UploadCoverGraphics()
{
    if (auto cover = _cover.Lock())
    {
        cover->SetTexVramOffset(_texVramOffset, _plttVramOffset);
        _vblankTextureLoader->CancelLoad(_textureLoadRequest);
        _textureLoadRequest = cover->CreateTextureLoadRequest();
        _vblankTextureLoader->RequestLoad(_textureLoadRequest);
    }
}

bool CoverView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    return _inputHandler.HandleInput(inputProvider, focusManager)
        || View::HandleInput(inputProvider, focusManager);
}

void CoverView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenDown(touchPoint, focusManager);
}

void CoverView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenMove(touchPoint, focusManager);
}

void CoverView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenUp(lastTouchPoint, focusManager);
}

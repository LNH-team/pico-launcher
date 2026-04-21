#include "common.h"
#include "core/math/SinTable.h"
#include "gui/VramContext.h"
#include "gui/Gx.h"
#include "gui/materialDesign.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "MaterialCoverView.h"

void MaterialCoverView::InitVram(const VramContext& vramContext)
{
    const auto texVramManager = vramContext.GetTexVramManager();
    const auto texPlttVramManager = vramContext.GetTexPlttVramManager();
    if (texVramManager && texPlttVramManager)
    {
        _texVramOffset = texVramManager->Alloc(128 * 96);
        _plttVramOffset = texPlttVramManager->Alloc(256 * 2);
    }
}

void MaterialCoverView::Update()
{
    _viewModel->DisposeQueueTaskWhenComplete();
}

void MaterialCoverView::Draw(GraphicsContext& graphicsContext)
{
    if (_cover.Lock() && _textureLoadRequest.GetState() == VBlankTextureLoadRequestState::LoadComplete)
    {
        Gx::TexImageParam(_texVramOffset >> 3, false, true, false, true, GX_TEXSIZE_128,
            GX_TEXSIZE_128, GX_TEXFMT_PLTT256, false, GX_TEXGEN_NONE);
        Gx::TexPlttBase(_plttVramOffset >> 4);

        u32 polygonId = graphicsContext.GetPolygonId();
        Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
            false, false, false, GX_DEPTH_FUNC_EQUAL, false, 31, polygonId);

        Gx::Color(31, 31, 31);
        Gx::Begin(GX_PRIMITIVE_QUAD);
        Gx::TexCoord(0, -COVER_HEIGHT);
        REG_GX_VTX_16 = GX_VTX_PACK(_position.x << 6, _position.y << 3);
        REG_GX_VTX_16 = 0;
        Gx::TexCoord(0, 0);
        REG_GX_VTX_XY = GX_VTX_PACK(_position.x << 6, (_position.y + COVER_HEIGHT) << 3);
        Gx::TexCoord(COVER_WIDTH, 0);
        REG_GX_VTX_XY = GX_VTX_PACK((_position.x + COVER_WIDTH) << 6, (_position.y + COVER_HEIGHT) << 3);
        Gx::TexCoord(COVER_WIDTH, -COVER_HEIGHT);
        REG_GX_VTX_XY = GX_VTX_PACK((_position.x + COVER_WIDTH) << 6, _position.y << 3);
        Gx::End();
    }
}

bool MaterialCoverView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    return _inputHandler.HandleInput(inputProvider, focusManager)
        || View::HandleInput(inputProvider, focusManager);
}

void MaterialCoverView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenDown(touchPoint, focusManager);
}

void MaterialCoverView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenMove(touchPoint, focusManager);
}

void MaterialCoverView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    _inputHandler.HandlePenUp(lastTouchPoint, focusManager);
}

void MaterialCoverView::UploadCoverGraphics()
{
    if (auto cover = _cover.Lock())
    {
        cover->SetTexVramOffset(_texVramOffset, _plttVramOffset);
        _vblankTextureLoader->CancelLoad(_textureLoadRequest);
        _textureLoadRequest = cover->CreateTextureLoadRequest();
        _vblankTextureLoader->RequestLoad(_textureLoadRequest);
    }
}

#include "common.h"
#include "gui/Gx.h"
#include "gui/VramContext.h"
#include "romBrowser/views/IconButton3DView.h"
#include "CustomAppBarView.h"

CustomAppBarView::CustomAppBarView(int x, int y, Orientation orientation,
    int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme,
    u32 scrimTexVramOffset, u32 scrimPlttVramOffset)
    : AppBarView(x, y, orientation, startButtonCount, endButtonCount, materialColorScheme)
    , _scrimTexVramOffset(scrimTexVramOffset), _scrimPlttVramOffset(scrimPlttVramOffset)
{
    for (int i = 0; i < _startButtonCount + _endButtonCount; i++)
    {
        _buttons[i] = IconButton3DView::CreateShared(
            IconButtonView::Type::Tonal,
            IconButtonView::State::NoToggle,
            md::sys::color::inverseOnSurface,
            materialColorScheme);
        AddChildTail(_buttons[i].GetPointer());
    }
}

void CustomAppBarView::Draw(GraphicsContext& graphicsContext)
{
    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 63);
    Gx::Color(0);
    Gx::TexImageParam(_scrimTexVramOffset >> 3, true, false, false, false, GX_TEXSIZE_8,
        GX_TEXSIZE_64, GX_TEXFMT_A5I3, false, GX_TEXGEN_NONE);
    Gx::TexPlttBase(_scrimPlttVramOffset >> 4);
    Gx::Begin(GX_PRIMITIVE_QUAD);
    if (GetOrientation() == Orientation::Horizontal)
    {
        Gx::TexCoord(0, 0);
        Gx::Vtx16(0, 0, -1.0 / 64);
        Gx::TexCoord(0, 42);
        Gx::Vtx16(0, 42.0 / 512.0, -1.0 / 64);
        Gx::TexCoord(256, 42);
        Gx::Vtx16(256.0 / 64, 42.0 / 512.0, -1.0 / 64);
        Gx::TexCoord(256, 0);
        Gx::Vtx16(256.0 / 64, 0, -1.0 / 64);
    }
    else
    {
        Gx::TexCoord(0, 0);
        Gx::Vtx16(0, 192.0 / 512, -1.0 / 64);
        Gx::TexCoord(0, 42);
        Gx::Vtx16(42.0 / 64, 192.0 / 512, -1.0 / 64);
        Gx::TexCoord(192, 42);
        Gx::Vtx16(42.0 / 64, 0, -1.0 / 64);
        Gx::TexCoord(192, 0);
        Gx::Vtx16(0, 0, -1.0 / 64);
        Gx::End();
    }
    AppBarView::Draw(graphicsContext);
}
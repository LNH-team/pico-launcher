#include "common.h"
#include "gui/Gx.h"
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/GraphicsContext.h"
#include "themes/custom/CustomThemeInfo.h"
#include "CustomIconGridItemView.h"

#define X_OFFSET    (-2)
#define Y_OFFSET    (-2)
#define Z_OFFSET    (-5)
#define WIDTH       48
#define HEIGHT      48

void CustomIconGridItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(Rectangle(_position.x + X_OFFSET, _position.y + Y_OFFSET, WIDTH, HEIGHT)))
    {
        return;
    }

    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);
    Gx::Color(0x7FFF);

    u32 tex = (_isFocused || _inputHandler.IsPenDown()) ? _selectedTexVramOffset : _texVramOffset;
    u32 pltt = (_isFocused || _inputHandler.IsPenDown()) ? _selectedPlttVramOffset : _plttVramOffset;
    Gx::TexImageParam(tex >> 3, false, false, false, false, GX_TEXSIZE_64,
        GX_TEXSIZE_64, GX_TEXFMT_A3I5, false, GX_TEXGEN_NONE);
    Gx::TexPlttBase(pltt >> 4);
    Gx::Begin(GX_PRIMITIVE_QUAD);
    Gx::TexCoord(0, 0);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET) << 6, (_position.y + Y_OFFSET) << 3);
    REG_GX_VTX_16 = Z_OFFSET << 6;
    Gx::TexCoord(0, HEIGHT);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET) << 6, (_position.y + Y_OFFSET + HEIGHT) << 3);
    REG_GX_VTX_16 = Z_OFFSET << 6;
    Gx::TexCoord(WIDTH, HEIGHT);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET + WIDTH) << 6, (_position.y + Y_OFFSET + HEIGHT) << 3);
    REG_GX_VTX_16 = Z_OFFSET << 6;
    Gx::TexCoord(WIDTH, 0);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET + WIDTH) << 6, (_position.y + Y_OFFSET) << 3);
    REG_GX_VTX_16 = Z_OFFSET << 6;
    Gx::End();

    if (_icon)
    {
        _icon->SetPosition(6 + _position.x, 6 + _position.y);
        _icon->Draw(graphicsContext, _customThemeInfo->gridIconInfo.GetBlendColor());
    }
}

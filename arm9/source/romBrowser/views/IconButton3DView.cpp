#include "common.h"
#include "gui/Gx.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/input/InputProvider.h"
#include "iconButtonSelectorTexture.h"
#include "core/math/RgbMixer.h"
#include "core/math/ColorConverter.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "IconButton3DView.h"

u32 IconButton3DView::sSelectorTextureVramOffset = 0;

void IconButton3DView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
        return;

    u32 iconPaletteRow;
    if (_isFocused || _penDown)
    {
        const auto& selectorBaseColor = _materialColorScheme->GetColor(GetCircleBackgroundColor());
        const auto& fgColor = _materialColorScheme->GetColor(GetForegroundColor());
        auto selectorColor = RgbMixer::Lerp(selectorBaseColor, fgColor, 12, 100);
        DrawSelector(graphicsContext, selectorColor);

        iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(selectorColor, fgColor), _position.y + 8, _position.y + 24);
    }
    else
    {
        if (IsCircleBackgroundVisible())
        {
            auto circleBgColor = GetCircleBackgroundColor();
            DrawSelector(graphicsContext, _materialColorScheme->GetColor(circleBgColor));
            iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(
                    _materialColorScheme->GetColor(circleBgColor),
                    _materialColorScheme->GetColor(GetForegroundColor())),
                _position.y + 8, _position.y + 24);
        }
        else
        {
            iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(
                    _materialColorScheme->GetColor(_backgroundColor),
                    _materialColorScheme->GetColor(GetForegroundColor())),
                _position.y + 8, _position.y + 24);
        }
    }
    gfx_oam_entry_t* iconOam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<16, 16>(
            _position.x + 8,
            _position.y + 8, _iconVramOffset >> 7)
        .WithPalette16(iconPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(iconOam[0]);
}

void IconButton3DView::UploadGraphics(const VramContext& vramContext)
{
    const auto textureVramManager = vramContext.GetTexVramManager();
    if (textureVramManager)
    {
        sSelectorTextureVramOffset = textureVramManager->Alloc(iconButtonSelectorTextureBitmapLen);
        dma_ntrCopy32(3, iconButtonSelectorTextureBitmap, textureVramManager->GetVramAddress(sSelectorTextureVramOffset),
            iconButtonSelectorTextureBitmapLen);
    }
}

void IconButton3DView::DrawSelector(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& color)
{
    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 62);
    Gx::TexImageParam(sSelectorTextureVramOffset >> 3, true, true, true, true, GX_TEXSIZE_16,
        GX_TEXSIZE_16, GX_TEXFMT_A5I3, false, GX_TEXGEN_NONE);

    graphicsContext.GetRgb6Palette()->ApplyColor(Rgb<6, 6, 6>(color));
    Gx::Begin(GX_PRIMITIVE_QUAD);
    Gx::TexCoord(0, 0);
    Gx::Vtx16(fix16<12>(fix32<12>(_position.x) >> 6), fix16<12>(fix32<12>(_position.y) >> 9), -1.0 / 64);
    Gx::TexCoord(0, 32);
    Gx::Vtx16(fix16<12>(fix32<12>(_position.x) >> 6), fix16<12>(fix32<12>(_position.y + 32) >> 9), -1.0 / 64);
    Gx::TexCoord(32, 32);
    Gx::Vtx16(fix16<12>(fix32<12>(_position.x + 32) >> 6), fix16<12>(fix32<12>(_position.y + 32) >> 9), -1.0 / 64);
    Gx::TexCoord(32, 0);
    Gx::Vtx16(fix16<12>(fix32<12>(_position.x + 32) >> 6), fix16<12>(fix32<12>(_position.y) >> 9), -1.0 / 64);
    Gx::End();
}

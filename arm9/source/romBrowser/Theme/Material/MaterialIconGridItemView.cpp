#include "common.h"
#include <libtwl/gfx/gfx.h>
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "iconCell2.h"
#include "iconCell3.h"
#include "core/math/ColorConverter.h"
#include "core/math/RgbMixer.h"
#include "themes/material/MaterialColorScheme.h"
#include "gui/palette/DirectPalette.h"
#include "MaterialIconGridItemView.h"

void MaterialIconGridItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(Rectangle(_position.x - 2, _position.y - 2, 48, 48)))
        return;

    auto backColor = _materialColorScheme->inverseOnSurface;
    auto frontColor = (_isFocused || _inputHandler.IsPenDown())
        ? _materialColorScheme->mainIconBg
        : _materialColorScheme->surfaceBright;
    u16 selectedIconCellPltt[16];
    for (int i = 0; i < 16; i++)
    {
        auto blendFactors = ColorConverter::FromXBGR555(iconCell3Pal[i]);
        auto palColor = Rgb<8, 8, 8>(
            (backColor.r * blendFactors.r + frontColor.r * blendFactors.g + 16) / 31,
            (backColor.g * blendFactors.r + frontColor.g * blendFactors.g + 16) / 31,
            (backColor.b * blendFactors.r + frontColor.b * blendFactors.g + 16) / 31);
        palColor = palColor.Clamped();
        selectedIconCellPltt[i] = ColorConverter::ToGBGR565(palColor);
    }
    u32 cellPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(selectedIconCellPltt), _position.y - 2, _position.y - 2 + 48);

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<64, 64>(
            _position.x - 2,
            _position.y - 2, _bgVramOffset >> 7)
        .WithPalette16(cellPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);

    if (_icon)
    {
        _icon->SetPosition(6 + _position.x, 6 + _position.y);
        _icon->Draw(graphicsContext, frontColor);
    }

    DrawFavoriteBadge(graphicsContext);
}

MaterialIconGridItemView::VramToken MaterialIconGridItemView::UploadGraphics(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    u32 vramOffset = 0;
    if (objVramManager)
    {
        vramOffset = objVramManager->Alloc(iconCell3TilesLen);
        dma_ntrCopy32(3, iconCell3Tiles, objVramManager->GetVramAddress(vramOffset), iconCell3TilesLen);
    }

    return MaterialIconGridItemView::VramToken(vramOffset);
}

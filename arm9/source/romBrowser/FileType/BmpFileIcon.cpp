#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/OamBuilder.h"
#include "gui/palette/DirectPalette.h"
#include "gui/GraphicsContext.h"
#include "BmpFileIcon.h"

void BmpFileIcon::UploadGraphics()
{
    if (_vramAddress != nullptr)
    {
        dma_ntrCopy32(3, _iconData->GetGfx(), _vramAddress, BmpFileIconData::GfxSize);
    }
}

void BmpFileIcon::Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor)
{
    if (!graphicsContext.IsVisible(Rectangle(_position, 32, 32)) ||
        _vramAddress == nullptr)
    {
        return;
    }

    u32 paletteRowIdx = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(_iconData->GetPltt()), _position.y, _position.y + 32);

    // Character offset in OBJ VRAM is in 128-byte units for 32x32 4 bpp sprites
    auto builder = OamBuilder::OamWithSize<32, 32>(
            _position, _vramOffset >> 7)
        .WithPalette16(paletteRowIdx)
        .WithPriority(graphicsContext.GetPriority());

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    builder.Build(oam[0]);
}

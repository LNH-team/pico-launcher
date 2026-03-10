#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/GraphicsContext.h"
#include "gui/palette/DirectPalette.h"
#include "StaticIcon.h"

void StaticIcon::UploadGraphics(vu16* vram)
{
    dma_ntrCopy32(3, _tileData, vram, 512);
}

void StaticIcon::Update()
{

}

void StaticIcon::Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor)
{
    if (!graphicsContext.IsVisible(Rectangle(_position, 32, 32)))
        return;

    u32 paletteRowIdx = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(_paletteData), _position.y, _position.y + 32);

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<32, 32>(_position, _vramOffset >> 7)
        .WithPalette16(paletteRowIdx)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);
}
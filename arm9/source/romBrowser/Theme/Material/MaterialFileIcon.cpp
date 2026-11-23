#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/PaletteManager.h"
#include "gui/font/nitroFont2.h"
#include "core/math/RgbMixer.h"
#include "core/StringUtil.h"
#include "gui/OamBuilder.h"
#include "largeFolderIcon.h"
#include "gui/palette/GradientPalette.h"
#include "themes/IFontRepository.h"
#include "MaterialFileIcon.h"

MaterialFileIcon::MaterialFileIcon(const TCHAR* name, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _materialColorScheme(materialColorScheme), _fontRepository(fontRepository)
{
    // Filenames are UTF-8; copy complete characters rather than UTF-8 bytes.
    u32 length = StringUtil::Copy(_displayName, name, 4);
    auto font = _fontRepository->GetFont(FontType::Medium11);
    while (length > 0)
    {
        u32 textWidth, textHeight;
        nft2_measureString(font, _displayName, textWidth, textHeight);
        if (textWidth <= 32)
            break;
        // A 32-pixel icon fits two full-width Chinese characters, but normally
        // retains the original three-character abbreviation for Latin names.
        _displayName[--length] = 0;
    }
}

void MaterialFileIcon::UploadGraphics()
{
    if (_vramAddress != nullptr)
    {
        dma_ntrCopy32(3, GetIconTiles(), _vramAddress, 32 * 32 / 2);

        auto font = _fontRepository->GetFont(FontType::Medium11);
        u8 tileBuffer[32 * 16 / 2];
        memset(tileBuffer, 0, sizeof(tileBuffer));
        u32 textWidth, textHeight;
        nft2_measureString(font, _displayName, textWidth, textHeight);
        nft2_string_render_params_t renderParams;
        renderParams.x = ((int)32 - (int)textWidth) / 2;
        renderParams.y = 0;
        renderParams.width = 32;
        renderParams.height = 16;
        renderParams.a5i3 = false;
        nft2_renderString(font, _displayName, tileBuffer, 32, &renderParams);
        memcpy((u8*)_vramAddress + largeFolderIconTilesLen, tileBuffer, sizeof(tileBuffer));
    }
}

void MaterialFileIcon::Draw(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& backgroundColor)
{
    auto iconColor = GetIconColor();
    auto nameColor = GetTextColor();

    auto oams = graphicsContext.GetOamManager().AllocOams(2);

    u32 iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(backgroundColor, iconColor), _position.y, _position.y + 32);
    OamBuilder::OamWithSize<32, 32>(_position.x, _position.y, _vramOffset >> 7)
        .WithPalette16(iconPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[1]);

    u32 namePaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(iconColor, nameColor), _position.y, _position.y + 32);
    OamBuilder::OamWithSize<32, 16>(_position.x, _position.y + GetTextYOffset(), (_vramOffset + largeFolderIconTilesLen) >> 7)
        .WithPalette16(namePaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[0]);
}
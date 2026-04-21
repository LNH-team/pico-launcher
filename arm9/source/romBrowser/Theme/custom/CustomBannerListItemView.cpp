#include "common.h"
#include <libtwl/gfx/gfx.h>
#include "core/math/Rgb.h"
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/Gx.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "core/math/ColorConverter.h"
#include "core/math/RgbMixer.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "themes/custom/CustomThemeInfo.h"
#include "bannerListItemBg0.h"
#include "bannerListItemBg1.h"
#include "bannerListItemBg2.h"
#include "gui/palette/GradientPalette.h"
#include "gui/palette/DirectPalette.h"
#include "gui/views/Label3DView.h"
#include "CustomBannerListItemView.h"

#define X_OFFSET    (-3)
#define Y_OFFSET    (-2)
#define Z_OFFSET    300
#define WIDTH       209
#define HEIGHT      49

#define LINE_WIDTH              160
#define LINE_HEIGHT             16
#define MAX_LINE_STRING_LENGTH  50

CustomBannerListItemView::CustomBannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel,
    const CustomThemeInfo* customThemeInfo, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository, u32 texVramOffset, u32 plttVramOffset,
    u32 selectedTexVramOffset, u32 selectedPlttVramOffset,
    VBlankTextureLoader* vblankTextureLoader)
    : BannerListItemView(std::move(viewModel),
        Label3DView::CreateShared(LINE_WIDTH, LINE_HEIGHT, MAX_LINE_STRING_LENGTH,
            fontRepository->GetFont(FontType::Medium10), vblankTextureLoader),
        Label3DView::CreateShared(LINE_WIDTH, LINE_HEIGHT, MAX_LINE_STRING_LENGTH,
            fontRepository->GetFont(FontType::Regular10), vblankTextureLoader),
        Label3DView::CreateShared(LINE_WIDTH, LINE_HEIGHT, MAX_LINE_STRING_LENGTH,
            fontRepository->GetFont(FontType::Regular10), vblankTextureLoader))
    , _customThemeInfo(customThemeInfo)
    , _materialColorScheme(materialColorScheme)
    , _texVramOffset(texVramOffset)
    , _plttVramOffset(plttVramOffset)
    , _selectedTexVramOffset(selectedTexVramOffset)
    , _selectedPlttVramOffset(selectedPlttVramOffset) { }

void CustomBannerListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(Rectangle(_position.x + X_OFFSET, _position.y + Y_OFFSET, WIDTH, HEIGHT)))
    {
        return;
    }

    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);
    Gx::Color(0x7FFF);

    u32 tex = _isFocused ? _selectedTexVramOffset : _texVramOffset;
    u32 pltt = _isFocused ? _selectedPlttVramOffset : _plttVramOffset;
    Gx::TexImageParam(tex >> 3, false, false, false, false, GX_TEXSIZE_256,
        GX_TEXSIZE_64, GX_TEXFMT_A3I5, false, GX_TEXGEN_NONE);
    Gx::TexPlttBase(pltt >> 4);
    Gx::Begin(GX_PRIMITIVE_QUAD);
    Gx::TexCoord(0, 0);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET) << 6, (_position.y + Y_OFFSET) << 3);
    REG_GX_VTX_16 = (Z_OFFSET) << 6;
    Gx::TexCoord(0, HEIGHT);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET) << 6, (_position.y + Y_OFFSET + HEIGHT) << 3);
    REG_GX_VTX_16 = (Z_OFFSET) << 6;
    Gx::TexCoord(WIDTH, HEIGHT);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET + WIDTH) << 6, (_position.y + Y_OFFSET + HEIGHT) << 3);
    REG_GX_VTX_16 = (Z_OFFSET) << 6;
    Gx::TexCoord(WIDTH, 0);
    REG_GX_VTX_16 = GX_VTX_PACK((_position.x + X_OFFSET + WIDTH) << 6, (_position.y + Y_OFFSET) << 3);
    REG_GX_VTX_16 = (Z_OFFSET) << 6;
    Gx::End();

    graphicsContext.SetPolygonId(1);

    _firstLine->SetForegroundColor(_customThemeInfo->bannerListTextLine0Info.GetTextColor());
    _secondLine->SetForegroundColor(_customThemeInfo->bannerListTextLine1Info.GetTextColor());
    _thirdLine->SetForegroundColor(_customThemeInfo->bannerListTextLine2Info.GetTextColor());

    if (_lines == 1)
    {
        _firstLine->SetPosition(_position.x + 6 + 32 + 6, _position.y + 14);
    }
    else if (_lines == 2)
    {
        _firstLine->SetPosition(_position.x + 6 + 32 + 6, _position.y + 8);
        _secondLine->SetPosition(_position.x + 6 + 32 + 6, _position.y + 20);
    }
    else
    {
        _firstLine->SetPosition(_position.x + 6 + 32 + 6, _position.y + 2);
        _secondLine->SetPosition(_position.x + 6 + 32 + 6, _position.y + 14);
        _thirdLine->SetPosition(_position.x + 6 + 32 + 6, _position.y + 26);
    }

    if (_lines >= 1)
    {
        _firstLine->Draw(graphicsContext);
    }
    if (_lines >= 2)
    {
        _secondLine->Draw(graphicsContext);
    }
    if (_lines >= 3)
    {
        _thirdLine->Draw(graphicsContext);
    }

    if (_icon)
    {
        _icon->SetPosition(6 + _position.x, 6 + _position.y);
        _icon->Draw(graphicsContext, _customThemeInfo->bannerListIconInfo.GetBlendColor());
    }
}

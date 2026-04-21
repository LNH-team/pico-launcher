#include "common.h"
#include <libtwl/gfx/gfx.h>
#include "gui/PaletteManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "core/math/ColorConverter.h"
#include "core/math/RgbMixer.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "bannerListItemBg0.h"
#include "bannerListItemBg1.h"
#include "bannerListItemBg2.h"
#include "gui/palette/GradientPalette.h"
#include "gui/palette/DirectPalette.h"
#include "gui/views/Label2DView.h"
#include "MaterialBannerListItemView.h"

MaterialBannerListItemView::MaterialBannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : BannerListItemView(std::move(viewModel),
        Label2DView::CreateShared(160, 16, 50, fontRepository->GetFont(FontType::Medium10)),
        Label2DView::CreateShared(160, 16, 50, fontRepository->GetFont(FontType::Regular10)),
        Label2DView::CreateShared(160, 16, 50, fontRepository->GetFont(FontType::Regular10)))
    , _materialColorScheme(materialColorScheme) { }

void MaterialBannerListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(Rectangle(_position.x - 2, _position.y - 2, 207, 48)))
        return;

    auto backColor = _materialColorScheme->inverseOnSurface;
    auto frontColor = _isFocused
        ? _materialColorScheme->mainIconBg
        : _materialColorScheme->surfaceBright;
    u16 bgPltt[16];
    for (int i = 0; i < 16; i++)
    {
        auto blendFactors = ColorConverter::FromXBGR555(bannerListItemBg0Pal[i]);
        auto palColor = Rgb<8, 8, 8>(
            (backColor.r * blendFactors.r + frontColor.r * blendFactors.g + 16) / 31,
            (backColor.g * blendFactors.r + frontColor.g * blendFactors.g + 16) / 31,
            (backColor.b * blendFactors.r + frontColor.b * blendFactors.g + 16) / 31);
        palColor = palColor.Clamped();
        bgPltt[i] = ColorConverter::ToGBGR565(palColor);
    }
    u32 bgPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
        DirectPalette(bgPltt), _position.y - 2, _position.y - 2 + 48);

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(4);
    OamBuilder::OamWithSize<64, 64>(
            _position.x - 2,
            _position.y - 2, _bgVramOffset >> 7)
        .WithPalette16(bgPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);
    OamBuilder::OamWithSize<64, 64>(
            _position.x - 2 + 64,
            _position.y - 2, (_bgVramOffset + bannerListItemBg0TilesLen) >> 7)
        .WithPalette16(bgPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[1]);
    OamBuilder::OamWithSize<64, 64>(
            _position.x - 2 + 64 + 64,
            _position.y - 2, (_bgVramOffset + bannerListItemBg0TilesLen) >> 7)
        .WithPalette16(bgPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[2]);
    OamBuilder::OamWithSize<32, 64>(
            _position.x - 2 + 64 + 64 + 64,
            _position.y - 2, (_bgVramOffset + bannerListItemBg0TilesLen + bannerListItemBg1TilesLen) >> 7)
        .WithPalette16(bgPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[3]);

    if (_isFocused)
    {
        _firstLine->SetBackgroundColor(frontColor);
        _firstLine->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
        _secondLine->SetBackgroundColor(frontColor);
        _secondLine->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
        _thirdLine->SetBackgroundColor(frontColor);
        _thirdLine->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
    }
    else
    {
        _firstLine->SetForegroundColor(_materialColorScheme->onSurface);
        _firstLine->SetBackgroundColor(_materialColorScheme->surfaceBright);
        _secondLine->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _secondLine->SetBackgroundColor(_materialColorScheme->surfaceBright);
        _thirdLine->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _thirdLine->SetBackgroundColor(_materialColorScheme->surfaceBright);
    }

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
        _firstLine->Draw(graphicsContext);
    if (_lines >= 2)
        _secondLine->Draw(graphicsContext);
    if (_lines >= 3)
        _thirdLine->Draw(graphicsContext);

    if (_icon)
    {
        _icon->SetPosition(6 + _position.x, 6 + _position.y);
        _icon->Draw(graphicsContext, frontColor);
    }
}

BannerListItemView::VramToken MaterialBannerListItemView::UploadGraphics(const VramContext& vramContext)
{
    const auto objVramManager = vramContext.GetObjVramManager();
    u32 vramOffset = 0;
    if (objVramManager)
    {
        vramOffset = objVramManager->Alloc(
            bannerListItemBg0TilesLen + bannerListItemBg1TilesLen + bannerListItemBg2TilesLen);
        dma_ntrCopy32(3, bannerListItemBg0Tiles,
            objVramManager->GetVramAddress(vramOffset),
            bannerListItemBg0TilesLen);
        dma_ntrCopy32(3, bannerListItemBg1Tiles,
            objVramManager->GetVramAddress(vramOffset + bannerListItemBg0TilesLen),
            bannerListItemBg1TilesLen);
        dma_ntrCopy32(3, bannerListItemBg2Tiles,
            objVramManager->GetVramAddress(vramOffset + bannerListItemBg0TilesLen + bannerListItemBg1TilesLen),
            bannerListItemBg2TilesLen);
    }
    return BannerListItemView::VramToken(vramOffset);
}
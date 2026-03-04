#include "common.h"
#include <algorithm>
#include <libtwl/dma/dmaNitro.h>
#include "gui/IVramManager.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/GraphicsContext.h"
#include "core/math/ColorConverter.h"
#include "core/math/RgbMixer.h"
#include "chipFilled.h"
#include "gui/palette/DirectPalette.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "ChipView.h"

void ChipView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
        return;

    u32 paletteRow;
    auto bgColor = _materialColorScheme->GetColor(_backgroundColor);
    Rgb<8, 8, 8> fgColor;
    if (_isSelected)
    {
        fgColor = _materialColorScheme->secondaryContainer;
        if (_isFocused)
            fgColor = RgbMixer::Lerp(fgColor, _materialColorScheme->onSurfaceVariant, 12, 100);
        u16 chipPltt[16];
        for (int i = 0; i < 16; i++)
        {
            auto blendFactors = ColorConverter::FromXBGR555(chipFilledPal[i]);
            auto palColor = Rgb<8, 8, 8>(
                (bgColor.r * blendFactors.g + fgColor.r * (blendFactors.r + blendFactors.b) + 16) / 31,
                (bgColor.g * blendFactors.g + fgColor.g * (blendFactors.r + blendFactors.b) + 16) / 31,
                (bgColor.b * blendFactors.g + fgColor.b * (blendFactors.r + blendFactors.b) + 16) / 31);
            chipPltt[i] = ColorConverter::ToGBGR565(palColor.Clamped());
        }
        paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            DirectPalette(chipPltt), _position.y, _position.y + 20);
        if (!_isFocused)
        {
            _label.SetBackgroundColor(_materialColorScheme->secondaryContainer);
            _label.SetForegroundColor(_materialColorScheme->onSecondaryContainer);
            _secondaryLabel.SetBackgroundColor(_materialColorScheme->secondaryContainer);
            _secondaryLabel.SetForegroundColor(_materialColorScheme->onSecondaryContainer);
        }
        else
        {
            _label.SetBackgroundColor(fgColor);
            _label.SetForegroundColor(_materialColorScheme->onSecondaryContainer);
            _secondaryLabel.SetBackgroundColor(fgColor);
            _secondaryLabel.SetForegroundColor(_materialColorScheme->onSecondaryContainer);
        }
    }
    else
    {
        auto selectorFullColor = _materialColorScheme->onSurfaceVariant;
        auto outlineColor = _isFocused ? _materialColorScheme->onSurfaceVariant : _materialColorScheme->outline;
        fgColor = _isFocused ? RgbMixer::Lerp(bgColor, selectorFullColor, 12, 100) : bgColor;

        u16 chipPltt[16];
        for (int i = 0; i < 16; i++)
        {
            auto blendFactors = ColorConverter::FromXBGR555(chipFilledPal[i]);
            auto palColor = Rgb<8, 8, 8>(
                (fgColor.r * blendFactors.r + bgColor.r * blendFactors.g + outlineColor.r * blendFactors.b + 16) / 31,
                (fgColor.g * blendFactors.r + bgColor.g * blendFactors.g + outlineColor.g * blendFactors.b + 16) / 31,
                (fgColor.b * blendFactors.r + bgColor.b * blendFactors.g + outlineColor.b * blendFactors.b + 16) / 31);
            chipPltt[i] = ColorConverter::ToGBGR565(palColor.Clamped());
        }
        paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            DirectPalette(chipPltt), _position.y, _position.y + 20);

        if (!_isFocused)
        {
            _label.SetBackgroundColor(_materialColorScheme->GetColor(_backgroundColor));
            _label.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _secondaryLabel.SetBackgroundColor(_materialColorScheme->GetColor(_backgroundColor));
            _secondaryLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        }
        else
        {
            _label.SetBackgroundColor(fgColor);
            _label.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _secondaryLabel.SetBackgroundColor(fgColor);
            _secondaryLabel.SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        }
    }

    int textWidth = _label.GetStringWidth();
    int finalWidth = GetWidth();

    int rightX = _position.x + finalWidth - 64;
    int middleCount = 0;
    for (int x = _position.x + 32; x < rightX; x += 32)
        middleCount++;

    auto oams = graphicsContext.GetOamManager().AllocOams(2 + middleCount);

    int oamIdx = 0;
    OamBuilder::OamWithSize<64, 32>(_position, _vramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[oamIdx++]);

    for (int x = _position.x + 32; x < rightX; x += 32)
    {
        OamBuilder::OamWithSize<64, 32>(
                x,
                _position.y, _vramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oams[oamIdx++]);
    }

    OamBuilder::OamWithSize<64, 32>(
            rightX,
            _position.y, _vramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .WithHFlip()
        .Build(oams[oamIdx]);

    DrawIcon(graphicsContext, fgColor);

    int labelX;
    if (_iconVramOffset == 0xFFFFFFFF)
    {
        if (_centeredText)
            labelX = _position.x + (finalWidth - textWidth) / 2;
        else
            labelX = _position.x + 8;
    }
    else
    {
        labelX = _position.x + 22;
    }

    _label.SetPosition(labelX, _position.y + (_hasSecondaryText ? 1 : 3));
    _label.Draw(graphicsContext);

    if (_hasSecondaryText)
    {
        int secondaryTextWidth = _secondaryLabel.GetStringWidth();
        int secondaryX;
        if (_iconVramOffset == 0xFFFFFFFF)
        {
            if (_centeredText)
                secondaryX = _position.x + (finalWidth - secondaryTextWidth) / 2;
            else
                secondaryX = _position.x + 8;
        }
        else
        {
            secondaryX = _position.x + 22;
        }

        _secondaryLabel.SetPosition(secondaryX, _position.y + 13);
        _secondaryLabel.Draw(graphicsContext);
    }
}

void ChipView::DrawIcon(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& fgColor)
{
    if (_iconVramOffset == 0xFFFFFFFF)
        return;

    u32 iconPaletteRow;
    if (_isFocused)
    {
        iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(fgColor, _materialColorScheme->primary),
            _position.y, _position.y + 20);
    }
    else
    {
        iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(
                _isSelected
                    ? _materialColorScheme->secondaryContainer
                    : _materialColorScheme->GetColor(_backgroundColor),
                _materialColorScheme->primary),
            _position.y, _position.y + 20);
    }
    auto iconOam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<16, 16>(_position.x + 5, _position.y + 4, _iconVramOffset >> 7)
        .WithPalette16(iconPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(iconOam[0]);
}

ChipView::VramToken ChipView::UploadGraphics(IVramManager& vramManager)
{
    u32 vramOffset = vramManager.Alloc(chipFilledTilesLen);
    dma_ntrCopy32(3, chipFilledTiles, vramManager.GetVramAddress(vramOffset), chipFilledTilesLen);
    return ChipView::VramToken(vramOffset);
}
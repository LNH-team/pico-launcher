#pragma once
#include "common.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "core/math/Rgb.h"
#include "favoriteBadge.h"

class FavoriteBadgeVramToken
{
    u32 _vramOffset;
public:
    FavoriteBadgeVramToken()
        : _vramOffset(0) { }

    explicit FavoriteBadgeVramToken(u32 offset)
        : _vramOffset(offset) { }

    constexpr u32 GetVramOffset() const { return _vramOffset; }
};

namespace FavoriteBadge
{
    inline FavoriteBadgeVramToken UploadGraphics(const VramContext& vramContext)
    {
        const auto objVramManager = vramContext.GetObjVramManager();
        u32 vramOffset = 0;
        if (objVramManager)
        {
            vramOffset = objVramManager->Alloc(favoriteBadgeTilesLen);
            dma_ntrCopy32(3, favoriteBadgeTiles, objVramManager->GetVramAddress(vramOffset), favoriteBadgeTilesLen);
        }
        return FavoriteBadgeVramToken(vramOffset);
    }

    inline void Draw(GraphicsContext& graphicsContext, int badgeX, int badgeY, u32 favoriteBadgeVramOffset,
        const Rgb<8, 8, 8>& badgeColor)
    {
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(Rgb<8, 8, 8>(255, 255, 255), badgeColor),
            badgeY, badgeY + 16);

        gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
        OamBuilder::OamWithSize<16, 16>(badgeX, badgeY, favoriteBadgeVramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oam[0]);
    }
}

/// @brief The badge's fixed ink color, chosen by theme darkness (same MD3 error40/error80
/// tones used for the battery warning ink elsewhere in this project). Themes with no
/// light/dark concept of their own (custom themes) pass false and get the light-mode tone.
inline Rgb<8, 8, 8> FavoriteBadgeColor(bool darkTheme)
{
    return darkTheme
        ? Rgb<8, 8, 8>(0xFF, 0xB4, 0xAB)  // MD3 error80
        : Rgb<8, 8, 8>(0xB3, 0x26, 0x1E); // MD3 error40
}

/// @brief Composes the favorite-badge VRAM token and draw color so BannerListItemView and
/// IconGridItemView (which share no common base today) don't each repeat the same upload/
/// draw wrapper logic.
class FavoriteBadgeRenderer
{
    FavoriteBadgeVramToken _vramToken;
    Rgb<8, 8, 8> _color = FavoriteBadgeColor(false);

public:
    static FavoriteBadgeVramToken UploadGraphics(const VramContext& vramContext)
    {
        return FavoriteBadge::UploadGraphics(vramContext);
    }

    void SetVramToken(const FavoriteBadgeVramToken& vramToken) { _vramToken = vramToken; }
    void SetColor(const Rgb<8, 8, 8>& color) { _color = color; }

    void Draw(GraphicsContext& graphicsContext, const Rectangle& bounds, bool isFavorite) const
    {
        if (!isFavorite)
        {
            return;
        }
        FavoriteBadge::Draw(graphicsContext, bounds.GetRight() - 16, bounds.GetBottom() - 16,
            _vramToken.GetVramOffset(), _color);
    }
};

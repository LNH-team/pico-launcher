#include "common.h"
#include <string.h>
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "romBrowser/status/StatusViewModel.h"
#include "romBrowser/status/StatusColors.h"
#include "batteryEmpty.h"
#include "batteryLow.h"
#include "batteryMedium.h"
#include "batteryHigh.h"
#include "batteryFull.h"
#include "batteryCharging.h"
#include "MaterialStatusView.h"

namespace
{
    struct BatteryGraphic
    {
        const void* tiles;
        u32 length;
    };

    /// @brief Indexed by BatteryIcon (Empty, Low, Medium, High, Full, Charging).
    const BatteryGraphic kBatteryGraphics[] = {
        { batteryEmptyTiles,    batteryEmptyTilesLen },
        { batteryLowTiles,      batteryLowTilesLen },
        { batteryMediumTiles,   batteryMediumTilesLen },
        { batteryHighTiles,     batteryHighTilesLen },
        { batteryFullTiles,     batteryFullTilesLen },
        { batteryChargingTiles, batteryChargingTilesLen },
    };
}

MaterialStatusView::MaterialStatusView(StatusViewModel* viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _clock(Label2DView::CreateShared(72, 16, 16, fontRepository->GetFont(FontType::Medium10Clock)))
    , _viewModel(viewModel)
    , _materialColorScheme(materialColorScheme)
{
    const auto& background = materialColorScheme->inverseOnSurface;
    _darkTheme = ((int)background.r + (int)background.g + (int)background.b) < (128 * 3);

    _clock->SetPosition(4, 2);
    AddChildTail(_clock.GetPointer());
}

void MaterialStatusView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        for (int i = 0; i < BatteryIconCount; i++)
        {
            const auto& graphic = kBatteryGraphics[i];
            _batteryVramOffsets[i] = objVramManager->Alloc(graphic.length);
            dma_ntrCopy32(3, graphic.tiles,
                objVramManager->GetVramAddress(_batteryVramOffsets[i]), graphic.length);
        }
    }
}

void MaterialStatusView::Update()
{
    _viewModel->Update();

    _batteryIcon = _viewModel->GetBatteryIcon();

    const char* clockText = _viewModel->GetClockText();
    if (strcmp(clockText, _lastClockText) != 0)
    {
        _clock->SetText(clockText);
        strcpy(_lastClockText, clockText);
    }

    ViewContainer::Update();
}

void MaterialStatusView::Draw(GraphicsContext& graphicsContext)
{
    const auto& background = _materialColorScheme->inverseOnSurface;
    const Rgb<8, 8, 8> ink = IsWarning(_batteryIcon)
        ? StatusWarningColor(_darkTheme)
        : _materialColorScheme->onSurface;

    _clock->SetBackgroundColor(background);
    _clock->SetForegroundColor(_materialColorScheme->onSurface);

    const int batteryX = 236;
    const int batteryY = 1;
    u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(background, ink), batteryY, batteryY + 16);

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<16, 16>(batteryX, batteryY, _batteryVramOffsets[(int)_batteryIcon] >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oam[0]);

    ViewContainer::Draw(graphicsContext);
}

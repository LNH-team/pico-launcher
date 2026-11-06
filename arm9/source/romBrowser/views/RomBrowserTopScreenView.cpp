#include "common.h"
#include <algorithm>
#include <string.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
#include <libtwl/dma/dmaNitro.h>
#include "core/mini-printf.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "MarkerColors.h"
#include "heartMarker.h"
#include "checkMarker.h"
#include "starMarker.h"
#include "stripChipBg.h"
#include "RomBrowserTopScreenView.h"

// The launch info text next to the markers - "3x 1h20", "3x · 16 Jul" - is
// switched off for now, and kept behind this rather than removed: the time it
// shows is the clock running while a game was open, not time played (issue #9),
// so it overstates. The favorite heart and completed check stay on screen.
#define SHOW_TOP_LAUNCH_INFO_TEXT 0

// Markers are 16 px sprites with 12 px of shape inside, laid 15 apart so that
// three of them read as one group rather than three things in a row.
#define MARKER_STRIDE 15

RomBrowserTopScreenView::RomBrowserTopScreenView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const IFontRepository* fontRepository,
    const MaterialColorScheme* materialColorScheme)
    : _viewModel(std::move(viewModel))
    , _themeFileIconFactory(themeFileIconFactory)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _coverPosition(romBrowserViewFactory->GetTopCoverPosition())
{
    AddChildTail(_fileInfoView.GetPointer());

    // the markers' row is placed by the theme (see TopStripElementLayout), so a
    // custom theme can move or hide it away from its own top art; clamp to the
    // screen so a malformed theme.json can't place the OBJs at coordinates that
    // wrap in OAM. The game count that used to sit in the top-left corner now
    // opens the statistics panel instead.
    auto launchInfoLayout = romBrowserViewFactory->GetTopLaunchInfoLayout();
    _launchInfoHidden = launchInfoLayout.hidden;
    _launchInfoCentered = launchInfoLayout.centered;
    _launchInfoBare = launchInfoLayout.bare;
    _launchInfoPosition = Point(std::clamp(launchInfoLayout.position.x, 0, 256),
        std::clamp(launchInfoLayout.position.y, 0, 192));

    _gameDataService = _viewModel->GetRomBrowserController()->GetGameDataService();
    _materialColorScheme = materialColorScheme;
    // launch info for the selected game ("3x 16/07"), packed right to left in
    // the top strip with the completed check + favorite heart; Draw() positions
    // it so the cluster's chip is sized to its content
    _launchInfoLabel = Label2DView::CreateShared(96, 16, 15, fontRepository->GetFont(FontType::Medium10));
    _launchInfoLabel->SetHorizontalAlignment(Alignment::End);
    _launchInfoLabel->SetPosition(_launchInfoPosition.x - 96, _launchInfoPosition.y + 2);
    _launchInfoLabel->SetBackgroundColor(materialColorScheme->surfaceBright);
    _launchInfoLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
#if SHOW_TOP_LAUNCH_INFO_TEXT
    if (!_launchInfoHidden)
        AddChildTail(_launchInfoLabel.GetPointer());
#endif
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        // The marker versions of the heart and check: crisp pixel shapes with a
        // one pixel outline and no anti-aliasing, because these sit over
        // whatever the theme paints and a 16-tone edge can only melt into one
        // colour. The smooth versions stay in the sheets, which have one.
        _heartVramOffset = objVramManager->Alloc(heartMarkerTilesLen);
        dma_ntrCopy32(3, heartMarkerTiles,
            objVramManager->GetVramAddress(_heartVramOffset), heartMarkerTilesLen);
        _checkVramOffset = objVramManager->Alloc(checkMarkerTilesLen);
        dma_ntrCopy32(3, checkMarkerTiles,
            objVramManager->GetVramAddress(_checkVramOffset), checkMarkerTilesLen);
        _starVramOffset = objVramManager->Alloc(starMarkerTilesLen);
        dma_ntrCopy32(3, starMarkerTiles,
            objVramManager->GetVramAddress(_starVramOffset), starMarkerTilesLen);
        _chipVramOffset = objVramManager->Alloc(stripChipBgTilesLen);
        dma_ntrCopy32(3, stripChipBgTiles,
            objVramManager->GetVramAddress(_chipVramOffset), stripChipBgTilesLen);
    }
    int tileIndex = 0;
    vu16* mapPtr = (vu16*)((u8*)GFX_BG_SUB + 0x3800);
    for (int y = 0; y < 12; y++)
    {
        for (int x = 0; x < 14; x++)
        {
            *mapPtr++ = tileIndex;
            tileIndex++;
        }
        mapPtr += 2;
    }
}

void RomBrowserTopScreenView::Update()
{
    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem && selectedItem >= 0)
    {
        auto& fileInfoManager = _viewModel->GetFileInfoManager();
        // GetInternalFileInfo() covers both game banners and custom icon overrides (the
        // latter apply to any file type, including folders), so check it directly instead
        // of branching on FileType::HasInternalFileInfo() - that's a static per-type property
        // and knows nothing about a per-item custom icon. IsFileInfoLoaded() distinguishes
        // "still loading" from "loaded, and there's legitimately nothing" so this waits for
        // the io thread instead of flashing the previous item's icon while undecided.
        if (fileInfoManager.IsFileInfoLoaded(selectedItem))
        {
            const auto& item = fileInfoManager.GetItem(selectedItem);
            auto info = fileInfoManager.GetInternalFileInfo(selectedItem);

            // A name the user set replaces the banner title (these are the
            // file types that have none) and leaves the file name in the chip.
            const auto* entry = _gameDataService->GetEntry(item.GetFileName());
            bool fileNameAsTitle = true;
            if (entry && entry->displayName.GetString()[0] != 0)
            {
                _fileInfoView->SetTitleText(entry->displayName.GetString());
                // the developer and the publisher, one per line under the title
                _fileInfoView->SetCreditsText(
                    entry->developer.GetString(), entry->publisher.GetString());
                fileNameAsTitle = false;
            }
            else
            {
                const char16_t* gameTitle = info ? info->GetGameTitle() : nullptr;
                if (gameTitle && gameTitle[0] != 0)
                {
                    _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                    fileNameAsTitle = false;
                }
            }

            _selectedFileIcon = info ? info->CreateGameIcon() : nullptr;
            if (!_selectedFileIcon)
            {
                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
            }
            if (_selectedFileIcon)
            {
                _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                _iconGraphicsUploaded = false;
            }
            _fileInfoView->SetIcon(std::move(_selectedFileIcon));
            _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), fileNameAsTitle);

            _lastSelectedItem = selectedItem;

            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;
            }
        }
    }

    u32 gameDataVersion = _gameDataService->GetVersion();
    if (selectedItem != _lastGameDataItem || gameDataVersion != _lastGameDataVersion)
    {
        if (!_mostPlayedKnown || gameDataVersion != _mostPlayedVersion)
            RefreshMostPlayed(gameDataVersion);
        _selectedFavorite = false;
        _selectedCompleted = false;
        _selectedStarred = false;
        char info[24];
        info[0] = 0;
        if (selectedItem >= 0)
        {
            const auto& item = _viewModel->GetFileInfoManager().GetItem(selectedItem);
            // by file name, exactly like the browser filter resolves it. This
            // used to also try the loaded rom header's game code, which made the
            // heart appear for a file the filter could not match (issue #7), and
            // meant the strip had to wait for the header to load before it could
            // show anything.
            const auto* entry = _gameDataService->GetEntry(item.GetFileName());
            if (entry)
            {
                _selectedFavorite = entry->favorite;
                _selectedCompleted = entry->completed;
                _selectedStarred = _mostPlayedFileName.GetString()[0] != 0 &&
                    strcmp(entry->fileName.GetString(), _mostPlayedFileName.GetString()) == 0;
                if (entry->launchCount > 0)
                {
                    if (entry->playMinutes >= 60)
                    {
                        mini_snprintf(info, sizeof(info), "%ux %uh%02u", entry->launchCount,
                            entry->playMinutes / 60, entry->playMinutes % 60);
                    }
                    else if (entry->playMinutes > 0)
                    {
                        mini_snprintf(info, sizeof(info), "%ux %um", entry->launchCount,
                            entry->playMinutes);
                    }
                    else if (strlen(entry->lastPlayed.GetString()) >= 10)
                    {
                        // stored as "YYYY-MM-DD HH:MM", shown as "3x · 16 Jul" (a bare
                        // "16/07" reads like a fraction to new users). The separator is
                        // the middle dot U+00B7, which the Medium10 font provides.
                        static const char* const sMonthNames[12] = { "Jan", "Feb", "Mar", "Apr",
                            "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
                        const char* lastPlayed = entry->lastPlayed.GetString();
                        u32 month = (lastPlayed[5] - '0') * 10 + (lastPlayed[6] - '0');
                        u32 day = (lastPlayed[8] - '0') * 10 + (lastPlayed[9] - '0');
                        if (month >= 1 && month <= 12)
                        {
                            mini_snprintf(info, sizeof(info), "%ux · %u %s", entry->launchCount,
                                day, sMonthNames[month - 1]);
                        }
                        else
                        {
                            mini_snprintf(info, sizeof(info), "%ux", entry->launchCount);
                        }
                    }
                    else
                    {
                        mini_snprintf(info, sizeof(info), "%ux", entry->launchCount);
                    }
                }
            }
        }
        _launchInfoLabel->SetText(info);
        _lastGameDataItem = selectedItem;
        _lastGameDataVersion = gameDataVersion;
    }
    ViewContainer::Update();
}

// The star goes to the game launched most, by the same comparator that orders
// the statistics panel's list (LaunchedMoreThan), so the two never disagree:
// ties go to the name that sorts first, not to whichever was stored first. Play
// time is not consulted: it counts the clock, not the game (issue #9).
void RomBrowserTopScreenView::RefreshMostPlayed(u32 gameDataVersion)
{
    _mostPlayedFileName = "";
    const GameDataEntry* best = nullptr;
    u32 count = _gameDataService->GetEntryCount();
    for (u32 i = 0; i < count; i++)
    {
        const GameDataEntry& entry = _gameDataService->GetEntryByIndex(i);
        if (entry.launchCount == 0)
            continue;
        if (!best || LaunchedMoreThan(entry, *best))
            best = &entry;
    }
    if (best)
        _mostPlayedFileName = best->fileName.GetString();
    _mostPlayedVersion = gameDataVersion;
    _mostPlayedKnown = true;
}

void RomBrowserTopScreenView::Draw(GraphicsContext& graphicsContext)
{
    // a hidden launch info suppresses its text and all three markers together
    bool hidden = _launchInfoHidden;
#if SHOW_TOP_LAUNCH_INFO_TEXT
    // the width follows the currently displayed string (updated at vblank), so
    // the chip always matches the text on screen
    u32 launchInfoWidth = hidden ? 0 : _launchInfoLabel->GetStringWidth();
#else
    u32 launchInfoWidth = 0;
#endif

    // The markers in the order they are laid out, left to right. Centred over
    // the icon they go star, heart, check, as the card was mocked up; a pill
    // keeps the order it always had, check then heart, and gets the star at
    // the left of it.
    struct Marker
    {
        bool show;
        u32 vramOffset;
        Rgb<8, 8, 8> from;
        Rgb<8, 8, 8> to;
    };
    // The marker sprites are two tones: fill at 15 and a one pixel outline at
    // 1, so the gradient's first colour is the outline's. On the pill it is the
    // pill's own colour and the outline vanishes into it; bare over the theme's
    // art it is a dark shade of the marker's colour, the way the star's is,
    // and it reads as a drawn edge on any background.
    auto edgeOf = [&](const Rgb<8, 8, 8>& tint)
    {
        if (!_launchInfoBare)
            return _materialColorScheme->surfaceBright;
        return Rgb<8, 8, 8>(tint.r * 2 / 5, tint.g * 2 / 5, tint.b * 2 / 5);
    };
    const Marker star = { _selectedStarred && !hidden, _starVramOffset, kStarOutlineColor, kStarFillColor };
    const Marker heart = { _selectedFavorite && !hidden, _heartVramOffset,
        edgeOf(kFavoriteRed), kFavoriteRed };
    const Marker check = { _selectedCompleted && !hidden, _checkVramOffset,
        edgeOf(kCompletedGreen), kCompletedGreen };
    Marker markers[3] = { star, check, heart };
    if (_launchInfoCentered)
    {
        markers[1] = heart;
        markers[2] = check;
    }

    int markerCount = 0;
    for (const auto& marker : markers)
        if (marker.show)
            markerCount++;
    // the text, a 2 px gap if markers follow it, then the markers at their stride
    int clusterWidth = launchInfoWidth > 0 ? (int)launchInfoWidth : 0;
    if (markerCount > 0)
        clusterWidth += (clusterWidth > 0 ? 2 : 0) + 16 + (markerCount - 1) * MARKER_STRIDE;

    // Icons sit one pixel down inside the 18 px pill, and on the point itself
    // when bare.
    int iconY = _launchInfoPosition.y + (_launchInfoBare ? 0 : 1);
    // Centred: the row's middle stays on the point whether one marker shows
    // or three. Otherwise the pill below decides where the row starts.
    int x = _launchInfoCentered ? _launchInfoPosition.x - clusterWidth / 2 : _launchInfoPosition.x;
    if (clusterWidth > 0 && !_launchInfoBare)
    {
        // On a pill whose right edge sits at the themed x, content centred with
        // 6px padding, growing to the left.
        u32 chipPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(_materialColorScheme->outline, _materialColorScheme->surfaceBright),
            _launchInfoPosition.y, _launchInfoPosition.y + 18);
        int chipWidth = std::max(clusterWidth + 12, 38);
        int chipX = _launchInfoPosition.x - chipWidth;
        x = chipX + (chipWidth - clusterWidth) / 2;
        DrawChip(graphicsContext, chipX, _launchInfoPosition.y, chipWidth, chipPaletteRow);
    }
    if (launchInfoWidth > 0)
    {
        _launchInfoLabel->SetPosition(x + (int)launchInfoWidth - 96, _launchInfoPosition.y + 2);
        x += launchInfoWidth + 2;
    }
    int markerX[3] = {};
    for (int i = 0; i < 3; i++)
    {
        if (!markers[i].show)
            continue;
        markerX[i] = x;
        x += MARKER_STRIDE;
    }
    // the labels draw after the chips and therefore get lower oam indices,
    // which puts them in front
    ViewContainer::Draw(graphicsContext);
    for (int i = 0; i < 3; i++)
    {
        if (!markers[i].show)
            continue;
        auto oams = graphicsContext.GetOamManager().AllocOams(1);
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(markers[i].from, markers[i].to), iconY, iconY + 16);
        OamBuilder::OamWithSize<16, 16>(markerX[i], iconY, markers[i].vramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oams[0]);
    }
}

// 18px tall pill behind a strip cluster, built from 32x32 pieces whose left 6
// columns are rounded: a left cap, middle pieces at a 26px stride and an h-flipped
// right cap. Oams come out of AllocOams front to back, so each piece hides the
// rounded columns of the piece to its right. Minimum width is 38 to keep the caps'
// flat columns out of each other's corners. Pieces are semi-transparent OBJs (see
// the blend setup in VBlank) so the theme art shows faintly through the pill.
void RomBrowserTopScreenView::DrawChip(GraphicsContext& graphicsContext, int x, int y, int width, u32 paletteRow)
{
    int middleCount = width > 64 ? (width - 64 + 25) / 26 : 0;
    auto oams = graphicsContext.GetOamManager().AllocOams(middleCount + 2);
    OamBuilder::OamWithSize<32, 32>(x, y, _chipVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .AsTranslucent()
        .Build(oams[0]);
    for (int i = 0; i < middleCount; i++)
    {
        OamBuilder::OamWithSize<32, 32>(x + 26 * (i + 1), y, _chipVramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .AsTranslucent()
            .Build(oams[1 + i]);
    }
    OamBuilder::OamWithSize<32, 32>(x + width - 32, y, _chipVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .WithHFlip()
        .AsTranslucent()
        .Build(oams[middleCount + 1]);
}

void RomBrowserTopScreenView::MirrorToMainEngine() const
{
    // Same numbers as VBlank puts on the sub engine, aimed at the main one.
    int x0 = std::clamp(_coverPosition.x, 0, 256);
    int x1 = std::clamp(_coverPosition.x + 106, 0, 256);
    int y0 = std::clamp(_coverPosition.y, 0, 192);
    int y1 = std::clamp(_coverPosition.y + 96, 0, 192);
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover() ||
        x0 >= x1 || y0 >= y1)
    {
        // No cover on screen, so nothing to restate: the mirrored display
        // control already has this background and its window switched off.
        return;
    }
    REG_BG3PA = 0x100;
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = -0x100;
    REG_BG3X = (-_coverPosition.x) << 8;
    REG_BG3Y = (96 + _coverPosition.y - 1) << 8;
    gfx_setWindow0(x0, y0, x1, y1);
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    // the chip pieces are semi-transparent OBJs so the pills reveal a hint of the
    // theme art behind them (the labels/heart/check stay opaque normal OBJs in
    // front). Reuse the splash's REG_BLDCNT_SUB value: BG1 (its 1st target) is off
    // in the browser, so its only live effect is picking the theme backgrounds and
    // backdrop as 2nd targets, and matching the value keeps the startup fade - which
    // also drives these registers until it hands off - from glitching. 12/16 pill +
    // 4/16 backdrop keeps the labels readable over busy art.
    REG_BLDCNT_SUB = 0x3D42;
    REG_BLDALPHA_SUB = (4 << 8) | 12;

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (_showCover && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            GFX_PLTT_BG_SUB[0] = *(vu16*)0x0689E000;
            mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
        }
        _coverGraphicsUploaded = true;
    }
    int x0 = std::clamp(_coverPosition.x, 0, 256);
    int x1 = std::clamp(_coverPosition.x + 106, 0, 256);
    int y0 = std::clamp(_coverPosition.y, 0, 192);
    int y1 = std::clamp(_coverPosition.y + 96, 0, 192);
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover() ||
        x0 >= x1 || y0 >= y1)
    {
        // hide cover
        REG_DISPCNT_SUB &= ~(((1 << 3) | (1 << 5)) << 8);
    }
    else
    {
        // display cover
        REG_BG3PA_SUB = 0x100;
        REG_BG3PB_SUB = 0;
        REG_BG3PC_SUB = 0;
        REG_BG3PD_SUB = -0x100;
        REG_BG3X_SUB = (-_coverPosition.x) << 8;
        REG_BG3Y_SUB = (96 + _coverPosition.y - 1) << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(x0, y0, x1, y1);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}

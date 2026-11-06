#include "common.h"
#include <string.h>
#include <algorithm>
#include "core/mini-printf.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "../MarkerColors.h"
#include "folderIcon.h"
#include "gamesIcon.h"
#include "smallHeartIconFilled.h"
#include "checkIcon.h"
#include "recentIcon.h"
#include "Version.h"
#include "StatisticsBottomSheetView.h"

// Everything is placed from the sheet's top edge, which rests at y 32 once the
// sheet is open, so the panel runs from screen y 48 (title) to 180 (last row).

#define TITLE_X             20
#define TITLE_Y             16
#define TITLE_WIDTH         128

// Version against the right edge of the title row, in the small font
// and the faint colour: there to be found when it is needed, not to be read
// every time the panel opens.
#define VERSION_X           140
#define VERSION_WIDTH       96
#define VERSION_DY          3

// Four equal tiles across the 216 px the other sheets use for content: a 16 px
// icon, the figure beside it in the largest font there is, and a small caption
// centred under both.
#define TILES_X             20
#define TILE_WIDTH          54
#define TILE_Y              34
#define TILE_ICON_DX        7
#define TILE_NUMBER_DX      27
#define TILE_NUMBER_DY      1
#define TILE_NUMBER_WIDTH   32
#define CAPTION_Y           52

#define HEADING_X           20
#define HEADING_Y           70
#define HEADING_WIDTH       160

// Rank, name and count on one line, the count against the right edge so the
// three numbers line up and read as a column.
#define ROW_FIRST_Y         82
#define ROW_SPACING         14
#define RANK_X              20
#define RANK_WIDTH          12
#define NAME_X              32
#define NAME_WIDTH          156
#define NAME_MAX_CHARS      120
#define COUNT_X             196
#define COUNT_WIDTH         40

#define LAST_Y              132
#define LAST_ICON_X         20
#define LAST_TEXT_X         40
#define LAST_TEXT_WIDTH     132
#define DATE_X              172
#define DATE_WIDTH          64
#define DATE_DY             2

#define ICON_SIZE           16

// The totals line - "265 launches, 57h 24m played" - is switched off for now and
// kept behind this rather than removed: the play time in it is the clock running
// while a game was open, not time played (issue #9), so it overstates. Switched
// on, it takes the place of the "Most played" heading, which it is the total of.
#define SHOW_STATISTICS_LAUNCHES_AND_TIME 0

StatisticsBottomSheetView::StatisticsBottomSheetView(SharedPtr<StatisticsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel = AddLabel(fontRepository, FontType::Medium11, TITLE_WIDTH, 25, "Statistics");

    char text[144];

    // Just the version here; the build hash is the boot page's and the about
    // sheet's to show.
    FormatLauncherVersion(text, sizeof(text), false);
    _versionLabel = AddLabel(fontRepository, FontType::Medium7_5, VERSION_WIDTH, 32, text, Alignment::End);

    // The folder count is the one the top screen used to show in its corner: the
    // folder being browsed, not the whole card. The other three are library wide.
    const u32 figures[TILE_COUNT] =
    {
        _viewModel->GetFolderGameCount(),
        _viewModel->GetPlayedCount(),
        _viewModel->GetFavoriteCount(),
        _viewModel->GetCompletedCount()
    };
    static const char* const kCaptions[TILE_COUNT] = { "in folder", "played", "favorites", "completed" };
    for (u32 i = 0; i < TILE_COUNT; i++)
    {
        mini_snprintf(text, sizeof(text), "%u", figures[i]);
        _tileNumbers[i] = AddLabel(fontRepository, FontType::Medium11, TILE_NUMBER_WIDTH, 10, text);
        _tileCaptions[i] = AddLabel(fontRepository, FontType::Medium7_5, TILE_WIDTH, 15, kCaptions[i],
            Alignment::Center);
    }

    if (_viewModel->GetPlayedCount() == 0)
    {
        _headingLabel = AddLabel(fontRepository, FontType::Regular10, HEADING_WIDTH, 32, "Nothing played yet.");
        return;
    }

#if SHOW_STATISTICS_LAUNCHES_AND_TIME
    u32 playMinutes = _viewModel->GetTotalPlayMinutes();
    if (playMinutes > 0)
    {
        mini_snprintf(text, sizeof(text), "%u launch%s, %uh %02um played",
            _viewModel->GetTotalLaunches(), _viewModel->GetTotalLaunches() == 1 ? "" : "es",
            playMinutes / 60, playMinutes % 60);
    }
    else
    {
        mini_snprintf(text, sizeof(text), "%u launch%s in total",
            _viewModel->GetTotalLaunches(), _viewModel->GetTotalLaunches() == 1 ? "" : "es");
    }
    _headingLabel = AddLabel(fontRepository, FontType::Medium7_5, HEADING_WIDTH, 48, text);
#else
    _headingLabel = AddLabel(fontRepository, FontType::Medium7_5, HEADING_WIDTH, 32, "Most played");
#endif

    char name[100];
    _topCount = std::min(_viewModel->GetTopCount(), TOP_COUNT);
    for (u32 t = 0; t < _topCount; t++)
    {
        const auto& entry = _viewModel->GetTopEntry(t);
        mini_snprintf(text, sizeof(text), "%u", t + 1);
        _rankLabels[t] = AddLabel(fontRepository, FontType::Regular10, RANK_WIDTH, 3, text);
        if (entry.displayName.GetString()[0] != 0)
            mini_snprintf(name, sizeof(name), "%s", entry.displayName.GetString());
        else
            CopyNameWithoutExtension(name, sizeof(name), entry.fileName.GetString());
        _nameLabels[t] = AddLabel(fontRepository, FontType::Regular10, NAME_WIDTH, NAME_MAX_CHARS, name);
        mini_snprintf(text, sizeof(text), "%ux", entry.launchCount);
        _countLabels[t] = AddLabel(fontRepository, FontType::Regular10, COUNT_WIDTH, 10, text, Alignment::End);
    }

    const char* lastPlayed = _viewModel->GetLastPlayed().lastPlayed.GetString();
    if (strlen(lastPlayed) >= 16)
    {
        const auto& lastEntry = _viewModel->GetLastPlayed();
        if (lastEntry.displayName.GetString()[0] != 0)
            mini_snprintf(name, sizeof(name), "%s", lastEntry.displayName.GetString());
        else
            CopyNameWithoutExtension(name, sizeof(name), lastEntry.fileName.GetString());
        mini_snprintf(text, sizeof(text), "Last: %s", name);
        _lastLabel = AddLabel(fontRepository, FontType::Regular10, LAST_TEXT_WIDTH, NAME_MAX_CHARS, text);
        // stored as "YYYY-MM-DD HH:MM", shown as "16/07 20:41"
        mini_snprintf(text, sizeof(text), "%c%c/%c%c %c%c:%c%c",
            lastPlayed[8], lastPlayed[9], lastPlayed[5], lastPlayed[6],
            lastPlayed[11], lastPlayed[12], lastPlayed[14], lastPlayed[15]);
        _dateLabel = AddLabel(fontRepository, FontType::Medium7_5, DATE_WIDTH, 16, text, Alignment::End);
        _hasLast = true;
    }
}

SharedPtr<Label2DView> StatisticsBottomSheetView::AddLabel(const IFontRepository* fontRepository,
    FontType fontType, u32 width, u32 maxChars, const char* text, Alignment alignment)
{
    auto label = Label2DView::CreateShared(width, 16, maxChars, fontRepository->GetFont(fontType));
    label->SetHorizontalAlignment(alignment);
    label->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    label->SetText(text);
    AddChildTail(label.GetPointer());
    return label;
}

// "Game.nds" reads as a title here rather than as a file, so the extension goes
// - when it is one to four characters long, which every rom type the launcher
// lists has. Anything else after a dot stays, so a dotted name is not cut short.
void StatisticsBottomSheetView::CopyNameWithoutExtension(char* dst, u32 dstSize, const char* fileName)
{
    u32 length = strlen(fileName);
    const char* dot = strrchr(fileName, '.');
    if (dot && dot != fileName)
    {
        u32 extensionLength = length - (u32)(dot - fileName) - 1;
        if (extensionLength >= 1 && extensionLength <= 4)
            length = (u32)(dot - fileName);
    }
    if (length >= dstSize)
        length = dstSize - 1;
    memcpy(dst, fileName, length);
    dst[length] = 0;
}

void StatisticsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        static const unsigned int* const kTiles[TILE_COUNT] =
            { folderIconTiles, gamesIconTiles, smallHeartIconFilledTiles, checkIconTiles };
        static const u32 kTilesLength[TILE_COUNT] =
            { folderIconTilesLen, gamesIconTilesLen, smallHeartIconFilledTilesLen, checkIconTilesLen };
        for (u32 i = 0; i < TILE_COUNT; i++)
            _tileIconVramOffsets[i] = LoadSprite(*objVramManager, kTiles[i], kTilesLength[i]);
        _clockIconVramOffset = LoadSprite(*objVramManager, recentIconTiles, recentIconTilesLen);
    }
}

u32 StatisticsBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}

void StatisticsBottomSheetView::Update()
{
    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    _versionLabel->SetPosition(VERSION_X, y + TITLE_Y + VERSION_DY);
    for (u32 i = 0; i < TILE_COUNT; i++)
    {
        int tileX = TILES_X + (int)i * TILE_WIDTH;
        _tileNumbers[i]->SetPosition(tileX + TILE_NUMBER_DX, y + TILE_Y + TILE_NUMBER_DY);
        _tileCaptions[i]->SetPosition(tileX, y + CAPTION_Y);
    }
    if (_headingLabel)
        _headingLabel->SetPosition(HEADING_X, y + HEADING_Y);
    for (u32 t = 0; t < _topCount; t++)
    {
        int rowY = y + ROW_FIRST_Y + (int)t * ROW_SPACING;
        _rankLabels[t]->SetPosition(RANK_X, rowY);
        _nameLabels[t]->SetPosition(NAME_X, rowY);
        _countLabels[t]->SetPosition(COUNT_X, rowY);
    }
    if (_hasLast)
    {
        _lastLabel->SetPosition(LAST_TEXT_X, y + LAST_Y);
        _dateLabel->SetPosition(DATE_X, y + LAST_Y + DATE_DY);
    }
    BottomSheetView::Update();
}

void StatisticsBottomSheetView::DrawIcon(GraphicsContext& graphicsContext, int x, int y, u32 vramOffset,
    const Rgb<8, 8, 8>& backColor, const Rgb<8, 8, 8>& tint) const
{
    // The sheet slides in from the bottom, and an oam y past the screen wraps.
    if (!graphicsContext.IsVisible(Rectangle(x, y, ICON_SIZE, ICON_SIZE)))
        return;
    u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(backColor, tint), y, y + ICON_SIZE);
    auto oams = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<ICON_SIZE, ICON_SIZE>(x, y, vramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[0]);
}

void StatisticsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        const auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        const auto& bright = _materialColorScheme->onSurface;
        const auto& body = _materialColorScheme->onSurfaceVariant;
        const auto& faint = _materialColorScheme->outline;

        auto paint = [&](const SharedPtr<Label2DView>& label, const Rgb<8, 8, 8>& color)
        {
            if (!label)
                return;
            label->SetBackgroundColor(backColor);
            label->SetForegroundColor(color);
            label->Draw(graphicsContext);
        };

        paint(_titleLabel, bright);
        paint(_versionLabel, faint);

        // the heart and the check in the top screen's red and green
        const Rgb<8, 8, 8> tints[TILE_COUNT] = { body, body, kFavoriteRed, kCompletedGreen };
        for (u32 i = 0; i < TILE_COUNT; i++)
        {
            paint(_tileNumbers[i], bright);
            paint(_tileCaptions[i], faint);
            DrawIcon(graphicsContext, TILES_X + (int)i * TILE_WIDTH + TILE_ICON_DX, _position.y + TILE_Y,
                _tileIconVramOffsets[i], backColor, tints[i]);
        }

        paint(_headingLabel, body);
        for (u32 t = 0; t < _topCount; t++)
        {
            paint(_rankLabels[t], faint);
            paint(_nameLabels[t], body);
            paint(_countLabels[t], bright);
        }
        if (_hasLast)
        {
            paint(_lastLabel, body);
            paint(_dateLabel, faint);
            DrawIcon(graphicsContext, LAST_ICON_X, _position.y + LAST_Y, _clockIconVramOffset, backColor, body);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool StatisticsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void StatisticsBottomSheetView::Focus(FocusManager& focusManager)
{
    // focus a CHILD of the sheet: FocusManager::Update skips parent-less
    // focused views, so focusing the sheet itself would never deliver keys.
    // Input bubbles from the label up to this sheet's HandleInput (B).
    focusManager.Focus(_titleLabel->SharedFromThis());
}

void StatisticsBottomSheetView::Close()
{
    _viewModel->Close();
}

#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "themes/material/MaterialColorScheme.h"
#include "services/gamedata/GameDataEntry.h"
#include "themes/IFontRepository.h"
#include "gui/palette/GradientPalette.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/input/InputProvider.h"
#include "../MarkerColors.h"
#include "RecentListItemView.h"

#define HEART_X            4
#define HEART_Y            4

#define NAME_LABEL_X       24
#define NAME_LABEL_Y       5
#define NAME_LABEL_WIDTH   124

#define DATE_LABEL_X       (NAME_LABEL_X + NAME_LABEL_WIDTH + 4)
#define DATE_LABEL_Y       7
#define DATE_LABEL_WIDTH   68

RecentListItemView::RecentListItemView(SharedPtr<RecentsViewModel> viewModel, const VramOffsets& vramOffsets,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _nameLabel(Label2DView::CreateShared(NAME_LABEL_WIDTH, 16, 256, fontRepository->GetFont(FontType::Regular10)))
    , _dateLabel(Label2DView::CreateShared(DATE_LABEL_WIDTH, 16, 17, fontRepository->GetFont(FontType::Medium7_5)))
    , _vramOffsets(vramOffsets)
    , _materialColorScheme(materialColorScheme)
{
    _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _dateLabel->SetHorizontalAlignment(Alignment::End);
    AddChildTail(_nameLabel.GetPointer());
    AddChildTail(_dateLabel.GetPointer());
}

void RecentListItemView::SetEntry(const GameDataEntry* entry, int index)
{
    _entry = entry;
    _index = index;
    _nameLabel->SetText(GameDataDisplayName(entry, entry->fileName.GetString()));
    char subtitle[16];
    if (_viewModel->GetKind() == GameListKind::Favorites)
    {
        // favorites show accumulated play time (same format as the top strip)
        u32 minutes = entry->playMinutes;
        if (minutes >= 60)
            mini_snprintf(subtitle, sizeof(subtitle), "%uh%02u", minutes / 60, minutes % 60);
        else if (minutes > 0)
            mini_snprintf(subtitle, sizeof(subtitle), "%um", minutes);
        else
            subtitle[0] = 0; // never played: no time to show
    }
    else
    {
        // recents show the last-played date: "YYYY-MM-DD HH:MM" -> "DD/MM HH:MM"
        const char* lastPlayed = entry->lastPlayed.GetString();
        if (strlen(lastPlayed) >= 16)
        {
            mini_snprintf(subtitle, sizeof(subtitle), "%c%c/%c%c %c%c:%c%c",
                lastPlayed[8], lastPlayed[9], lastPlayed[5], lastPlayed[6],
                lastPlayed[11], lastPlayed[12], lastPlayed[14], lastPlayed[15]);
        }
        else
        {
            subtitle[0] = 0;
        }
    }
    _dateLabel->SetText(subtitle);
}

void RecentListItemView::Update()
{
    _nameLabel->SetPosition(_position.x + NAME_LABEL_X, _position.y + NAME_LABEL_Y);
    _dateLabel->SetPosition(_position.x + DATE_LABEL_X, _position.y + DATE_LABEL_Y);
    if (IsFocused())
    {
        _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    }
    else
    {
        _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    }
    ViewContainer::Update();
}

void RecentListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
    {
        return;
    }

    auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
    if (IsFocused())
    {
        auto selectorFullColor = _materialColorScheme->GetColor(md::sys::color::onSurface);
        backColor = RgbMixer::Lerp(backColor, selectorFullColor, 10, 100);
    }

    _nameLabel->SetBackgroundColor(backColor);
    _nameLabel->SetForegroundColor(_materialColorScheme->onSurface);
    _dateLabel->SetBackgroundColor(backColor);
    _dateLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);

    if (IsFocused())
    {
        u32 selectorPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, backColor),
            _position.y, _position.y + 24);
        auto selectorOam = graphicsContext.GetOamManager().AllocOams(4);
        OamBuilder::OamWithSize<64, 32>(_position.x, _position.y, _vramOffsets.selectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[0]);
        OamBuilder::OamWithSize<64, 32>(_position.x + 64, _position.y, _vramOffsets.selectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[1]);
        OamBuilder::OamWithSize<64, 32>(_position.x + 2 * 64, _position.y, _vramOffsets.selectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[2]);
        OamBuilder::OamWithSize<64, 32>(_position.x + 2 * 64 + 32, _position.y, _vramOffsets.selectorVramOffset >> 7)
            .WithPalette16(selectorPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[3]);
    }

    ViewContainer::Draw(graphicsContext);

    if (_entry != nullptr && _entry->favorite &&
        graphicsContext.IsVisible(Rectangle(_position.x + HEART_X, _position.y + HEART_Y, 16, 16)))
    {
        u32 iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(backColor, kFavoriteRed),
            _position.y + HEART_Y, _position.y + HEART_Y + 16);
        auto iconOam = graphicsContext.GetOamManager().AllocOams(1);
        OamBuilder::OamWithSize<16, 16>(_position.x + HEART_X, _position.y + HEART_Y, _vramOffsets.heartIconVramOffset >> 7)
            .WithPalette16(iconPaletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(iconOam[0]);
    }
}

bool RecentListItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        _viewModel->ActivateItem(_index);
        return true;
    }

    return ViewContainer::HandleInput(inputProvider, focusManager);
}

void RecentListItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (GetBounds().Contains(touchPoint))
    {
        _penDown = true;
    }
}

void RecentListItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!GetBounds().Contains(touchPoint))
    {
        _penDown = false;
    }
}

void RecentListItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && GetBounds().Contains(lastTouchPoint))
    {
        if (focusManager.GetCurrentFocus().GetPointer() == this)
        {
            _viewModel->ActivateItem(_index);
        }
        else
        {
            focusManager.Focus(SharedFromThis());
        }
    }

    _penDown = false;
}

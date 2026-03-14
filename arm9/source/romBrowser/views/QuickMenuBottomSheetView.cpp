#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "gui/Alignment.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "gamesIcon.h"
#include "listIcon.h"
#include "settingsIcon.h"
#include "hGridIcon.h"
#include "unknownIcon.h"
#include "themes/FontType.h"
#include "../IRomBrowserController.h"
#include "QuickMenuBottomSheetView.h"
#include "services/Localization/Localization.h"

namespace
{
    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength)
    {
        u32 vramOffset = vramManager.Alloc(tilesLength);
        dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
        return vramOffset;
    }
}

QuickMenuBottomSheetView::QuickMenuBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    bool hasSelectedRom, bool isNdsRom)
    : _romBrowserController(romBrowserController)
    , _materialColorScheme(materialColorScheme)
    , _titleLabel(kTitleWidth, kTitleHeight, 24, fontRepository->GetFont(FontType::Medium11))
    , _chips {
        ChipView(md::sys::color::surfaceContainerHighest, materialColorScheme, fontRepository),
        ChipView(md::sys::color::surfaceContainerHighest, materialColorScheme, fontRepository),
        ChipView(md::sys::color::surfaceContainerHighest, materialColorScheme, fontRepository),
        ChipView(md::sys::color::surfaceContainerHighest, materialColorScheme, fontRepository),
        ChipView(md::sys::color::surfaceContainerHighest, materialColorScheme, fontRepository)
    }
{
    _position.x = 0;

    _titleLabel.SetText(u"Quick Menu");
    _titleLabel.SetHorizontalAlignment(Alignment::Center);
    AddChildTail(&_titleLabel);

    _chipCount = 0;

    if (hasSelectedRom)
    {
        _chips[_chipCount].SetText(Localization::Translate("game_details"));
        _chips[_chipCount].SetCenteredText(false);
        _chipMenuItem[_chipCount] = MenuItem::GameDetails;
        AddChildTail(&_chips[_chipCount]);
        _chipCount++;
    }

    if (isNdsRom)
    {
        _chips[_chipCount].SetText(Localization::Translate("cheats"));
        _chips[_chipCount].SetCenteredText(false);
        _chipMenuItem[_chipCount] = MenuItem::Cheats;
        AddChildTail(&_chips[_chipCount]);
        _chipCount++;
    }

    _chips[_chipCount].SetText(Localization::Translate("display_settings"));
    _chips[_chipCount].SetCenteredText(false);
    _chipMenuItem[_chipCount] = MenuItem::DisplaySettings;
    AddChildTail(&_chips[_chipCount]);
    _chipCount++;

    _chips[_chipCount].SetText(u"Layout Editor");
    _chips[_chipCount].SetCenteredText(false);
    _chipMenuItem[_chipCount] = MenuItem::LayoutEditor;
    AddChildTail(&_chips[_chipCount]);
    _chipCount++;

    _chips[_chipCount].SetText(Localization::Translate("information"));
    _chips[_chipCount].SetCenteredText(false);
    _chipMenuItem[_chipCount] = MenuItem::Information;
    AddChildTail(&_chips[_chipCount]);
    _chipCount++;
}

void QuickMenuBottomSheetView::SetGraphics(const ChipView::VramToken& chipVramToken)
{
    _chipVramToken = chipVramToken;
    for (int i = 0; i < _chipCount; i++)
        _chips[i].SetGraphics(chipVramToken);
}

void QuickMenuBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    auto* objVramManager = vramContext.GetObjVramManager();
    const bool haveIcons = objVramManager != nullptr;
    if (haveIcons)
    {
        _iconVramOffsets[static_cast<int>(MenuItem::GameDetails)] =
            LoadIcon(*objVramManager, gamesIconTiles, gamesIconTilesLen);
        _iconVramOffsets[static_cast<int>(MenuItem::Cheats)] =
            LoadIcon(*objVramManager, listIconTiles, listIconTilesLen);
        _iconVramOffsets[static_cast<int>(MenuItem::DisplaySettings)] =
            LoadIcon(*objVramManager, settingsIconTiles, settingsIconTilesLen);
        _iconVramOffsets[static_cast<int>(MenuItem::LayoutEditor)] =
            LoadIcon(*objVramManager, hGridIconTiles, hGridIconTilesLen);
        _iconVramOffsets[static_cast<int>(MenuItem::Information)] =
            LoadIcon(*objVramManager, unknownIconTiles, unknownIconTilesLen);
    }

    for (int i = 0; i < _chipCount; i++)
    {
        const auto item = _chipMenuItem[i];
        if (haveIcons)
            _chips[i].SetIcon(true, _iconVramOffsets[static_cast<int>(item)]);
        else
            _chips[i].SetIcon(false, 0);
        _chips[i].SetCenteredText(!haveIcons);
    }
}

void QuickMenuBottomSheetView::Update()
{
    BottomSheetView::Update();

    const int panelX = _position.x + kPanelMarginLeft;
    const int panelY = _position.y;
    const int chipHeight = _chipCount > 0 ? _chips[0].GetHeight() : 0;
    const int gridHeight = _chipCount > 0
        ? _chipCount * chipHeight + (_chipCount - 1) * kRowSpacing
        : 0;
    const int totalHeight = kPanelPaddingTop + kTitleHeight + kTitleSpacing
        + gridHeight + kPanelPaddingBottom;

    _panelBounds = Rectangle(panelX, panelY, kPanelWidth, totalHeight);

    const int contentX = panelX + kPanelPaddingX;

    _titleLabel.SetPosition(contentX, panelY + kPanelPaddingTop);
    _titleLabel.SetBackgroundColor(_materialColorScheme->surfaceContainerHighest);
    _titleLabel.SetForegroundColor(_materialColorScheme->onSurface);

    const int chipAreaY = panelY + kPanelPaddingTop + kTitleHeight + kTitleSpacing;
    int chipY = chipAreaY;
    for (int i = 0; i < _chipCount; i++)
    {
        _chips[i].SetFixedWidth(kChipWidth);
        _chips[i].SetPosition(contentX, chipY);
        chipY += chipHeight + kRowSpacing;
    }
}

void QuickMenuBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    BottomSheetView::Draw(graphicsContext);
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void QuickMenuBottomSheetView::VBlank()
{
    BottomSheetView::VBlank();
}

void QuickMenuBottomSheetView::Focus(FocusManager& focusManager)
{
    if (_chipCount > 0)
        focusManager.Focus(&_chips[0]);
}

View* QuickMenuBottomSheetView::MoveFocus(View* currentFocus,
    FocusMoveDirection direction, View* source)
{
    int idx = FindChipIndex(currentFocus);
    if (idx < 0)
        return nullptr;

    if (direction == FocusMoveDirection::Up)
    {
        int newIdx = idx - 1;
        if (newIdx < 0)
            newIdx = _chipCount - 1;
        return &_chips[newIdx];
    }
    if (direction == FocusMoveDirection::Down)
    {
        int newIdx = idx + 1;
        if (newIdx >= _chipCount)
            newIdx = 0;
        return &_chips[newIdx];
    }
    return nullptr;
}

bool QuickMenuBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        int idx = FindChipIndex(focusManager.GetCurrentFocus());
        if (idx >= 0)
        {
            ActivateChip(idx);
            return true;
        }
    }
    if (inputProvider.Triggered(InputKey::B))
    {
        _romBrowserController->HideQuickMenu();
        return true;
    }
    return false;
}

bool QuickMenuBottomSheetView::HandleTouch(
    const TouchEvent& event, FocusManager& focusManager)
{
    if (event.type == TouchEventType::Down)
        return false;

    if (event.type != TouchEventType::Up || event.holdFrames > 24)
        return false;

    for (int i = 0; i < _chipCount; i++)
    {
        if (_chips[i].GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_chips[i]);
            ActivateChip(i);
            return true;
        }
    }
    return false;
}

void QuickMenuBottomSheetView::OnDismissed()
{
    _romBrowserController->HideQuickMenu();
}

void QuickMenuBottomSheetView::ActivateChip(int index)
{
    if (index < 0 || index >= _chipCount)
        return;

    MenuItem item = _chipMenuItem[index];

    switch (item)
    {
        case MenuItem::GameDetails:
        {
            _romBrowserController->SetQuickMenuAction(IRomBrowserController::QuickMenuAction::GameDetails);
            _romBrowserController->HideQuickMenu();
            break;
        }
        case MenuItem::Cheats:
        {
            _romBrowserController->SetDirectMenuAccess(true);
            _romBrowserController->SetQuickMenuAction(IRomBrowserController::QuickMenuAction::Cheats);
            _romBrowserController->HideQuickMenu();
            break;
        }
        case MenuItem::DisplaySettings:
        {
            _romBrowserController->SetQuickMenuAction(IRomBrowserController::QuickMenuAction::DisplaySettings);
            _romBrowserController->HideQuickMenu();
            break;
        }
        case MenuItem::LayoutEditor:
        {
            _romBrowserController->SetDirectMenuAccess(true);
            _romBrowserController->SetQuickMenuAction(IRomBrowserController::QuickMenuAction::LayoutEditor);
            _romBrowserController->HideQuickMenu();
            break;
        }
        case MenuItem::Information:
        {
            _romBrowserController->SetDirectMenuAccess(true);
            _romBrowserController->SetQuickMenuAction(IRomBrowserController::QuickMenuAction::Information);
            _romBrowserController->HideQuickMenu();
            break;
        }
        default:
            break;
    }
}

int QuickMenuBottomSheetView::FindChipIndex(const View* view) const
{
    for (int i = 0; i < _chipCount; i++)
    {
        if (view == &_chips[i])
            return i;
    }
    return -1;
}


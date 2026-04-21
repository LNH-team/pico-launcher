#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "smallHeartIcon.h"
#include "smallHeartIconFilled.h"
#include "../IRomBrowserController.h"
#include "NdsGameDetailsBottomSheetView.h"

NdsGameDetailsBottomSheetView::NdsGameDetailsBottomSheetView(
    IRomBrowserController* romBrowserController, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _romBrowserController(romBrowserController)
    , _cheatsChip(ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository))
    , _favoriteChip(ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository))
{
    _cheatsChip->SetText(u"Cheats");
    _cheatsChip->SetSelected(false);
    AddChildTail(_cheatsChip.GetPointer());
    _favoriteChip->SetText(u"Favorite");
    _favoriteChip->SetSelected(true);
    AddChildTail(_favoriteChip.GetPointer());
}

void NdsGameDetailsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _smallHeartIconVramOffset = objVramManager->Alloc(smallHeartIconTilesLen);
        dma_ntrCopy32(3, smallHeartIconTiles, objVramManager->GetVramAddress(_smallHeartIconVramOffset), smallHeartIconTilesLen);

        _smallHeartIconFilledVramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
        dma_ntrCopy32(3, smallHeartIconFilledTiles, objVramManager->GetVramAddress(_smallHeartIconFilledVramOffset), smallHeartIconFilledTilesLen);

        _favoriteChip->SetIcon(true, _smallHeartIconFilledVramOffset);
    }
}

void NdsGameDetailsBottomSheetView::Update()
{
    BottomSheetView::Update();
    _cheatsChip->SetPosition(92, _position.y + 21);
    _favoriteChip->SetPosition(162, _position.y + 21);
}

void NdsGameDetailsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

SharedPtr<View> NdsGameDetailsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (currentFocus.GetPointer() == _cheatsChip.GetPointer() && direction == FocusMoveDirection::Right)
        return _favoriteChip;
    else if (currentFocus.GetPointer() == _favoriteChip.GetPointer() && direction == FocusMoveDirection::Left)
        return _cheatsChip;
    return nullptr;
}

bool NdsGameDetailsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _romBrowserController->HideGameInfo();
        return true;
    }
    return false;
}

void NdsGameDetailsBottomSheetView::Close()
{
    _romBrowserController->HideGameInfo();
}

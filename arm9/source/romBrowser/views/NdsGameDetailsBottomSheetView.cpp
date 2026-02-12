#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "smallHeartIcon.h"
#include "smallHeartIconFilled.h"
#include "../IRomBrowserController.h"
#include "../FileInfo.h"
#include "NdsGameDetailsBottomSheetView.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "services/localization/Localization.h"

NdsGameDetailsBottomSheetView::NdsGameDetailsBottomSheetView(
    IRomBrowserController* romBrowserController, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _romBrowserController(romBrowserController)
    , _cheatsChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _favoriteChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
{
    bool isNds = false;
    if (_romBrowserController) {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid()) {
            int selected = viewModel->GetSelectedItem();
            if (selected >= 0) {
                const auto& fileInfo = viewModel->GetFileInfoManager().GetItem(selected);
                if (fileInfo.GetFileType() == &NdsFileType::sInstance) {
                    // Check extension for .nds, .dsi, .srl
                    const TCHAR* name = fileInfo.GetFileName();
                    const char* ext = strrchr(name, '.');
                    if (ext) {
                        ext++;
                        if (!strcasecmp(ext, "nds") || !strcasecmp(ext, "dsi") || !strcasecmp(ext, "srl")) {
                            isNds = true;
                        }
                    }
                }
            }
        }
    }
    if (isNds) {
        _cheatsChip.SetText(Localization::Translate("cheats"));
        _cheatsChip.SetSelected(false);
        AddChildTail(&_cheatsChip);
        _hasCheatsChip = true;
    }
    _favoriteChip.SetText(Localization::Translate("favorites"));
    _isFavorite = _romBrowserController->IsSelectedFileFavorite();
    _favoriteChip.SetSelected(_isFavorite);
    AddChildTail(&_favoriteChip);
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

        UpdateFavoriteChipIcon();
    }
}

void NdsGameDetailsBottomSheetView::Update()
{
    BottomSheetView::Update();
    if (_hasCheatsChip) {
        _cheatsChip.SetPosition(92, _position.y + 21);
        _favoriteChip.SetPosition(162, _position.y + 21);
    } else {
        _favoriteChip.SetPosition(92, _position.y + 21);
    }
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

View* NdsGameDetailsBottomSheetView::MoveFocus(View* currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (_hasCheatsChip) {
        if (currentFocus == &_cheatsChip && direction == FocusMoveDirection::Right)
            return &_favoriteChip;
        else if (currentFocus == &_favoriteChip && direction == FocusMoveDirection::Left)
            return &_cheatsChip;
    }
    return nullptr;
}

bool NdsGameDetailsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        if (focusManager.GetCurrentFocus() == &_favoriteChip)
        {
            bool wasFavorite = _isFavorite;
            
            // Toggle favorite status via controller and sync local state
            _romBrowserController->ToggleSelectedFileFavorite();
            _isFavorite = _romBrowserController->IsSelectedFileFavorite();
            UpdateFavoriteChipIcon();

            // If the item was removed from favorites while in "Favorites View", close the panel
            if (wasFavorite && !_isFavorite && _romBrowserController->IsFavoritesViewActive()) 
            {
                // Check if any favorite items remain in the list
                const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
                int favoritesCount = 0;
                if (viewModel.IsValid()) {
                    favoritesCount = viewModel->GetFileInfoManager().GetItemCount();
                }

                // Close the game details view since the item is no longer in the filtered list
                _romBrowserController->HideGameInfo();

                // Focus Management Logic:
                if (favoritesCount > 0) {
                    // Focus will automatically return to the list (handled by ViewModel/View update)
                } else {
                    // No favorites left: focus should be moved to the toolbar favorite icon.
                    // This needs to be handled at a higher level (App or RomBrowserBottomScreenView).
                }
                return true;
            }
            
            return true;
        }
        if (_hasCheatsChip && focusManager.GetCurrentFocus() == &_cheatsChip) {
            // Handle cheats chip action here if needed
            return true;
        }
    }
    if (inputProvider.Triggered(InputKey::B))
    {
        _romBrowserController->HideGameInfo();
        return true;
    }
    return false;
}
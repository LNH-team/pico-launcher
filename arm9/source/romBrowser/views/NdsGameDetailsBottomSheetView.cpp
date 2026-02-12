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
            _romBrowserController->ToggleSelectedFileFavorite();
            _isFavorite = _romBrowserController->IsSelectedFileFavorite();
            UpdateFavoriteChipIcon();

            // Se era un favorito e ora non lo è più, controlla la modalità solo favoriti
            if (wasFavorite && !_isFavorite && _romBrowserController->IsFavoritesViewActive()) {
                // Controlla quanti favoriti sono rimasti
                const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
                int favoritesCount = 0;
                if (viewModel.IsValid()) {
                    favoritesCount = viewModel->GetFileInfoManager().GetItemCount();
                }
                // Chiudi il menu info game
                _romBrowserController->HideGameInfo();
                // Gestisci il focus
                if (favoritesCount > 0) {
                    // Metti il focus sul primo elemento rimasto
                    // (il ViewModel aggiornerà la selezione, quindi il focus tornerà sulla lista)
                } else {
                    // Nessun favorito rimasto: metti il focus sull'icona dei favoriti nella toolbar
                    // Serve accedere al FocusManager e alla toolbar, quindi qui si può solo segnalare
                    // che il focus va aggiornato a livello superiore (App/RomBrowserBottomScreenView)
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
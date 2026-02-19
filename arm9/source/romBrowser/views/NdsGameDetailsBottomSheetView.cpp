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
#include "themes/material/MaterialColorScheme.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "services/localization/Localization.h"
#include "services/launchStats/LaunchStatsService.h"
#include "../FileType/Nds/NdsFileType.h"
#include "../FileType/Nds/NdsInternalFileInfo.h"
#include "core/mini-printf.h"
#include "cheats/CheatCodelist.h"
#include "fat/File.h"

NdsGameDetailsBottomSheetView::NdsGameDetailsBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _titleLabel(128, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _gameCodeLabel(40, 14, 20, fontRepository->GetFont(FontType::Medium7_5))
    , _crcLabel(60, 14, 20, fontRepository->GetFont(FontType::Medium7_5))
    , _romBrowserController(romBrowserController)
    , _cheatsChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _favoriteChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _countLaunchLabel(80, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _countLaunchValueLabel(30, 16, 20, fontRepository->GetFont(FontType::Regular10))
{
    _titleLabel.SetText((const char16_t*)L"Game Details"); 
    const char16_t* localizedTitle = Localization::Translate("game_details");
    if (localizedTitle && localizedTitle[0] != 0)
        _titleLabel.SetText(localizedTitle);
        
    _titleLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _titleLabel.SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
    AddChildTail(&_titleLabel);

    _gameCodeLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _gameCodeLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_gameCodeLabel);

    _crcLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _crcLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_crcLabel);

    bool isNds = false;

    if (_romBrowserController) {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid()) {
            int selected = viewModel->GetSelectedItem();
            if (selected >= 0) {
                const auto& fileInfo = viewModel->GetFileInfoManager().GetItem(selected);
                
                if (fileInfo.GetFileType() == &NdsFileType::sInstance) {
                    const TCHAR* name = fileInfo.GetFileName();
                    const char* ext = strrchr(name, '.');
                    if (ext) {
                        ext++;
                        if (!strcasecmp(ext, "nds") || !strcasecmp(ext, "dsi") || !strcasecmp(ext, "srl")) {
                            isNds = true;
                        }
                    }

                    if (isNds) {
                        std::unique_ptr<InternalFileInfo> internalInfo(fileInfo.CreateInternalFileInfo());
                        const char* gameCode = nullptr;
                        if (internalInfo) {
                            gameCode = internalInfo->GetGameCode();
                        }
                        u32 crc32 = 0;
                        {
                            File romFile;
                            romFile.Open(fileInfo.GetFastFileRef(), FA_READ);
                            if (romFile.GetSize() >= 512) {
                                u8 header[512];
                                if (romFile.ReadExact(header, sizeof(header))) {
                                    crc32 = CheatCodelist::ComputeCrc32(header, sizeof(header));
                                }
                            }
                        }
                        if (gameCode) {
                            strncpy(_gameCode, gameCode, 4);
                            _gameCode[4] = 0;
                        } else {
                            _gameCode[0] = 0;
                        }
                        _crc = crc32;

                        char16_t wc[16];
                        int i = 0;
                        if (gameCode) {
                            for (; gameCode[i] && i < 4; ++i) wc[i] = (char16_t)(unsigned char)gameCode[i];
                        }
                        wc[i] = 0;
                        _gameCodeLabel.SetText(wc);

                        char buf[16];
                        mini_snprintf(buf, sizeof(buf), "%08X", _crc);
                        for (i = 0; buf[i]; ++i) wc[i] = (char16_t)(unsigned char)buf[i];
                        wc[i] = 0;
                        _crcLabel.SetText(wc);
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
    InitLaunchCountLabel(materialColorScheme);
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
    
    _titleLabel.SetPosition(12, _position.y + 12);

    int codeX = 180;
    int codeY = _position.y + 8; // match cheats menu, move higher
    _gameCodeLabel.SetPosition(codeX, codeY);
    int codeW = _gameCodeLabel.GetStringWidth();
    _crcLabel.SetPosition(codeX + codeW + 8, codeY);

    if (_hasCheatsChip) {
        _cheatsChip.SetPosition(92, _position.y + 35); 
        _favoriteChip.SetPosition(162, _position.y + 35);
    } else {
        _favoriteChip.SetPosition(92, _position.y + 35);
    }
    _countLaunchLabel.SetPosition(10, _position.y + 40);
    _countLaunchValueLabel.SetPosition(20, _position.y + 55);
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

            if (wasFavorite && !_isFavorite && _romBrowserController->IsFavoritesViewActive()) 
            {
                _romBrowserController->HideGameInfo();
            }
            return true;
        }
        if (_hasCheatsChip && focusManager.GetCurrentFocus() == &_cheatsChip) {
            _romBrowserController->ShowCheats();
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

void NdsGameDetailsBottomSheetView::InitLaunchCountLabel(const MaterialColorScheme* materialColorScheme)
{
    u32 launchCount = 0;

    if (_romBrowserController) {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid()) {
            int selected = viewModel->GetSelectedItem();
            if (selected >= 0) {
                const auto& fileInfo = viewModel->GetFileInfoManager().GetItem(selected);
                const TCHAR* fullPath = fileInfo.GetFullPath();
                char pathBuf[256];

                if (fullPath && fullPath[0] != 0) {
                    strncpy(pathBuf, fullPath, sizeof(pathBuf));
                    pathBuf[sizeof(pathBuf) - 1] = 0;
                } else {
                    if (f_getcwd(pathBuf, sizeof(pathBuf)) == FR_OK) {
                        int idx = strlcat(pathBuf, "/", sizeof(pathBuf));
                        if (idx > 1 && pathBuf[idx - 2] == '/')
                            pathBuf[idx - 1] = 0;
                        strlcat(pathBuf, fileInfo.GetFileName(), sizeof(pathBuf));
                    } else {
                        pathBuf[0] = 0;
                    }
                }

                if (pathBuf[0] != 0) {
                    const char* normalizedPath = strchr(pathBuf, ':');
                    if (normalizedPath) {
                        launchCount = LaunchStatsService::Instance().GetCount(normalizedPath);
                    }
                }
            }
        }
    }

    _countLaunchLabel.SetText(Localization::Translate("total_launches"));
    _countLaunchLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _countLaunchLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_countLaunchLabel);

    char countText[12];
    snprintf(countText, sizeof(countText), "%lu", launchCount);
    
    _countLaunchValueLabel.SetText(countText);
    _countLaunchValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _countLaunchValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_countLaunchValueLabel);
}
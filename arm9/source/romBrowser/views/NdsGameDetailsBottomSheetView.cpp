#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "smallHeartIcon.h"
#include "smallHeartIconFilled.h"
#include "../IRomBrowserController.h"
#include "../FileInfo.h"
#include "NdsGameDetailsBottomSheetView.h"
#include "themes/material/MaterialColorScheme.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "services/Localization/Localization.h"
#include "services/LaunchStats/LaunchStatsService.h"
#include "../FileType/Nds/NdsFileType.h"
#include "../FileType/Gba/GbaFileType.h"
#include "core/mini-printf.h"
#include "fat/File.h"

static constexpr int GAME_DETAILS_CHEATS_CHIP_WIDTH    = 64;
static constexpr int GAME_DETAILS_FAVORITES_CHIP_WIDTH = 80;

static bool BuildNormalizedPath(const FileInfo& fileInfo, char* outBuf, u32 bufSize)
{
    const TCHAR* fullPath = fileInfo.GetFullPath();
    if (fullPath && fullPath[0] != 0)
    {
        strncpy(outBuf, fullPath, bufSize - 1);
        outBuf[bufSize - 1] = '\0';
    }
    else
    {
        if (f_getcwd(outBuf, bufSize) != FR_OK)
        {
            outBuf[0] = '\0';
            return false;
        }
        int idx = strlcat(outBuf, "/", bufSize);
        if (idx > 1 && outBuf[idx - 2] == '/')
            outBuf[idx - 1] = '\0';
        strlcat(outBuf, fileInfo.GetFileName(), bufSize);
    }

    const char* colon = strchr(outBuf, ':');
    if (colon && colon < outBuf + 6)
    {
        size_t len = strlen(colon);
        memmove(outBuf, colon, len + 1);
        return outBuf[0] != '\0';
    }
    return outBuf[0] != '\0';
}

NdsGameDetailsBottomSheetView::NdsGameDetailsBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _titleLabel(128, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _romBrowserController(romBrowserController)
    , _cheatsChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _favoriteChip(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository)
    , _countLaunchLabel(80, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _countLaunchValueLabel(30, 16, 20, fontRepository->GetFont(FontType::Regular10))
    , _lastLaunchLabel(80, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _lastLaunchDateValueLabel(140, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _lastLaunchTimeValueLabel(140, 16, 24, fontRepository->GetFont(FontType::Regular10))
{
    _titleLabel.SetText(Localization::Translate("game_details"));
    _titleLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _titleLabel.SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
    AddChildTail(&_titleLabel);

    bool isNds = false;

    if (_romBrowserController)
    {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid())
        {
            int selected = viewModel->GetSelectedItem();
            if (selected >= 0)
            {
                const auto& fileInfo = viewModel->GetFileInfoManager().GetItem(selected);

                if (fileInfo.GetFileType() == &NdsFileType::sInstance)
                {
                    const TCHAR* name = fileInfo.GetFileName();
                    const char* ext = strrchr(name, '.');
                    if (ext)
                    {
                        ext++;
                        if (!strcasecmp(ext, "nds") || !strcasecmp(ext, "dsi") || !strcasecmp(ext, "srl"))
                            isNds = true;
                    }
                }
            }
        }
    }

    if (isNds)
    {
        _cheatsChip.SetText(Localization::Translate("cheats"));
        _cheatsChip.SetSecondaryText(u"");
        _cheatsChip.SetCenteredText(true);
        _cheatsChip.SetMinWidth(GAME_DETAILS_CHEATS_CHIP_WIDTH);
        _cheatsChip.SetFixedWidth(GAME_DETAILS_CHEATS_CHIP_WIDTH);
        _cheatsChip.SetSelected(false);
        AddChildTail(&_cheatsChip);
        _hasCheatsChip = true;
    }

    _favoriteChip.SetText(Localization::Translate("favorites"));
    _favoriteChip.SetCenteredText(true);
    _favoriteChip.SetFixedWidth(GAME_DETAILS_FAVORITES_CHIP_WIDTH);
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
        dma_ntrCopy32(3, smallHeartIconTiles,
            objVramManager->GetVramAddress(_smallHeartIconVramOffset),
            smallHeartIconTilesLen);

        _smallHeartIconFilledVramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
        dma_ntrCopy32(3, smallHeartIconFilledTiles,
            objVramManager->GetVramAddress(_smallHeartIconFilledVramOffset),
            smallHeartIconFilledTilesLen);

        UpdateFavoriteChipIcon();
    }
}

void NdsGameDetailsBottomSheetView::Update()
{
    BottomSheetView::Update();

    constexpr int screenWidth  = 256;
    constexpr int rightPadding = 12;
    constexpr int chipGap      = 8;

    _titleLabel.SetPosition(12, _position.y + 12);

    if (_hasCheatsChip)
    {
        int totalChipWidth = GAME_DETAILS_CHEATS_CHIP_WIDTH + chipGap + GAME_DETAILS_FAVORITES_CHIP_WIDTH;
        int chipsStartX = screenWidth - rightPadding - totalChipWidth;
        _cheatsChip.SetPosition(chipsStartX, _position.y + 35);
        _favoriteChip.SetPosition(chipsStartX + GAME_DETAILS_CHEATS_CHIP_WIDTH + chipGap, _position.y + 35);
    }
    else
    {
        _favoriteChip.SetPosition(screenWidth - rightPadding - GAME_DETAILS_FAVORITES_CHIP_WIDTH, _position.y + 35);
    }

    _countLaunchLabel.SetPosition(12, _position.y + 40);
    _countLaunchValueLabel.SetPosition(20, _position.y + 55);
    _lastLaunchLabel.SetPosition(12, _position.y + 72);
    _lastLaunchDateValueLabel.SetPosition(20, _position.y + 87);
    _lastLaunchTimeValueLabel.SetPosition(20, _position.y + 101);
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
    if (_hasCheatsChip)
    {
        if (currentFocus == &_cheatsChip && direction == FocusMoveDirection::Right)
            return &_favoriteChip;
        if (currentFocus == &_favoriteChip && direction == FocusMoveDirection::Left)
            return &_cheatsChip;
    }
    return nullptr;
}

bool NdsGameDetailsBottomSheetView::HandleInput(const InputProvider& inputProvider,
    FocusManager& focusManager)
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
                _romBrowserController->HideGameInfo();
            return true;
        }
        if (_hasCheatsChip && focusManager.GetCurrentFocus() == &_cheatsChip)
        {
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

void NdsGameDetailsBottomSheetView::OnDismissed()
{
    _romBrowserController->HideGameInfo();
}

bool NdsGameDetailsBottomSheetView::HandleTouch(const TouchEvent& event,
    FocusManager& focusManager)
{
    if (event.type == TouchEventType::Move)
    {
        if (_hasCheatsChip && _cheatsChip.GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_cheatsChip);
            return true;
        }
        if (_favoriteChip.GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_favoriteChip);
            return true;
        }
        return false;
    }

    if (event.type == TouchEventType::Up && event.holdFrames <= 15)
    {
        if (_hasCheatsChip && _cheatsChip.GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_cheatsChip);
            _romBrowserController->ShowCheats();
            return true;
        }
        if (_favoriteChip.GetBounds().Contains(event.position))
        {
            focusManager.Focus(&_favoriteChip);
            bool wasFavorite = _isFavorite;
            _romBrowserController->ToggleSelectedFileFavorite();
            _isFavorite = _romBrowserController->IsSelectedFileFavorite();
            UpdateFavoriteChipIcon();
            if (wasFavorite && !_isFavorite && _romBrowserController->IsFavoritesViewActive())
                _romBrowserController->HideGameInfo();
            return true;
        }
    }
    return false;
}

void NdsGameDetailsBottomSheetView::InitLaunchCountLabel(
    const MaterialColorScheme* materialColorScheme)
{
    u32  launchCount     = 0;
    char lastLaunchDate[16] = {};
    char lastLaunchTime[16] = {};

    if (_romBrowserController)
    {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid())
        {
            int selected = viewModel->GetSelectedItem();
            if (selected >= 0)
            {
                const auto& fileInfo = viewModel->GetFileInfoManager().GetItem(selected);
                char pathBuf[256];
                if (BuildNormalizedPath(fileInfo, pathBuf, sizeof(pathBuf)))
                {
                    LaunchStatsService::Instance().TryGetInfo(pathBuf,
                        &launchCount,
                        lastLaunchDate, sizeof(lastLaunchDate),
                        lastLaunchTime, sizeof(lastLaunchTime));
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

    _lastLaunchLabel.SetText(Localization::Translate("last_launch"));
    _lastLaunchLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchLabel);

    _lastLaunchDateValueLabel.SetText(lastLaunchDate[0] != '\0' ? lastLaunchDate : "-");
    _lastLaunchDateValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchDateValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchDateValueLabel);

    _lastLaunchTimeValueLabel.SetText(lastLaunchTime[0] != '\0' ? lastLaunchTime : "-");
    _lastLaunchTimeValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchTimeValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchTimeValueLabel);
}
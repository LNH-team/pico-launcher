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
#include "services/localization/Localization.h"
#include "services/launchStats/LaunchStatsService.h"
#include "../FileType/Nds/NdsFileType.h"
#include "../FileType/Nds/NdsInternalFileInfo.h"
#include "core/mini-printf.h"
#include "fat/File.h"

#define CRCPOLY 0xEDB88320
static constexpr int GAME_DETAILS_CHEATS_CHIP_WIDTH = 64;
static constexpr int GAME_DETAILS_FAVORITES_CHIP_WIDTH = 80;

static u32 ComputeCrc32(const void* buffer, u32 length)
{
    u32 crc = ~0u;
    const u8* p = (const u8*)buffer;
    while (length--)
    {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY : 0);
        }
    }

    return crc;
}

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
    , _lastLaunchLabel(80, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _lastLaunchDateValueLabel(140, 16, 24, fontRepository->GetFont(FontType::Regular10))
    , _lastLaunchTimeValueLabel(140, 16, 24, fontRepository->GetFont(FontType::Regular10))
{
    _titleLabel.SetText(Localization::Translate("game_details"));
        
    _titleLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _titleLabel.SetForegroundColor(materialColorScheme->GetColor(md::sys::color::onSurface));
    AddChildTail(&_titleLabel);

    _gameCodeLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _gameCodeLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    _gameCodeLabel.SetText(u"");

    _crcLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _crcLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    _crcLabel.SetText(u"");

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
                        char gameCodeBuf[5] = { 0 };
                        u32 crc32 = 0;
                        bool hasCachedCrc = false;

                        char statsPath[256];
                        bool hasStatsPath = false;

                        {
                            const TCHAR* fullPath = fileInfo.GetFullPath();
                            if (fullPath && fullPath[0] != 0) {
                                strncpy(statsPath, fullPath, sizeof(statsPath) - 1);
                                statsPath[sizeof(statsPath) - 1] = 0;
                            } else {
                                if (f_getcwd(statsPath, sizeof(statsPath)) == FR_OK) {
                                    int idx = strlcat(statsPath, "/", sizeof(statsPath));
                                    if (idx > 1 && statsPath[idx - 2] == '/')
                                        statsPath[idx - 1] = 0;
                                    strlcat(statsPath, fileInfo.GetFileName(), sizeof(statsPath));
                                } else {
                                    statsPath[0] = 0;
                                }
                            }
                            const char* normalizedPath = strchr(statsPath, ':');
                            if (normalizedPath) {
                                if (normalizedPath != statsPath) {
                                    size_t len = strlen(normalizedPath);
                                    memmove(statsPath, normalizedPath, len + 1);
                                }
                                hasStatsPath = statsPath[0] != 0;
                            } else {
                                statsPath[0] = 0;
                            }
                        }

                        char cachedId[16] = { 0 };
                        bool hasCachedID = false;
                        u8 cachedRomVersion = 0;
                        bool hasCachedRomVersion = false;

                        if (hasStatsPath)
                        {
                            LaunchStatsService::Instance().TryGetCachedData(statsPath,
                                &cachedRomVersion, &hasCachedRomVersion,
                                cachedId, sizeof(cachedId), &hasCachedID,
                                &crc32, &hasCachedCrc);
                        }

                        bool hasGameCode = false;
                        std::unique_ptr<InternalFileInfo> internalInfo;

                        if (hasCachedID && cachedId[0] != 0)
                        {
                            const char* dash1 = strchr(cachedId, '-');
                            if (dash1 && dash1[1] != 0)
                            {
                                strncpy(gameCodeBuf, dash1 + 1, 4);
                                gameCodeBuf[4] = 0;
                                hasGameCode = true;
                            }
                        }

                        if (!hasCachedID)
                        {
                            internalInfo.reset(fileInfo.CreateInternalFileInfo());
                            const char* gameCode = internalInfo ? internalInfo->GetGameCode() : nullptr;

                            if (gameCode && gameCode[0] != 0)
                            {
                                strncpy(gameCodeBuf, gameCode, 4);
                                gameCodeBuf[4] = 0;
                                hasGameCode = true;
                            }
                        }

                        if (!hasCachedCrc)
                        {
                            crc32 = 0;
                            File romFile;
                            romFile.Open(fileInfo.GetFastFileRef(), FA_READ);
                            if (romFile.GetSize() >= 512)
                            {
                                u8 header[512];
                                if (romFile.ReadExact(header, sizeof(header)))
                                {
                                    crc32 = ComputeCrc32(header, sizeof(header));
                                }
                            }
                            hasCachedCrc = true;
                        }

                        if (hasStatsPath && (!hasCachedID || !hasCachedCrc))
                        {
                            char idBuf[16] = { 0 };
                            bool hasID = false;
                            u8 romVersion = 0;
                            bool hasRomVersion = false;

                            if (hasCachedID)
                            {
                                mini_snprintf(idBuf, sizeof(idBuf), "%s", cachedId);
                                hasID = true;
                            }
                            else if (hasGameCode)
                            {
                                if (!internalInfo)
                                    internalInfo.reset(fileInfo.CreateInternalFileInfo());
                                if (internalInfo)
                                {
                                    auto* ndsInfo = static_cast<NdsInternalFileInfo*>(internalInfo.get());
                                    const u8 unitCode = ndsInfo->GetUnitCode();
                                    const char* platform = (unitCode == 0x02 || unitCode == 0x03) ? "TWL" : "NTR";
                                    const char* region = "UNK";
                                    if (gameCodeBuf[3]) {
                                        switch (gameCodeBuf[3]) {
                                            case 'J': region = "JPN"; break;
                                            case 'E': region = "USA"; break;
                                            case 'P': region = "EUR"; break;
                                            case 'D': region = "NOE"; break;
                                            case 'F': region = "FRA"; break;
                                            case 'S': region = "SPA"; break;
                                            case 'I': region = "ITA"; break;
                                            case 'K': region = "KOR"; break;
                                            case 'C': region = "CHN"; break;
                                            case 'W': region = "TWN"; break;
                                            case 'H': region = "NLD"; break;
                                            case 'R': region = "RUS"; break;
                                            case 'U': region = "AUS"; break;
                                            case 'V': region = "EUR"; break;
                                            default: break;
                                        }
                                    }
                                    mini_snprintf(idBuf, sizeof(idBuf), "%s-%c%c%c%c-%s",
                                        platform, gameCodeBuf[0], gameCodeBuf[1],
                                        gameCodeBuf[2], gameCodeBuf[3], region);
                                    hasID = true;
                                    romVersion = ndsInfo->GetRomVersion();
                                    hasRomVersion = true;
                                }
                            }

                            if (hasCachedRomVersion)
                            {
                                romVersion = cachedRomVersion;
                                hasRomVersion = true;
                            }

                            LaunchStatsService::Instance().SetCachedData(statsPath,
                                romVersion, hasRomVersion,
                                idBuf, hasID,
                                crc32, hasCachedCrc);
                        }

                        if (hasGameCode) {
                            memcpy(_gameCode, gameCodeBuf, 4);
                            _gameCode[4] = '\0';
                        } else {
                            _gameCode[0] = '\0';
                        }

                        if (crc32 != 0)
                        {
                            _crc = crc32;
                            _hasValidCrc = true;
                        }
                        else
                        {
                            _crc = 0;
                            _hasValidCrc = false;
                        }

                        char16_t wc[16];
                        int i = 0;
                        if (_gameCode[0] != 0) {
                            for (; _gameCode[i] && i < 4; ++i) wc[i] = (char16_t)(unsigned char)_gameCode[i];
                        }
                        wc[i] = 0;
                        _gameCodeLabel.SetText(wc);

                        if (_hasValidCrc)
                        {
                            char buf[16];
                            mini_snprintf(buf, sizeof(buf), "%08X", _crc);
                            for (i = 0; buf[i]; ++i) wc[i] = (char16_t)(unsigned char)buf[i];
                            wc[i] = 0;
                            _crcLabel.SetText(wc);
                        }
                        else
                        {
                            _crcLabel.SetText(u"");
                        }
                    }
                }
            }
        }
    }

    if (isNds) {
        AddChildTail(&_gameCodeLabel);
        AddChildTail(&_crcLabel);

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
        dma_ntrCopy32(3, smallHeartIconTiles, objVramManager->GetVramAddress(_smallHeartIconVramOffset), smallHeartIconTilesLen);

        _smallHeartIconFilledVramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
        dma_ntrCopy32(3, smallHeartIconFilledTiles, objVramManager->GetVramAddress(_smallHeartIconFilledVramOffset), smallHeartIconFilledTilesLen);

        UpdateFavoriteChipIcon();
    }
}

void NdsGameDetailsBottomSheetView::Update()
{
    BottomSheetView::Update();

    constexpr int screenWidth = 256;
    constexpr int rightPadding = 12;
    constexpr int chipGap = 8;
    
    _titleLabel.SetPosition(12, _position.y + 12);

    if (_hasCheatsChip)
    {
        int codeX = 180;
        int codeY = _position.y + 8;
        _gameCodeLabel.SetPosition(codeX, codeY);
        int codeW = _gameCodeLabel.GetStringWidth();
        if (_hasValidCrc)
        {
            _crcLabel.SetPosition(codeX + codeW + 8, codeY);
        }
    }

    if (_hasCheatsChip) {
        int totalChipWidth = GAME_DETAILS_CHEATS_CHIP_WIDTH + chipGap + GAME_DETAILS_FAVORITES_CHIP_WIDTH;
        int chipsStartX = screenWidth - rightPadding - totalChipWidth;

        _cheatsChip.SetPosition(chipsStartX, _position.y + 35);
        _favoriteChip.SetPosition(chipsStartX + GAME_DETAILS_CHEATS_CHIP_WIDTH + chipGap, _position.y + 35);
    } else {
        _cheatsChip.SetSecondaryText(u"");
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

void NdsGameDetailsBottomSheetView::OnDismissed()
{
    _romBrowserController->HideGameInfo();
}

bool NdsGameDetailsBottomSheetView::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
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
            {
                _romBrowserController->HideGameInfo();
            }
            return true;
        }
    }
    return false;
}

void NdsGameDetailsBottomSheetView::InitLaunchCountLabel(const MaterialColorScheme* materialColorScheme)
{
    u32 launchCount = 0;
    char lastLaunchDate[16] = {0};
    char lastLaunchTime[16] = {0};

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
                        LaunchStatsService::Instance().TryGetInfo(normalizedPath,
                            &launchCount,
                            lastLaunchDate, sizeof(lastLaunchDate),
                            lastLaunchTime, sizeof(lastLaunchTime));
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

    _lastLaunchLabel.SetText(Localization::Translate("last_launch"));
    _lastLaunchLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchLabel);

    _lastLaunchDateValueLabel.SetText(lastLaunchDate[0] != 0 ? lastLaunchDate : "-");
    _lastLaunchDateValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchDateValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchDateValueLabel);

    _lastLaunchTimeValueLabel.SetText(lastLaunchTime[0] != 0 ? lastLaunchTime : "-");
    _lastLaunchTimeValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchTimeValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchTimeValueLabel);
}
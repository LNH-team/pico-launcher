#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include "gui/Alignment.h"
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
#include "../FileType/Nds/NdsInternalFileInfo.h"
#include "../FileType/Gba/GbaInternalFileInfo.h"
#include "../RomHeaderUtil.h"
#include "core/mini-printf.h"
#include "fat/File.h"

static constexpr int GAME_DETAILS_CHEATS_CHIP_WIDTH    = 64;
static constexpr int GAME_DETAILS_FAVORITES_CHIP_WIDTH = 80;
static constexpr int GAME_DETAILS_IDENTITY_WIDTH       = 108;

static bool HasFileExtensionIgnoreCase(const char* path, const char* ext)
{
    if (!path || !ext)
        return false;

    size_t len = strlen(path);
    size_t extLen = strlen(ext);
    if (len < extLen)
        return false;

    const char* tail = path + len - extLen;
    for (size_t i = 0; i < extLen; i++)
    {
        char a = tail[i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z') a = a - 'A' + 'a';
        if (b >= 'A' && b <= 'Z') b = b - 'A' + 'a';
        if (a != b)
            return false;
    }

    return true;
}

static bool IsNdsFamilyRomFile(const FileInfo& fileInfo)
{
    const FileType* fileType = fileInfo.GetFileType();
    const char* name = fileInfo.GetFullPath();
    if (!name)
        name = fileInfo.GetFileName();

    return (fileType && strcmp(fileType->GetShortName(), "nds") == 0)
        || HasFileExtensionIgnoreCase(name, ".nds")
        || HasFileExtensionIgnoreCase(name, ".dsi")
        || HasFileExtensionIgnoreCase(name, ".srl");
}

static bool IsGbaRomFile(const FileInfo& fileInfo)
{
    const FileType* fileType = fileInfo.GetFileType();
    return fileType && strcmp(fileType->GetShortName(), "gba") == 0;
}

static bool BuildRomIdentityText(const FileInfo& fileInfo, char16_t* outText, u32 outTextLength)
{
    if (!outText || outTextLength == 0)
        return false;

    outText[0] = 0;

    char titleId[5] = { 0 };
    const char* prefix = nullptr;
    u8 headerBuffer[RomHeaderUtil::kHeaderReadSize];
    const bool hasHeader = RomHeaderUtil::ReadHeader(fileInfo.GetFastFileRef(), headerBuffer, sizeof(headerBuffer));

    if (IsNdsFamilyRomFile(fileInfo))
    {
        std::unique_ptr<InternalFileInfo> internalFileInfo(fileInfo.CreateInternalFileInfo());
        const auto* ndsInfo = static_cast<const NdsInternalFileInfo*>(internalFileInfo.get());

        prefix = (ndsInfo && ndsInfo->GetUnitCode() != 0) ? "TWL" : "NTR";
        if (ndsInfo && ndsInfo->GetGameCode() && ndsInfo->GetGameCode()[0] != 0)
        {
            StringUtil::Copy(titleId, ndsInfo->GetGameCode(), sizeof(titleId));
        }
        else if (hasHeader)
        {
            RomHeaderUtil::CopyTrimmedAsciiField(titleId, sizeof(titleId), headerBuffer + 0x0C, 4);
        }
    }
    else if (IsGbaRomFile(fileInfo))
    {
        std::unique_ptr<InternalFileInfo> internalFileInfo(fileInfo.CreateInternalFileInfo());
        const auto* gbaInfo = static_cast<const GbaInternalFileInfo*>(internalFileInfo.get());

        prefix = "AGB";
        if (gbaInfo && gbaInfo->GetGameCode() && gbaInfo->GetGameCode()[0] != 0)
        {
            StringUtil::Copy(titleId, gbaInfo->GetGameCode(), sizeof(titleId));
        }
        else if (hasHeader)
        {
            RomHeaderUtil::CopyTrimmedAsciiField(titleId, sizeof(titleId), headerBuffer + 0xAC, 4);
        }
    }

    if (!prefix || titleId[0] == 0)
        return false;

    char regionFallback[2] = { 0 };
    const char* regionText = RomHeaderUtil::GetRegionCodeText(titleId[3]);
    if (!regionText && titleId[3] != 0)
    {
        regionFallback[0] = RomHeaderUtil::ToUpperAscii(titleId[3]);
        regionText = regionFallback;
    }

    char buffer[24];
    if (regionText && regionText[0] != 0)
        mini_snprintf(buffer, sizeof(buffer), "%s - %s - %s", prefix, titleId, regionText);
    else
        mini_snprintf(buffer, sizeof(buffer), "%s - %s", prefix, titleId);

    StringUtil::Copy(outText, buffer, outTextLength);
    return true;
}

NdsGameDetailsBottomSheetView::NdsGameDetailsBottomSheetView(
    IRomBrowserController* romBrowserController,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _titleLabel(128, 16, 25, fontRepository->GetFont(FontType::Medium11))
    , _romIdentityLabel(GAME_DETAILS_IDENTITY_WIDTH, 16, 23, fontRepository->GetFont(FontType::Medium7_5))
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
        const FileInfo& fileInfo = _romBrowserController->GetTriggerFileInfo();
        isNds = IsNdsFamilyRomFile(fileInfo);

        char16_t romIdentityText[24] = { 0 };
        if (BuildRomIdentityText(fileInfo, romIdentityText, sizeof(romIdentityText) / sizeof(romIdentityText[0])))
        {
            _romIdentityLabel.SetText(romIdentityText);
            _romIdentityLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
            _romIdentityLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
            _romIdentityLabel.SetHorizontalAlignment(Alignment::End);
            AddChildTail(&_romIdentityLabel);
            _hasRomIdentity = true;
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

    _countLaunchLabel.SetText(Localization::Translate("total_launches"));
    _countLaunchLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _countLaunchLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_countLaunchLabel);

    _countLaunchValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _countLaunchValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_countLaunchValueLabel);

    _lastLaunchLabel.SetText(Localization::Translate("last_launch"));
    _lastLaunchLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchLabel);

    _lastLaunchDateValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchDateValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchDateValueLabel);

    _lastLaunchTimeValueLabel.SetBackgroundColor(materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
    _lastLaunchTimeValueLabel.SetForegroundColor(materialColorScheme->onSurfaceVariant);
    AddChildTail(&_lastLaunchTimeValueLabel);

    InitLaunchCountLabel();
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
    if (_hasRomIdentity)
        _romIdentityLabel.SetPosition(screenWidth - rightPadding - GAME_DETAILS_IDENTITY_WIDTH, _position.y + 12);

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

void NdsGameDetailsBottomSheetView::InitLaunchCountLabel()
{
    u32 launchCount = 0;
    char lastLaunchDate[16] = {};
    char lastLaunchTime[16] = {};

    if (_romBrowserController)
    {
        const FileInfo& fileInfo = _romBrowserController->GetTriggerFileInfo();
        const char* fileName = fileInfo.GetFileName();
        if (fileName && fileName[0] != '\0')
        {
            LaunchStatsService::Instance().TryGetInfo(fileName,
                &launchCount,
                lastLaunchDate, sizeof(lastLaunchDate),
                lastLaunchTime, sizeof(lastLaunchTime));
        }
    }

    char countText[12];
    snprintf(countText, sizeof(countText), "%lu", launchCount);
    _countLaunchValueLabel.SetText(countText);

    _lastLaunchDateValueLabel.SetText(lastLaunchDate[0] != '\0' ? lastLaunchDate : "-");
    _lastLaunchTimeValueLabel.SetText(lastLaunchTime[0] != '\0' ? lastLaunchTime : "-");
}

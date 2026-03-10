#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "BannerView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "../layout/LayoutService.h"

class RomBrowserViewModel;
class IRomBrowserViewFactory;
class IBgmService;
class IFontRepository;
struct MaterialColorScheme;

class RomBrowserTopScreenView : public ViewContainer
{
public:
    RomBrowserTopScreenView(const SharedPtr<RomBrowserViewModel>& viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        const IBgmService* bgmService,
        const LayoutService* layoutService);

    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

private:
    SharedPtr<RomBrowserViewModel> _viewModel;
    const IThemeFileIconFactory* _themeFileIconFactory;
    std::unique_ptr<BannerView> _fileInfoView;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;

    Label2DView _dateTime1Label;
    Label2DView _dateTime2Label;
    Label2DView _prefixLabel;
    Label2DView _gameIdTagLabel;
    Label2DView _gameIdLabel;
    Label2DView _regionLabel;
    Label2DView _versionLabel;
    Label2DView _crcLabel;
    Label2DView _usernameLabel;

    bool _iconGraphicsUploaded = false;
    bool _coverGraphicsUploaded = false;
    bool _lastIconVisible = true;
    bool _showCover;
    const IBgmService* _bgmService;
    const IFontRepository* _fontRepository;
    const LayoutService* _layoutService;

    u64 _lastTimeUpdateTick = 0;
    u8 _lastYear     = 0xFF;
    u8 _lastMonth    = 0xFF;
    u8 _lastMonthDay = 0xFF;
    u8 _lastHour     = 0xFF;
    u8 _lastMinute   = 0xFF;
    u8 _lastSecond   = 0xFF;

    u8 _lastDt1Format = 0xFF;
    u8 _lastDt1Sep    = 0xFF;
    u8 _lastDt2Format = 0xFF;
    u8 _lastDt2Sep    = 0xFF;
    u8 _lastDt1Font   = 0xFF;
    u8 _lastDt2Font   = 0xFF;
    u8 _lastPrefixFont = 0xFF;
    u8 _lastGameIdTagFont = 0xFF;
    u8 _lastGameIdFont = 0xFF;
    u8 _lastRegionFont = 0xFF;
    u8 _lastVersionFont = 0xFF;
    u8 _lastCrcFont   = 0xFF;
    u8 _lastUsernameFont = 0xFF;

    char _cachedIdPrefix[24] = { 0 };
    char _cachedIdGameId[5] = { 0 };
    char _cachedIdRegion[4] = { 0 };
    bool _hasCachedIdPrefix = false;
    bool _hasCachedIdGameId = false;
    bool _hasCachedIdRegion = false;
    u8 _cachedRomVersion = 0;
    bool _hasCachedRomVersion = false;

    char16_t _cachedUserName[24] = { 0 };

    void UpdateDateTimeLabels(bool forceUpdate);
    void UpdateLayoutFonts();
    void UpdateIdAndVersionLabels();
    void UpdateStaticLabels();
};

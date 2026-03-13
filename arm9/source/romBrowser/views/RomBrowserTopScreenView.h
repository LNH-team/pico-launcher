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
    const IBgmService* _bgmService;
    bool _showCover;
    const IFontRepository* _fontRepository;
    const LayoutService* _layoutService;

    std::unique_ptr<BannerView> _fileInfoView;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;

    Label2DView _dateTime1Label;
    Label2DView _dateTime2Label;
    Label2DView _usernameLabel;
    Label2DView _gameTitleLabel;
    Label2DView _prefixLabel;
    Label2DView _TitleIDTagLabel;
    Label2DView _TitleIDLabel;
    Label2DView _regionLabel;
    Label2DView _crcLabel;
    Label2DView _versionLabel;

    bool _coverGraphicsUploaded = false;
    bool _lastIconVisible = true;

    u64 _lastTimeUpdateTick = 0;
    u8 _lastYear = 0xFF, _lastMonth = 0xFF, _lastMonthDay = 0xFF;
    u8 _lastHour = 0xFF, _lastMinute = 0xFF, _lastSecond = 0xFF;

    u8 _lastDt1Format = 0xFF, _lastDt1Sep = 0xFF, _lastDt1Font = 0xFF;
    u8 _lastDt2Format = 0xFF, _lastDt2Sep = 0xFF, _lastDt2Font = 0xFF;
    u8 _lastUsernameFont = 0xFF;
    u8 _lastGameTitleFont = 0xFF;
    u8 _lastPrefixFont = 0xFF;
    u8 _lastTitleIDTagFont = 0xFF;
    u8 _lastTitleIDFont = 0xFF;
    u8 _lastRegionFont = 0xFF;
    u8 _lastCrcFont = 0xFF;
    u8 _lastVersionFont = 0xFF;

    char16_t _cachedUserName[24] = { 0 };
    
    char _cachedGameTitle[13] = { 0 };
    bool _hasCachedGameTitle = false;

    char _cachedIdPrefix[24] = { 0 };
    bool _hasCachedIdPrefix = false;
    
    char _cachedIdTitleID[5] = { 0 };
    bool _hasCachedIdTitleID = false;
    
    char _cachedIdRegion[4] = { 0 };
    bool _hasCachedIdRegion = false;

    u8   _cachedRomVersion = 0;
    bool _hasCachedRomVersion = false;

    void UpdateDateTimeLabels(bool forceUpdate);
    void UpdateLayoutFonts();
    void UpdateIdAndVersionLabels();
    void UpdateStaticLabels();
};

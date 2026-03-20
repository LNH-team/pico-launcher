#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "BannerView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "services/Layout/LayoutService.h"

class RomBrowserViewModel;
class IRomBrowserViewFactory;
class IBgmService;
class IFontRepository;
class InternalFileInfo;
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
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    const LayoutService* _layoutService;

    std::unique_ptr<BannerView> _fileInfoView;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    std::unique_ptr<InternalFileInfo> _selectedInternalFileInfo;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;

    Label2DView _dateTime1Label;
    Label2DView _dateTime2Label;
    Label2DView _usernameLabel;
    Label2DView _gameTitleLabel;
    Label2DView _prefixLabel;
    Label2DView _titleIdLabel;
    Label2DView _titleIdTagLabel;
    Label2DView _regionLabel;
    Label2DView _crcLabel;
    Label2DView _versionLabel;

    enum class SelectedRomType : u8
    {
        None,
        Gba,
        Ntr,
        Twl,
    };

    struct SelectedRomMetadata
    {
        SelectedRomType type = SelectedRomType::None;
        bool hasGameTitle = false;
        bool hasTitleId = false;
        bool hasRegion = false;
        bool hasCrc = false;
        bool hasVersion = false;
        char16_t gameTitle[32] = { 0 };
        char titleId[5] = { 0 };
        char16_t region[8] = { 0 };
        u32 crc = 0;
        u8 version = 0;
    };

    bool _coverGraphicsUploaded = false;
    bool _lastIconVisible = true;
    bool _useMaterialCardBackgrounds = false;

    u64 _lastTimeUpdateTick = 0;
    u8 _lastYear = 0xFF, _lastMonth = 0xFF, _lastMonthDay = 0xFF;
    u8 _lastHour = 0xFF, _lastMinute = 0xFF, _lastSecond = 0xFF;

    u8 _lastDt1Format = 0xFF, _lastDt1Sep = 0xFF, _lastDt1Font = 0xFF;
    u8 _lastDt2Format = 0xFF, _lastDt2Sep = 0xFF, _lastDt2Font = 0xFF;
    u8 _lastUsernameFont = 0xFF;

    char16_t _cachedUserName[24] = { 0 };
    SelectedRomMetadata _selectedRomMetadata;
    char16_t _gameTitleText[32] = { 0 };
    char16_t _prefixText[32] = { 0 };
    char16_t _titleIdText[16] = { 0 };
    char16_t _titleIdTagText[8] = { 0 };
    char16_t _regionText[8] = { 0 };
    char16_t _crcText[16] = { 0 };
    char16_t _versionText[8] = { 0 };

    void UpdateDateTimeLabels(bool forceUpdate);
    void UpdateLayoutFonts();
    void UpdateLabelBackgrounds();
    void UpdateStaticLabels();
    void UpdateRomMetadataLabels();
    void RefreshSelectedRomMetadata(const InternalFileInfo* internalFileInfo = nullptr);
};

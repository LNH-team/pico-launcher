#include "common.h"
#include <string.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
#include <nds/system.h>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "bgm/IBgmService.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "../FileType/Gba/GbaInternalFileInfo.h"
#include "../FileType/Nds/NdsInternalFileInfo.h"
#include "../RomHeaderUtil.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "rtcIpc.h"
#include "RomBrowserTopScreenView.h"

static u8 bcdToDecimal(u8 bcd)
{
    u8 ones = bcd & 0x0F;
    u8 tens = (bcd >> 4) & 0x0F;
    if (ones > 9 || tens > 9)
        return 0;
    return (u8)(tens * 10 + ones);
}

static void sanitizeDateTime(u8& month, u8& monthDay, u8& hour, u8& minute, u8& second)
{
    if (month < 1 || month > 12)   month = 1;
    if (monthDay < 1 || monthDay > 31) monthDay = 1;
    if (hour > 23)   hour = 0;
    if (minute > 59) minute = 0;
    if (second > 59) second = 0;
}

static void FormatLayoutDateTime(char16_t* outText, u32 outLen,
    u8 year, u8 month, u8 monthDay, u8 hour, u8 minute, u8 second,
    u8 format, u8 sepIdx)
{
    if (outLen == 0) return;

    char sep = (sepIdx < LAYOUT_SEP_COUNT) ? kLayoutSeparatorChars[sepIdx] : '/';
    char sepStr[2] = { sep, '\0' };

    char buf[28];
    buf[0] = '\0';

    u32 fullYear = 2000u + year;
    u32 shortYear = year;
    u32 hour12 = hour % 12;
    if (hour12 == 0) hour12 = 12;

    switch (format)
    {
        case LAYOUT_FMT_DD_MM_YYYY:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%04u",
                monthDay, sep, month, sep, fullYear);
            break;
        case LAYOUT_FMT_YYYY_MM_DD:
            mini_snprintf(buf, sizeof(buf), "%04u%c%02u%c%02u",
                fullYear, sep, month, sep, monthDay);
            break;
        case LAYOUT_FMT_MM_DD_YYYY:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%04u",
                month, sep, monthDay, sep, fullYear);
            break;
        case LAYOUT_FMT_YY_MM_DD:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%02u",
                shortYear, sep, month, sep, monthDay);
            break;
        case LAYOUT_FMT_HH_mm:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u",
                hour, sep, minute);
            break;
        case LAYOUT_FMT_HH_mm_ss:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%02u",
                hour, sep, minute, sep, second);
            break;
        case LAYOUT_FMT_hh_mm:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u",
                hour12, sep, minute);
            break;
        case LAYOUT_FMT_hh_mm_ss:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%02u",
                hour12, sep, minute, sep, second);
            break;
        case LAYOUT_FMT_YYYY_MM_DD_HH_mm:
            mini_snprintf(buf, sizeof(buf), "%04u%c%02u%c%02u %02u%c%02u",
                fullYear, sep, month, sep, monthDay, hour, sep, minute);
            break;
        case LAYOUT_FMT_YYYY_MM_DD_HH_mm_ss:
            mini_snprintf(buf, sizeof(buf), "%04u%c%02u%c%02u %02u%c%02u%c%02u",
                fullYear, sep, month, sep, monthDay, hour, sep, minute, sep, second);
            break;
        case LAYOUT_FMT_DD_MM_YYYY_HH_mm:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%04u %02u%c%02u",
                monthDay, sep, month, sep, fullYear, hour, sep, minute);
            break;
        case LAYOUT_FMT_DD_MM_YYYY_HH_mm_ss:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%04u %02u%c%02u%c%02u",
                monthDay, sep, month, sep, fullYear, hour, sep, minute, sep, second);
            break;
        case LAYOUT_FMT_YY_MM_DD_HH_mm:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%02u %02u%c%02u",
                shortYear, sep, month, sep, monthDay, hour, sep, minute);
            break;
        case LAYOUT_FMT_YY_MM_DD_HH_mm_ss:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%02u %02u%c%02u%c%02u",
                shortYear, sep, month, sep, monthDay, hour, sep, minute, sep, second);
            break;
        default:
            mini_snprintf(buf, sizeof(buf), "%02u%c%02u%c%04u",
                monthDay, sep, month, sep, fullYear);
            break;
    }

    StringUtil::Copy(outText, buf, outLen);
    (void)sepStr;
}

static void CopyUserNameFromFirmware(char16_t* outText, u32 outTextLength)
{
    if (!outText || outTextLength == 0)
        return;

    outText[0] = 0;
    u32 nameLen = PersonalData->nameLen;
    if (nameLen == 0 || nameLen > 10)
        return;

    u32 outIdx = 0;
    for (u32 i = 0; i < nameLen && outIdx + 1 < outTextLength; i++)
    {
        s16 ch = PersonalData->name[i];
        if (ch == 0)
            break;
        outText[outIdx++] = (char16_t)ch;
    }
    outText[outIdx] = 0;
}

static bool TextEquals(const char16_t* lhs, const char16_t* rhs)
{
    if (lhs == rhs)
    {
        return true;
    }

    if (!lhs || !rhs)
    {
        return false;
    }

    while (*lhs == *rhs)
    {
        if (*lhs == 0)
        {
            return true;
        }

        lhs++;
        rhs++;
    }

    return false;
}

static void SetLabelTextIfChanged(Label2DView& label, char16_t* cachedText, u32 cachedTextLength,
    const char16_t* newText)
{
    if (!newText)
    {
        newText = u"";
    }

    if (TextEquals(cachedText, newText))
    {
        return;
    }

    StringUtil::Copy(cachedText, newText, cachedTextLength);
    label.SetText(cachedText);
}

static constexpr Rectangle kMaterialSubCardBounds(10, 116, 236, 66);

static const Rgb<8, 8, 8>& GetMaterialFieldBackground(
    const MaterialColorScheme& materialColorScheme, const Rectangle& bounds)
{
    return kMaterialSubCardBounds.Contains(bounds.GetCenter())
        ? materialColorScheme.secondaryContainer
        : materialColorScheme.inverseOnSurface;
}

static bool IsDefaultLayoutTextColor(const Rgb<8, 8, 8>& color)
{
    const bool isBlack = color.r == 0 && color.g == 0 && color.b == 0;
    const bool isWhite = color.r == 255 && color.g == 255 && color.b == 255;
    return isBlack || isWhite;
}

static Rgb<8, 8, 8> GetMaterialFieldForeground(
    const MaterialColorScheme& materialColorScheme,
    const Rectangle& bounds,
    const Rgb<8, 8, 8>& configuredColor)
{
    if (!IsDefaultLayoutTextColor(configuredColor))
    {
        return configuredColor;
    }

    return kMaterialSubCardBounds.Contains(bounds.GetCenter())
        ? materialColorScheme.onSecondaryContainer
        : materialColorScheme.onSurface;
}

RomBrowserTopScreenView::RomBrowserTopScreenView(
    const SharedPtr<RomBrowserViewModel>& viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    const IBgmService* bgmService,
    const LayoutService* layoutService)
    : _viewModel(viewModel)
    , _themeFileIconFactory(themeFileIconFactory)
    , _bgmService(bgmService)
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _layoutService(layoutService)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _selectedFileIcon(nullptr)
    , _selectedFileCover(nullptr)
    
    , _dateTime1Label(120, 16, 26,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().dateTime1.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().dateTime1.font : LAYOUT_FONT_REGULAR10)))
    , _dateTime2Label(120, 16, 26,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().dateTime2.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().dateTime2.font : LAYOUT_FONT_REGULAR10)))
    , _usernameLabel(80, 16, 20,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().username.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().username.font : LAYOUT_FONT_REGULAR10)))
    , _gameTitleLabel(100, 16, 31,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().gameTitle.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().gameTitle.font : LAYOUT_FONT_REGULAR10)))
    , _prefixLabel(90, 16, 31,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().prefix.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().prefix.font : LAYOUT_FONT_REGULAR10)))
    , _titleIdLabel(48, 16, 15,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().TitleID.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().TitleID.font : LAYOUT_FONT_REGULAR10)))
    , _titleIdTagLabel(48, 16, 7,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().TitleID.labelFont < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().TitleID.labelFont : LAYOUT_FONT_REGULAR10)))
    , _regionLabel(48, 16, 7,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().region.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().region.font : LAYOUT_FONT_REGULAR10)))
    , _crcLabel(96, 16, 15,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().crc.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().crc.font : LAYOUT_FONT_REGULAR10)))
    , _versionLabel(48, 16, 7,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().version.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().version.font : LAYOUT_FONT_REGULAR10)))
{
    const auto& layout = layoutService->GetCurrentLayout();
    
    // Colors
    _dateTime1Label.SetForegroundColor({ 255, 255, 255 });
    _dateTime1Label.SetBackgroundColor({ 0, 0, 0 });
    _dateTime2Label.SetForegroundColor({ 255, 255, 255 });
    _dateTime2Label.SetBackgroundColor({ 0, 0, 0 });
    _usernameLabel.SetForegroundColor({ 255, 255, 255 });
    _usernameLabel.SetBackgroundColor({ 0, 0, 0 });
    _gameTitleLabel.SetForegroundColor({ 255, 255, 255 });
    _gameTitleLabel.SetBackgroundColor({ 0, 0, 0 });
    _prefixLabel.SetForegroundColor({ 255, 255, 255 });
    _prefixLabel.SetBackgroundColor({ 0, 0, 0 });
    _titleIdLabel.SetForegroundColor({ 255, 255, 255 });
    _titleIdLabel.SetBackgroundColor({ 0, 0, 0 });
    _titleIdTagLabel.SetForegroundColor({ 255, 255, 255 });
    _titleIdTagLabel.SetBackgroundColor({ 0, 0, 0 });
    _regionLabel.SetForegroundColor({ 255, 255, 255 });
    _regionLabel.SetBackgroundColor({ 0, 0, 0 });
    _crcLabel.SetForegroundColor({ 255, 255, 255 });
    _crcLabel.SetBackgroundColor({ 0, 0, 0 });
    _versionLabel.SetForegroundColor({ 255, 255, 255 });
    _versionLabel.SetBackgroundColor({ 0, 0, 0 });

    // Position
    _dateTime1Label.SetPosition(layout.dateTime1.x, layout.dateTime1.y);
    _dateTime2Label.SetPosition(layout.dateTime2.x, layout.dateTime2.y);
    _usernameLabel.SetPosition(layout.username.x, layout.username.y);

    // Initialize DateTime
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    _lastYear     = bcdToDecimal(dateTime.date.year);
    _lastMonth    = bcdToDecimal(dateTime.date.month);
    _lastMonthDay = bcdToDecimal(dateTime.date.monthDay);
    _lastHour     = bcdToDecimal(dateTime.time.hour);
    _lastMinute   = bcdToDecimal(dateTime.time.minute);
    _lastSecond   = bcdToDecimal(dateTime.time.second);
    sanitizeDateTime(_lastMonth, _lastMonthDay, _lastHour, _lastMinute, _lastSecond);

    char16_t buf[28];
    FormatLayoutDateTime(buf, sizeof(buf)/sizeof(char16_t),
        _lastYear, _lastMonth, _lastMonthDay, _lastHour, _lastMinute, _lastSecond,
        layout.dateTime1.format, layout.dateTime1.separator);
    _dateTime1Label.SetText(buf);
    _lastDt1Format = layout.dateTime1.format;
    _lastDt1Sep    = layout.dateTime1.separator;
    _lastDt1Font   = layout.dateTime1.font;

    FormatLayoutDateTime(buf, sizeof(buf)/sizeof(char16_t),
        _lastYear, _lastMonth, _lastMonthDay, _lastHour, _lastMinute, _lastSecond,
        layout.dateTime2.format, layout.dateTime2.separator);
    _dateTime2Label.SetText(buf);
    _lastDt2Format = layout.dateTime2.format;
    _lastDt2Sep    = layout.dateTime2.separator;
    _lastDt2Font   = layout.dateTime2.font;

    // Fonts
    _lastUsernameFont = layout.username.font;

    // Static texts
    CopyUserNameFromFirmware(_cachedUserName, sizeof(_cachedUserName) / sizeof(_cachedUserName[0]));
    _usernameLabel.SetText(_cachedUserName);
    _titleIdTagLabel.SetText(u"TID:");
    StringUtil::Copy(_titleIdTagText, u"TID: ", sizeof(_titleIdTagText) / sizeof(_titleIdTagText[0]));

    AddChildTail(_fileInfoView.get());
}

void RomBrowserTopScreenView::UpdateLayoutFonts()
{
    const auto& layout = _layoutService->GetCurrentLayout();

    u8 dt1Font = layout.dateTime1.font < LAYOUT_FONT_COUNT ? layout.dateTime1.font : LAYOUT_FONT_REGULAR10;
    u8 dt2Font = layout.dateTime2.font < LAYOUT_FONT_COUNT ? layout.dateTime2.font : LAYOUT_FONT_REGULAR10;
    u8 usernameFont = layout.username.font < LAYOUT_FONT_COUNT ? layout.username.font : LAYOUT_FONT_REGULAR10;

    if (dt1Font != _lastDt1Font)
    {
        _dateTime1Label.SetFont(_fontRepository->GetFont(static_cast<FontType>(dt1Font)));
        _lastDt1Font = dt1Font;
    }

    if (dt2Font != _lastDt2Font)
    {
        _dateTime2Label.SetFont(_fontRepository->GetFont(static_cast<FontType>(dt2Font)));
        _lastDt2Font = dt2Font;
    }

    if (usernameFont != _lastUsernameFont)
    {
        _usernameLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(usernameFont)));
        _lastUsernameFont = usernameFont;
    }
}

void RomBrowserTopScreenView::UpdateStaticLabels()
{
    const auto& layout = _layoutService->GetCurrentLayout();
    _usernameLabel.SetText(_cachedUserName);
    _usernameLabel.SetPosition(layout.username.visible ? layout.username.x : -320,
        layout.username.visible ? layout.username.y : -320);
    UpdateRomMetadataLabels();
}

void RomBrowserTopScreenView::UpdateLabelBackgrounds()
{
    const auto& layout = _layoutService->GetCurrentLayout();
    const Rgb<8, 8, 8> dateTime1Color(layout.dateTime1.colorR, layout.dateTime1.colorG, layout.dateTime1.colorB);
    const Rgb<8, 8, 8> dateTime2Color(layout.dateTime2.colorR, layout.dateTime2.colorG, layout.dateTime2.colorB);
    const Rgb<8, 8, 8> usernameColor(layout.username.colorR, layout.username.colorG, layout.username.colorB);
    const Rgb<8, 8, 8> gameTitleColor(layout.gameTitle.colorR, layout.gameTitle.colorG, layout.gameTitle.colorB);
    const Rgb<8, 8, 8> prefixColor(layout.prefix.colorR, layout.prefix.colorG, layout.prefix.colorB);
    const Rgb<8, 8, 8> titleIdColor(layout.TitleID.colorR, layout.TitleID.colorG, layout.TitleID.colorB);
    const Rgb<8, 8, 8> titleIdTagColor(
        layout.TitleID.labelColorR, layout.TitleID.labelColorG, layout.TitleID.labelColorB);
    const Rgb<8, 8, 8> regionColor(layout.region.colorR, layout.region.colorG, layout.region.colorB);
    const Rgb<8, 8, 8> crcColor(layout.crc.colorR, layout.crc.colorG, layout.crc.colorB);
    const Rgb<8, 8, 8> versionColor(layout.version.colorR, layout.version.colorG, layout.version.colorB);

    _dateTime1Label.SetForegroundColor(dateTime1Color);
    _dateTime2Label.SetForegroundColor(dateTime2Color);
    _usernameLabel.SetForegroundColor(usernameColor);

    if (!_useMaterialCardBackgrounds || !_materialColorScheme)
    {
        return;
    }

    auto applyMaterialStyle = [this](Label2DView& label, const Rgb<8, 8, 8>& configuredColor)
    {
        const auto bounds = label.GetBounds();
        label.SetBackgroundColor(GetMaterialFieldBackground(*_materialColorScheme, bounds));
        label.SetForegroundColor(GetMaterialFieldForeground(*_materialColorScheme, bounds, configuredColor));
    };

    applyMaterialStyle(_dateTime1Label, dateTime1Color);
    applyMaterialStyle(_dateTime2Label, dateTime2Color);
    applyMaterialStyle(_usernameLabel, usernameColor);
    applyMaterialStyle(_gameTitleLabel, gameTitleColor);
    applyMaterialStyle(_prefixLabel, prefixColor);
    applyMaterialStyle(_titleIdLabel, titleIdColor);
    applyMaterialStyle(_titleIdTagLabel, titleIdTagColor);
    applyMaterialStyle(_regionLabel, regionColor);
    applyMaterialStyle(_crcLabel, crcColor);
    applyMaterialStyle(_versionLabel, versionColor);
}

void RomBrowserTopScreenView::RefreshSelectedRomMetadata(const InternalFileInfo* internalFileInfo)
{
    _selectedRomMetadata = {};

    const int selectedItem = _viewModel->GetSelectedItem();
    auto& fileInfoManager = _viewModel->GetFileInfoManager();
    if (selectedItem < 0 || selectedItem >= (int)fileInfoManager.GetItemCount())
    {
        return;
    }

    const auto& item = fileInfoManager.GetItem(selectedItem);
    const char* shortName = item.GetFileType()->GetShortName();
    if (!shortName)
    {
        return;
    }

    u8 headerBuffer[RomHeaderUtil::kHeaderReadSize];
    if (!RomHeaderUtil::ReadHeader(item.GetFastFileRef(), headerBuffer, sizeof(headerBuffer)))
    {
        return;
    }

    if (!strcmp(shortName, "nds"))
    {
        std::unique_ptr<InternalFileInfo> ownedInternalFileInfo;
        if (!internalFileInfo)
        {
            ownedInternalFileInfo.reset(item.CreateInternalFileInfo());
            internalFileInfo = ownedInternalFileInfo.get();
        }

        const auto* ndsInfo = static_cast<const NdsInternalFileInfo*>(internalFileInfo);

        _selectedRomMetadata.type = (ndsInfo && ndsInfo->GetUnitCode() != 0)
            ? SelectedRomType::Twl
            : SelectedRomType::Ntr;

        RomHeaderUtil::CopyTrimmedAsciiField(
            _selectedRomMetadata.gameTitle,
            sizeof(_selectedRomMetadata.gameTitle) / sizeof(_selectedRomMetadata.gameTitle[0]),
            headerBuffer, 12);
        _selectedRomMetadata.hasGameTitle = _selectedRomMetadata.gameTitle[0] != 0;

        if (ndsInfo && ndsInfo->GetGameCode() && ndsInfo->GetGameCode()[0] != 0)
        {
            StringUtil::Copy(_selectedRomMetadata.titleId, ndsInfo->GetGameCode(),
                sizeof(_selectedRomMetadata.titleId));
        }
        else
        {
            RomHeaderUtil::CopyTrimmedAsciiField(
                _selectedRomMetadata.titleId, sizeof(_selectedRomMetadata.titleId),
                headerBuffer + 0x0C, 4);
        }
        _selectedRomMetadata.hasTitleId = _selectedRomMetadata.titleId[0] != 0;

        if (_selectedRomMetadata.hasTitleId)
        {
            RomHeaderUtil::CopyRegionCodeText(
                _selectedRomMetadata.region,
                sizeof(_selectedRomMetadata.region) / sizeof(_selectedRomMetadata.region[0]),
                _selectedRomMetadata.titleId[3]);
            _selectedRomMetadata.hasRegion = _selectedRomMetadata.region[0] != 0;
        }

        _selectedRomMetadata.crc = RomHeaderUtil::ComputeCrc32(headerBuffer, sizeof(headerBuffer));
        _selectedRomMetadata.hasCrc = true;
        _selectedRomMetadata.version = ndsInfo ? ndsInfo->GetRomVersion() : headerBuffer[0x1E];
        _selectedRomMetadata.hasVersion = true;
        return;
    }

    if (!strcmp(shortName, "gba"))
    {
        std::unique_ptr<InternalFileInfo> ownedInternalFileInfo;
        if (!internalFileInfo)
        {
            ownedInternalFileInfo.reset(item.CreateInternalFileInfo());
            internalFileInfo = ownedInternalFileInfo.get();
        }

        const auto* gbaInfo = static_cast<const GbaInternalFileInfo*>(internalFileInfo);

        _selectedRomMetadata.type = SelectedRomType::Gba;

        RomHeaderUtil::CopyTrimmedAsciiField(
            _selectedRomMetadata.gameTitle,
            sizeof(_selectedRomMetadata.gameTitle) / sizeof(_selectedRomMetadata.gameTitle[0]),
            headerBuffer + 0xA0, 12);
        _selectedRomMetadata.hasGameTitle = _selectedRomMetadata.gameTitle[0] != 0;

        if (gbaInfo && gbaInfo->GetGameCode() && gbaInfo->GetGameCode()[0] != 0)
        {
            StringUtil::Copy(_selectedRomMetadata.titleId, gbaInfo->GetGameCode(),
                sizeof(_selectedRomMetadata.titleId));
        }
        else
        {
            RomHeaderUtil::CopyTrimmedAsciiField(
                _selectedRomMetadata.titleId, sizeof(_selectedRomMetadata.titleId),
                headerBuffer + 0xAC, 4);
        }
        _selectedRomMetadata.hasTitleId = _selectedRomMetadata.titleId[0] != 0;

        if (_selectedRomMetadata.hasTitleId)
        {
            RomHeaderUtil::CopyRegionCodeText(
                _selectedRomMetadata.region,
                sizeof(_selectedRomMetadata.region) / sizeof(_selectedRomMetadata.region[0]),
                _selectedRomMetadata.titleId[3]);
            _selectedRomMetadata.hasRegion = _selectedRomMetadata.region[0] != 0;
        }

        _selectedRomMetadata.crc = RomHeaderUtil::ComputeCrc32(headerBuffer, sizeof(headerBuffer));
        _selectedRomMetadata.hasCrc = true;
    }
}

void RomBrowserTopScreenView::UpdateRomMetadataLabels()
{
    const auto& layout = _layoutService->GetCurrentLayout();

    char16_t prefixText[sizeof(_prefixText) / sizeof(_prefixText[0])] = { 0 };
    char16_t titleIdText[sizeof(_titleIdText) / sizeof(_titleIdText[0])] = { 0 };
    char16_t regionText[sizeof(_regionText) / sizeof(_regionText[0])] = { 0 };
    char16_t crcText[sizeof(_crcText) / sizeof(_crcText[0])] = { 0 };
    char16_t versionText[sizeof(_versionText) / sizeof(_versionText[0])] = { 0 };

    const char* prefixValue = nullptr;
    switch (_selectedRomMetadata.type)
    {
        case SelectedRomType::Gba:
            prefixValue = kLayoutPrefixGbaModeNames[layout.prefix.gbaPrefixMode % LAYOUT_PREFIX_GBA_COUNT];
            break;
        case SelectedRomType::Ntr:
            prefixValue = kLayoutPrefixNtrModeNames[layout.prefix.ntrPrefixMode % LAYOUT_PREFIX_NTR_COUNT];
            break;
        case SelectedRomType::Twl:
            prefixValue = kLayoutPrefixTwlModeNames[layout.prefix.twlPrefixMode % LAYOUT_PREFIX_TWL_COUNT];
            break;
        case SelectedRomType::None:
        default:
            break;
    }

    if (prefixValue)
    {
        StringUtil::Copy(prefixText, prefixValue, sizeof(prefixText) / sizeof(prefixText[0]));
        if (layout.prefix.trailingDash)
        {
            RomHeaderUtil::AppendTrailingDash(prefixText, sizeof(prefixText) / sizeof(prefixText[0]));
        }
    }

    if (_selectedRomMetadata.hasTitleId)
    {
        StringUtil::Copy(titleIdText, _selectedRomMetadata.titleId,
            sizeof(titleIdText) / sizeof(titleIdText[0]));
        if (layout.TitleID.trailingDash)
        {
            RomHeaderUtil::AppendTrailingDash(titleIdText, sizeof(titleIdText) / sizeof(titleIdText[0]));
        }
    }

    if (_selectedRomMetadata.hasRegion)
    {
        StringUtil::Copy(regionText, _selectedRomMetadata.region,
            sizeof(regionText) / sizeof(regionText[0]));
        if (layout.region.trailingDash)
        {
            RomHeaderUtil::AppendTrailingDash(regionText, sizeof(regionText) / sizeof(regionText[0]));
        }
    }

    if (_selectedRomMetadata.hasCrc)
    {
        RomHeaderUtil::FormatHexU32(crcText, sizeof(crcText) / sizeof(crcText[0]), _selectedRomMetadata.crc);
    }

    if (_selectedRomMetadata.hasVersion)
    {
        RomHeaderUtil::FormatUnsignedU32(
            versionText, sizeof(versionText) / sizeof(versionText[0]), _selectedRomMetadata.version);
    }

    SetLabelTextIfChanged(_gameTitleLabel, _gameTitleText,
        sizeof(_gameTitleText) / sizeof(_gameTitleText[0]), _selectedRomMetadata.gameTitle);
    SetLabelTextIfChanged(_prefixLabel, _prefixText,
        sizeof(_prefixText) / sizeof(_prefixText[0]), prefixText);
    SetLabelTextIfChanged(_titleIdLabel, _titleIdText,
        sizeof(_titleIdText) / sizeof(_titleIdText[0]), titleIdText);
    SetLabelTextIfChanged(_regionLabel, _regionText,
        sizeof(_regionText) / sizeof(_regionText[0]), regionText);
    SetLabelTextIfChanged(_crcLabel, _crcText,
        sizeof(_crcText) / sizeof(_crcText[0]), crcText);
    SetLabelTextIfChanged(_versionLabel, _versionText,
        sizeof(_versionText) / sizeof(_versionText[0]), versionText);

    _gameTitleLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.gameTitle.font < LAYOUT_FONT_COUNT ? layout.gameTitle.font : LAYOUT_FONT_REGULAR10)));
    _prefixLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.prefix.font < LAYOUT_FONT_COUNT ? layout.prefix.font : LAYOUT_FONT_REGULAR10)));
    _titleIdLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.TitleID.font < LAYOUT_FONT_COUNT ? layout.TitleID.font : LAYOUT_FONT_REGULAR10)));
    _titleIdTagLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.TitleID.labelFont < LAYOUT_FONT_COUNT ? layout.TitleID.labelFont : LAYOUT_FONT_REGULAR10)));
    _regionLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.region.font < LAYOUT_FONT_COUNT ? layout.region.font : LAYOUT_FONT_REGULAR10)));
    _crcLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.crc.font < LAYOUT_FONT_COUNT ? layout.crc.font : LAYOUT_FONT_REGULAR10)));
    _versionLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(
        layout.version.font < LAYOUT_FONT_COUNT ? layout.version.font : LAYOUT_FONT_REGULAR10)));

    _gameTitleLabel.SetForegroundColor({ layout.gameTitle.colorR, layout.gameTitle.colorG, layout.gameTitle.colorB });
    _prefixLabel.SetForegroundColor({ layout.prefix.colorR, layout.prefix.colorG, layout.prefix.colorB });
    _titleIdLabel.SetForegroundColor({ layout.TitleID.colorR, layout.TitleID.colorG, layout.TitleID.colorB });
    _titleIdTagLabel.SetForegroundColor({
        layout.TitleID.labelColorR, layout.TitleID.labelColorG, layout.TitleID.labelColorB });
    _regionLabel.SetForegroundColor({ layout.region.colorR, layout.region.colorG, layout.region.colorB });
    _crcLabel.SetForegroundColor({ layout.crc.colorR, layout.crc.colorG, layout.crc.colorB });
    _versionLabel.SetForegroundColor({ layout.version.colorR, layout.version.colorG, layout.version.colorB });

    const bool showGameTitle = layout.gameTitle.visible && _selectedRomMetadata.hasGameTitle;
    const bool showPrefix = layout.prefix.visible && prefixText[0] != 0;
    const bool showTitleId = layout.TitleID.visible && _selectedRomMetadata.hasTitleId;
    const bool showTitleIdTag = showTitleId && layout.TitleID.showLabelText;
    const bool showRegion = layout.region.visible && _selectedRomMetadata.hasRegion;
    const bool showCrc = layout.crc.visible && _selectedRomMetadata.hasCrc;
    const bool showVersion = layout.version.visible && _selectedRomMetadata.hasVersion;

    _gameTitleLabel.SetPosition(showGameTitle ? layout.gameTitle.x : -320,
        showGameTitle ? layout.gameTitle.y : -320);
    _prefixLabel.SetPosition(showPrefix ? layout.prefix.x : -320,
        showPrefix ? layout.prefix.y : -320);
    _titleIdLabel.SetPosition(showTitleId ? layout.TitleID.x : -320,
        showTitleId ? layout.TitleID.y : -320);
    _titleIdTagLabel.SetPosition(showTitleIdTag ? layout.TitleID.labelX : -320,
        showTitleIdTag ? layout.TitleID.labelY : -320);
    _regionLabel.SetPosition(showRegion ? layout.region.x : -320,
        showRegion ? layout.region.y : -320);
    _crcLabel.SetPosition(showCrc ? layout.crc.x : -320,
        showCrc ? layout.crc.y : -320);
    _versionLabel.SetPosition(showVersion ? layout.version.x : -320,
        showVersion ? layout.version.y : -320);
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    _dateTime1Label.InitVram(vramContext);
    _dateTime2Label.InitVram(vramContext);
    _usernameLabel.InitVram(vramContext);
    _gameTitleLabel.InitVram(vramContext);
    _prefixLabel.InitVram(vramContext);
    _titleIdLabel.InitVram(vramContext);
    _titleIdTagLabel.InitVram(vramContext);
    _regionLabel.InitVram(vramContext);
    _crcLabel.InitVram(vramContext);
    _versionLabel.InitVram(vramContext);

    int tileIndex = 0;
    vu16* mapPtr = (vu16*)((u8*)GFX_BG_SUB + 0x3800);
    for (int y = 0; y < 12; y++)
    {
        for (int x = 0; x < 14; x++)
        {
            *mapPtr++ = tileIndex;
            tileIndex++;
        }
        mapPtr += 2;
    }
}

void RomBrowserTopScreenView::UpdateDateTimeLabels(bool forceUpdate)
{
    const auto& layout = _layoutService->GetCurrentLayout();

    char16_t buf[28];

    bool dt1Changed = forceUpdate
        || _lastDt1Format != layout.dateTime1.format
        || _lastDt1Sep    != layout.dateTime1.separator;

    if (dt1Changed)
    {
        FormatLayoutDateTime(buf, sizeof(buf)/sizeof(char16_t),
            _lastYear, _lastMonth, _lastMonthDay, _lastHour, _lastMinute, _lastSecond,
            layout.dateTime1.format, layout.dateTime1.separator);
        _dateTime1Label.SetText(buf);
        _lastDt1Format = layout.dateTime1.format;
        _lastDt1Sep    = layout.dateTime1.separator;
    }

    bool dt2Changed = forceUpdate
        || _lastDt2Format != layout.dateTime2.format
        || _lastDt2Sep    != layout.dateTime2.separator;

    if (dt2Changed)
    {
        FormatLayoutDateTime(buf, sizeof(buf)/sizeof(char16_t),
            _lastYear, _lastMonth, _lastMonthDay, _lastHour, _lastMinute, _lastSecond,
            layout.dateTime2.format, layout.dateTime2.separator);
        _dateTime2Label.SetText(buf);
        _lastDt2Format = layout.dateTime2.format;
        _lastDt2Sep    = layout.dateTime2.separator;
    }

    _dateTime1Label.SetPosition(layout.dateTime1.visible ? layout.dateTime1.x : -320,
                                layout.dateTime1.visible ? layout.dateTime1.y : -320);
    _dateTime2Label.SetPosition(layout.dateTime2.visible ? layout.dateTime2.x : -320,
                                layout.dateTime2.visible ? layout.dateTime2.y : -320);
    _usernameLabel.SetPosition(layout.username.visible ? layout.username.x : -320,
                               layout.username.visible ? layout.username.y : -320);
}

void RomBrowserTopScreenView::Update()
{
    const auto& layout = _layoutService->GetCurrentLayout();

    UpdateLayoutFonts();

    BannerView::LayoutConfig fileInfoLayout;
    fileInfoLayout.iconVisible = layout.icon.visible != 0;
    fileInfoLayout.iconX = layout.icon.x;
    fileInfoLayout.iconY = layout.icon.y;
    fileInfoLayout.romNameRow1Visible = layout.romNameRow1.visible != 0;
    fileInfoLayout.romNameRow1X = layout.romNameRow1.x;
    fileInfoLayout.romNameRow1Y = layout.romNameRow1.y;
    fileInfoLayout.romNameRow1Font = layout.romNameRow1.font;
    fileInfoLayout.romNameRow2Visible = layout.romNameRow2.visible != 0;
    fileInfoLayout.romNameRow2X = layout.romNameRow2.x;
    fileInfoLayout.romNameRow2Y = layout.romNameRow2.y;
    fileInfoLayout.romNameRow2Font = layout.romNameRow2.font;
    fileInfoLayout.romNameRow3Visible = layout.romNameRow3.visible != 0;
    fileInfoLayout.romNameRow3X = layout.romNameRow3.x;
    fileInfoLayout.romNameRow3Y = layout.romNameRow3.y;
    fileInfoLayout.romNameRow3Font = layout.romNameRow3.font;
    fileInfoLayout.romNameRow1ColorR = layout.romNameRow1.colorR;
    fileInfoLayout.romNameRow1ColorG = layout.romNameRow1.colorG;
    fileInfoLayout.romNameRow1ColorB = layout.romNameRow1.colorB;
    fileInfoLayout.romNameRow2ColorR = layout.romNameRow2.colorR;
    fileInfoLayout.romNameRow2ColorG = layout.romNameRow2.colorG;
    fileInfoLayout.romNameRow2ColorB = layout.romNameRow2.colorB;
    fileInfoLayout.romNameRow3ColorR = layout.romNameRow3.colorR;
    fileInfoLayout.romNameRow3ColorG = layout.romNameRow3.colorG;
    fileInfoLayout.romNameRow3ColorB = layout.romNameRow3.colorB;
    fileInfoLayout.fileNameVisible = layout.fileName.visible != 0;
    fileInfoLayout.fileNameX = layout.fileName.x;
    fileInfoLayout.fileNameY = layout.fileName.y;
    fileInfoLayout.fileNameFont = layout.fileName.font;
    fileInfoLayout.fileNameColorR = layout.fileName.colorR;
    fileInfoLayout.fileNameColorG = layout.fileName.colorG;
    fileInfoLayout.fileNameColorB = layout.fileName.colorB;
    fileInfoLayout.fileNameScrollEnabled = layout.fileName.scroll != 0;
    fileInfoLayout.fileNameScrollSpeed = layout.fileName.scrollSpeed;
    _fileInfoView->SetLayoutConfig(fileInfoLayout);

    _lastIconVisible = fileInfoLayout.iconVisible;

    u64 tick = gTickCounter.GetValue();
    u32 elapsedMs = TickCounter::TicksToMilliSeconds((u32)(tick - _lastTimeUpdateTick));
    if (_lastTimeUpdateTick == 0 || elapsedMs >= 1000)
    {
        rtc_datetime_t dateTime;
        rtc_readDateTime(&dateTime);

        u8 year     = bcdToDecimal(dateTime.date.year);
        u8 month    = bcdToDecimal(dateTime.date.month);
        u8 monthDay = bcdToDecimal(dateTime.date.monthDay);
        u8 hour     = bcdToDecimal(dateTime.time.hour);
        u8 minute   = bcdToDecimal(dateTime.time.minute);
        u8 second   = bcdToDecimal(dateTime.time.second);
        sanitizeDateTime(month, monthDay, hour, minute, second);

        bool timeChanged = (year != _lastYear || month != _lastMonth
            || monthDay != _lastMonthDay || hour != _lastHour
            || minute != _lastMinute || second != _lastSecond);

        if (timeChanged)
        {
            _lastYear = year; _lastMonth = month; _lastMonthDay = monthDay;
            _lastHour = hour; _lastMinute = minute; _lastSecond = second;
        }

        UpdateDateTimeLabels(timeChanged);

        _lastTimeUpdateTick = tick;
    }

    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem)
    {
        auto& fileInfoManager = _viewModel->GetFileInfoManager();
        _lastSelectedItem = selectedItem;

        if (selectedItem < 0 || selectedItem >= (int)fileInfoManager.GetItemCount())
        {
            _selectedInternalFileInfo.reset();
            _selectedFileIcon.reset();
            _selectedFileCover.Reset();
            _coverGraphicsUploaded = false;
            _fileInfoView->SetIcon(nullptr);
            _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), u"");
            _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), "", false);
            RefreshSelectedRomMetadata(nullptr);
        }
        else
        {
            const auto& item = fileInfoManager.GetItem(selectedItem);
            _selectedInternalFileInfo.reset(item.CreateInternalFileInfo());
            const InternalFileInfo* internalFileInfo = _selectedInternalFileInfo.get();

            _selectedFileIcon = internalFileInfo ? internalFileInfo->CreateGameIcon() : nullptr;
            if (!_selectedFileIcon)
            {
                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
            }
            if (_selectedFileIcon)
                _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
            _fileInfoView->SetIcon(std::move(_selectedFileIcon));

            bool fileNameAsTitle = true;
            if (internalFileInfo)
            {
                const char16_t* gameTitle = internalFileInfo->GetGameTitle();
                if (gameTitle)
                {
                    _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                    fileNameAsTitle = false;
                }
            }
            _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), fileNameAsTitle);
            RefreshSelectedRomMetadata(internalFileInfo);

            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;
            }
        }
    }

    UpdateDateTimeLabels(false);
    UpdateStaticLabels();
    UpdateLabelBackgrounds();

    ViewContainer::Update();
}

void RomBrowserTopScreenView::Draw(GraphicsContext& graphicsContext)
{
    ViewContainer::Draw(graphicsContext);

    _dateTime1Label.Draw(graphicsContext);
    _dateTime2Label.Draw(graphicsContext);
    _usernameLabel.Draw(graphicsContext);
    _gameTitleLabel.Draw(graphicsContext);
    _prefixLabel.Draw(graphicsContext);
    _titleIdLabel.Draw(graphicsContext);
    _titleIdTagLabel.Draw(graphicsContext);
    _regionLabel.Draw(graphicsContext);
    _crcLabel.Draw(graphicsContext);
    _versionLabel.Draw(graphicsContext);
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    _dateTime1Label.VBlank();
    _dateTime2Label.VBlank();
    _usernameLabel.VBlank();
    _gameTitleLabel.VBlank();
    _prefixLabel.VBlank();
    _titleIdLabel.VBlank();
    _titleIdTagLabel.VBlank();
    _regionLabel.VBlank();
    _crcLabel.VBlank();
    _versionLabel.VBlank();

    const auto& layout = _layoutService->GetCurrentLayout();

    bool showCoverEffective = _showCover && layout.boxArt.visible;

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (showCoverEffective && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
        }
        _coverGraphicsUploaded = true;
    }
    if (!showCoverEffective || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover())
    {
        REG_DISPCNT_SUB &= ~(((1 << 3) | (1 << 5)) << 8);
    }
    else
    {
        REG_BG3PA_SUB = 0x100;
        REG_BG3PB_SUB = 0;
        REG_BG3PC_SUB = 0;
        REG_BG3PD_SUB = -0x100;
        REG_BG3X_SUB = -layout.boxArt.x << 8;
        REG_BG3Y_SUB = (95 + layout.boxArt.y) << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(layout.boxArt.x, layout.boxArt.y,
            layout.boxArt.x + 105, layout.boxArt.y + 95);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    _fileInfoView->UploadIconGraphics();
}

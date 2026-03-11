#include "common.h"
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
#include "../viewModels/RomBrowserViewModel.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "rtcIpc.h"
#include "services/launchStats/LaunchStatsService.h"
#include "../FileType/Nds/NdsInternalFileInfo.h"
#include "fat/File.h"
#include "fat/ff.h"
#include "RomBrowserTopScreenView.h"

#define CRCPOLY 0xEDB88320

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

static bool BuildStatsPath(char* outPath, u32 outSize, const FileInfo& fileInfo)
{
    const TCHAR* fullPath = fileInfo.GetFullPath();
    if (fullPath && fullPath[0] != 0) {
        mini_snprintf(outPath, outSize, "%s", fullPath);
    } else {
        if (f_getcwd(outPath, outSize) == FR_OK) {
            int idx = strlcat(outPath, "/", outSize);
            if (idx > 1 && outPath[idx - 2] == '/')
                outPath[idx - 1] = 0;
            strlcat(outPath, fileInfo.GetFileName(), outSize);
        } else {
            outPath[0] = 0;
            return false;
        }
    }
    const char* normalizedPath = strchr(outPath, ':');
    if (normalizedPath && normalizedPath != outPath) {
        size_t len = strlen(normalizedPath);
        memmove(outPath, normalizedPath, len + 1);
    }
    return outPath[0] != 0;
}

static void CopyCStringTrunc(char* dst, u32 dstSize, const char* src)
{
    if (!dst || dstSize == 0)
        return;
    mini_snprintf(dst, dstSize, "%s", (src != nullptr) ? src : "");
}

static bool IsTwlUnitCode(u8 unitCode)
{
    return unitCode == 0x02 || unitCode == 0x03;
}

static void BuildNdsId(char* out, u32 outSize, u8 unitCode, const char* gameCode)
{
    const char* platform = IsTwlUnitCode(unitCode) ? "TWL" : "NTR";
    const char* region = "UNK";
    if (gameCode[3]) {
        switch (gameCode[3]) {
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
            default:  region = "UNK"; break;
        }
    }
    mini_snprintf(out, outSize, "%s-%c%c%c%c-%s",
        platform, gameCode[0], gameCode[1], gameCode[2], gameCode[3], region);
}

static void BuildGbaId(char* out, u32 outSize, const char* gameCode)
{
    mini_snprintf(out, outSize, "AGB-%c%c%c%c",
        gameCode[0], gameCode[1], gameCode[2], gameCode[3]);
}

static void FormatPrefixDisplay(char* out, u32 outSize, const char* prefix,
    u8 gbaMode, u8 ntrMode, u8 twlMode)
{
    if (!out || outSize == 0)
        return;
    out[0] = 0;
    if (!prefix || prefix[0] == 0)
        return;

    if (!strcasecmp(prefix, "AGB"))
    {
        switch (gbaMode % LAYOUT_PREFIX_GBA_COUNT)
        {
            case LAYOUT_PREFIX_GBA_GBA: mini_snprintf(out, outSize, "GBA"); return;
            case LAYOUT_PREFIX_GBA_NATIVE:
            default: mini_snprintf(out, outSize, "AGB"); return;
        }
    }

    if (!strcasecmp(prefix, "NTR"))
    {
        switch (ntrMode % LAYOUT_PREFIX_NTR_COUNT)
        {
            case LAYOUT_PREFIX_NTR_DS: mini_snprintf(out, outSize, "DS"); return;
            case LAYOUT_PREFIX_NTR_NATIVE_PLUS: mini_snprintf(out, outSize, "NTR DS"); return;
            case LAYOUT_PREFIX_NTR_NATIVE:
            default: mini_snprintf(out, outSize, "NTR"); return;
        }
    }

    if (!strcasecmp(prefix, "TWL"))
    {
        switch (twlMode % LAYOUT_PREFIX_TWL_COUNT)
        {
            case LAYOUT_PREFIX_TWL_DSI_ENHANCED: mini_snprintf(out, outSize, "DSi Enhanced"); return;
            case LAYOUT_PREFIX_TWL_DSI: mini_snprintf(out, outSize, "DSi"); return;
            case LAYOUT_PREFIX_TWL_NATIVE_DSI: mini_snprintf(out, outSize, "TWL DSi"); return;
            case LAYOUT_PREFIX_TWL_NATIVE_ENH: mini_snprintf(out, outSize, "TWL DSi Enhanced"); return;
            case LAYOUT_PREFIX_TWL_NATIVE:
            default: mini_snprintf(out, outSize, "TWL"); return;
        }
    }

    mini_snprintf(out, outSize, "%s", prefix);
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

struct IdParts
{
    char prefix[24];
    char gameId[5];
    char region[4];
    bool hasPrefix;
    bool hasGameId;
    bool hasRegion;
};

static void ParseIdParts(const char* fullId, IdParts& parts)
{
    memset(&parts, 0, sizeof(parts));

    if (!fullId || fullId[0] == 0)
        return;

    const char* dash1 = strchr(fullId, '-');
    if (!dash1)
    {
        CopyCStringTrunc(parts.gameId, sizeof(parts.gameId), fullId);
        parts.hasGameId = parts.gameId[0] != 0;
        return;
    }

    int prefixLen = (int)(dash1 - fullId);
    if (prefixLen > (int)(sizeof(parts.prefix) - 1))
        prefixLen = (int)(sizeof(parts.prefix) - 1);
    if (prefixLen > 0)
    {
        memcpy(parts.prefix, fullId, prefixLen);
        parts.prefix[prefixLen] = 0;
        parts.hasPrefix = true;
    }

    const char* gameIdStart = dash1 + 1;
    const char* dash2 = strchr(gameIdStart, '-');

    if (dash2)
    {
        int gameIdLen = (int)(dash2 - gameIdStart);
        if (gameIdLen > 4)
            gameIdLen = 4;
        if (gameIdLen > 0)
        {
            memcpy(parts.gameId, gameIdStart, gameIdLen);
            parts.hasGameId = true;
        }

        CopyCStringTrunc(parts.region, sizeof(parts.region), dash2 + 1);
        parts.hasRegion = parts.region[0] != 0;
    }
    else
    {
        CopyCStringTrunc(parts.gameId, sizeof(parts.gameId), gameIdStart);
        parts.hasGameId = parts.gameId[0] != 0;
    }
}

static void FormatIdPartText(char* out, u32 outSize, const char* value, bool trailingDash)
{
    if (!out || outSize == 0)
        return;

    out[0] = 0;
    if (!value || value[0] == 0)
        return;

    if (trailingDash)
        mini_snprintf(out, outSize, "%s -", value);
    else
        mini_snprintf(out, outSize, "%s", value);
}

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
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _dateTime1Label(128, 16, 26,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().dateTime1.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().dateTime1.font : LAYOUT_FONT_REGULAR10)))
    , _dateTime2Label(128, 16, 26,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().dateTime2.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().dateTime2.font : LAYOUT_FONT_REGULAR10)))
    , _prefixLabel(128, 16, 24,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().prefix.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().prefix.font : LAYOUT_FONT_REGULAR10)))
    , _gameIdTagLabel(32, 16, 8,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().gameId.labelFont < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().gameId.labelFont : LAYOUT_FONT_REGULAR10)))
    , _gameIdLabel(128, 16, 8,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().gameId.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().gameId.font : LAYOUT_FONT_REGULAR10)))
    , _regionLabel(64, 16, 8,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().region.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().region.font : LAYOUT_FONT_REGULAR10)))
    , _versionLabel(64, 16, 8,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().version.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().version.font : LAYOUT_FONT_REGULAR10)))
    , _crcLabel(64, 16, 8,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().crc.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().crc.font : LAYOUT_FONT_REGULAR10)))
    , _usernameLabel(128, 16, 20,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().username.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().username.font : LAYOUT_FONT_REGULAR10)))
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _bgmService(bgmService)
    , _fontRepository(fontRepository)
    , _layoutService(layoutService)
{
    const auto& layout = layoutService->GetCurrentLayout();

    _dateTime1Label.SetForegroundColor({ 255, 255, 255 });
    _dateTime1Label.SetBackgroundColor({ 0, 0, 0 });
    _dateTime2Label.SetForegroundColor({ 255, 255, 255 });
    _dateTime2Label.SetBackgroundColor({ 0, 0, 0 });
    _prefixLabel.SetForegroundColor({ 255, 255, 255 });
    _prefixLabel.SetBackgroundColor({ 0, 0, 0 });
    _gameIdTagLabel.SetForegroundColor({ 255, 255, 255 });
    _gameIdTagLabel.SetBackgroundColor({ 0, 0, 0 });
    _gameIdLabel.SetForegroundColor({ 255, 255, 255 });
    _gameIdLabel.SetBackgroundColor({ 0, 0, 0 });
    _regionLabel.SetForegroundColor({ 255, 255, 255 });
    _regionLabel.SetBackgroundColor({ 0, 0, 0 });
    _versionLabel.SetForegroundColor({ 255, 255, 255 });
    _versionLabel.SetBackgroundColor({ 0, 0, 0 });
    _crcLabel.SetForegroundColor({ 255, 255, 255 });
    _crcLabel.SetBackgroundColor({ 0, 0, 0 });
    _usernameLabel.SetForegroundColor({ 255, 255, 255 });
    _usernameLabel.SetBackgroundColor({ 0, 0, 0 });

    _dateTime1Label.SetPosition(layout.dateTime1.x, layout.dateTime1.y);
    _dateTime2Label.SetPosition(layout.dateTime2.x, layout.dateTime2.y);
    _prefixLabel.SetPosition(layout.prefix.x, layout.prefix.y);
    _gameIdTagLabel.SetPosition(layout.gameId.x, layout.gameId.y);
    _gameIdLabel.SetPosition(layout.gameId.x, layout.gameId.y);
    _regionLabel.SetPosition(layout.region.x, layout.region.y);
    _versionLabel.SetPosition(layout.version.x, layout.version.y);
    _crcLabel.SetPosition(layout.crc.x, layout.crc.y);
    _usernameLabel.SetPosition(layout.username.x, layout.username.y);

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

    _lastPrefixFont = layout.prefix.font;
    _lastGameIdTagFont = layout.gameId.labelFont;
    _lastGameIdFont = layout.gameId.font;
    _lastRegionFont = layout.region.font;
    _lastVersionFont = layout.version.font;
    _lastCrcFont  = layout.crc.font;
    _lastUsernameFont = layout.username.font;

    _prefixLabel.SetText("");
    _gameIdLabel.SetText("");
    _regionLabel.SetText("");
    _versionLabel.SetText("");
    _crcLabel.SetText("");
    CopyUserNameFromFirmware(_cachedUserName, sizeof(_cachedUserName) / sizeof(_cachedUserName[0]));
    _usernameLabel.SetText(_cachedUserName);

    AddChildTail(_fileInfoView.get());
}

void RomBrowserTopScreenView::UpdateLayoutFonts()
{
    const auto& layout = _layoutService->GetCurrentLayout();

    u8 dt1Font = layout.dateTime1.font < LAYOUT_FONT_COUNT ? layout.dateTime1.font : LAYOUT_FONT_REGULAR10;
    u8 dt2Font = layout.dateTime2.font < LAYOUT_FONT_COUNT ? layout.dateTime2.font : LAYOUT_FONT_REGULAR10;
    u8 prefixFont = layout.prefix.font < LAYOUT_FONT_COUNT ? layout.prefix.font : LAYOUT_FONT_REGULAR10;
    u8 gameIdTagFont = layout.gameId.labelFont < LAYOUT_FONT_COUNT ? layout.gameId.labelFont : LAYOUT_FONT_REGULAR10;
    u8 gameIdFont = layout.gameId.font < LAYOUT_FONT_COUNT ? layout.gameId.font : LAYOUT_FONT_REGULAR10;
    u8 regionFont = layout.region.font < LAYOUT_FONT_COUNT ? layout.region.font : LAYOUT_FONT_REGULAR10;
    u8 versionFont = layout.version.font < LAYOUT_FONT_COUNT ? layout.version.font : LAYOUT_FONT_REGULAR10;
    u8 crcFont = layout.crc.font < LAYOUT_FONT_COUNT ? layout.crc.font : LAYOUT_FONT_REGULAR10;
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

    if (prefixFont != _lastPrefixFont)
    {
        _prefixLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(prefixFont)));
        _lastPrefixFont = prefixFont;
    }

    if (gameIdTagFont != _lastGameIdTagFont)
    {
        _gameIdTagLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(gameIdTagFont)));
        _lastGameIdTagFont = gameIdTagFont;
    }

    if (gameIdFont != _lastGameIdFont)
    {
        _gameIdLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(gameIdFont)));
        _lastGameIdFont = gameIdFont;
    }

    if (regionFont != _lastRegionFont)
    {
        _regionLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(regionFont)));
        _lastRegionFont = regionFont;
    }

    if (versionFont != _lastVersionFont)
    {
        _versionLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(versionFont)));
        _lastVersionFont = versionFont;
    }

    if (crcFont != _lastCrcFont)
    {
        _crcLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(crcFont)));
        _lastCrcFont = crcFont;
    }

    if (usernameFont != _lastUsernameFont)
    {
        _usernameLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(usernameFont)));
        _lastUsernameFont = usernameFont;
    }
}

void RomBrowserTopScreenView::UpdateIdAndVersionLabels()
{
    const auto& layout = _layoutService->GetCurrentLayout();

    _dateTime1Label.SetForegroundColor(Rgb<8, 8, 8>(layout.dateTime1.colorR, layout.dateTime1.colorG, layout.dateTime1.colorB));
    _dateTime2Label.SetForegroundColor(Rgb<8, 8, 8>(layout.dateTime2.colorR, layout.dateTime2.colorG, layout.dateTime2.colorB));
    _prefixLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.prefix.colorR, layout.prefix.colorG, layout.prefix.colorB));
    _gameIdLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.gameId.colorR, layout.gameId.colorG, layout.gameId.colorB));
    _gameIdTagLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.gameId.labelColorR, layout.gameId.labelColorG, layout.gameId.labelColorB));
    _regionLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.region.colorR, layout.region.colorG, layout.region.colorB));
    _versionLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.version.colorR, layout.version.colorG, layout.version.colorB));
    _crcLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.crc.colorR, layout.crc.colorG, layout.crc.colorB));
    _usernameLabel.SetForegroundColor(Rgb<8, 8, 8>(layout.username.colorR, layout.username.colorG, layout.username.colorB));

    if (_hasCachedIdPrefix || _hasCachedIdGameId || _hasCachedIdRegion)
    {
        IdParts parts;
        memset(&parts, 0, sizeof(parts));

        if (_hasCachedIdPrefix)
        {
            CopyCStringTrunc(parts.prefix, sizeof(parts.prefix), _cachedIdPrefix);
            parts.hasPrefix = true;
        }
        if (_hasCachedIdGameId)
        {
            CopyCStringTrunc(parts.gameId, sizeof(parts.gameId), _cachedIdGameId);
            parts.hasGameId = true;
        }
        if (_hasCachedIdRegion)
        {
            CopyCStringTrunc(parts.region, sizeof(parts.region), _cachedIdRegion);
            parts.hasRegion = true;
        }

        char prefixBuf[48] = {0};
        char gameIdBuf[32] = {0};
        char regionBuf[32] = {0};

        char nativePrefix[8] = {0};
        if (parts.hasPrefix)
            mini_snprintf(nativePrefix, sizeof(nativePrefix), "%s", parts.prefix);
        char mappedPrefix[32] = {0};
        FormatPrefixDisplay(mappedPrefix, sizeof(mappedPrefix), nativePrefix,
            layout.prefix.gbaPrefixMode, layout.prefix.ntrPrefixMode, layout.prefix.twlPrefixMode);
        mini_snprintf(parts.prefix, sizeof(parts.prefix), "%s", mappedPrefix);
        parts.hasPrefix = parts.prefix[0] != 0;

        FormatIdPartText(prefixBuf, sizeof(prefixBuf), parts.prefix, layout.prefix.trailingDash != 0);
        FormatIdPartText(gameIdBuf, sizeof(gameIdBuf), parts.gameId, layout.gameId.trailingDash != 0);
        FormatIdPartText(regionBuf, sizeof(regionBuf), parts.region, layout.region.trailingDash != 0);

        _prefixLabel.SetText(prefixBuf);
        _gameIdLabel.SetText(gameIdBuf);
        _regionLabel.SetText(regionBuf);

        if (layout.gameId.showLabelText)
        {
            _gameIdTagLabel.SetText("TID:");
            _gameIdTagLabel.SetPosition(layout.gameId.visible ? layout.gameId.labelX : -320,
                layout.gameId.visible ? layout.gameId.labelY : -320);
            _gameIdLabel.SetPosition(layout.gameId.visible ? layout.gameId.x : -320,
                layout.gameId.visible ? layout.gameId.y : -320);
        }
        else
        {
            _gameIdTagLabel.SetText("");
            _gameIdLabel.SetPosition(layout.gameId.visible ? layout.gameId.x : -320,
                layout.gameId.visible ? layout.gameId.y : -320);
        }
    }
    else
    {
        _prefixLabel.SetText("");
        _gameIdTagLabel.SetText("");
        _gameIdLabel.SetText("");
        _regionLabel.SetText("");
    }

    if (_hasCachedRomVersion)
    {
        char versionBuf[12];
        mini_snprintf(versionBuf, sizeof(versionBuf), "v%u", (unsigned)_cachedRomVersion);
        _versionLabel.SetText(versionBuf);
    }
    else
    {
        _versionLabel.SetText("");
    }
}

void RomBrowserTopScreenView::UpdateStaticLabels()
{
    const auto& layout = _layoutService->GetCurrentLayout();
    _usernameLabel.SetText(_cachedUserName);
    _usernameLabel.SetPosition(layout.username.visible ? layout.username.x : -320,
        layout.username.visible ? layout.username.y : -320);
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    _dateTime1Label.InitVram(vramContext);
    _dateTime2Label.InitVram(vramContext);
    _prefixLabel.InitVram(vramContext);
    _gameIdTagLabel.InitVram(vramContext);
    _gameIdLabel.InitVram(vramContext);
    _regionLabel.InitVram(vramContext);
    _versionLabel.InitVram(vramContext);
    _crcLabel.InitVram(vramContext);
    _usernameLabel.InitVram(vramContext);

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
    _prefixLabel.SetPosition(layout.prefix.visible ? layout.prefix.x : -320,
                             layout.prefix.visible ? layout.prefix.y : -320);
    _gameIdTagLabel.SetPosition(-320, -320);
    _gameIdLabel.SetPosition(layout.gameId.visible ? layout.gameId.x : -320,
                             layout.gameId.visible ? layout.gameId.y : -320);
    _regionLabel.SetPosition(layout.region.visible ? layout.region.x : -320,
                             layout.region.visible ? layout.region.y : -320);
    _versionLabel.SetPosition(layout.version.visible ? layout.version.x : -320,
                              layout.version.visible ? layout.version.y : -320);
    _crcLabel.SetPosition(layout.crc.visible ? layout.crc.x : -320,
                           layout.crc.visible ? layout.crc.y : -320);
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

    if (fileInfoLayout.iconVisible && !_lastIconVisible)
        _iconGraphicsUploaded = false;
    _lastIconVisible = fileInfoLayout.iconVisible;

    UpdateDateTimeLabels(false);
    UpdateIdAndVersionLabels();
    UpdateStaticLabels();

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
        const auto& item = fileInfoManager.GetItem(selectedItem);

        memset(_cachedIdPrefix, 0, sizeof(_cachedIdPrefix));
        memset(_cachedIdGameId, 0, sizeof(_cachedIdGameId));
        memset(_cachedIdRegion, 0, sizeof(_cachedIdRegion));
        _hasCachedIdPrefix = false;
        _hasCachedIdGameId = false;
        _hasCachedIdRegion = false;
        _hasCachedRomVersion = false;
        _cachedRomVersion = 0;

        char statsPath[256];
        bool hasStatsPath = BuildStatsPath(statsPath, sizeof(statsPath), item);

        if (item.GetFileType()->HasInternalFileInfo())
        {
            auto info = fileInfoManager.GetInternalFileInfo(selectedItem);
            if (info)
            {
                bool fileNameAsTitle = true;
                const char16_t* gameTitle = info->GetGameTitle();
                if (gameTitle)
                {
                    _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                    fileNameAsTitle = false;
                }

                // Try to get cached ID and CRC from stats
                char cachedId[16] = {0};
                bool hasCachedId = false;
                u32 cachedCrc = 0;
                bool hasCachedCrc = false;
                u8 cachedRomVersion = 0;
                bool hasCachedRomVersion = false;

                if (hasStatsPath)
                {
                    LaunchStatsService::Instance().TryGetCachedData(statsPath,
                        &cachedRomVersion, &hasCachedRomVersion,
                        cachedId, sizeof(cachedId), &hasCachedId,
                        &cachedCrc, &hasCachedCrc);
                }

                char idStr[16] = {0};
                bool hasId = false;
                u32 headerCrc = 0;
                bool hasHeaderCrc = false;
                u8 romVersion = 0;
                bool hasRomVersion = false;
                bool cacheNeedsRefresh = false;

                if (hasCachedId && cachedId[0] != 0)
                {
                    CopyCStringTrunc(idStr, sizeof(idStr), cachedId);
                    hasId = true;
                }

                if (hasCachedCrc)
                {
                    headerCrc = cachedCrc;
                    hasHeaderCrc = true;
                }

                if (!hasHeaderCrc)
                {
                    File romFile;
                    romFile.Open(item.GetFastFileRef(), FA_READ);
                    if (romFile.GetSize() >= 512)
                    {
                        u8 header[512];
                        if (romFile.ReadExact(header, sizeof(header)))
                        {
                            headerCrc = ComputeCrc32(header, sizeof(header));
                            hasHeaderCrc = true;
                        }
                    }
                }

                if (hasCachedRomVersion)
                {
                    romVersion = cachedRomVersion;
                    hasRomVersion = true;
                }

                const char* shortName = item.GetFileType()->GetShortName();
                if (shortName && !strcasecmp(shortName, "nds"))
                {
                    const char* gameCode = info->GetGameCode();
                    if (gameCode && gameCode[0] != 0)
                    {
                        char canonicalId[16] = {0};
                        auto* ndsInfo = static_cast<const NdsInternalFileInfo*>(info);
                        BuildNdsId(canonicalId, sizeof(canonicalId), ndsInfo->GetUnitCode(), gameCode);
                        mini_snprintf(idStr, sizeof(idStr), "%s", canonicalId);
                        hasId = true;

                        const u8 actualRomVersion = ndsInfo->GetRomVersion();
                        if (!hasCachedId || strcasecmp(cachedId, canonicalId) != 0)
                            cacheNeedsRefresh = true;
                        if (!hasCachedRomVersion || cachedRomVersion != actualRomVersion)
                            cacheNeedsRefresh = true;

                        romVersion = actualRomVersion;
                        hasRomVersion = true;
                    }
                }
                else if (!hasId)
                {
                    const char* gameCode = info->GetGameCode();
                    if (gameCode && gameCode[0] != 0 && shortName && !strcasecmp(shortName, "gba"))
                    {
                        BuildGbaId(idStr, sizeof(idStr), gameCode);
                        hasId = true;
                    }
                }

                // Save to stats if anything was not cached
                if (hasStatsPath && (!hasCachedId || !hasCachedCrc || !hasCachedRomVersion || cacheNeedsRefresh))
                {
                    LaunchStatsService::Instance().SetCachedData(statsPath,
                        romVersion, hasRomVersion,
                        idStr, hasId,
                        headerCrc, hasHeaderCrc);
                }

                _hasCachedRomVersion = hasRomVersion;
                _cachedRomVersion = romVersion;

                // Display PREFIX/GAME ID/REGION labels
                if (hasId)
                {
                    IdParts parts;
                    ParseIdParts(idStr, parts);

                    memset(_cachedIdPrefix, 0, sizeof(_cachedIdPrefix));
                    memset(_cachedIdGameId, 0, sizeof(_cachedIdGameId));
                    memset(_cachedIdRegion, 0, sizeof(_cachedIdRegion));
                    _hasCachedIdPrefix = parts.hasPrefix;
                    _hasCachedIdGameId = parts.hasGameId;
                    _hasCachedIdRegion = parts.hasRegion;

                    if (_hasCachedIdPrefix)
                        CopyCStringTrunc(_cachedIdPrefix, sizeof(_cachedIdPrefix), parts.prefix);
                    if (_hasCachedIdGameId)
                        CopyCStringTrunc(_cachedIdGameId, sizeof(_cachedIdGameId), parts.gameId);
                    if (_hasCachedIdRegion)
                        CopyCStringTrunc(_cachedIdRegion, sizeof(_cachedIdRegion), parts.region);
                }
                else
                {
                    memset(_cachedIdPrefix, 0, sizeof(_cachedIdPrefix));
                    memset(_cachedIdGameId, 0, sizeof(_cachedIdGameId));
                    memset(_cachedIdRegion, 0, sizeof(_cachedIdRegion));
                    _hasCachedIdPrefix = false;
                    _hasCachedIdGameId = false;
                    _hasCachedIdRegion = false;
                }

                // Display CRC
                if (hasHeaderCrc)
                {
                    char crcBuf[12];
                    mini_snprintf(crcBuf, sizeof(crcBuf), "%08X", headerCrc);
                    _crcLabel.SetText(crcBuf);
                }
                else
                {
                    _crcLabel.SetText("");
                }

                _selectedFileIcon = info->CreateGameIcon();
                if (!_selectedFileIcon)
                    _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
                if (_selectedFileIcon)
                {
                    _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                    _iconGraphicsUploaded = false;
                }
                _fileInfoView->SetIcon(std::move(_selectedFileIcon));
                _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), fileNameAsTitle);

                _lastSelectedItem = selectedItem;

                auto cover = fileInfoManager.GetFileCover(selectedItem);
                if (cover.IsValid())
                {
                    _selectedFileCover = std::move(cover);
                    _coverGraphicsUploaded = false;
                }
            }
        }
        else
        {
            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;

                _prefixLabel.SetText("");
                _gameIdLabel.SetText("");
                _regionLabel.SetText("");
                _crcLabel.SetText("");

                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
                if (_selectedFileIcon)
                {
                    _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                    _iconGraphicsUploaded = false;
                }
                _fileInfoView->SetIcon(std::move(_selectedFileIcon));
                _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), true);

                memset(_cachedIdPrefix, 0, sizeof(_cachedIdPrefix));
                memset(_cachedIdGameId, 0, sizeof(_cachedIdGameId));
                memset(_cachedIdRegion, 0, sizeof(_cachedIdRegion));
                _hasCachedIdPrefix = false;
                _hasCachedIdGameId = false;
                _hasCachedIdRegion = false;
                _hasCachedRomVersion = false;
                _cachedRomVersion = 0;

                _lastSelectedItem = selectedItem;
            }
        }
    }

    ViewContainer::Update();
}

void RomBrowserTopScreenView::Draw(GraphicsContext& graphicsContext)
{
    ViewContainer::Draw(graphicsContext);

    _dateTime1Label.Draw(graphicsContext);
    _dateTime2Label.Draw(graphicsContext);
    _prefixLabel.Draw(graphicsContext);
    _gameIdTagLabel.Draw(graphicsContext);
    _gameIdLabel.Draw(graphicsContext);
    _regionLabel.Draw(graphicsContext);
    _versionLabel.Draw(graphicsContext);
    _crcLabel.Draw(graphicsContext);
    _usernameLabel.Draw(graphicsContext);
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    _dateTime1Label.VBlank();
    _dateTime2Label.VBlank();
    _prefixLabel.VBlank();
    _gameIdTagLabel.VBlank();
    _gameIdLabel.VBlank();
    _regionLabel.VBlank();
    _versionLabel.VBlank();
    _crcLabel.VBlank();
    _usernameLabel.VBlank();

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
        REG_BG3Y_SUB = (96 + layout.boxArt.y) << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(layout.boxArt.x, layout.boxArt.y,
            layout.boxArt.x + 106, layout.boxArt.y + 96);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}

#include "common.h"
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
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
    , _dateTime1Label(160, 16, 26,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().dateTime1.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().dateTime1.font : LAYOUT_FONT_REGULAR10)))
    , _dateTime2Label(160, 16, 26,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().dateTime2.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().dateTime2.font : LAYOUT_FONT_REGULAR10)))
    , _romIdCodeLabel(64, 16, 8,
        fontRepository->GetFont(static_cast<FontType>(
            layoutService->GetCurrentLayout().romIdCode.font < LAYOUT_FONT_COUNT
                ? layoutService->GetCurrentLayout().romIdCode.font : LAYOUT_FONT_REGULAR10)))
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
    _romIdCodeLabel.SetForegroundColor({ 255, 255, 255 });
    _romIdCodeLabel.SetBackgroundColor({ 0, 0, 0 });

    _dateTime1Label.SetPosition(layout.dateTime1.x, layout.dateTime1.y);
    _dateTime2Label.SetPosition(layout.dateTime2.x, layout.dateTime2.y);
    _romIdCodeLabel.SetPosition(layout.romIdCode.x, layout.romIdCode.y);

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

    _lastIdFont    = layout.romIdCode.font;

    _romIdCodeLabel.SetText("");

    AddChildTail(_fileInfoView.get());
}

void RomBrowserTopScreenView::UpdateLayoutFonts()
{
    const auto& layout = _layoutService->GetCurrentLayout();

    u8 dt1Font = layout.dateTime1.font < LAYOUT_FONT_COUNT ? layout.dateTime1.font : LAYOUT_FONT_REGULAR10;
    u8 dt2Font = layout.dateTime2.font < LAYOUT_FONT_COUNT ? layout.dateTime2.font : LAYOUT_FONT_REGULAR10;
    u8 idFont = layout.romIdCode.font < LAYOUT_FONT_COUNT ? layout.romIdCode.font : LAYOUT_FONT_REGULAR10;

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

    if (idFont != _lastIdFont)
    {
        _romIdCodeLabel.SetFont(_fontRepository->GetFont(static_cast<FontType>(idFont)));
        _lastIdFont = idFont;
    }
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    _dateTime1Label.InitVram(vramContext);
    _dateTime2Label.InitVram(vramContext);
    _romIdCodeLabel.InitVram(vramContext);

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
    _romIdCodeLabel.SetPosition(layout.romIdCode.visible ? layout.romIdCode.x : -320,
                                layout.romIdCode.visible ? layout.romIdCode.y : -320);
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
    fileInfoLayout.fileNameVisible = layout.fileName.visible != 0;
    fileInfoLayout.fileNameX = layout.fileName.x;
    fileInfoLayout.fileNameY = layout.fileName.y;
    fileInfoLayout.fileNameFont = layout.fileName.font;
    fileInfoLayout.fileNameScrollEnabled = layout.fileName.scroll != 0;
    fileInfoLayout.fileNameScrollSpeed = layout.fileName.scrollSpeed;
    _fileInfoView->SetLayoutConfig(fileInfoLayout);

    if (fileInfoLayout.iconVisible && !_lastIconVisible)
        _iconGraphicsUploaded = false;
    _lastIconVisible = fileInfoLayout.iconVisible;

    UpdateDateTimeLabels(false);

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

                {
                    const char* gameCode = info->GetGameCode();
                    _romIdCodeLabel.SetText(gameCode ? gameCode : "");
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

                _romIdCodeLabel.SetText("");

                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
                if (_selectedFileIcon)
                {
                    _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                    _iconGraphicsUploaded = false;
                }
                _fileInfoView->SetIcon(std::move(_selectedFileIcon));
                _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), true);

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
    _romIdCodeLabel.Draw(graphicsContext);
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    _dateTime1Label.VBlank();
    _dateTime2Label.VBlank();
    _romIdCodeLabel.VBlank();

    const auto& layout = _layoutService->GetCurrentLayout();

    bool showCoverEffective = _showCover && layout.boxArt.visible;

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (showCoverEffective && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            GFX_PLTT_BG_SUB[0] = *(vu16*)0x0689E000;
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

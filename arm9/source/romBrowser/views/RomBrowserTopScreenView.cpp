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

static void sanitizeDateTime(u8& month, u8& monthDay, u8& hour, u8& minute)
{
    if (month < 1 || month > 12)
        month = 1;
    if (monthDay < 1 || monthDay > 31)
        monthDay = 1;
    if (hour > 23)
        hour = 0;
    if (minute > 59)
        minute = 0;
}

static void formatDateTimeText(char16_t* outText, u32 outTextLength, u8 year, u8 month, u8 monthDay, u8 hour, u8 minute)
{
    char dateTimeText[24];
    mini_snprintf(dateTimeText, sizeof(dateTimeText), "%02u/%02u/%04u %02u:%02u",
        monthDay, month, 2000 + year, hour, minute);
    StringUtil::Copy(outText, dateTimeText, outTextLength);
}

RomBrowserTopScreenView::RomBrowserTopScreenView(
    const SharedPtr<RomBrowserViewModel>& viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository,
    const IBgmService* bgmService)
    : _viewModel(viewModel)
    , _themeFileIconFactory(themeFileIconFactory)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _dateTimeChip(md::sys::color::surfaceContainerHighest, materialColorScheme, fontRepository)
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _bgmService(bgmService)
{
    constexpr int DATE_TIME_CHIP_WIDTH = 90;
    constexpr int DATE_TIME_CHIP_X = 0;
    constexpr int DATE_TIME_CHIP_Y = -3;

    _dateTimeChip.SetCenteredText(true);
    _dateTimeChip.SetFixedWidth(DATE_TIME_CHIP_WIDTH);
    _dateTimeChip.SetPosition(DATE_TIME_CHIP_X, DATE_TIME_CHIP_Y);

    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);

    _lastYear = bcdToDecimal(dateTime.date.year);
    _lastMonth = bcdToDecimal(dateTime.date.month);
    _lastMonthDay = bcdToDecimal(dateTime.date.monthDay);
    _lastHour = bcdToDecimal(dateTime.time.hour);
    _lastMinute = bcdToDecimal(dateTime.time.minute);

    sanitizeDateTime(_lastMonth, _lastMonthDay, _lastHour, _lastMinute);

    char16_t dateTimeText16[24];
    formatDateTimeText(dateTimeText16, sizeof(dateTimeText16) / sizeof(dateTimeText16[0]),
        _lastYear, _lastMonth, _lastMonthDay, _lastHour, _lastMinute);
    _dateTimeChip.SetText(dateTimeText16);

    AddChildTail(_fileInfoView.get());
    AddChildTail(&_dateTimeChip);
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        auto chipGraphics = ChipView::UploadGraphics(*objVramManager);
        _dateTimeChip.SetGraphics(chipGraphics);
    }

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

void RomBrowserTopScreenView::Update()
{
    u64 tick = gTickCounter.GetValue();
    u32 elapsedMs = TickCounter::TicksToMilliSeconds((u32)(tick - _lastTimeUpdateTick));
    if (_lastTimeUpdateTick == 0 || elapsedMs >= 1000)
    {
        rtc_datetime_t dateTime;
        rtc_readDateTime(&dateTime);

        u8 year = bcdToDecimal(dateTime.date.year);
        u8 month = bcdToDecimal(dateTime.date.month);
        u8 monthDay = bcdToDecimal(dateTime.date.monthDay);

        u8 hour = bcdToDecimal(dateTime.time.hour);
        u8 minute = bcdToDecimal(dateTime.time.minute);

        sanitizeDateTime(month, monthDay, hour, minute);

        if (_lastYear != year || _lastMonth != month || _lastMonthDay != monthDay
            || _lastHour != hour || _lastMinute != minute)
        {
            char16_t dateTimeText16[24];
            formatDateTimeText(dateTimeText16, sizeof(dateTimeText16) / sizeof(dateTimeText16[0]),
                year, month, monthDay, hour, minute);
            _dateTimeChip.SetText(dateTimeText16);

            _lastYear = year;
            _lastMonth = month;
            _lastMonthDay = monthDay;

            _lastHour = hour;
            _lastMinute = minute;
        }

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

                _selectedFileIcon = info->CreateGameIcon();
                if (!_selectedFileIcon)
                {
                    _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
                }
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

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (_showCover && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            GFX_PLTT_BG_SUB[0] = *(vu16*)0x0689E000;
            mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
        }
        _coverGraphicsUploaded = true;
    }
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover())
    {
        // hide cover
        REG_DISPCNT_SUB &= ~(((1 << 3) | (1 << 5)) << 8);
    }
    else
    {
        // display cover
        REG_BG3PA_SUB = 0x100;
        REG_BG3PB_SUB = 0;
        REG_BG3PC_SUB = 0;
        REG_BG3PD_SUB = -0x100;
        REG_BG3X_SUB = -75 << 8;
        REG_BG3Y_SUB = 113 << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(75, 18, 75 + 106, 18 + 96);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}

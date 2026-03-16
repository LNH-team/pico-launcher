#include "common.h"
#include <algorithm>
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "core/StringUtil.h"
#include "core/math/RgbMixer.h"
#include "core/math/ColorConverter.h"
#include "iconCell.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "themes/FontType.h"
#include "MaterialFileInfoCardView.h"

#define FILE_NAME_SCROLL_SEPARATOR u"   "
#define FILE_NAME_SCROLL_START_PAUSE_FRAMES 120
#define FILE_NAME_SCROLL_CYCLE_PAUSE_FRAMES 120
#define FILE_NAME_SCROLL_SPEED_Q8_BASE 24

MaterialFileInfoCardView::MaterialFileInfoCardView(const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _firstLine(176, 16, 50, fontRepository->GetFont(FontType::Medium11))
    , _secondLine(156, 16, 50, fontRepository->GetFont(FontType::Regular10))
    , _thirdLine(156, 16, 50, fontRepository->GetFont(FontType::Regular10))
    , _filenameLabelView(220, 16, 200, fontRepository->GetFont(FontType::Medium7_5))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
{
    AddChildTail(&_firstLine);
    AddChildTail(&_secondLine);
    AddChildTail(&_thirdLine);
    AddChildTail(&_filenameLabelView);
}

void MaterialFileInfoCardView::SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool /*ellipsis*/)
{
    if (taskQueue)
        _firstLine.SetTextAsync(taskQueue, firstLine);
    else
        _firstLine.SetText(firstLine);
}

void MaterialFileInfoCardView::SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool /*ellipsis*/)
{
    if (taskQueue)
        _firstLine.SetTextAsync(taskQueue, firstLine, length);
    else
        _firstLine.SetText(firstLine, length);
}

void MaterialFileInfoCardView::SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length)
{
    if (taskQueue)
        _secondLine.SetTextAsync(taskQueue, secondLine, length);
    else
        _secondLine.SetText(secondLine, length);
}

void MaterialFileInfoCardView::SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length)
{
    if (taskQueue)
        _thirdLine.SetTextAsync(taskQueue, thirdLine, length);
    else
        _thirdLine.SetText(thirdLine, length);
}

void MaterialFileInfoCardView::SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle)
{
    BannerView::SetFileNameAsync(taskQueue, fileName, useAsTitle);
    StringUtil::Copy(_baseFileName, fileName ? fileName : "", sizeof(_baseFileName) / sizeof(_baseFileName[0]));
    _fileNameScrollPrepared = false;
    _fileNameScrollOffsetQ8 = 0;
    _fileNameScrollCycleQ8 = 0;
    _fileNameScrollPauseFrames = FILE_NAME_SCROLL_START_PAUSE_FRAMES;
    if (taskQueue)
        _filenameLabelView.SetTextAsync(taskQueue, fileName);
    else
        _filenameLabelView.SetText(fileName);
}

void MaterialFileInfoCardView::PrepareFileNameScrollIfNeeded(const BannerView::LayoutConfig& cfg)
{
    if (_fileNameScrollPrepared)
        return;

    _fileNameScrollPrepared = true;
    _fileNameScrollOffsetQ8 = 0;
    _fileNameScrollCycleQ8 = 0;
    _fileNameScrollPauseFrames = FILE_NAME_SCROLL_START_PAUSE_FRAMES;

    if (!cfg.fileNameScrollEnabled || _baseFileName[0] == 0)
        return;

    FontType measureFont = cfg.fileNameFont < LAYOUT_FONT_COUNT
        ? static_cast<FontType>(cfg.fileNameFont)
        : FontType::Medium7_5;

    u32 baseWidth = 0;
    u32 baseHeight = 0;
    nft2_measureString(_fontRepository->GetFont(measureFont), _baseFileName, baseWidth, baseHeight);
    if ((int)baseWidth <= 220)
        return;

    char16_t separator[8];
    StringUtil::Copy(separator, FILE_NAME_SCROLL_SEPARATOR, sizeof(separator) / sizeof(separator[0]));
    u32 sepWidth = 0;
    u32 sepHeight = 0;
    nft2_measureString(_fontRepository->GetFont(measureFont), separator, sepWidth, sepHeight);

    u32 out = 0;
    auto appendText = [&](const char16_t* src)
    {
        if (!src)
            return;
        for (u32 i = 0; src[i] != 0 && out < 319; i++)
            _fileNameScroller[out++] = src[i];
    };
    appendText(_baseFileName);
    appendText(separator);
    appendText(_baseFileName);
    appendText(separator);
    appendText(_baseFileName);
    _fileNameScroller[out] = 0;

    _filenameLabelView.SetText(_fileNameScroller);
    _fileNameScrollCycleQ8 = ((int)baseWidth + (int)sepWidth) << 8;
}

void MaterialFileInfoCardView::UpdateFileNameScroll(const BannerView::LayoutConfig& cfg)
{
    if (!cfg.fileNameScrollEnabled || !cfg.fileNameVisible)
    {
        _filenameLabelView.SetTextOffsetX(0);
        if (_filenameLabelView.GetStringWidth() != 0)
            _filenameLabelView.SetText(_baseFileName);
        _fileNameScrollPrepared = false;
        return;
    }

    PrepareFileNameScrollIfNeeded(cfg);
    if (_fileNameScrollCycleQ8 <= 0)
    {
        _filenameLabelView.SetTextOffsetX(0);
        return;
    }

    if (_fileNameScrollPauseFrames > 0)
    {
        _fileNameScrollPauseFrames--;
        return;
    }

    int speedQ8 = FILE_NAME_SCROLL_SPEED_Q8_BASE * std::max<int>(1, cfg.fileNameScrollSpeed);
    _fileNameScrollOffsetQ8 += speedQ8;

    if (_fileNameScrollOffsetQ8 >= _fileNameScrollCycleQ8)
    {
        _fileNameScrollOffsetQ8 = 0;
        _filenameLabelView.SetTextOffsetX(0);
        _fileNameScrollPauseFrames = FILE_NAME_SCROLL_CYCLE_PAUSE_FRAMES;
        return;
    }

    _filenameLabelView.SetTextOffsetX(-(_fileNameScrollOffsetQ8 >> 8));
}

void MaterialFileInfoCardView::ApplyLayoutFonts(const BannerView::LayoutConfig& cfg)
{
    auto safeFont = [](u8 idx, FontType fallback) -> FontType
    {
        return idx < LAYOUT_FONT_COUNT ? static_cast<FontType>(idx) : fallback;
    };

    _firstLine.SetFont(_fontRepository->GetFont(safeFont(cfg.romNameRow1Font, FontType::Medium11)));
    _secondLine.SetFont(_fontRepository->GetFont(safeFont(cfg.romNameRow2Font, FontType::Regular10)));
    _thirdLine.SetFont(_fontRepository->GetFont(safeFont(cfg.romNameRow3Font, FontType::Regular10)));
    _filenameLabelView.SetFont(_fontRepository->GetFont(safeFont(cfg.fileNameFont, FontType::Medium7_5)));
}

void MaterialFileInfoCardView::InitVram(const VramContext& vramContext)
{
    BannerView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _iconCellVramOffset = objVramManager->Alloc(iconCellTilesLen);
        dma_ntrCopy32(3, iconCellTiles, objVramManager->GetVramAddress(_iconCellVramOffset), iconCellTilesLen);
    }
}

void MaterialFileInfoCardView::Update()
{
    BannerView::Update();
    const auto& cfg = GetLayoutConfig();
    ApplyLayoutFonts(cfg);
    UpdateFileNameScroll(cfg);
    _firstLine.SetPosition(cfg.romNameRow1Visible ? cfg.romNameRow1X : -320,
        cfg.romNameRow1Visible ? cfg.romNameRow1Y : -320);
    _secondLine.SetPosition(cfg.romNameRow2Visible ? cfg.romNameRow2X : -320,
        cfg.romNameRow2Visible ? cfg.romNameRow2Y : -320);
    _thirdLine.SetPosition(cfg.romNameRow3Visible ? cfg.romNameRow3X : -320,
        cfg.romNameRow3Visible ? cfg.romNameRow3Y : -320);
    _filenameLabelView.SetPosition(cfg.fileNameVisible ? cfg.fileNameX : -320,
        cfg.fileNameVisible ? cfg.fileNameY : -320);
    if (_icon)
    {
        _icon->SetPosition(cfg.iconVisible ? cfg.iconX : -320,
            cfg.iconVisible ? cfg.iconY : -320);
        _icon->Update();
    }
}

void MaterialFileInfoCardView::Draw(GraphicsContext& graphicsContext)
{
    const auto& cfg = GetLayoutConfig();
    Rgb<8, 8, 8> textColor1(cfg.romNameRow1ColorR, cfg.romNameRow1ColorG, cfg.romNameRow1ColorB);
    Rgb<8, 8, 8> textColor2(cfg.romNameRow2ColorR, cfg.romNameRow2ColorG, cfg.romNameRow2ColorB);
    Rgb<8, 8, 8> textColor3(cfg.romNameRow3ColorR, cfg.romNameRow3ColorG, cfg.romNameRow3ColorB);
    Rgb<8, 8, 8> fileNameColor(cfg.fileNameColorR, cfg.fileNameColorG, cfg.fileNameColorB);
    _firstLine.SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _firstLine.SetForegroundColor(textColor1);
    _secondLine.SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _secondLine.SetForegroundColor(textColor2);
    _thirdLine.SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _thirdLine.SetForegroundColor(textColor3);
    _filenameLabelView.SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _filenameLabelView.SetForegroundColor(fileNameColor);

    BannerView::Draw(graphicsContext);

    const auto& bgColor = _materialColorScheme->secondaryContainer;
    const auto& fgColor = _materialColorScheme->mainIconBg;
    u32 iconCellPlttRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(bgColor, fgColor));

    gfx_oam_entry_t* oam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<64, 64>(_position.x + 18, _position.y + 130 - 8, _iconCellVramOffset >> 7)
        .WithPalette16(iconCellPlttRow)
        .WithPriority(3)
        .Build(oam[0]);

    if (_icon && GetLayoutConfig().iconVisible)
    {
        _icon->SetObjVramOffset(_iconVramOffset);
        _icon->Draw(graphicsContext, fgColor);
    }
}

#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "core/math/RgbMixer.h"
#include "core/math/ColorConverter.h"
#include "iconCell.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "MaterialFileInfoCardView.h"

MaterialFileInfoCardView::MaterialFileInfoCardView(const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _firstLine(Label2DView::CreateShared(176, 16, 50, fontRepository->GetFont(FontType::Medium11)))
    , _secondLine(Label2DView::CreateShared(176, 16, 50, fontRepository->GetFont(FontType::Regular10)))
    , _thirdLine(Label2DView::CreateShared(176, 16, 50, fontRepository->GetFont(FontType::Regular10)))
    , _filenameLabelView(Label2DView::CreateShared(220, 16, 200, fontRepository->GetFont(FontType::Medium7_5)))
    , _materialColorScheme(materialColorScheme)
{
    AddChildTail(_firstLine.GetPointer());
    AddChildTail(_secondLine.GetPointer());
    AddChildTail(_thirdLine.GetPointer());
    _filenameLabelView->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    AddChildTail(_filenameLabelView.GetPointer());
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
    _firstLine->SetPosition(_position.x + 70, _position.y + 130 - 8);
    _secondLine->SetPosition(_position.x + 70, _position.y + 145 - 8);
    _thirdLine->SetPosition(_position.x + 70, _position.y + 159 - 8);
    _filenameLabelView->SetPosition(_position.x + 18, _position.y + 168);
    if (_icon)
    {
        _icon->SetPosition(_position.x + 24, _position.y + 136 - 8);
        _icon->Update();
    }
}

void MaterialFileInfoCardView::Draw(GraphicsContext& graphicsContext)
{
    _firstLine->SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _firstLine->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
    _secondLine->SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _secondLine->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
    _thirdLine->SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _thirdLine->SetForegroundColor(_materialColorScheme->onSecondaryContainer);
    _filenameLabelView->SetBackgroundColor(_materialColorScheme->secondaryContainer);
    _filenameLabelView->SetForegroundColor(_materialColorScheme->onSurfaceVariant);

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

    if (_icon)
    {
        _icon->Draw(graphicsContext, fgColor);
    }
}

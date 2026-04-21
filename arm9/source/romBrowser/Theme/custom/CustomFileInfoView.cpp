#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "themes/IFontRepository.h"
#include "themes/custom/CustomThemeInfo.h"
#include "CustomFileInfoView.h"

CustomFileInfoView::CustomFileInfoView(const CustomThemeInfo* customThemeInfo, const IFontRepository* fontRepository)
    : _firstLine(Label2DView::CreateShared(
        customThemeInfo->topBannerTextLine0Info.GetWidth(), 16, 50, fontRepository->GetFont(FontType::Medium11)))
    , _secondLine(Label2DView::CreateShared(
        customThemeInfo->topBannerTextLine1Info.GetWidth(), 16, 50, fontRepository->GetFont(FontType::Regular10)))
    , _thirdLine(Label2DView::CreateShared(
        customThemeInfo->topBannerTextLine2Info.GetWidth(), 16, 50, fontRepository->GetFont(FontType::Regular10)))
    , _filenameLabelView(Label2DView::CreateShared(
        customThemeInfo->topFileNameTextInfo.GetWidth(), 16, 256, fontRepository->GetFont(FontType::Medium7_5)))
    , _customThemeInfo(customThemeInfo)
{
    AddChildTail(_firstLine.GetPointer());
    AddChildTail(_secondLine.GetPointer());
    AddChildTail(_thirdLine.GetPointer());
    _filenameLabelView->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    AddChildTail(_filenameLabelView.GetPointer());
}

void CustomFileInfoView::Update()
{
    BannerView::Update();
    _firstLine->SetPosition(_customThemeInfo->topBannerTextLine0Info.GetPosition());
    _secondLine->SetPosition(_customThemeInfo->topBannerTextLine1Info.GetPosition());
    _thirdLine->SetPosition(_customThemeInfo->topBannerTextLine2Info.GetPosition());
    _filenameLabelView->SetPosition(_customThemeInfo->topFileNameTextInfo.GetPosition());
    if (_icon)
    {
        _icon->SetPosition(_customThemeInfo->topIconInfo.GetPosition());
        _icon->Update();
    }
}

void CustomFileInfoView::Draw(GraphicsContext& graphicsContext)
{
    _firstLine->SetBackgroundColor(_customThemeInfo->topBannerTextLine0Info.GetBlendColor());
    _firstLine->SetForegroundColor(_customThemeInfo->topBannerTextLine0Info.GetTextColor());
    _secondLine->SetBackgroundColor(_customThemeInfo->topBannerTextLine1Info.GetBlendColor());
    _secondLine->SetForegroundColor(_customThemeInfo->topBannerTextLine1Info.GetTextColor());
    _thirdLine->SetBackgroundColor(_customThemeInfo->topBannerTextLine2Info.GetBlendColor());
    _thirdLine->SetForegroundColor(_customThemeInfo->topBannerTextLine2Info.GetTextColor());
    _filenameLabelView->SetBackgroundColor(_customThemeInfo->topFileNameTextInfo.GetBlendColor());
    _filenameLabelView->SetForegroundColor(_customThemeInfo->topFileNameTextInfo.GetTextColor());

    BannerView::Draw(graphicsContext);

    if (_icon)
    {
        _icon->Draw(graphicsContext, _customThemeInfo->topIconInfo.GetBlendColor());
    }
}

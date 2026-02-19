#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "themes/IFontRepository.h"
#include "CustomFileInfoView.h"

CustomFileInfoView::CustomFileInfoView(const IFontRepository* fontRepository)
    : _firstLine(176, 16, 50, fontRepository->GetFont(FontType::Medium11))
    , _secondLine(176, 16, 50, fontRepository->GetFont(FontType::Regular10))
    , _thirdLine(176, 16, 50, fontRepository->GetFont(FontType::Regular10))
    , _filenameLabelView(220, 16, 200, fontRepository->GetFont(FontType::Medium7_5))
    , _backgroundColor(200, 200, 200), _textColor(30, 30, 30)
{
    AddChildTail(&_firstLine);
    AddChildTail(&_secondLine);
    AddChildTail(&_thirdLine);
    AddChildTail(&_filenameLabelView);
}

void CustomFileInfoView::Update()
{
    BannerView::Update();
    _firstLine.SetPosition(_position.x + 70, _position.y + 130 - 4);
    _secondLine.SetPosition(_position.x + 70, _position.y + 145 - 4);
    _thirdLine.SetPosition(_position.x + 70, _position.y + 159 - 4);
    _filenameLabelView.SetPosition(_position.x + 18, _position.y + 170);
    if (_icon)
    {
        _icon->SetPosition(_position.x + 24, _position.y + 136 - 4);
        _icon->Update();
    }
}

void CustomFileInfoView::Draw(GraphicsContext& graphicsContext)
{
    _firstLine.SetBackgroundColor(_backgroundColor);
    _firstLine.SetForegroundColor(_textColor);
    _secondLine.SetBackgroundColor(_backgroundColor);
    _secondLine.SetForegroundColor(_textColor);
    _thirdLine.SetBackgroundColor(_backgroundColor);
    _thirdLine.SetForegroundColor(_textColor);
    _filenameLabelView.SetBackgroundColor(_backgroundColor);
    _filenameLabelView.SetForegroundColor(_textColor);

    BannerView::Draw(graphicsContext);

    if (_icon)
    {
        _icon->SetObjVramOffset(_iconVramOffset);
        _icon->Draw(graphicsContext, _backgroundColor);
    }
}

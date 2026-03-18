#include "common.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/GraphicsContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "BgmListItemView.h"

#define NAME_LABEL_X       14
#define NAME_LABEL_Y       1
#define NAME_LABEL_WIDTH   216

BgmListItemView::BgmListItemView(const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _arrowLabel(10, 14, 8, fontRepository->GetFont(FontType::Regular10))
    , _nameLabel(NAME_LABEL_WIDTH, 14, 64, fontRepository->GetFont(FontType::Regular10))
    , _materialColorScheme(materialColorScheme)
{
    _arrowLabel.SetText(u"");
    AddChildTail(&_arrowLabel);
    AddChildTail(&_nameLabel);
}

void BgmListItemView::SetText(const char16_t* text)
{
    _nameLabel.SetText(text);
}

void BgmListItemView::Update()
{
    _arrowLabel.SetPosition(_position.x + 2, _position.y + NAME_LABEL_Y);
    _nameLabel.SetPosition(_position.x + NAME_LABEL_X, _position.y + NAME_LABEL_Y);

    if (IsFocused())
        _arrowLabel.SetText(u">");
    else
        _arrowLabel.SetText(u"");

    ViewContainer::Update();
}

void BgmListItemView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
    {
        return;
    }

    auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
    auto textColor = _materialColorScheme->onSurfaceVariant;

    if (IsFocused())
    {
        // Focused: highlight background
        backColor = _materialColorScheme->GetColor(md::sys::color::secondaryContainer);
        textColor = _materialColorScheme->GetColor(md::sys::color::onSecondaryContainer);
    }
    else if (_currentlyPlaying)
    {
        // Currently playing but not focused: use primary color for text
        textColor = _materialColorScheme->primary;
    }
    else if (_isCategory)
    {
        // Category headers use onSurface for more prominent appearance
        textColor = _materialColorScheme->onSurface;
    }

    _arrowLabel.SetBackgroundColor(backColor);
    _arrowLabel.SetForegroundColor(textColor);
    _nameLabel.SetBackgroundColor(backColor);
    _nameLabel.SetForegroundColor(textColor);

    ViewContainer::Draw(graphicsContext);
}

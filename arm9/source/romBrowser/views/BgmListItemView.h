#pragma once
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"

class MaterialColorScheme;
class IFontRepository;

class BgmListItemView : public ViewContainer
{
public:
    BgmListItemView(const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 224, 16);
    }

    void SetText(const char16_t* text);

private:
    Label2DView _nameLabel;
    const MaterialColorScheme* _materialColorScheme;
};

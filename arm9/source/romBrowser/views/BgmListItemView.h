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
    void SetCurrentlyPlaying(bool playing) { _currentlyPlaying = playing; }
    void SetIsCategory(bool isCategory) { _isCategory = isCategory; }
    void SetIndentLevel(int level) { _indentLevel = level; }

private:
    Label2DView _arrowLabel;
    Label2DView _nameLabel;
    const MaterialColorScheme* _materialColorScheme;
    bool _currentlyPlaying = false;
    bool _isCategory = false;
    int _indentLevel = 0;
};

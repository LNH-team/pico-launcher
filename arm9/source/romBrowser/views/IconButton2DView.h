#pragma once
#include "IconButtonView.h"

class IconButton2DView : public IconButtonView
{
    SHARED_ONLY(IconButton2DView)

public:
    class VramToken
    {
        u32 _vramOffset;
    public:
        VramToken()
            : _vramOffset(0) { }

        explicit VramToken(u32 offset)
            : _vramOffset(offset) { }

        constexpr u32 GetVramOffset() const { return _vramOffset; }
    };

    void Draw(GraphicsContext& graphicsContext) override;

    void SetGraphics(const VramToken& vramToken)
    {
        _selectorVramOffset = vramToken.GetVramOffset();
    }

    static VramToken UploadGraphics(IVramManager& vramManager);

private:
    u32 _selectorVramOffset;

    IconButton2DView(Type type, State state,
        md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme)
        : IconButtonView(type, state, backgroundColor, materialColorScheme) { }
};

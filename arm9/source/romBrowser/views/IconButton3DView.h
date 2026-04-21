#pragma once
#include "IconButtonView.h"
#include "core/math/Rgb.h"
#include "gui/Rgb6Palette.h"

class IconButton3DView : public IconButtonView
{
    SHARED_ONLY(IconButton3DView)

public:
    void Draw(GraphicsContext& graphicsContext) override;

    static void UploadGraphics(const VramContext& vramContext);

private:
    static u32 sSelectorTextureVramOffset;

    IconButton3DView(Type type, State state,
        md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme)
        : IconButtonView(type, state, backgroundColor, materialColorScheme) { }

    void DrawSelector(GraphicsContext& graphicsContext, const Rgb<8, 8, 8>& color);
};

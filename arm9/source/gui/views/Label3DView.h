#pragma once
#include "LabelView.h"
#include "gui/VBlankTextureLoader.h"

class alignas(32) Label3DView : public LabelView
{
    SHARED_ONLY(Label3DView)

public:
    ~Label3DView() override;

    void InitVram(const VramContext& vramContext) override;
    void Draw(GraphicsContext& graphicsContext) override;

    /// @brief Sets the depth the text quad is drawn at. Smaller is nearer, and
    ///        negative is allowed: it puts the text in front of everything the
    ///        launcher draws at zero.
    void SetDepth(int depth) { _depth = depth; }

    /// @brief Sets the polygon alpha, 0 to 31, so a caller can fade the text.
    void SetAlpha(u32 alpha) { _alpha = alpha; }

private:
    Label3DView(u32 width, u32 height, u32 maxStringLength, const nft2_header_t* font,
        VBlankTextureLoader* vblankTextureLoader);

    void UpdateTileBuffer() override;

    u32 _texVramOffset = 0;
    int _depth = 200;
    u32 _alpha = 31;
    VBlankTextureLoader* _vblankTextureLoader;
    VBlankTextureLoadRequest _textureLoadRequest;
};

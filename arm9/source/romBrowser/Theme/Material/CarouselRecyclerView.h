#pragma once
#include <array>
#include "romBrowser/views/CoverFlowRecyclerViewBase.h"
#include "animation/Animator.h"

class MaterialColorScheme;

class CarouselRecyclerView : public CoverFlowRecyclerViewBase
{
public:
    explicit CarouselRecyclerView(const MaterialColorScheme* materialColorScheme)
        : _materialColorScheme(materialColorScheme) { }

    static void UploadGraphics(const VramContext& vramContext);

    void Update() override;
    void Draw(GraphicsContext &graphicsContext) override;

private:
    struct ViewPoolEntryEx
    {
        Animator<fix32<12>> xPositionAnimator;
        Animator<fix32<12>> widthAnimator;
    };

    std::array<ViewPoolEntryEx, 10> _viewPoolEx;
    const MaterialColorScheme* _materialColorScheme;

    static u32 sMaskTextureVramOffset;

    void RenderCoverMask(GraphicsContext& graphicsContext, fix32<12> left, fix32<12> right) const;
    void RenderRoundedCorners(GraphicsContext& graphicsContext, fix32<12> x, fix32<12> width) const;

    void UpdateItemPosition(int viewPoolIndex, bool initial) override;

    Rectangle GetSelectedItemTapBounds() const override;

    void SwapViewPoolEntry(int indexA, int indexB) override
    {
        CoverFlowRecyclerViewBase::SwapViewPoolEntry(indexA, indexB);
        std::swap(_viewPoolEx[indexA], _viewPoolEx[indexB]);
    }
};

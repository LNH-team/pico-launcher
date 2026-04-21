#pragma once
#include <array>
#include "romBrowser/views/CoverFlowRecyclerViewBase.h"
#include "animation/Animator.h"

class MaterialColorScheme;

class CarouselRecyclerView : public CoverFlowRecyclerViewBase
{
    SHARED_ONLY(CarouselRecyclerView)

public:
    static void UploadGraphics(const VramContext& vramContext);

    void Update() override;
    void Draw(GraphicsContext &graphicsContext) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

private:
    struct ViewPoolEntryEx
    {
        fix32<12> xPosition;
        fix32<12> width;
    };

    std::array<ViewPoolEntryEx, 10> _viewPoolEx;
    const MaterialColorScheme* _materialColorScheme;
    Animator<fix32<12>> _scrollAnimator = Animator<fix32<12>>(0);
    bool _penDown = false;
    Point _penDownPosition = Point(0, 0);
    bool _hasScrollStarted = false;
    fix32<12> _penDownScrollOffset = 0;

    static u32 sMaskTextureVramOffset;

    CarouselRecyclerView(const MaterialColorScheme* materialColorScheme)
        : _materialColorScheme(materialColorScheme) { }

    void RenderCoverMask(GraphicsContext& graphicsContext, fix32<12> left, fix32<12> right) const;
    void RenderRoundedCorners(GraphicsContext& graphicsContext, fix32<12> x, fix32<12> width) const;

    void SetSelectedItem(int itemIdx, bool initial) override;
    void UpdateItemPosition(int viewPoolIndex);

    void SwapViewPoolEntry(int indexA, int indexB) override
    {
        CoverFlowRecyclerViewBase::SwapViewPoolEntry(indexA, indexB);
        std::swap(_viewPoolEx[indexA], _viewPoolEx[indexB]);
    }
};

#pragma once
#include <array>
#include "CoverFlowRecyclerViewBase.h"
#include "animation/Animator.h"

class CoverFlowRecyclerView : public CoverFlowRecyclerViewBase
{
    SHARED_ONLY(CoverFlowRecyclerView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

private:
    struct ViewPoolEntryEx
    {
        int yAngle;
        fix32<12> xPosition;
        fix32<12> zPosition;
    };

    std::array<ViewPoolEntryEx, 10> _viewPoolEx;
    Animator<fix32<12>> _scrollAnimator = Animator<fix32<12>>(0);
    bool _penDown = false;
    Point _penDownPosition = Point(0, 0);
    bool _hasScrollStarted = false;
    fix32<12> _penDownScrollOffset = 0;

    explicit CoverFlowRecyclerView() { }

    void SetSelectedItem(int itemIdx, bool initial) override;

    void UpdateItemPosition(int viewPoolIndex);

    void SwapViewPoolEntry(int indexA, int indexB) override
    {
        CoverFlowRecyclerViewBase::SwapViewPoolEntry(indexA, indexB);
        std::swap(_viewPoolEx[indexA], _viewPoolEx[indexB]);
    }
};

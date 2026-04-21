#pragma once
#include "animation/Animator.h"
#include "gui/views/DialogView.h"

class BottomSheetView : public DialogView
{
public:
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 256 - _position.x, 192 - _position.y);
    }

    Rectangle GetFullyCoveredArea() const override
    {
        return Rectangle(_position.x, _position.y + 12, 256 - _position.x, 148);
    }

    constexpr DialogType GetDialogType() const override { return DialogType::BottomSheet; }

protected:
    virtual void Close() = 0;

private:
    bool _oobPenDown = false;
};

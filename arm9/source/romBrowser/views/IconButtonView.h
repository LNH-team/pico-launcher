#pragma once
#include "gui/views/View.h"
#include "gui/materialDesign.h"

class MaterialColorScheme;
class IVramManager;

class IconButtonView : public View
{
public:
    typedef void (*button_action_t)(IconButtonView* sender, void* arg);

    enum class Type
    {
        Standard,
        Filled,
        Tonal
    };

    enum class State
    {
        NoToggle,
        ToggleUnselected,
        ToggleSelected,
        // Dedicated "this toggle is on" look for the favorite button specifically - distinct
        // from ToggleSelected/ToggleUnselected, which DisplaySettingsBottomSheetView's Tonal
        // option toggles already use for their own "picked vs not" pair (colored vs muted
        // capsule). Reusing either of those for the favorite button couples its look to an
        // unrelated component's semantics.
        ToggleActive
    };

    void SetIconVramOffset(u32 vramOffset) { _iconVramOffset = vramOffset; }

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, 32, 32);
    }

    void SetAction(button_action_t action, void* arg)
    {
        _action = action;
        _actionArg = arg;
    }

    void SetState(State state)
    {
        _state = state;
    }

    void SetDisabled(bool disabled)
    {
        _disabled = disabled;
    }

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

protected:
    u32 _iconVramOffset;
    md::sys::color _backgroundColor;
    button_action_t _action;
    void* _actionArg;
    Type _type;
    State _state;
    const MaterialColorScheme* _materialColorScheme;
    bool _penDown = false;
    bool _disabled = false;

    IconButtonView(Type type, State state,
        md::sys::color backgroundColor, const MaterialColorScheme* materialColorScheme)
        : _iconVramOffset(0), _backgroundColor(backgroundColor)
        , _action(nullptr), _actionArg(nullptr), _type(type), _state(state)
        , _materialColorScheme(materialColorScheme) { }

    bool IsCircleBackgroundVisible() const;
    md::sys::color GetCircleBackgroundColor() const;
    md::sys::color GetForegroundColor() const;
};
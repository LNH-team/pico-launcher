#pragma once
#include <memory>
#include "gui/views/ViewContainer.h"
#include "IconButtonView.h"

class MaterialColorScheme;

class AppBarView : public ViewContainer
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    virtual ~AppBarView();

    void SetButtonIcon(int button, u32 vramOffset)
    {
        _buttons[button]->SetIconVramOffset(vramOffset);
    }

    void SetButtonAction(int button, IconButtonView::button_action_t action, void* arg)
    {
        _buttons[button]->SetAction(action, arg);
    }

    void SetButtonState(int button, IconButtonView::State state)
    {
        _buttons[button]->SetState(state);
    }

    Rectangle GetBounds() const override;
    void Update() override;
    View* MoveFocus(View* currentFocus, FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager, int button);

    int GetButtonIndex(const View* view) const
    {
        return FindButtonIndex(view);
    }

    constexpr Orientation GetOrientation() const { return _orientation; }

protected:
    Orientation _orientation;
    std::unique_ptr<IconButtonView*[]> _buttons;
    int _startButtonCount;
    int _endButtonCount;

    void UpdateButtonPositionsHorizontal();
    void UpdateButtonPositionsVertical();
    int FindButtonIndex(const View* view) const;

    AppBarView(int x, int y, Orientation orientation,
        int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme);
};

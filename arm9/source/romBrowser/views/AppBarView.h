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

    void SetButtonIcon(int button, u32 vramOffset)
    {
        _buttons[button]->SetIconVramOffset(vramOffset);
    }

    void SetButtonAction(int button, IconButtonView::button_action_t action, void* arg)
    {
        _buttons[button]->SetAction(action, arg);
    }

    void SetButtonDisabled(int button, bool disabled)
    {
        _buttons[button]->SetDisabled(disabled);
    }

    Rectangle GetBounds() const override;
    void Update() override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager, int button);

    constexpr Orientation GetOrientation() const { return _orientation; }

protected:
    Orientation _orientation;
    std::unique_ptr<SharedPtr<IconButtonView>[]> _buttons;
    int _startButtonCount;
    int _endButtonCount;

    void UpdateButtonPositionsHorizontal();
    void UpdateButtonPositionsVertical();
    int FindButtonIndex(const View* view);

    AppBarView(int x, int y, Orientation orientation,
        int startButtonCount, int endButtonCount, const MaterialColorScheme* materialColorScheme);
};

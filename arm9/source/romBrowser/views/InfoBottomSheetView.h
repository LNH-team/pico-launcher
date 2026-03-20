#pragma once
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"

class MaterialColorScheme;
class IFontRepository;
class IRomBrowserController;
struct TouchEvent;

class SettingsInfoBottomSheetView : public BottomSheetView
{
public:
    SettingsInfoBottomSheetView(IRomBrowserController* romBrowserController,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void Focus(FocusManager& focusManager) override { focusManager.Unfocus(); }
    bool AllowDismissGestures() const override { return false; }
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager) override;
    void OnDismissed() override;

private:
    void UpdateTouchLabelText(int x, int y);

    IRomBrowserController* _romBrowserController;
    const MaterialColorScheme* _materialColorScheme;

    Label2DView _titleLabel;
    Label2DView _userLabel;
    Label2DView _birthdayLabel;
    Label2DView _messageLabel;
    Label2DView _colorLabel;
    Label2DView _consoleLanguageLabel;
    Label2DView _modeLabel;
    Label2DView _consoleLabel;
    Label2DView _usrcheatLabel;
    Label2DView _touchLabel;

    int  _touchX = 0;
    int  _touchY = 0;
    int  _displayedTouchX = -1;
    int  _displayedTouchY = -1;
};

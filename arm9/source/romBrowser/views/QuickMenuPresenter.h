#pragma once
#include <memory>
#include "animation/Animator.h"
#include "romBrowser/views/QuickMenuBottomSheetView.h"

class StackVramManager;
class FocusManager;
class GraphicsContext;
class InputProvider;
struct TouchEvent;

class QuickMenuPresenter
{
public:
    QuickMenuPresenter(FocusManager* focusManager, StackVramManager* vramManager);

    void Show(std::unique_ptr<QuickMenuBottomSheetView> view);
    void Close();
    void Update();
    void Draw(GraphicsContext& graphicsContext);
    void VBlank();

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager);
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager);

    bool IsIdle() const { return _state == State::Idle; }
    bool IsActive() const { return _state != State::Idle; }
    bool IsTransitioning() const { return _state == State::Opening || _state == State::Closing; }

    void ClearOldFocus() { _oldFocus = nullptr; }
    constexpr View* GetOldFocus() const { return _oldFocus; }

private:
    enum class State
    {
        Idle,
        Opening,
        Visible,
        Closing
    };

    void BeginOpen();
    void BeginClose();
    void ClearBg1Map();

private:
    FocusManager* _focusManager;
    StackVramManager* _vramManager;
    u32 _baseVramState;
    std::unique_ptr<QuickMenuBottomSheetView> _currentView;
    bool _initVram = false;
    View* _oldFocus = nullptr;
    Animator<int> _scrimAnimator;
    Animator<int> _yAnimator;
    int _scrimTargetBlend = 0;
    State _state = State::Idle;

    static constexpr int kHiddenY = 192;
    static constexpr int kVisibleY = 6;
};

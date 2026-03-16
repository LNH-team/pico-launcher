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
    void CloseUpward();
    void Update();
    void Draw(GraphicsContext& graphicsContext);
    void VBlank();
    void DismissImmediately();

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager);
    bool HandleTouch(const TouchEvent& event, FocusManager& focusManager);

    bool IsIdle() const { return _state == State::Idle; }
    bool IsActive() const { return _state != State::Idle; }
    bool ShouldBlockNonBInput() const;
    bool CanInterruptOpeningWithB() const { return _currentView && _state == State::Opening; }

    void ClearOldFocus()
    {
        if (_oldFocus)
            _oldFocus->SetVisualFocusRetained(false);
        _oldFocus = nullptr;
    }
    View* DetachOldFocus()
    {
        View* oldFocus = _oldFocus;
        _oldFocus = nullptr;
        return oldFocus;
    }
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
    void BeginClose(int hiddenY);
    void ClearBg1Map();
    void RestoreOldFocus();

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

    static constexpr int kHiddenBottomY = 192;
    static constexpr int kHiddenTopY = -192;
    static constexpr int kVisibleY = 6;
};

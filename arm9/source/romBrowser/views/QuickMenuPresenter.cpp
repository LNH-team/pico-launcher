#include "common.h"
#include <nds/arm9/background.h>
#include "gui/materialDesign.h"
#include "gui/VramContext.h"
#include "gui/StackVramManager.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "bottomSheetBg.h"
#include "QuickMenuPresenter.h"

namespace
{
    constexpr u32 kOpeningInputUnlockProgressNumerator = 1;
    constexpr u32 kOpeningInputUnlockProgressDenominator = 2;
    constexpr u32 kClosingInputUnlockProgressNumerator = 2;
    constexpr u32 kClosingInputUnlockProgressDenominator = 3;

    bool HasReachedInputUnlockPoint(
        const Animator<int>& animator, u32 numerator, u32 denominator)
    {
        const u32 duration = animator.GetDuration();
        if (duration == 0)
            return true;

        return animator.GetFrame() * denominator >= duration * numerator;
    }
}

QuickMenuPresenter::QuickMenuPresenter(FocusManager* focusManager, StackVramManager* vramManager)
    : _focusManager(focusManager)
    , _vramManager(vramManager)
    , _scrimAnimator(0)
    , _yAnimator(kHiddenBottomY)
{
    _baseVramState = _vramManager->GetState();
}

void QuickMenuPresenter::ClearBg1Map()
{
    vu16* bgMap = reinterpret_cast<vu16*>((vu8*)BG_GFX + 0x4000);
    for (u32 i = 0; i < bottomSheetBgMapLen / sizeof(u16); i++)
        bgMap[i] = 0;
}

void QuickMenuPresenter::Show(std::unique_ptr<QuickMenuBottomSheetView> view)
{
    if (!view)
        return;

    const bool reopeningWhileClosing = _currentView && _state == State::Closing;
    if (_currentView && !reopeningWhileClosing)
        return;

    int startY = kHiddenBottomY;
    int startScrimBlend = 0;
    if (reopeningWhileClosing)
    {
        startY = _yAnimator.GetValue();
        startScrimBlend = _scrimAnimator.GetValue();

        if (_focusManager->IsFocusInside(_currentView.get()))
            _focusManager->Unfocus();
    }

    _currentView = std::move(view);
    _initVram = true;
    _scrimTargetBlend = _currentView->GetScrimTargetBlend();
    _yAnimator = Animator<int>(startY);
    _scrimAnimator = Animator<int>(startScrimBlend);
    _currentView->SetPosition(0, startY);

    ClearBg1Map();
    BeginOpen();
}

void QuickMenuPresenter::Close()
{
    if (!_currentView || _state == State::Closing)
        return;

    BeginClose(kHiddenBottomY);
}

void QuickMenuPresenter::CloseUpward()
{
    if (!_currentView || _state == State::Closing)
        return;

    BeginClose(kHiddenTopY);
}

void QuickMenuPresenter::DismissImmediately()
{
    if (!_currentView)
        return;

    if (_focusManager->IsFocusInside(_currentView.get()))
        _focusManager->Unfocus();

    if (_oldFocus)
        _oldFocus->SetVisualFocusRetained(false);
    _oldFocus = nullptr;

    _currentView.reset();
    _initVram = false;
    _state = State::Idle;
    _scrimAnimator = Animator<int>(0);
    _yAnimator = Animator<int>(kHiddenBottomY);
    ClearBg1Map();
    REG_BLDALPHA = (16 << 8) | 0;
}

void QuickMenuPresenter::RestoreOldFocus()
{
    if (!_oldFocus)
        return;

    _focusManager->Focus(_oldFocus);
    _oldFocus->SetVisualFocusRetained(false);
    _oldFocus = nullptr;
}

void QuickMenuPresenter::BeginOpen()
{
    _state = State::Opening;
    _scrimAnimator.Goto(_scrimTargetBlend, md::sys::motion::duration::short2,
        &md::sys::motion::easing::linear);
    _yAnimator.Goto(kVisibleY, md::sys::motion::duration::medium2,
        &md::sys::motion::easing::emphasizedDecelerate);

    if (!_oldFocus)
    {
        _oldFocus = _focusManager->GetCurrentFocus();
        if (_oldFocus)
            _oldFocus->SetVisualFocusRetained(true);
    }
    _currentView->Focus(*_focusManager);
}

void QuickMenuPresenter::BeginClose(int hiddenY)
{
    _state = State::Closing;
    _scrimAnimator.Goto(0, md::sys::motion::duration::short3,
        &md::sys::motion::easing::emphasizedAccelerate);
    _yAnimator.Goto(hiddenY, md::sys::motion::duration::short3,
        &md::sys::motion::easing::emphasizedAccelerate);
}

void QuickMenuPresenter::Update()
{
    if (!_currentView)
        return;

    if (_state == State::Opening)
    {
        if (!_yAnimator.IsFinished())
            _yAnimator.Update();
        if (_yAnimator.IsFinished())
            _state = State::Visible;
    }
    else if (_state == State::Closing)
    {
        if (_oldFocus && HasReachedInputUnlockPoint(_yAnimator,
            kClosingInputUnlockProgressNumerator, kClosingInputUnlockProgressDenominator))
            RestoreOldFocus();

        if (!_yAnimator.IsFinished())
            _yAnimator.Update();
        if (_yAnimator.IsFinished())
        {
            _state = State::Idle;
            _currentView.reset();
            RestoreOldFocus();
            return;
        }
    }

    if (_currentView)
    {
        _currentView->SetPosition(0, _yAnimator.GetValue());
        _currentView->Update();
    }
}

void QuickMenuPresenter::Draw(GraphicsContext& graphicsContext)
{
    if (_currentView)
        _currentView->Draw(graphicsContext);
}

void QuickMenuPresenter::VBlank()
{
    if (_initVram && _currentView)
    {
        _vramManager->SetState(_baseVramState);
        _currentView->InitVram(VramContext(nullptr, _vramManager, nullptr, nullptr));
        _initVram = false;
    }

    if (_currentView)
        _currentView->VBlank();

    if (_state != State::Idle || !_scrimAnimator.IsFinished())
    {
        if (!_scrimAnimator.IsFinished())
            _scrimAnimator.Update();
        int scrimBlend = _scrimAnimator.GetValue();
        REG_BLDALPHA = ((16 - scrimBlend) << 8) | scrimBlend;
    }
}

bool QuickMenuPresenter::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (!_currentView)
        return false;

    if (_state == State::Opening && inputProvider.Triggered(InputKey::B))
        return _currentView->HandleInput(inputProvider, focusManager);

    if (_state != State::Visible)
        return false;

    return _currentView->HandleInput(inputProvider, focusManager);
}

bool QuickMenuPresenter::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (!_currentView || _state == State::Idle)
        return false;

    if (_state != State::Visible)
        return true;

    const auto bounds = _currentView->GetBounds();
    if (bounds.Contains(event.position) || bounds.Contains(event.startPosition))
        return _currentView->HandleTouch(event, focusManager);

    if (event.type == TouchEventType::Up && event.holdFrames <= 24)
        _currentView->OnDismissed();

    return true;
}

bool QuickMenuPresenter::ShouldBlockNonBInput() const
{
    if (_state == State::Opening)
    {
        return !HasReachedInputUnlockPoint(_yAnimator,
            kOpeningInputUnlockProgressNumerator, kOpeningInputUnlockProgressDenominator);
    }

    if (_state == State::Closing)
    {
        return !HasReachedInputUnlockPoint(_yAnimator,
            kClosingInputUnlockProgressNumerator, kClosingInputUnlockProgressDenominator);
    }

    return false;
}

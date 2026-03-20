#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include <nds/arm9/background.h>
#include "gui/StackVramManager.h"
#include "gui/VramContext.h"
#include "bottomSheetBg.h"
#include "scrim.h"
#include "gui/materialDesign.h"
#include "gui/GraphicsContext.h"
#include "gui/input/TouchEvent.h"
#include "DialogPresenter.h"

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

#define DIALOG_DRAG_START_THRESHOLD      22
#define DIALOG_DISMISS_DISTANCE          96
#define DIALOG_DISMISS_MIN_DISTANCE      56
#define DIALOG_DISMISS_VELOCITY          32
#define DIALOG_TAP_MAX_FRAMES            24

DialogPresenter::DialogPresenter(FocusManager* focusManager, StackVramManager* vramManager)
    : _focusManager(focusManager), _vramManager(vramManager)
    , _scrimAnimator(0), _yAnimator(kHiddenY)
{
    _baseVramState = _vramManager->GetState();
}

void DialogPresenter::ApplyBottomSheetBg(bool visible)
{
    vu16* bgMap = reinterpret_cast<vu16*>((vu8*)BG_GFX + 0x4000);
    if (visible)
    {
        dma_ntrCopy32(3, bottomSheetBgMap, bgMap, bottomSheetBgMapLen);
    }
    else
    {
        for (u32 i = 0; i < bottomSheetBgMapLen / sizeof(u16); i++)
            bgMap[i] = 0;
    }
}

void DialogPresenter::ShowDialog(std::unique_ptr<DialogView> dialog)
{
    if (!_nextDialog)
        _nextDialog = std::move(dialog);
}

void DialogPresenter::CloseDialog()
{
    if (!_currentDialog || _curState != State::BottomSheetVisible)
        return;

    _newState = State::BottomSheetClosing;
}

void DialogPresenter::Update()
{
    if (_curState != _newState)
    {
        _curState = _newState;
        switch (_curState)
        {
            case State::BottomSheetVisible:
            {
                _useBottomSheetBg = _currentDialog->UseBottomSheetBackground();
                _scrimTargetBlend = _currentDialog->GetScrimTargetBlend();
                ApplyBottomSheetBg(_useBottomSheetBg);

                _scrimAnimator.Goto(_scrimTargetBlend, md::sys::motion::duration::short2,
                    &md::sys::motion::easing::linear);
                _yAnimator.Goto(kVisibleY, md::sys::motion::duration::long2,
                    &md::sys::motion::easing::emphasizedDecelerate);
                if (!_oldFocus)
                {
                    _oldFocus = _focusManager->GetCurrentFocus();
                    if (_oldFocus)
                        _oldFocus->SetVisualFocusRetained(true);
                }
                _currentDialog->Focus(*_focusManager);
                break;
            }
            case State::BottomSheetClosing:
            {
                _scrimAnimator.Goto(0, md::sys::motion::duration::short4,
                    &md::sys::motion::easing::emphasizedAccelerate);
                _yAnimator.Goto(kHiddenY, md::sys::motion::duration::short4,
                    &md::sys::motion::easing::emphasizedAccelerate);
                if (_oldFocus && !_nextDialog)
                {
                    _focusManager->Focus(_oldFocus);
                    _oldFocus->SetVisualFocusRetained(false);
                    _oldFocus = nullptr;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
    switch (_curState)
    {
        case State::Idle:
        {
            if (!_currentDialog && _nextDialog)
            {
                _currentDialog = std::move(_nextDialog);
                _initVram = true;
                _newState = State::BottomSheetVisible;
            }
            break;
        }
        case State::BottomSheetVisible:
        {
            if (!_yAnimator.IsFinished())
                _yAnimator.Update();
            break;
        }
        case State::BottomSheetClosing:
        {
            if (!_yAnimator.IsFinished())
            {
                _yAnimator.Update();
            }
            else
            {
                _newState = State::Idle;
                _currentDialog.reset();
            }
            break;
        }
    }
    if (_currentDialog)
    {
        _currentDialog->SetPosition(_currentDialog->GetPosition().x, _yAnimator.GetValue());
        _currentDialog->Update();
    }
}

void DialogPresenter::ApplyClipArea(GraphicsContext& graphicsContext) const
{
    if (_currentDialog)
    {
        graphicsContext.SetClipArea(_currentDialog->GetFullyCoveredArea(), true);
    }
}

bool DialogPresenter::HandleTouch(const TouchEvent& event, FocusManager& focusManager)
{
    if (!_currentDialog || _curState != State::BottomSheetVisible)
        return false;

    int dialogTopY = _yAnimator.GetValue();
    const bool allowDismissGestures = _currentDialog->AllowDismissGestures();

    if (!allowDismissGestures)
    {
        if (event.position.y >= dialogTopY || event.startPosition.y >= dialogTopY)
            _currentDialog->HandleTouch(event, focusManager);
        return true;
    }

    switch (event.type)
    {
        case TouchEventType::Down:
        {
            _touchStartYAnimatorValue = dialogTopY;
            _touchCapturedByDialogContent = false;
            if (event.position.y < dialogTopY)
            {
                _touchDraggingDialog = false;
                return true;
            }
            else
            {
                _touchDraggingDialog = false;
                _touchCapturedByDialogContent = _currentDialog->HandleTouch(event, focusManager);
                return true;
            }
        }
        case TouchEventType::Move:
        {
            if (event.startPosition.y >= _touchStartYAnimatorValue)
            {
                if (_touchCapturedByDialogContent)
                {
                    _currentDialog->HandleTouch(event, focusManager);
                    return true;
                }

                int deltaY = event.position.y - event.startPosition.y;
                int deltaX = event.position.x - event.startPosition.x;
                int absDeltaX = deltaX < 0 ? -deltaX : deltaX;
                if (!_touchDraggingDialog &&
                    deltaY > DIALOG_DRAG_START_THRESHOLD &&
                    deltaY > absDeltaX + 8)
                {
                    _touchDraggingDialog = true;
                }
                if (_touchDraggingDialog)
                {
                    int newY = _touchStartYAnimatorValue + deltaY;
                    if (newY < _touchStartYAnimatorValue)
                        newY = _touchStartYAnimatorValue; 
                    _yAnimator = Animator<int>(newY);
                }
                else
                {
                    _currentDialog->HandleTouch(event, focusManager);
                }
            }
            return true;
        }
        case TouchEventType::Up:
        {
            if (event.startPosition.y < _touchStartYAnimatorValue)
            {
                if (event.holdFrames <= DIALOG_TAP_MAX_FRAMES)
                {
                    _currentDialog->OnDismissed();
                }
                return true;
            }

            if (_touchCapturedByDialogContent)
            {
                _currentDialog->HandleTouch(event, focusManager);
                _touchCapturedByDialogContent = false;
                return true;
            }

            if (_touchDraggingDialog)
            {
                int deltaY = event.position.y - event.startPosition.y;
                int velocityY = event.velocityY;
                bool shouldDismiss = (deltaY > DIALOG_DISMISS_DISTANCE)
                    || (deltaY > DIALOG_DISMISS_MIN_DISTANCE && velocityY > DIALOG_DISMISS_VELOCITY);

                if (shouldDismiss)
                {
                    _currentDialog->OnDismissed();
                }
                else
                {
                    _yAnimator.Goto(_touchStartYAnimatorValue,
                        md::sys::motion::duration::medium1,
                        &md::sys::motion::easing::emphasizedDecelerate);
                }
                _touchDraggingDialog = false;
                return true;
            }

            if (_currentDialog && event.holdFrames <= DIALOG_TAP_MAX_FRAMES)
            {
                _currentDialog->HandleTouch(event, focusManager);
            }
            _touchCapturedByDialogContent = false;
            return true;
        }
    }
    return true;
}

void DialogPresenter::VBlank()
{
    REG_BG1VOFS = -_yAnimator.GetValue();

    if (_initVram && _currentDialog)
    {
        _vramManager->SetState(_baseVramState);
        _currentDialog->InitVram(VramContext(nullptr, _vramManager, nullptr, nullptr));
        _initVram = false;
    }

    if (_currentDialog)
        _currentDialog->VBlank();

    if (!_scrimAnimator.IsFinished())
    {
        _scrimAnimator.Update();
        int scrimBlend = _scrimAnimator.GetValue();
        REG_BLDALPHA = ((16 - scrimBlend) << 8) | scrimBlend;
    }
}

void DialogPresenter::InitVram()
{
    dma_ntrCopy32(3, scrimTiles, (vu8*)BG_GFX, scrimTilesLen);
    dma_ntrCopy32(3, bottomSheetBgTiles, (vu8*)BG_GFX + 64, bottomSheetBgTilesLen);
    dma_ntrCopy32(3, bottomSheetBgMap, (vu8*)BG_GFX + 0x4000, bottomSheetBgMapLen);
    dma_ntrCopy32(3, scrimMap, (vu8*)BG_GFX + 0x5000, scrimMapLen);

    REG_BG1CNT = BG_32x64 | BG_PRIORITY_1 | BG_COLOR_16 | BG_MAP_BASE(8) | BG_TILE_BASE(0);
    REG_BG1HOFS = 0;
    REG_BG1VOFS = 0;

    REG_BG2CNT = BG_32x32 | BG_PRIORITY_2 | BG_COLOR_16 | BG_MAP_BASE(10) | BG_TILE_BASE(0);
    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0;

    REG_BLDCNT = 0x3944;
    REG_BLDALPHA = (16 << 8) | 0;
}

bool DialogPresenter::ShouldBlockNonBInput() const
{
    if (_nextDialog || _curState != _newState)
        return true;

    if (_curState == State::BottomSheetClosing)
    {
        return !HasReachedInputUnlockPoint(_yAnimator,
            kClosingInputUnlockProgressNumerator, kClosingInputUnlockProgressDenominator);
    }

    if (_curState == State::BottomSheetVisible && !_yAnimator.IsFinished())
    {
        return !HasReachedInputUnlockPoint(_yAnimator,
            kOpeningInputUnlockProgressNumerator, kOpeningInputUnlockProgressDenominator);
    }

    return false;
}

bool DialogPresenter::CanInterruptOpeningWithB() const
{
    return _currentDialog
        && _curState == State::BottomSheetVisible
        && _newState == State::BottomSheetVisible
        && !_yAnimator.IsFinished()
        && _yAnimator.GetTargetValue() == kVisibleY;
}

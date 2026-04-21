#include "common.h"
#include <libtwl/dma/dmaNitro.h>
#include <nds/arm9/background.h>
#include "gui/StackVramManager.h"
#include "gui/VramContext.h"
#include "bottomSheetBg.h"
#include "scrim.h"
#include "gui/materialDesign.h"
#include "gui/GraphicsContext.h"
#include "DialogPresenter.h"

DialogPresenter::DialogPresenter(FocusManager* focusManager, StackVramManager* vramManager)
    : _focusManager(focusManager), _vramManager(vramManager)
    , _scrimAnimator(0), _yAnimator(192)
{
    _baseVramState = _vramManager->GetState();
}

void DialogPresenter::ShowDialog(SharedPtr<DialogView> dialog)
{
    if (!_nextDialog)
    {
        _nextDialog = std::move(dialog);
    }
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
                _scrimAnimator.Goto(5, md::sys::motion::duration::short2,
                    &md::sys::motion::easing::linear);
                _yAnimator.Goto(32, md::sys::motion::duration::long2,
                    &md::sys::motion::easing::emphasizedDecelerate);
                _oldFocus = _focusManager->GetCurrentFocus();
                _currentDialog->Focus(*_focusManager);
                break;
            }
            case State::BottomSheetClosing:
            {
                _scrimAnimator.Goto(0, md::sys::motion::duration::short4,
                    &md::sys::motion::easing::emphasizedAccelerate);
                _yAnimator.Goto(192, md::sys::motion::duration::short4,
                    &md::sys::motion::easing::emphasizedAccelerate);
                if (_oldFocus)
                {
                    _focusManager->Focus(_oldFocus);
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
                _currentDialog.Reset();
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

void DialogPresenter::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (_curState == State::BottomSheetVisible && _currentDialog)
    {
        _currentDialog->HandlePenDown(touchPoint, focusManager);
    }
}

void DialogPresenter::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (_curState == State::BottomSheetVisible && _currentDialog)
    {
        _currentDialog->HandlePenMove(touchPoint, focusManager);
    }
}

void DialogPresenter::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_curState == State::BottomSheetVisible && _currentDialog)
    {
        _currentDialog->HandlePenUp(lastTouchPoint, focusManager);
    }
}

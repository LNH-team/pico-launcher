#include "common.h"
#include <nds/arm9/background.h>
#include "gui/materialDesign.h"
#include "gui/VramContext.h"
#include "gui/StackVramManager.h"
#include "gui/input/InputProvider.h"
#include "gui/input/TouchEvent.h"
#include "bottomSheetBg.h"
#include "QuickMenuPresenter.h"

QuickMenuPresenter::QuickMenuPresenter(FocusManager* focusManager, StackVramManager* vramManager)
    : _focusManager(focusManager)
    , _vramManager(vramManager)
    , _scrimAnimator(0)
    , _yAnimator(kHiddenY)
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
    if (_currentView)
        return;

    _currentView = std::move(view);
    _initVram = true;
    _scrimTargetBlend = _currentView->GetScrimTargetBlend();
    _yAnimator = Animator<int>(kHiddenY);
    _scrimAnimator = Animator<int>(0);
    _currentView->SetPosition(0, kHiddenY);

    ClearBg1Map();
    BeginOpen();
}

void QuickMenuPresenter::Close()
{
    if (!_currentView || _state == State::Closing)
        return;

    BeginClose();
}

void QuickMenuPresenter::BeginOpen()
{
    _state = State::Opening;
    _scrimAnimator.Goto(_scrimTargetBlend, md::sys::motion::duration::short2,
        &md::sys::motion::easing::linear);
    _yAnimator.Goto(kVisibleY, md::sys::motion::duration::medium2,
        &md::sys::motion::easing::emphasizedDecelerate);

    if (!_oldFocus)
        _oldFocus = _focusManager->GetCurrentFocus();
    _currentView->Focus(*_focusManager);
}

void QuickMenuPresenter::BeginClose()
{
    _state = State::Closing;
    _scrimAnimator.Goto(0, md::sys::motion::duration::short3,
        &md::sys::motion::easing::emphasizedAccelerate);
    _yAnimator.Goto(kHiddenY, md::sys::motion::duration::short3,
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
        if (!_yAnimator.IsFinished())
            _yAnimator.Update();
        if (_yAnimator.IsFinished())
        {
            _state = State::Idle;
            _currentView.reset();
            if (_oldFocus)
            {
                _focusManager->Focus(_oldFocus);
                _oldFocus = nullptr;
            }
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
    if (!_currentView || _state != State::Visible)
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

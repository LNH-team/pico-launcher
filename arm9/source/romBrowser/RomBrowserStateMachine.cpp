#include "common.h"
#include "StateMachineTriggerChecker.h"
#include "RomBrowserStateMachine.h"

void RomBrowserStateMachine::Fire(RomBrowserStateTrigger trigger)
{
    LOG_DEBUG("Fire %d\n", trigger);
    _newTrigger = trigger;
}

bool RomBrowserStateMachine::FireDirect(RomBrowserStateTrigger trigger)
{
    LOG_DEBUG("FireDirect %d\n", trigger);
    RomBrowserState newState;
    if (StateMachineTriggerChecker(_curState, trigger)
        .In(RomBrowserState::Start)
            .Trigger(RomBrowserStateTrigger::Navigate).GoesTo(RomBrowserState::LoadingFolder)
        .In(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::ChangeDisplayMode).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::Navigate).GoesTo(RomBrowserState::LoadingFolder)
            .Trigger(RomBrowserStateTrigger::ShowGameInfo).GoesTo(RomBrowserState::GameInfo)
            .Trigger(RomBrowserStateTrigger::Launch).GoesTo(RomBrowserState::Launching)
            .Trigger(RomBrowserStateTrigger::ShowSearch).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::HideSearch).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::ShowDisplaySettings).GoesTo(RomBrowserState::DisplaySettings)
        .In(RomBrowserState::GameInfo)
            .Trigger(RomBrowserStateTrigger::HideGameInfo).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::LoadingFolder)
            .Trigger(RomBrowserStateTrigger::FolderLoadDone).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::DisplaySettings)
            .Trigger(RomBrowserStateTrigger::ChangeDisplayMode).GoesTo(RomBrowserState::DisplaySettings)
            .Trigger(RomBrowserStateTrigger::HideDisplaySettings).GoesTo(RomBrowserState::Browser)
        .Check(newState))
    {
        _prevState = _curState;
        _curState = newState;
        _lastTrigger = trigger;
        return true;
    }
    return false;
}

void RomBrowserStateMachine::Update()
{
    _stateChanged = false;

    if (_newTrigger == RomBrowserStateTrigger::None)
        return;

    if (!FireDirect(_newTrigger))
    {
        LOG_ERROR("Trigger %d invalid in state %d\n", _newTrigger, _curState);
    }
    else
    {
        _stateChanged = true;
    }

    _newTrigger = RomBrowserStateTrigger::None;
}

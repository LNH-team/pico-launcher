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
            .Trigger(RomBrowserStateTrigger::ShowDisplaySettings).GoesTo(RomBrowserState::DisplaySettings)
        .In(RomBrowserState::GameInfo)
            .Trigger(RomBrowserStateTrigger::HideGameInfo).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::ShowCheats).GoesTo(RomBrowserState::Cheats)
        .In(RomBrowserState::Cheats)
            .Trigger(RomBrowserStateTrigger::HideCheats).GoesTo(RomBrowserState::GameInfo)
            .Trigger(RomBrowserStateTrigger::ShowCheatDescription).GoesTo(RomBrowserState::CheatDescription)
        .In(RomBrowserState::CheatDescription)
            .Trigger(RomBrowserStateTrigger::HideCheatDescription).GoesTo(RomBrowserState::Cheats)
        .In(RomBrowserState::LoadingFolder)
            .Trigger(RomBrowserStateTrigger::FolderLoadDone).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::DisplaySettings)
            .Trigger(RomBrowserStateTrigger::ChangeDisplayMode).GoesTo(RomBrowserState::DisplaySettings)
            .Trigger(RomBrowserStateTrigger::ShowDisplayInfo).GoesTo(RomBrowserState::DisplayInfo)
            .Trigger(RomBrowserStateTrigger::ShowLayoutEditor).GoesTo(RomBrowserState::LayoutEditor)
            .Trigger(RomBrowserStateTrigger::HideDisplaySettings).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::DisplayInfo)
            .Trigger(RomBrowserStateTrigger::HideDisplayInfo).GoesTo(RomBrowserState::DisplaySettings)
        .In(RomBrowserState::LayoutEditor)
            .Trigger(RomBrowserStateTrigger::HideLayoutEditor).GoesTo(RomBrowserState::DisplaySettings)
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
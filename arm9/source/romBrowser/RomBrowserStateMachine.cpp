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
            .Trigger(RomBrowserStateTrigger::ShowRecents).GoesTo(RomBrowserState::Recents)
            .Trigger(RomBrowserStateTrigger::ShowFavorites).GoesTo(RomBrowserState::Favorites)
            .Trigger(RomBrowserStateTrigger::ShowStatistics).GoesTo(RomBrowserState::Statistics)
            .Trigger(RomBrowserStateTrigger::ShowDeleteConfirm).GoesTo(RomBrowserState::DeleteConfirm)
            .Trigger(RomBrowserStateTrigger::ShowMenu).GoesTo(RomBrowserState::Menu)
        // The menu hands over to whatever is picked from it. A filter toggle
        // fires ChangeDisplayMode, which from here also means "and close the menu".
        .In(RomBrowserState::Menu)
            .Trigger(RomBrowserStateTrigger::HideMenu).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::ChangeDisplayMode).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::ShowRecents).GoesTo(RomBrowserState::Recents)
            .Trigger(RomBrowserStateTrigger::ShowFavorites).GoesTo(RomBrowserState::Favorites)
            .Trigger(RomBrowserStateTrigger::ShowStatistics).GoesTo(RomBrowserState::Statistics)
            .Trigger(RomBrowserStateTrigger::ShowDeleteConfirm).GoesTo(RomBrowserState::DeleteConfirm)
            .Trigger(RomBrowserStateTrigger::ShowAbout).GoesTo(RomBrowserState::About)
            .Trigger(RomBrowserStateTrigger::ShowExitCombo).GoesTo(RomBrowserState::ExitCombo)
            .Trigger(RomBrowserStateTrigger::ShowGameInfoEdit).GoesTo(RomBrowserState::GameInfoEdit)
        .In(RomBrowserState::About)
            .Trigger(RomBrowserStateTrigger::HideAbout).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::ExitCombo)
            .Trigger(RomBrowserStateTrigger::HideExitCombo).GoesTo(RomBrowserState::Browser)
        // Saving game info rebuilds the browser (ChangeDisplayMode) so the row
        // and the top screen pick up the new name; B closes without a rebuild.
        .In(RomBrowserState::GameInfoEdit)
            .Trigger(RomBrowserStateTrigger::HideGameInfoEdit).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::ChangeDisplayMode).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::Recents)
            .Trigger(RomBrowserStateTrigger::HideRecents).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::Navigate).GoesTo(RomBrowserState::LoadingFolder)
        .In(RomBrowserState::Favorites)
            .Trigger(RomBrowserStateTrigger::HideFavorites).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::Navigate).GoesTo(RomBrowserState::LoadingFolder)
        .In(RomBrowserState::Statistics)
            .Trigger(RomBrowserStateTrigger::HideStatistics).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::DeleteConfirm)
            .Trigger(RomBrowserStateTrigger::HideDeleteConfirm).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::Navigate).GoesTo(RomBrowserState::LoadingFolder)
        .In(RomBrowserState::GameInfo)
            .Trigger(RomBrowserStateTrigger::HideGameInfo).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::LoadingFolder)
            .Trigger(RomBrowserStateTrigger::FolderLoadDone).GoesTo(RomBrowserState::Browser)
        .In(RomBrowserState::DisplaySettings)
            .Trigger(RomBrowserStateTrigger::ChangeDisplayMode).GoesTo(RomBrowserState::DisplaySettings)
            .Trigger(RomBrowserStateTrigger::HideDisplaySettings).GoesTo(RomBrowserState::Browser)
            .Trigger(RomBrowserStateTrigger::GotoSettingsScreen).GoesTo(RomBrowserState::GoingToSettingsScreen)
        // A launch the console cannot carry out (a DSi-only game on a DS) comes
        // straight back, before anything about it was recorded.
        .In(RomBrowserState::Launching)
            .Trigger(RomBrowserStateTrigger::LaunchRefused).GoesTo(RomBrowserState::Browser)
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
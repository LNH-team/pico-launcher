#include "common.h"
#include "cheats/ICheatRepository.h"
#include "fat/File.h"
#include "CheatsViewModel.h"

CheatsViewModel::CheatsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController)
    : _romFileInfo(romFileInfo), _romBrowserController(romBrowserController)
{
    _categoryStack.fill({ nullptr, 0 });
    _loadCheatsTask = _romBrowserController->GetIoTaskQueue()->Enqueue([this] (const vu8& cancelRequested)
    {
        _cheats = _romBrowserController->GetCheatRepository().GetCheatsForGame(_romFileInfo.GetFastFileRef());
        if (_cheats)
        {
            _categoryStack[0] = { _cheats.get(), 0 };
            _state = State::DisplayCheats;
        }
        else
        {
            _state = State::NoCheats;
        }

        return TaskResult<void>::Completed();
    });
}

void CheatsViewModel::ActivateItem(int index)
{
    if (_state != State::DisplayCheats)
    {
        // No cheats or not yet loaded
        return;
    }

    auto cheatCategory = GetCurrentCheatCategory();
    u32 numberOfSubEntries = 0;
    auto subEntries = cheatCategory->GetSubEntries(numberOfSubEntries);

    if (numberOfSubEntries == 0 || index < 0 || index >= (int)numberOfSubEntries)
    {
        // There is nothing to activate
        return;
    }

    auto& entry = subEntries[index];
    if (entry.IsCheatCategory())
    {
        // Category activated
        if (_categoryStackLevel + 1 != _categoryStack.size())
        {
            _categoryStack[++_categoryStackLevel] = { &entry, (u32)index };
            _selectedItem = 0;
        }
    }
    else
    {
        // Toggle cheat on/off
        bool isEnabled = !entry.GetIsCheatActive();
        if (isEnabled && cheatCategory->GetIsMaxOneCheatActive())
        {
            for (u32 i = 0; i < numberOfSubEntries; i++)
            {
                if (!subEntries[i].IsCheatCategory())
                {
                    subEntries[i].SetIsCheatActive(false);
                }
            }
        }
        entry.SetIsCheatActive(isEnabled);
        _changed = true;
    }
}

bool CheatsViewModel::NavigateUp()
{
    if (_categoryStackLevel == 0)
    {
        Close();
        return false;
    }
    else
    {
        _selectedItem = _categoryStack[_categoryStackLevel].index;
        _categoryStack[_categoryStackLevel--] = { nullptr, 0 };
        return true;
    }
}

void CheatsViewModel::Close()
{
    _categoryStack.fill({ nullptr, 0 });

    if (_changed)
    {
        // Save which cheats are enabled/disabled
        _romBrowserController->GetIoTaskQueue()->Enqueue(
            [romBrowserController = _romBrowserController, cheats = move(_cheats)] (const vu8& cancelRequested)
            {
                romBrowserController->GetCheatRepository().UpdateEnabledCheatsForGame(cheats);
                return TaskResult<void>::Completed();
            });
    }

    _romBrowserController->HideGameInfo();
}

void CheatsViewModel::DisableAllCheats()
{
    if (_state == State::DisplayCheats)
    {
        DisableAllCheats(_cheats.get());
    }
}

void CheatsViewModel::DisableAllCheats(const CheatEntry* cheatCategory)
{
    u32 numberOfSubEntries = 0;
    auto subEntries = cheatCategory->GetSubEntries(numberOfSubEntries);
    for (u32 i = 0; i < numberOfSubEntries; i++)
    {
        auto& entry = subEntries[i];
        if (entry.IsCheatCategory())
        {
            DisableAllCheats(&entry);
        }
        else
        {
            if (entry.GetIsCheatActive())
            {
                entry.SetIsCheatActive(false);
                _changed = true;
            }
        }
    }
}

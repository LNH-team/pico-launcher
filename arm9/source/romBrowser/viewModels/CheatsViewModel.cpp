#include "common.h"
#include "cheats/ICheatRepository.h"
#include "fat/File.h"
#include "CheatsViewModel.h"

CheatsViewModel::CheatsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController)
    : _romFileInfo(romFileInfo), _romBrowserController(romBrowserController)
{
    _categoryStack.fill(nullptr);
    _categoryNameStack.fill(nullptr);
    _loadCheatsTask = _romBrowserController->GetIoTaskQueue()->Enqueue([this] (const vu8& cancelRequested)
    {
        _cheats = _romBrowserController->GetCheatRepository().GetCheatsForGame(_romFileInfo.GetFastFileRef());
        if (_cheats)
        {
            _categoryStack[0] = _cheats.get();
            _state = State::DisplayCheats;
        }
        else
        {
            _state = State::NoCheats;
        }

        return TaskResult<void>::Completed();
    });
}

void CheatsViewModel::ItemActivated()
{
    auto cheatCategory = GetCurrentCheatCategory();
    u32 numberOfCategories = 0;
    auto categories = cheatCategory->GetCategories(numberOfCategories);
    u32 numberOfCheats = 0;
    auto cheats = cheatCategory->GetCheats(numberOfCheats);

    if (_selectedItem < (int)numberOfCategories)
    {
        // Category activated
        if (_categoryStackLevel + 1 != _categoryStack.size())
        {
            _categoryStack[++_categoryStackLevel] = &categories[_selectedItem];
            _categoryNameStack[_categoryStackLevel] = categories[_selectedItem].GetName();
        }
    }
    else
    {
        // Toggle cheat on/off
        auto& cheat = cheats[_selectedItem - numberOfCategories];
        bool isEnabled = !cheat.GetIsCheatActive();
        if (isEnabled && cheatCategory->GetIsMaxOneCheatActive())
        {
            for (u32 i = 0; i < numberOfCheats; i++)
            {
                cheats[i].SetIsCheatActive(false);
            }
        }
        cheat.SetIsCheatActive(isEnabled);
        _changed = true;
    }
}

void CheatsViewModel::Back()
{
    if (_categoryStackLevel == 0)
    {
        Close();
    }
    else
    {
        _categoryNameStack[_categoryStackLevel] = nullptr;
        _categoryStack[_categoryStackLevel--] = nullptr;
    }
}

void CheatsViewModel::Close()
{
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

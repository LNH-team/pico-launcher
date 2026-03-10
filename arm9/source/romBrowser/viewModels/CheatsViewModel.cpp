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
            _isUsrCheatDatMissing = false;
            _state = State::DisplayCheats;
            UpdateRomCheatStatsFromTree();
        }
        else
        {
            FILINFO usrCheatFileInfo;
            _isUsrCheatDatMissing = f_stat("/_pico/extras/usrcheat.dat", &usrCheatFileInfo) != FR_OK;
            _state = State::NoCheats;
            _romActiveCheatCount = 0;
            _romTotalCheatCount = 0;
        }

        return TaskResult<void>::Completed();
    });
}

bool CheatsViewModel::ItemActivated()
{
    if (_selectedOnlyMode)
    {
        if (_selectedItem >= 0 && (u32)_selectedItem < _numberOfSelectedCheats)
        {
            auto& cheat = _selectedCheats[_selectedItem];
            cheat.SetIsCheatActive(!cheat.GetIsCheatActive());
            _changed = true;
        }
        return false;
    }

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
            return true;
        }
    }
    else
    {
        // Toggle cheat on/off
        auto& cheat = cheats[_selectedItem - numberOfCategories];
        bool wasEnabled = cheat.GetIsCheatActive();
        bool isEnabled = !cheat.GetIsCheatActive();
        if (isEnabled && cheatCategory->GetIsMaxOneCheatActive())
        {
            for (u32 i = 0; i < numberOfCheats; i++)
            {
                cheats[i].SetIsCheatActive(false);
            }
        }
        cheat.SetIsCheatActive(isEnabled);
        if (wasEnabled != isEnabled || cheatCategory->GetIsMaxOneCheatActive())
        {
            _changed = true;
            UpdateRomCheatStatsFromTree();
        }
    }

    return false;
}

void CheatsViewModel::DisableAllCheats()
{
    if (_cheats == nullptr)
    {
        return;
    }

    SetCheatsActive(_cheats.get(), false);
    _changed = true;
    _romActiveCheatCount = 0;
    _romTotalCheatCount = CountCheats(_cheats.get());

    if (_selectedOnlyMode)
    {
        BuildSelectedCheatsList();
    }
}

void CheatsViewModel::Back()
{
    if (_selectedOnlyMode)
    {
        return;
    }

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

    _romBrowserController->HideCheats();
}

void CheatsViewModel::SetSelectedOnlyMode(bool selectedOnlyMode)
{
    _selectedOnlyMode = selectedOnlyMode;
    if (_selectedOnlyMode)
    {
        BuildSelectedCheatsList();
    }
}

const char* CheatsViewModel::GetCurrentFolderName() const
{
    if (_selectedOnlyMode)
    {
        return "Selected";
    }

    return _categoryNameStack[_categoryStackLevel];
}

void CheatsViewModel::GetRomCheatStats(u32& activeCount, u32& totalCount) const
{
    activeCount = _romActiveCheatCount;
    totalCount = _romTotalCheatCount;
}

void CheatsViewModel::GetCurrentScopeCheatStats(u32& activeCount, u32& totalCount) const
{
    activeCount = 0;
    totalCount = 0;

    if (_selectedOnlyMode)
    {
        totalCount = _numberOfSelectedCheats;
        activeCount = CountActiveCheats(_selectedCheats.get(), _numberOfSelectedCheats);
        return;
    }

    auto currentCategory = GetCurrentCheatCategory();
    if (currentCategory == nullptr)
    {
        return;
    }

    totalCount = CountCheats(currentCategory);
    activeCount = CountActiveCheats(currentCategory);
}

u32 CheatsViewModel::CountCheats(const ICheatCategory* category) const
{
    u32 total = 0;

    u32 numberOfCategories = 0;
    auto categories = category->GetCategories(numberOfCategories);
    for (u32 i = 0; i < numberOfCategories; i++)
    {
        total += CountCheats(&categories[i]);
    }

    u32 numberOfCheats = 0;
    category->GetCheats(numberOfCheats);
    total += numberOfCheats;

    return total;
}

u32 CheatsViewModel::CountActiveCheats(const ICheatCategory* category) const
{
    u32 total = 0;

    u32 numberOfCategories = 0;
    auto categories = category->GetCategories(numberOfCategories);
    for (u32 i = 0; i < numberOfCategories; i++)
    {
        total += CountActiveCheats(&categories[i]);
    }

    u32 numberOfCheats = 0;
    auto cheats = category->GetCheats(numberOfCheats);
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        if (cheats[i].GetIsCheatActive())
        {
            total++;
        }
    }

    return total;
}

u32 CheatsViewModel::CountActiveCheats(const Cheat* cheats, u32 numberOfCheats) const
{
    u32 total = 0;
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        if (cheats[i].GetIsCheatActive())
        {
            total++;
        }
    }
    return total;
}

void CheatsViewModel::SetCheatsActive(const ICheatCategory* category, bool isActive) const
{
    u32 numberOfCategories = 0;
    auto categories = category->GetCategories(numberOfCategories);
    for (u32 i = 0; i < numberOfCategories; i++)
    {
        SetCheatsActive(&categories[i], isActive);
    }

    u32 numberOfCheats = 0;
    auto cheats = category->GetCheats(numberOfCheats);
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        cheats[i].SetIsCheatActive(isActive);
    }
}

void CheatsViewModel::CopyActiveCheats(const ICheatCategory* category, Cheat* cheats, u32& offset) const
{
    u32 numberOfCategories = 0;
    auto categories = category->GetCategories(numberOfCategories);
    for (u32 i = 0; i < numberOfCategories; i++)
    {
        CopyActiveCheats(&categories[i], cheats, offset);
    }

    u32 numberOfCheats = 0;
    auto categoryCheats = category->GetCheats(numberOfCheats);
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        if (categoryCheats[i].GetIsCheatActive())
        {
            cheats[offset++] = categoryCheats[i];
        }
    }
}

void CheatsViewModel::BuildSelectedCheatsList()
{
    _selectedCheats.reset();
    _numberOfSelectedCheats = 0;

    if (_cheats == nullptr)
    {
        return;
    }

    _numberOfSelectedCheats = CountActiveCheats(_cheats.get());
    if (_numberOfSelectedCheats == 0)
    {
        return;
    }

    _selectedCheats = std::make_unique<Cheat[]>(_numberOfSelectedCheats);
    u32 offset = 0;
    CopyActiveCheats(_cheats.get(), _selectedCheats.get(), offset);
}

void CheatsViewModel::UpdateRomCheatStatsFromTree()
{
    if (_cheats == nullptr)
    {
        _romActiveCheatCount = 0;
        _romTotalCheatCount = 0;
    }
    else
    {
        _romTotalCheatCount = CountCheats(_cheats.get());
        _romActiveCheatCount = CountActiveCheats(_cheats.get());
    }
}

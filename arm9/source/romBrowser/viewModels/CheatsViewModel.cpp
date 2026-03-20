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
            auto cheat = _selectedCheats[_selectedItem];
            cheat->SetIsCheatActive(!cheat->GetIsCheatActive());
            _changed = true;
            UpdateRomCheatStatsFromTree();
        }
        return false;
    }

    auto entry = TryGetCurrentEntry(_selectedItem);
    if (entry == nullptr)
    {
        return false;
    }

    auto cheatCategory = GetCurrentCheatCategory();
    if (entry->IsCheatCategory())
    {
        if (_categoryStackLevel + 1 != _categoryStack.size())
        {
            _categoryStack[++_categoryStackLevel] = entry;
            _categoryNameStack[_categoryStackLevel] = entry->GetName();
            return true;
        }
    }
    else
    {
        bool wasEnabled = entry->GetIsCheatActive();
        bool isEnabled = !entry->GetIsCheatActive();
        if (isEnabled && cheatCategory->GetIsMaxOneCheatActive())
        {
            u32 numberOfSubEntries = 0;
            auto subEntries = cheatCategory->GetSubEntries(numberOfSubEntries);
            for (u32 i = 0; i < numberOfSubEntries; i++)
            {
                if (!subEntries[i].IsCheatCategory())
                {
                    subEntries[i].SetIsCheatActive(false);
                }
            }
        }
        entry->SetIsCheatActive(isEnabled);
        if (wasEnabled != isEnabled || cheatCategory->GetIsMaxOneCheatActive())
        {
            _changed = true;
            UpdateRomCheatStatsFromTree();
        }
    }

    return false;
}

const CheatEntry* CheatsViewModel::TryGetCurrentEntry(int selectedItem) const
{
    if (selectedItem < 0)
    {
        return nullptr;
    }

    auto currentCategory = GetCurrentCheatCategory();
    if (currentCategory == nullptr || !currentCategory->IsCheatCategory())
    {
        return nullptr;
    }

    u32 numberOfSubEntries = 0;
    auto subEntries = currentCategory->GetSubEntries(numberOfSubEntries);
    if ((u32)selectedItem >= numberOfSubEntries)
    {
        return nullptr;
    }

    return &subEntries[selectedItem];
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

u32 CheatsViewModel::CountCheats(const CheatEntry* category) const
{
    if (category == nullptr)
    {
        return 0;
    }

    if (!category->IsCheatCategory())
    {
        return 1;
    }

    u32 total = 0;
    u32 numberOfSubEntries = 0;
    auto subEntries = category->GetSubEntries(numberOfSubEntries);
    for (u32 i = 0; i < numberOfSubEntries; i++)
    {
        total += CountCheats(&subEntries[i]);
    }

    return total;
}

u32 CheatsViewModel::CountActiveCheats(const CheatEntry* category) const
{
    if (category == nullptr)
    {
        return 0;
    }

    if (!category->IsCheatCategory())
    {
        return category->GetIsCheatActive() ? 1 : 0;
    }

    u32 total = 0;
    u32 numberOfSubEntries = 0;
    auto subEntries = category->GetSubEntries(numberOfSubEntries);
    for (u32 i = 0; i < numberOfSubEntries; i++)
    {
        total += CountActiveCheats(&subEntries[i]);
    }

    return total;
}

u32 CheatsViewModel::CountActiveCheats(const CheatEntry* const* cheats, u32 numberOfCheats) const
{
    u32 total = 0;
    for (u32 i = 0; i < numberOfCheats; i++)
    {
        if (cheats[i] != nullptr && cheats[i]->GetIsCheatActive())
        {
            total++;
        }
    }
    return total;
}

void CheatsViewModel::SetCheatsActive(const CheatEntry* category, bool isActive) const
{
    if (category == nullptr)
    {
        return;
    }

    if (!category->IsCheatCategory())
    {
        category->SetIsCheatActive(isActive);
        return;
    }

    u32 numberOfSubEntries = 0;
    auto subEntries = category->GetSubEntries(numberOfSubEntries);
    for (u32 i = 0; i < numberOfSubEntries; i++)
    {
        SetCheatsActive(&subEntries[i], isActive);
    }
}

void CheatsViewModel::CopyActiveCheats(const CheatEntry* category, const CheatEntry** cheats, u32& offset) const
{
    if (category == nullptr)
    {
        return;
    }

    if (!category->IsCheatCategory())
    {
        if (category->GetIsCheatActive())
        {
            cheats[offset++] = category;
        }
        return;
    }

    u32 numberOfSubEntries = 0;
    auto subEntries = category->GetSubEntries(numberOfSubEntries);
    for (u32 i = 0; i < numberOfSubEntries; i++)
    {
        CopyActiveCheats(&subEntries[i], cheats, offset);
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

    _selectedCheats = std::unique_ptr<const CheatEntry*[]>(new const CheatEntry*[_numberOfSelectedCheats]);
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

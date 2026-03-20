#pragma once
#include <array>
#include <memory>
#include "core/task/TaskQueue.h"
#include "cheats/GameCheats.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/IRomBrowserController.h"

class CheatsViewModel
{
public:
    enum class State
    {
        Loading,
        NoCheats,
        DisplayCheats
    };

    CheatsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController);

    bool ItemActivated();
    void DisableAllCheats();
    void Back();
    void Close();
    void SetSelectedOnlyMode(bool selectedOnlyMode);

    State GetState() const { return _state; }
    const CheatEntry* GetCurrentCheatCategory() const { return _categoryStack[_categoryStackLevel]; }
    const char* GetCurrentFolderName() const;
    bool GetIsSelectedOnlyMode() const { return _selectedOnlyMode; }
    bool GetIsUsrCheatDatMissing() const { return _isUsrCheatDatMissing; }
    void GetRomCheatStats(u32& activeCount, u32& totalCount) const;
    void GetCurrentScopeCheatStats(u32& activeCount, u32& totalCount) const;
    const CheatEntry* const* GetSelectedCheats(u32& numberOfCheats) const
    {
        numberOfCheats = _numberOfSelectedCheats;
        return _selectedCheats.get();
    }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

private:
    FileInfo _romFileInfo;
    IRomBrowserController* _romBrowserController;
    QueueTask<void> _loadCheatsTask;
    std::unique_ptr<GameCheats> _cheats;
    State _state = State::Loading;
    int _selectedItem = -1;
    bool _changed = false;
    bool _selectedOnlyMode = false;
    u32 _categoryStackLevel = 0;
    std::array<const CheatEntry*, 8> _categoryStack;
    std::array<const char*, 8> _categoryNameStack;
    std::unique_ptr<const CheatEntry*[]> _selectedCheats;
    u32 _numberOfSelectedCheats = 0;
    bool _isUsrCheatDatMissing = false;

    u32 CountCheats(const CheatEntry* category) const;
    u32 CountActiveCheats(const CheatEntry* category) const;
    u32 CountActiveCheats(const CheatEntry* const* cheats, u32 numberOfCheats) const;
    void SetCheatsActive(const CheatEntry* category, bool isActive) const;
    void CopyActiveCheats(const CheatEntry* category, const CheatEntry** cheats, u32& offset) const;
    void BuildSelectedCheatsList();
    void UpdateRomCheatStatsFromTree();
    const CheatEntry* TryGetCurrentEntry(int selectedItem) const;

    u32 _romActiveCheatCount = 0;
    u32 _romTotalCheatCount = 0;
};

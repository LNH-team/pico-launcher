#pragma once
#include <memory>
#include "fat/File.h"
#include "ICheatRepository.h"
#include "UsrCheatDat.h"

/// @brief Class implementing a cheat repository for usrcheat.dat.
class UsrCheatRepository : public ICheatRepository
{
public:
    UsrCheatRepository(std::unique_ptr<File> usrCheatDatFile,
        std::unique_ptr<usr_cheat_index_entry_t[]> sortedIndices, u32 numberOfIndices)
        : _usrCheatFile(std::move(usrCheatDatFile))
        , _sortedIndices(std::move(sortedIndices)), _numberOfIndices(numberOfIndices) { }

    std::unique_ptr<GameCheats> GetCheatsForGame(const FastFileRef& romFile) const override;
    void UpdateEnabledCheatsForGame(const std::unique_ptr<GameCheats>& cheats) const override;

private:
    std::unique_ptr<File> _usrCheatFile;
    std::unique_ptr<usr_cheat_index_entry_t[]> _sortedIndices;
    u32 _numberOfIndices;

    std::unique_ptr<GameCheats> GetCheatsForGame(u32 gameCode, u32 headerCrc32) const;
    const usr_cheat_index_entry_t* FindIndex(u32 gameCode, u32 headerCrc32) const;
    CheatEntry ParseCategory(u8*& ptr, const u8* endPtr) const;
    CheatEntry ParseCheat(u8*& ptr, const u8* endPtr) const;
};

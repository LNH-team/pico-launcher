#pragma once
#include "CheatEntry.h"

/// @brief Class holding the cheats for a game.
class GameCheats : public CheatEntry
{
public:
    GameCheats(std::unique_ptr<u8[]> cheatData, u32 cheatDataLength, u32 fileOffset, const char* gameName,
        CheatEntry* subEntries, u32 numberOfSubEntries)
        : CheatEntry(gameName, "", false, subEntries, numberOfSubEntries)
        , _cheatData(std::move(cheatData)), _cheatDataLength(cheatDataLength)
        , _fileOffset(fileOffset) { }

    /// @brief Gets a pointer to the cheat data.
    /// @param cheatDataLength The length of the cheat data is returned in this reference.
    /// @return A pointer to the cheat data. This pointer is only valid for the lifetime of this \see GameCheats instance.
    u8* GetCheatData(u32& cheatDataLength) const
    {
        cheatDataLength = _cheatDataLength;
        return _cheatData.get();
    }

    u32 GetFileOffset() const
    {
        return _fileOffset;
    }

private:
    std::unique_ptr<u8[]> _cheatData;
    u32 _cheatDataLength;
    u32 _fileOffset;
};
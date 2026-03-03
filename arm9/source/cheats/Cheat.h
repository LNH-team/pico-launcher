#pragma once
#include "CheatTreeItem.h"

class Cheat : public CheatTreeItem
{
public:
    Cheat()
        : _cheatData(nullptr), _cheatDataLength(0) { }

    Cheat(const char* name, const char* description, u32* flagsPointer, const void* cheatData, u32 cheatDataLength)
        : CheatTreeItem(name, description), _flagsPointer(flagsPointer)
        , _cheatData(cheatData), _cheatDataLength(cheatDataLength) { }

    const void* GetCheatData(u32& cheatDataLength) const
    {
        cheatDataLength = _cheatDataLength;
        return _cheatData;
    }

    bool GetIsCheatActive() const
    {
        return ((*_flagsPointer >> 24) & 1) == 1;
    }

    void SetIsCheatActive(bool isCheatActive) const
    {
        u32 flags = *_flagsPointer;
        flags &= ~(1 << 24);
        if (isCheatActive)
        {
            flags |= 1 << 24;
        }
        *_flagsPointer = flags;
    }

private:
    u32* _flagsPointer;
    const void* _cheatData;
    u32 _cheatDataLength;
};

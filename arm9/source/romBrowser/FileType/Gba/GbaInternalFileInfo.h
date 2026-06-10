#pragma once
#include "../InternalFileInfo.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for gba roms.
class alignas(32) GbaInternalFileInfo : public InternalFileInfo
{
public:
    explicit GbaInternalFileInfo(const FastFileRef& fastFileRef);

    constexpr const char* GetGameCode() const override { return _gameCode; }

private:
    char _gameCode[5];
};

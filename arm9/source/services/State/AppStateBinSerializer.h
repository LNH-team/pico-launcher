#pragma once
#include <memory>
#include "common.h"

struct AppStateBin;

/// @brief  
///
/// Binary format – version 1:
///   [0]      u8   version 
///   [1..64]  char[64]  appliedThemeName  (null-padded, NOT null-terminated in last byte)
///   [65]     u8   primaryColorR
///   [66]     u8   primaryColorG
///   [67]     u8   primaryColorB
///   [68]     u8   darkTheme  (0 = false, 1 = true)
///   [69..72] u32  layoutSlot  (little-endian / native ARM LE)
///   [73..76] u32  numberOfFavorites
///   followed by numberOfFavorites entries, each:
///     u16  length  (character count, no null)
///     char[length]
class AppStateBinSerializer
{
public:
    /// @brief Serialize state to file.  Creates or truncates the file.
    void Serialize(const AppStateBin* state, const char* filePath) const;

    /// @brief Deserialize state from file.
    /// @return true on success, false if file missing / corrupt / wrong version.
    bool Deserialize(AppStateBin* state, const char* filePath) const;

    /// @brief Serialize state to an in-memory buffer (safe to call from main thread).
    /// @param outLength  Receives the number of bytes written.
    std::unique_ptr<u8[]> SerializeToBuffer(const AppStateBin* state, u32& outLength) const;

    /// @brief Write a pre-serialized buffer to disk (safe to call from IO thread).
    void WriteBufferToFile(const u8* data, u32 length, const char* filePath) const;
};

/*
    Adapted from TWiLightMenu
*/

#pragma once
#include <nds/ndstypes.h>
#include <stdlib.h>
#include <string.h>
#include "fat/File.h"

/// @brief Result of parsing usrcheat.dat.
enum class CheatParseResult
{
    Success,           ///< Cheats found and parsed.
    DatFileNotFound,   ///< usrcheat.dat not found on the SD card.
    NoCheatsFound,     ///< usrcheat.dat exists but no cheats for this ROM.
};

/// @brief Represents a single parsed cheat item (folder or cheat entry).
struct CheatItem
{
    enum Flags : u32
    {
        EFolder     = (1 << 0),
        EInFolder   = (1 << 1),
        EOne        = (1 << 2),   
        ESelected   = (1 << 3),
        EOpen       = (1 << 4),
    };

    char name[64];
    char note[128];
    u32 flags;
    long dataOffset; 
    u32* cheatCodes;
    u32 cheatCodeCount;

    CheatItem()
        : flags(0), dataOffset(0), cheatCodes(nullptr), cheatCodeCount(0)
    {
        memset(name, 0, sizeof(name));
        memset(note, 0, sizeof(note));
    }

    CheatItem(const char* aName, const char* aNote, u32 aFlags, long aOffset = 0)
        : flags(aFlags), dataOffset(aOffset), cheatCodes(nullptr), cheatCodeCount(0)
    {
        strncpy(name, aName ? aName : "", sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;
        strncpy(note, aNote ? aNote : "", sizeof(note) - 1);
        note[sizeof(note) - 1] = 0;
    }

    ~CheatItem()
    {
        free(cheatCodes);
    }

    CheatItem(CheatItem&& other)
        : flags(other.flags), dataOffset(other.dataOffset)
        , cheatCodes(other.cheatCodes), cheatCodeCount(other.cheatCodeCount)
    {
        memcpy(name, other.name, sizeof(name));
        memcpy(note, other.note, sizeof(note));
        other.cheatCodes = nullptr;
        other.cheatCodeCount = 0;
    }

    CheatItem& operator=(CheatItem&& other)
    {
        if (this != &other)
        {
            free(cheatCodes);
            memcpy(name, other.name, sizeof(name));
            memcpy(note, other.note, sizeof(note));
            flags = other.flags;
            dataOffset = other.dataOffset;
            cheatCodes = other.cheatCodes;
            cheatCodeCount = other.cheatCodeCount;
            other.cheatCodes = nullptr;
            other.cheatCodeCount = 0;
        }
        return *this;
    }

    CheatItem(const CheatItem&) = delete;
    CheatItem& operator=(const CheatItem&) = delete;

    void SetCheatCodes(const u32* data, u32 count)
    {
        free(cheatCodes);
        cheatCodeCount = count;
        if (count > 0)
        {
            cheatCodes = (u32*)malloc(count * sizeof(u32));
            memcpy(cheatCodes, data, count * sizeof(u32));
        }
        else
        {
            cheatCodes = nullptr;
        }
    }

    bool IsFolder() const { return (flags & EFolder) != 0; }
    bool IsInFolder() const { return (flags & EInFolder) != 0; }
    bool IsSelected() const { return (flags & ESelected) != 0; }

    void ToggleSelected()
    {
        flags ^= ESelected;
    }

    void SetSelected(bool selected)
    {
        if (selected) flags |= ESelected;
        else flags &= ~ESelected;
    }

    void ToggleOpen()
    {
        flags ^= EOpen;
    }

    bool IsOpen() const { return (flags & EOpen) != 0; }
};

/// @brief Simple dynamic array for CheatItems (avoids std::vector to prevent
///        libstdc++ exception handling code from being linked in).
struct CheatItemArray
{
    CheatItem* data;
    int count;
    int capacity;

    CheatItemArray() : data(nullptr), count(0), capacity(0) {}
    ~CheatItemArray() { Clear(); }

    CheatItemArray(const CheatItemArray&) = delete;
    CheatItemArray& operator=(const CheatItemArray&) = delete;

    void Clear()
    {
        for (int i = 0; i < count; i++)
            data[i].~CheatItem();
        free(data);
        data = nullptr;
        count = 0;
        capacity = 0;
    }

    void Push(CheatItem&& item)
    {
        if (count >= capacity)
        {
            int newCap = capacity == 0 ? 16 : capacity * 2;
            auto* newData = (CheatItem*)malloc(newCap * sizeof(CheatItem));
            for (int i = 0; i < count; i++)
            {
                new (&newData[i]) CheatItem(static_cast<CheatItem&&>(data[i]));
                data[i].~CheatItem();
            }
            free(data);
            data = newData;
            capacity = newCap;
        }
        new (&data[count]) CheatItem(static_cast<CheatItem&&>(item));
        count++;
    }

    CheatItem& operator[](int i) { return data[i]; }
    const CheatItem& operator[](int i) const { return data[i]; }
    bool empty() const { return count == 0; }
    int size() const { return count; }
};

/// @brief Simple dynamic int array (avoids std::vector).
struct IntArray
{
    int* data;
    int count;
    int capacity;

    IntArray() : data(nullptr), count(0), capacity(0) {}
    ~IntArray() { free(data); }

    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;

    void Clear()
    {
        count = 0;
    }

    void Push(int val)
    {
        if (count >= capacity)
        {
            int newCap = capacity == 0 ? 16 : capacity * 2;
            data = (int*)realloc(data, newCap * sizeof(int));
            capacity = newCap;
        }
        data[count++] = val;
    }

    int operator[](int i) const { return data[i]; }
    int size() const { return count; }
};

/// @brief Parser for the usrcheat.dat (R4 cheat database format).
class CheatCodelist
{
public:
    /// @brief Parses the usrcheat.dat file for cheats matching the given ROM.
    /// @param romFastFileRef The fast file ref to the NDS ROM file.
    /// @return CheatParseResult indicating success or reason of failure.
    CheatParseResult Parse(const FastFileRef& romFastFileRef);

    /// @brief Gets all parsed cheat items.
    const CheatItemArray& GetItems() const { return _items; }

    /// @brief Gets mutable access to cheat items (for toggling selection).
    CheatItemArray& GetItems() { return _items; }

    /// @brief Builds a flat list of visible indices for display.
    ///        Folders that are closed hide their children.
    void BuildVisibleList();

    /// @brief Gets the visible item indices.
    const IntArray& GetVisibleIndices() const { return _visibleIndices; }

    /// @brief Returns the number of visible items.
    int GetVisibleCount() const { return _visibleIndices.size(); }

    /// @brief Gets a visible item by its display index.
    CheatItem& GetVisibleItem(int visibleIndex) { return _items[_visibleIndices[visibleIndex]]; }
    const CheatItem& GetVisibleItem(int visibleIndex) const { return _items[_visibleIndices[visibleIndex]]; }

    /// @brief Counts the total number of selected cheat codes (u32 words).
    u32 GetSelectedCheatCodeCount() const;

    /// @brief Copies selected cheat codes into the provided buffer.
    /// @param outBuf Output buffer (must be large enough).
    /// @return Number of u32 words written.
    u32 CopySelectedCheats(u32* outBuf) const;

    /// @brief Gets the gamecode string (4 chars).
    const char* GetGameCode() const { return _gameCode; }

    /// @brief Gets the CRC32 as u32.
    u32 GetCRC32() const { return _crc32; }

    /// @brief Return true if current view is inside a folder (folder items visible).
    bool IsInsideFolder(int visibleIndex) const;
    
    /// @brief Build visible list for specific folder view (or root).
    void BuildVisibleListForFolder(int folderIndex = -1);

    /// @brief Build visible list showing ONLY enabled cheats.
    void BuildVisibleListEnabledOnly();

    /// @brief Saves the current selections back to the main usrcheat.dat file.
    /// @param usrCheatPath Path to the usrcheat.dat file.
    /// @return True on success.
    bool UpdateUsrCheatDat(const char* usrCheatPath);

    /// @brief Returns whether any cheats were found.
    bool HasCheats() const { return !_items.empty(); }

    /// @brief Returns whether the visible list is currently in enabled-only mode.
    bool IsEnabledListMode() const { return _isEnabledListMode; }

public:
    struct DatIndex
    {
        u32 _gameCode;
        u32 _crc32;
        u64 _offset;
    };

    CheatItemArray _items;
    IntArray _visibleIndices;
    char _gameCode[5] = {};
    u32 _crc32 = 0;
    bool _isEnabledListMode = false;

    static u32 ComputeCrc32(const u8* data, u32 length);
    static bool ReadRomData(const FastFileRef& romFastFileRef, u32& outGameCode, u32& outCrc32);
    bool SearchCheatData(File& datFile, u32 gamecode, u32 crc32, long& pos, u32& size);
    bool ParseCheatData(File& datFile, u32 gamecode, u32 crc32);
};

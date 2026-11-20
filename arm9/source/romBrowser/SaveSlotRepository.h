#pragma once
#include <memory>

/// @brief Stores the selected save slot per rom, in the style of the Acekard 2i.
///        Slot 0 uses the default save file (rom.sav), slot N (1-9) uses rom.N.sav.
///        The selection is persisted as a text file with one "N<TAB>/path/to/rom.nds" line per rom.
class SaveSlotRepository
{
public:
    static constexpr u32 MaxSlot = 9;
    static constexpr u32 MaxEntries = 512;

    /// @brief Loads the save slot selections from the file at \p filePath. Must be called from the IO thread.
    void Load(const char* filePath);

    /// @brief Gets the selected save slot for the rom at \p romPath.
    /// @return The save slot, or 0 when the default save is used.
    u32 GetSlot(const char* romPath) const;

    /// @brief Sets the selected save slot for the rom at \p romPath.
    void SetSlot(const char* romPath, u32 slot);

    /// @brief Returns whether there are unsaved changes.
    bool IsDirty() const { return _dirty; }

    /// @brief Serializes the selections to a text buffer and clears the dirty flag.
    ///        Intended to be called from the UI thread, after which the buffer can be written by the IO thread.
    std::unique_ptr<char[]> Serialize(u32& length);

    /// @brief Writes a buffer created by \see Serialize to \p filePath. Must be called from the IO thread.
    static void WriteFile(const char* filePath, const char* data, u32 length);

    /// @brief Builds the save path for \p romPath and \p slot into \p savePath,
    ///        mirroring the default save path logic of Pico Loader.
    static bool TryCreateSavePath(const char* romPath, u32 slot, char* savePath, u32 savePathSize);

    /// @brief Builds the save path for \p romPath and \p slot, where slot 0 gives rom.sav.
    static bool CreateSavePath(const char* romPath, u32 slot, char* savePath, u32 savePathSize);

    /// @brief Checks which save files exist for \p romPath. Must be called from the IO thread.
    /// @return A mask with bit N set when the save file of slot N exists.
    static u16 GetExistingSavesMask(const char* romPath);

private:
    struct Entry
    {
        std::unique_ptr<char[]> romPath;
        u8 slot;
    };

    std::unique_ptr<Entry[]> _entries = std::make_unique<Entry[]>(MaxEntries);
    u32 _entryCount = 0;
    bool _dirty = false;

    int FindEntry(const char* romPath) const;
    void AddEntry(const char* romPath, u32 romPathLength, u32 slot);
};

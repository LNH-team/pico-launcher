#pragma once
#include "LayoutData.h"

// Maximum number of layout slots 
#define LAYOUT_MAX_SLOTS 9

/// Service for loading and saving layout slot files.
/// File path: /_pico/extras/layouts/layoutN.bin  (N = 1-based slot number)
class LayoutService
{
public:
    /// Initialize: scan available slots, load the one specified by slotIndex.
    /// If no slots exist, creates a default layout1.bin.
    /// @param slotIndex  1-based slot number to load (0 = use slot 1)
    void Initialize(u32 slotIndex);

    /// Returns the currently active layout data.
    const LayoutData& GetCurrentLayout() const { return _currentLayout; }

    /// Returns a mutable reference so the editor can modify it.
    LayoutData& GetCurrentLayoutMutable() { return _currentLayout; }

    /// Returns the currently active slot number (1-based).
    u32 GetCurrentSlot() const { return _currentSlot; }

    /// Returns the total number of available slots (at least 1).
    u32 GetSlotCount() const { return _slotCount; }

    /// Switch to a different slot (1-based). Loads its data.
    /// @param slot  1-based slot number (clamped to valid range)
    void SetCurrentSlot(u32 slot);

    /// Save the current layout data to the current slot file.
    /// @return true on success.
    bool SaveCurrentSlot();

    /// Reset the current slot to default values (does NOT save automatically).
    void ResetCurrentSlot();

    /// Reload the current slot from disk.
    void ReloadCurrentSlot();

private:
    LayoutData _currentLayout;
    u32 _currentSlot = 1;
    u32 _slotCount   = 1;

    /// Scan the layouts directory to count how many layout files exist.
    void ScanSlots();

    /// Load a specific slot. Returns true on success.
    bool LoadSlot(u32 slot);

    /// Write the layout data to a specific slot file.
    bool SaveSlot(u32 slot, const LayoutData& data);

    /// Ensure the layout directory exists (create it if needed).
    static void EnsureDirectory();
};

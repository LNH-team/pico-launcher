#pragma once
#include "common.h"
#include "PaletteManager.h"

#define HW_PALETTE_ROWS 16

class AdvancedPaletteManagerBase : public PaletteManager
{
public:
    void Reset();
    void VCount();

    void VBlank()
    {
        _curSchemeEntry = _nextSchemeEntry;
        VCount();
    }

protected:
    struct PaletteRow
    {
        u16 colors[16];
        u8 startY;
        u8 endY;
        u8 hwRow;
        u8 prev;
        u32 next;
        u32 hash;

        bool ColorsEqualTo(const PaletteRow& other) const;
    };

    union SchemeEntry
    {
        struct
        {
            const u16* src;
            vu16* dst;
        };
        struct
        {
            u32 endMarker;
            u32 nextVCount;
        };
    };

    explicit AdvancedPaletteManagerBase(vu16* targetPalette)
        : _targetPalette(targetPalette), _curSchemeEntry(nullptr), _nextSchemeEntry(nullptr)
    {
        Reset();
    }

    u32 AllocRowInternal(PaletteRow* rows, const IPalette& palette, int yStart, int yEnd);
    void CreateScheme(const PaletteRow* rows, SchemeEntry* scheme);

private:
    struct HwPaletteRow
    {
        u8 count;
        u8 first;

        int FindPlace(const PaletteRow* rows, u32 yStart, u32 yEnd) const;
    };

    u32 _usedRows = 0;
    HwPaletteRow _hwRows[HW_PALETTE_ROWS];
    vu16* _targetPalette;
    const SchemeEntry* _curSchemeEntry;
    const SchemeEntry* _nextSchemeEntry;

    u32 TryMerge(PaletteRow* rows, const PaletteRow& newRow, int yStart, int yEnd);
};

/// @brief Advanced palette manager that manages dynamic palette rows that are valid on a range of scanlines.
///        By reloading the palette during display this allows to use more than 16 palette rows in total
///        as long as only up to 16 are in use on a scanline.
/// @tparam MaxPaletteRows The maximum number of palette rows to manage.
template <u32 MaxPaletteRows>
class AdvancedPaletteManager : public AdvancedPaletteManagerBase
{
public:
    explicit AdvancedPaletteManager(vu16* targetPalette)
        : AdvancedPaletteManagerBase(targetPalette) { }

    u32 AllocRow(const IPalette& palette, int yStart, int yEnd) override
    {
        yEnd++; // avoid back to back allocation
        return AllocRowInternal(_rows[_curSet], palette, yStart, yEnd);
    }

    void EndOfFrame()
    {
        CreateScheme(_rows[_curSet], _scheme[_curSet]);
        _curSet = 1 - _curSet;
        Reset();
    }

private:
    u32 _curSet = 0;
    PaletteRow _rows[2][MaxPaletteRows];
    SchemeEntry _scheme[2][MaxPaletteRows * 2];
};

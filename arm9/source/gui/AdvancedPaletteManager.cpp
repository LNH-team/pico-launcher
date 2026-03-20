#include "common.h"
#include <libtwl/gfx/gfxStatus.h>
#include <algorithm>
#include "AdvancedPaletteManager.h"

bool AdvancedPaletteManagerBase::PaletteRow::ColorsEqualTo(const PaletteRow& other) const
{
    if (hash != other.hash)
    {
        return false;
    }

    for (int i = 0; i < 16; i++)
    {
        if (colors[i] != other.colors[i])
        {
            return false;
        }
    }

    return true;
}

int AdvancedPaletteManagerBase::HwPaletteRow::FindPlace(const PaletteRow* rows, u32 yStart, u32 yEnd) const
{
    if (count == 0)
    {
        return 0xFF;
    }
    u32 idx = first;
    u32 prevIdx = 0xFF;
    while (idx != 0xFF && rows[idx].endY < yStart)
    {
        prevIdx = idx;
        idx = rows[idx].next;
    }

    if (idx == 0xFF || rows[idx].startY >= yEnd)
    {
        return prevIdx;
    }

    return -1;
}

u32 AdvancedPaletteManagerBase::TryMerge(PaletteRow* rows, const PaletteRow& newRow, int yStart, int yEnd)
{
    for (u32 i = 0; i < _usedRows; i++)
    {
        if (rows[i].ColorsEqualTo(newRow))
        {
            if (yStart < rows[i].startY)
            {
                u32 prev = rows[i].prev;
                if (prev != 0xFF && yStart - rows[prev].endY <= 1)
                {
                    continue;
                }
            }

            if (yEnd > rows[i].endY)
            {
                u32 next = rows[i].next;
                if (next != 0xFF && rows[next].startY - yEnd <= 1)
                {
                    continue;
                }
            }

            // LOG_DEBUG("Could merge [%d,%d) into [%d,%d)\n", yStart, yEnd, rows[i].startY, rows[i].endY);

            if ((yEnd < rows[i].startY && rows[i].startY - yEnd > 1) ||
                (yStart > rows[i].endY && yStart - rows[i].endY > 1))
            {
                continue;
            }

            // LOG_DEBUG("Merging [%d,%d) into [%d,%d)\n", yStart, yEnd, rows[i].startY, rows[i].endY);

            // merging
            rows[i].startY = std::min((int)rows[i].startY, yStart);
            rows[i].endY = std::max((int)rows[i].endY, yEnd);

            return rows[i].hwRow;
        }
    }
    return PALETTE_MANAGER_ROW_IDX_INVALID;
}

u32 AdvancedPaletteManagerBase::AllocRowInternal(PaletteRow* rows, const IPalette& palette, int yStart, int yEnd)
{
    yStart = std::clamp(yStart, 0, 192);
    yEnd = std::clamp(yEnd, 0, 192);

    u32 newIdx = _usedRows;
    PaletteRow& newRow = rows[newIdx];
    newRow.hash = palette.GetHashCode();
    palette.GetColors(newRow.colors);

    u32 rowIdx = TryMerge(rows, newRow, yStart, yEnd);
    if (rowIdx != PALETTE_MANAGER_ROW_IDX_INVALID)
    {
        return rowIdx;
    }

    _usedRows++;

    int bestSlack = 0x100;//-1;
    u32 bestPrevIdx = 0xFF;
    u32 bestHwRow = 0xFF;
    for (int i = 0; i < HW_PALETTE_ROWS; i++)
    {
        int result = _hwRows[i].FindPlace(rows, yStart, yEnd);
        if (result < 0)
        {
            continue;
        }
        int slack = result >= 0xFF ? result : yStart - rows[result].endY;
        if (slack /*>*/ < bestSlack && slack > 1)
        {
            bestSlack = slack;
            bestPrevIdx = result;
            bestHwRow = i;
        }
    }

    rowIdx = bestHwRow;

    if (bestHwRow == 0xFF)
    {
        LOG_FATAL("Failed to allocate palette row\n");
        while (true);
        return PALETTE_MANAGER_ROW_IDX_INVALID;
    }

    // LOG_DEBUG("Allocated hw palette row %d for y range %d - %d\n", bestHwRow, yStart, yEnd);

    newRow.startY = yStart;
    newRow.endY = yEnd;

    if (bestPrevIdx >= 0xFF)
    {
        newRow.next = _hwRows[bestHwRow].first;
        if (newRow.next != 0xFF)
        {
            rows[newRow.next].prev = newIdx;
        }
        newRow.prev = 0xFF;
        _hwRows[bestHwRow].first = newIdx;
    }
    else
    {
        newRow.prev = bestPrevIdx;
        newRow.next = rows[bestPrevIdx].next;
        rows[bestPrevIdx].next = newIdx;
        if (newRow.next != 0xFF)
        {
            rows[newRow.next].prev = newIdx;
        }
    }
    _hwRows[bestHwRow].count++;

    newRow.hwRow = rowIdx;

    return rowIdx;
}

void AdvancedPaletteManagerBase::Reset()
{
    _usedRows = 0;
    for (int i = 0; i < HW_PALETTE_ROWS; i++)
    {
        _hwRows[i].count = 0;
        _hwRows[i].first = 0xFF;
    }
}

void AdvancedPaletteManagerBase::CreateScheme(const PaletteRow* rows, SchemeEntry* scheme)
{
    _nextSchemeEntry = scheme;
    u32 y = 0;
    u8 curRow[HW_PALETTE_ROWS];
    for (int i = 0; i < HW_PALETTE_ROWS; i++)
    {
        curRow[i] = _hwRows[i].first;
    }
    while (y != 0xFF)
    {
        u32 minY = 0xFF;
        for (int i = 0; i < HW_PALETTE_ROWS; i++)
        {
            if (curRow[i] == 0xFF)
            {
                continue;
            }
            const PaletteRow& row = rows[curRow[i]];
            u32 uploadLine = row.prev == 0xFF ? 0 : rows[row.prev].endY;
            if (uploadLine == y)
            {
                // LOG_DEBUG("At %d upload row %d (%d-%d) to hw row %d\n", uploadLine, curRow[i], row.startY, row.endY, i);
                scheme->src = row.colors;
                scheme->dst = _targetPalette + i * 16;
                scheme++;
                curRow[i] = row.next;
                uploadLine = row.endY;
            }

            if (curRow[i] == 0xFF)
            {
                continue;
            }

            minY = std::min(minY, uploadLine);
        }
        if (y != minY)
        {
            scheme->endMarker = 0;
            scheme->nextVCount = minY == 255 ? 0 : minY;
            scheme++;
        }
        y = minY;
    }

    scheme->endMarker = 0;
    scheme->nextVCount = 0;
}

void AdvancedPaletteManagerBase::VCount()
{
    while (_curSchemeEntry->endMarker != 0)
    {
        struct palette_row_t
        {
            u32 data[8];
        };
        *(palette_row_t*)_curSchemeEntry->dst = *(palette_row_t*)_curSchemeEntry->src;

        _curSchemeEntry++;
    }

    if (_curSchemeEntry->nextVCount == 0)
    {
        gfx_setVCountMatchIrqEnabled(false);
    }
    else
    {
        // LOG_DEBUG("vcount %d\n", _curSchemeEntry->nextVCount);
        gfx_setVCountMatchLine(_curSchemeEntry->nextVCount);
        gfx_setVCountMatchIrqEnabled(true);
        _curSchemeEntry++;
    }
}
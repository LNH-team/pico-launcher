#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "core/String.h"
#include "BgmListItemView.h"

class BgmAdapter : public RecyclerAdapter
{
public:
    // Item type constants for GetBgmFileIndex
    static constexpr int kItemRandom = -1;
    static constexpr int kItemCategory = -2;

    BgmAdapter(const String<char, 128>* bgmFileNames, int bgmFileCount,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        int currentBgmIndex = -1)
        : _bgmFileNames(bgmFileNames)
        , _bgmFileCount(bgmFileCount)
        , _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository)
        , _currentBgmIndex(currentBgmIndex)
        , _3dsExpanded(true)
        , _dsiExpanded(true)
        , _3dsStart(0), _3dsCount(0)
        , _dsiStart(0), _dsiCount(0)
        , _otherStart(0), _otherCount(0)
    {
        ClassifyFiles();
    }

    u32 GetItemCount() const override
    {
        u32 count = 1; // Random
        if (_3dsCount > 0)
            count += 1 + (_3dsExpanded ? _3dsCount : 0); // header + items
        if (_dsiCount > 0)
            count += 1 + (_dsiExpanded ? _dsiCount : 0); // header + items
        count += _otherCount; // other files (no header)
        return count;
    }

    void GetViewSize(int& width, int& height) const override
    {
        width = 224;
        height = 16;
    }

    View* CreateView() const override
    {
        return new BgmListItemView(_materialColorScheme, _fontRepository);
    }

    void DestroyView(View* view) const override
    {
        delete static_cast<BgmListItemView*>(view);
    }

    void BindView(View* view, int index) const override
    {
        auto* item = static_cast<BgmListItemView*>(view);

        int flatIdx = index;
        char16_t buf[64];
        int pos = 0;
        bool isCategory = false;
        bool isCurrent = false;
        int fileIdx = -1;

        // Decode flat index
        // 0: Random
        if (flatIdx == 0)
        {
            isCurrent = (_currentBgmIndex < 0);
            if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
            const char16_t* r = u"Random";
            for (int i = 0; r[i] && pos < 62; i++)
                buf[pos++] = r[i];
            buf[pos] = 0;
            item->SetIsCategory(false);
            item->SetCurrentlyPlaying(isCurrent);
            item->SetText(buf);
            return;
        }
        flatIdx--; // consumed Random

        // 3DS group
        if (_3dsCount > 0)
        {
            if (flatIdx == 0)
            {
                // 3DS category header
                isCategory = true;
                const char16_t* arrow = _3dsExpanded ? u"\x19 3DS" : u"\x1A 3DS";
                for (int i = 0; arrow[i] && pos < 62; i++)
                    buf[pos++] = arrow[i];
                buf[pos] = 0;
                item->SetIsCategory(true);
                item->SetCurrentlyPlaying(false);
                item->SetText(buf);
                return;
            }
            flatIdx--; // consumed 3DS header

            if (_3dsExpanded)
            {
                if (flatIdx < _3dsCount)
                {
                    fileIdx = _3dsStart + flatIdx;
                    isCurrent = (fileIdx == _currentBgmIndex);
                    if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
                    // Strip "3DS_" prefix, then replace '_' with ' '
                    const char* name = _bgmFileNames[fileIdx].GetString();
                    int j = 4; // skip "3DS_"
                    while (name[j] && name[j] != '.' && pos < 62)
                    {
                        buf[pos++] = (name[j] == '_') ? u' ' : (char16_t)(unsigned char)name[j];
                        j++;
                    }
                    buf[pos] = 0;
                    item->SetIsCategory(false);
                    item->SetCurrentlyPlaying(isCurrent);
                    item->SetText(buf);
                    return;
                }
                flatIdx -= _3dsCount;
            }
        }

        // DSi group
        if (_dsiCount > 0)
        {
            if (flatIdx == 0)
            {
                // DSi category header
                isCategory = true;
                const char16_t* arrow = _dsiExpanded ? u"\x19 DSi" : u"\x1A DSi";
                for (int i = 0; arrow[i] && pos < 62; i++)
                    buf[pos++] = arrow[i];
                buf[pos] = 0;
                item->SetIsCategory(true);
                item->SetCurrentlyPlaying(false);
                item->SetText(buf);
                return;
            }
            flatIdx--; // consumed DSi header

            if (_dsiExpanded)
            {
                if (flatIdx < _dsiCount)
                {
                    fileIdx = _dsiStart + flatIdx;
                    isCurrent = (fileIdx == _currentBgmIndex);
                    if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
                    // Strip "DSi_" prefix, then replace '_' with ' '
                    const char* name = _bgmFileNames[fileIdx].GetString();
                    int j = 4; // skip "DSi_"
                    while (name[j] && name[j] != '.' && pos < 62)
                    {
                        buf[pos++] = (name[j] == '_') ? u' ' : (char16_t)(unsigned char)name[j];
                        j++;
                    }
                    buf[pos] = 0;
                    item->SetIsCategory(false);
                    item->SetCurrentlyPlaying(isCurrent);
                    item->SetText(buf);
                    return;
                }
                flatIdx -= _dsiCount;
            }
        }

        // Other files (no header)
        if (flatIdx < _otherCount)
        {
            fileIdx = _otherStart + flatIdx;
            isCurrent = (fileIdx == _currentBgmIndex);
            if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
            const char* name = _bgmFileNames[fileIdx].GetString();
            int j = 0;
            while (name[j] && name[j] != '.' && pos < 62)
            {
                buf[pos++] = (name[j] == '_') ? u' ' : (char16_t)(unsigned char)name[j];
                j++;
            }
            buf[pos] = 0;
            item->SetIsCategory(false);
            item->SetCurrentlyPlaying(isCurrent);
            item->SetText(buf);
            return;
        }

        // Fallback (shouldn't happen)
        buf[0] = 0;
        item->SetIsCategory(false);
        item->SetCurrentlyPlaying(false);
        item->SetText(buf);
    }

    void ReleaseView(View* view, int index) const override
    {
    }

    /// Returns true if the item at the given flat index is a category header.
    bool IsCategoryItem(int index) const
    {
        if (index == 0) return false; // Random
        int flatIdx = index - 1;

        if (_3dsCount > 0)
        {
            if (flatIdx == 0) return true; // 3DS header
            flatIdx--;
            if (_3dsExpanded) flatIdx -= _3dsCount;
            if (flatIdx < 0) return false;
        }

        if (_dsiCount > 0)
        {
            if (flatIdx == 0) return true; // DSi header
        }

        return false;
    }

    /// Toggles the expand/collapse state for the category at the given flat index.
    void ToggleCategory(int index)
    {
        if (index == 0) return;
        int flatIdx = index - 1;

        if (_3dsCount > 0)
        {
            if (flatIdx == 0) { _3dsExpanded = !_3dsExpanded; return; }
            flatIdx--;
            if (_3dsExpanded) flatIdx -= _3dsCount;
            if (flatIdx < 0) return;
        }

        if (_dsiCount > 0)
        {
            if (flatIdx == 0) { _dsiExpanded = !_dsiExpanded; return; }
        }
    }

    /// Converts a flat list index to the file index in _bgmFileNames.
    /// Returns kItemRandom (-1) for Random, kItemCategory (-2) for category headers.
    int GetBgmFileIndex(int index) const
    {
        if (index == 0) return kItemRandom;
        int flatIdx = index - 1;

        if (_3dsCount > 0)
        {
            if (flatIdx == 0) return kItemCategory; // 3DS header
            flatIdx--;
            if (_3dsExpanded)
            {
                if (flatIdx < _3dsCount) return _3dsStart + flatIdx;
                flatIdx -= _3dsCount;
            }
        }

        if (_dsiCount > 0)
        {
            if (flatIdx == 0) return kItemCategory; // DSi header
            flatIdx--;
            if (_dsiExpanded)
            {
                if (flatIdx < _dsiCount) return _dsiStart + flatIdx;
                flatIdx -= _dsiCount;
            }
        }

        if (flatIdx < _otherCount)
            return _otherStart + flatIdx;

        return kItemCategory; // shouldn't happen
    }

private:
    const String<char, 128>* _bgmFileNames;
    int _bgmFileCount;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    int _currentBgmIndex; // -1 = Random is current

    bool _3dsExpanded;
    bool _dsiExpanded;
    int _3dsStart, _3dsCount;
    int _dsiStart, _dsiCount;
    int _otherStart, _otherCount;

    /// Scans _bgmFileNames and classifies them into 3DS, DSi, and other groups.
    /// Assumes files are already sorted by name (3DS_ first, then DSi_, then others).
    void ClassifyFiles()
    {
        _3dsStart = 0;
        _3dsCount = 0;
        _dsiStart = 0;
        _dsiCount = 0;
        _otherStart = 0;
        _otherCount = 0;

        // First pass: count each category
        for (int i = 0; i < _bgmFileCount; i++)
        {
            const char* name = _bgmFileNames[i].GetString();
            if (name[0] == '3' && name[1] == 'D' && name[2] == 'S' && name[3] == '_')
                _3dsCount++;
            else if (name[0] == 'D' && name[1] == 'S' && name[2] == 'i' && name[3] == '_')
                _dsiCount++;
            else
                _otherCount++;
        }

        // Compute start indices (files should already be grouped by prefix due to alphabetical sort)
        _3dsStart = 0;
        _dsiStart = _3dsStart + _3dsCount;
        _otherStart = _dsiStart + _dsiCount;
    }
};

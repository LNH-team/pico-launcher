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

    struct Category
    {
        const char* prefix;      // File name prefix, e.g. "3DS_"
        int prefixLen;           // Prefix length
        const char16_t* nameEn;  // English category name
        const char16_t* nameCn;  // Chinese category name (unused for now, same as En)
        int start;
        int count;
        bool expanded;
    };

    static constexpr int kMaxCategories = 8;

    BgmAdapter(const String<char, 128>* bgmFileNames, int bgmFileCount,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        int currentBgmIndex = -1, const char16_t* randomText = u"Random")
        : _bgmFileNames(bgmFileNames)
        , _bgmFileCount(bgmFileCount)
        , _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository)
        , _currentBgmIndex(currentBgmIndex)
        , _randomText(randomText)
        , _categoryCount(0)
        , _otherStart(0), _otherCount(0)
    {
        ClassifyFiles();
    }

    u32 GetItemCount() const override
    {
        u32 count = 1; // Random
        for (int c = 0; c < _categoryCount; c++)
        {
            if (_categories[c].count > 0)
                count += 1 + (_categories[c].expanded ? _categories[c].count : 0);
        }
        count += _otherCount;
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
        item->SetIndentLevel(0);

        int flatIdx = index;
        char16_t buf[64];
        int pos = 0;
        bool isCurrent = false;

        // 0: Random
        if (flatIdx == 0)
        {
            isCurrent = (_currentBgmIndex < 0);
            if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
            for (int i = 0; _randomText[i] && pos < 62; i++)
                buf[pos++] = _randomText[i];
            buf[pos] = 0;
            item->SetIsCategory(false);
            item->SetCurrentlyPlaying(isCurrent);
            item->SetText(buf);
            return;
        }
        flatIdx--;

        // Category groups
        for (int c = 0; c < _categoryCount; c++)
        {
            const Category& cat = _categories[c];
            if (cat.count <= 0)
                continue;

            // Category header
            if (flatIdx == 0)
            {
                buf[pos++] = cat.expanded ? u'-' : u'+';
                buf[pos++] = u' ';
                const char16_t* name = cat.nameEn;
                for (int i = 0; name[i] && pos < 62; i++)
                    buf[pos++] = name[i];
                buf[pos] = 0;
                item->SetIsCategory(true);
                item->SetCurrentlyPlaying(false);
                item->SetText(buf);
                return;
            }
            flatIdx--;

            if (cat.expanded)
            {
                if (flatIdx < cat.count)
                {
                    int fileIdx = cat.start + flatIdx;
                    isCurrent = (fileIdx == _currentBgmIndex);
                    item->SetIndentLevel(1);
                    if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
                    const char* name = _bgmFileNames[fileIdx].GetString();
                    int j = cat.prefixLen; // skip prefix
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
                flatIdx -= cat.count;
            }
        }

        // Other files (no header)
        if (flatIdx < _otherCount)
        {
            int fileIdx = _otherStart + flatIdx;
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

        // Fallback
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

        for (int c = 0; c < _categoryCount; c++)
        {
            if (_categories[c].count <= 0)
                continue;
            if (flatIdx == 0) return true; // category header
            flatIdx--;
            if (_categories[c].expanded)
                flatIdx -= _categories[c].count;
            if (flatIdx < 0) return false;
        }

        return false;
    }

    /// Toggles the expand/collapse state for the category at the given flat index.
    void ToggleCategory(int index)
    {
        if (index == 0) return;
        int flatIdx = index - 1;

        for (int c = 0; c < _categoryCount; c++)
        {
            if (_categories[c].count <= 0)
                continue;
            if (flatIdx == 0) { _categories[c].expanded = !_categories[c].expanded; return; }
            flatIdx--;
            if (_categories[c].expanded)
                flatIdx -= _categories[c].count;
            if (flatIdx < 0) return;
        }
    }

    /// Converts a flat list index to the file index in _bgmFileNames.
    /// Returns kItemRandom (-1) for Random, kItemCategory (-2) for category headers.
    int GetBgmFileIndex(int index) const
    {
        if (index == 0) return kItemRandom;
        int flatIdx = index - 1;

        for (int c = 0; c < _categoryCount; c++)
        {
            if (_categories[c].count <= 0)
                continue;
            if (flatIdx == 0) return kItemCategory; // category header
            flatIdx--;
            if (_categories[c].expanded)
            {
                if (flatIdx < _categories[c].count)
                    return _categories[c].start + flatIdx;
                flatIdx -= _categories[c].count;
            }
        }

        if (flatIdx < _otherCount)
            return _otherStart + flatIdx;

        return kItemCategory; // shouldn't happen
    }

    /// Returns the prefix length for a file at the given index, or 0 if uncategorized.
    int GetPrefixLenForFile(int fileIndex) const
    {
        for (int c = 0; c < _categoryCount; c++)
        {
            if (fileIndex >= _categories[c].start &&
                fileIndex < _categories[c].start + _categories[c].count)
                return _categories[c].prefixLen;
        }
        return 0;
    }

private:
    const String<char, 128>* _bgmFileNames;
    int _bgmFileCount;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    int _currentBgmIndex; // -1 = Random is current
    const char16_t* _randomText;

    Category _categories[kMaxCategories];
    int _categoryCount;
    int _otherStart, _otherCount;

    /// Known category definitions (prefix, prefixLen, nameEn, nameCn).
    /// Files are sorted alphabetically, so categories appear in alpha order.
    struct CategoryDef
    {
        const char* prefix;
        int prefixLen;
        const char16_t* nameEn;
        const char16_t* nameCn;
    };

    static constexpr int kKnownCategoryCount = 7;

    static bool MatchPrefix(const char* name, const char* prefix, int prefixLen)
    {
        for (int i = 0; i < prefixLen; i++)
        {
            if (name[i] != prefix[i])
                return false;
        }
        return true;
    }

    void ClassifyFiles()
    {
        static const CategoryDef kDefs[kKnownCategoryCount] = {
            { "3DS_",          4, u"3DS",            u"3DS" },
            { "DSi_",          4, u"DSi",            u"DSi" },
            { "NS2_",          4, u"NS2",            u"NS2" },
            { "PSV_",          4, u"PS Vita",        u"PS Vita" },
            { "SwitchSports_", 13, u"Switch Sports", u"Switch Sports" },
            { "Wii_",          4, u"Wii",            u"Wii" },
            { "WiiU_",         5, u"Wii U",          u"Wii U" },
        };

        _categoryCount = 0;
        _otherStart = 0;
        _otherCount = 0;

        // Initialize category slots
        for (int d = 0; d < kKnownCategoryCount; d++)
        {
            _categories[d].prefix = kDefs[d].prefix;
            _categories[d].prefixLen = kDefs[d].prefixLen;
            _categories[d].nameEn = kDefs[d].nameEn;
            _categories[d].nameCn = kDefs[d].nameCn;
            _categories[d].start = 0;
            _categories[d].count = 0;
            _categories[d].expanded = true;
        }

        // Count files per category
        for (int i = 0; i < _bgmFileCount; i++)
        {
            const char* name = _bgmFileNames[i].GetString();
            bool matched = false;
            for (int d = 0; d < kKnownCategoryCount; d++)
            {
                if (MatchPrefix(name, kDefs[d].prefix, kDefs[d].prefixLen))
                {
                    _categories[d].count++;
                    matched = true;
                    break;
                }
            }
            if (!matched)
                _otherCount++;
        }

        // Compute start indices (files are alphabetically sorted, so groups are contiguous)
        int pos = 0;
        _categoryCount = 0;
        for (int d = 0; d < kKnownCategoryCount; d++)
        {
            if (_categories[d].count > 0)
            {
                _categories[d].start = pos;
                pos += _categories[d].count;
                // Pack non-empty categories to the front
                if (_categoryCount != d)
                    _categories[_categoryCount] = _categories[d];
                _categoryCount++;
            }
        }
        _otherStart = pos;
    }
};

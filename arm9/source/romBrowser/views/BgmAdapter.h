#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "core/String.h"
#include "BgmListItemView.h"
#include "services/Localization/Localization.h"
#include "BgmTranslations.h"

class BgmAdapter : public RecyclerAdapter
{
public:
    static constexpr int kItemOff = -3;
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
        , _categoryCount(0)
    {
        ClassifyFiles();
    }

    u32 GetItemCount() const override
    {
        u32 count = 2; // Off + Random
        for (int c = 0; c < _categoryCount; c++)
            count += 1 + (_categories[c].expanded ? _categories[c].count : 0);
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
        item->SetIsCategory(false);

        char16_t buf[64];
        int pos = 0;

        // index 0: Off
        if (index == 0)
        {
            bool isCurrent = (_currentBgmIndex == kItemOff);
            item->SetCurrentlyPlaying(isCurrent);
            if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
            const char16_t* t = Localization::Translate("bgm_off");
            for (int i = 0; t[i] && pos < 62; i++) buf[pos++] = t[i];
            buf[pos] = 0;
            item->SetText(buf);
            return;
        }

        // index 1: Random
        if (index == 1)
        {
            bool isCurrent = (_currentBgmIndex == kItemRandom);
            item->SetCurrentlyPlaying(isCurrent);
            if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
            const char16_t* t = Localization::Translate("bgm_random");
            for (int i = 0; t[i] && pos < 62; i++) buf[pos++] = t[i];
            buf[pos] = 0;
            item->SetText(buf);
            return;
        }

        int flatIdx = index - 2;

        for (int c = 0; c < _categoryCount; c++)
        {
            // Category header
            if (flatIdx == 0)
            {
                item->SetIsCategory(true);
                item->SetCurrentlyPlaying(false);
                buf[pos++] = _categories[c].expanded ? u'-' : u'+';
                buf[pos++] = u' ';
                const char16_t* name = _categories[c].displayName;
                for (int i = 0; name[i] && pos < 62; i++) buf[pos++] = name[i];
                buf[pos] = 0;
                item->SetText(buf);
                return;
            }
            flatIdx--;

            if (_categories[c].expanded)
            {
                if (flatIdx < _categories[c].count)
                {
                    int fileIdx = _categories[c].start + flatIdx;
                    bool isCurrent = (fileIdx == _currentBgmIndex);
                    item->SetIndentLevel(1);
                    item->SetCurrentlyPlaying(isCurrent);
                    if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
                    // Try Chinese translation first
                    const char* name = _bgmFileNames[fileIdx].GetString();
                    // Build stripped key (without prefix and .bcstm)
                    char strippedKey[128];
                    int sk = 0;
                    int j = _categories[c].prefixLen;
                    while (name[j] && name[j] != '.' && sk < 126)
                        strippedKey[sk++] = name[j++];
                    strippedKey[sk] = 0;

                    const char16_t* zhName = FindBgmChineseTranslation(strippedKey);
                    if (zhName)
                    {
                        for (int i = 0; zhName[i] && pos < 62; i++)
                            buf[pos++] = zhName[i];
                    }
                    else
                    {
                        // Fallback: replace _ with space
                        for (int i = 0; strippedKey[i] && pos < 62; i++)
                            buf[pos++] = (strippedKey[i] == '_') ? u' ' : (char16_t)(unsigned char)strippedKey[i];
                    }
                    buf[pos] = 0;
                    item->SetText(buf);
                    return;
                }
                flatIdx -= _categories[c].count;
            }
        }

        // Fallback
        buf[0] = 0;
        item->SetIsCategory(false);
        item->SetCurrentlyPlaying(false);
        item->SetText(buf);
    }

    void ReleaseView(View* view, int index) const override { }

    bool IsCategoryItem(int index) const
    {
        if (index < 2) return false;
        int flatIdx = index - 2;
        for (int c = 0; c < _categoryCount; c++)
        {
            if (flatIdx == 0) return true;
            flatIdx--;
            if (_categories[c].expanded) flatIdx -= _categories[c].count;
            if (flatIdx < 0) return false;
        }
        return false;
    }

    void ToggleCategory(int index)
    {
        if (index < 2) return;
        int flatIdx = index - 2;
        for (int c = 0; c < _categoryCount; c++)
        {
            if (flatIdx == 0) { _categories[c].expanded = !_categories[c].expanded; return; }
            flatIdx--;
            if (_categories[c].expanded) flatIdx -= _categories[c].count;
            if (flatIdx < 0) return;
        }
    }

    int GetBgmFileIndex(int index) const
    {
        if (index == 0) return kItemOff;
        if (index == 1) return kItemRandom;
        int flatIdx = index - 2;
        for (int c = 0; c < _categoryCount; c++)
        {
            if (flatIdx == 0) return kItemCategory;
            flatIdx--;
            if (_categories[c].expanded)
            {
                if (flatIdx < _categories[c].count)
                    return _categories[c].start + flatIdx;
                flatIdx -= _categories[c].count;
            }
        }
        return kItemCategory;
    }

private:
    const String<char, 128>* _bgmFileNames;
    int _bgmFileCount;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    int _currentBgmIndex;

    struct Category
    {
        const char* prefix;
        int prefixLen;
        const char16_t* displayName;
        int start;
        int count;
        bool expanded;
    };

    static constexpr int kMaxCategories = 8;
    Category _categories[kMaxCategories];
    int _categoryCount;

    void ClassifyFiles()
    {
        // Known prefixes
        struct PrefixDef { const char* prefix; int len; const char16_t* name; };
        static const PrefixDef knownPrefixes[] = {
            { "3DS_",          4,  u"3DS" },
            { "DSi_",          4,  u"DSi" },
            { "NS2_",          4,  u"NS2" },
            { "PSV_",          4,  u"PS Vita" },
            { "SwitchSports_", 13, u"Switch Sports" },
            { "Wii_",          4,  u"Wii" },
            { "WiiU_",         5,  u"Wii U" },
        };
        static constexpr int numKnown = sizeof(knownPrefixes) / sizeof(knownPrefixes[0]);

        _categoryCount = 0;

        for (int p = 0; p < numKnown && _categoryCount < kMaxCategories; p++)
        {
            int start = -1, count = 0;
            for (int i = 0; i < _bgmFileCount; i++)
            {
                const char* name = _bgmFileNames[i].GetString();
                bool match = true;
                for (int j = 0; j < knownPrefixes[p].len; j++)
                {
                    if (name[j] != knownPrefixes[p].prefix[j]) { match = false; break; }
                }
                if (match)
                {
                    if (start < 0) start = i;
                    count++;
                }
            }
            if (count > 0)
            {
                auto& cat = _categories[_categoryCount++];
                cat.prefix = knownPrefixes[p].prefix;
                cat.prefixLen = knownPrefixes[p].len;
                cat.displayName = knownPrefixes[p].name;
                cat.start = start;
                cat.count = count;
                cat.expanded = false; // default collapsed
            }
        }
    }
};

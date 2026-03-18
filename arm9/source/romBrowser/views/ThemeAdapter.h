#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "core/String.h"
#include "fat/Directory.h"
#include "BgmListItemView.h"
#include "services/Localization/Localization.h"

struct ThemeTranslation { const char* key; const char16_t* zhName; };
static const ThemeTranslation kThemeTranslations[] = {
    { "material", u"Material 材质" },
    { "raspberry", u"Raspberry 树莓" },
    { "Minecraft v0.1", u"我的世界" },
    { "Rainbow Theme", u"彩虹主题" },
    { "Wintendows DiSta", u"Wintendows" },
    { "Ecruteak City", u"圆朱市" },
    { "Battrey", u"电池" },
    { "Samurai Champloo", u"混沌武士" },
    { "Samurai Champloo 2", u"混沌武士 2" },
    { "nge first children", u"EVA 初号机" },
    { "rurouni kenshin", u"浪客剑心" },
    { "Gojo Beach Fun", u"五条悟 海滩" },
    { "Gon And Killua Ocean Chill Vibes", u"小杰与奇犽 海洋" },
    { "Shinji Calming Beach Scene", u"碇真嗣 海边" },
    { "AE86 W EuroBeat", u"AE86 头文字D" },
    { "Skyline W Eurobeat", u"Skyline Eurobeat" },
};
static constexpr int kThemeTranslationCount = sizeof(kThemeTranslations) / sizeof(kThemeTranslations[0]);

inline const char16_t* FindThemeChineseTranslation(const char* name)
{
    if (!name) return nullptr;
    for (int i = 0; i < kThemeTranslationCount; i++)
        if (!strcasecmp(kThemeTranslations[i].key, name))
            return kThemeTranslations[i].zhName;
    return nullptr;
}

class ThemeAdapter : public RecyclerAdapter
{
public:
    static constexpr int kMaxThemes = 16;
    static constexpr int kItemCategory = -2;

    ThemeAdapter(const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        const char* currentThemeName)
        : _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository)
        , _builtinCount(0), _customCount(0)
        , _currentIndex(-1)
        , _builtinExpanded(false), _customExpanded(false)
    {
        ScanThemes();
        if (currentThemeName)
        {
            for (int i = 0; i < _builtinCount + _customCount; i++)
            {
                if (!strcasecmp(_allNames[i].GetString(), currentThemeName))
                {
                    _currentIndex = i;
                    break;
                }
            }
        }
    }

    u32 GetItemCount() const override
    {
        u32 count = 0;
        if (_builtinCount > 0)
            count += 1 + (_builtinExpanded ? _builtinCount : 0);
        if (_customCount > 0)
            count += 1 + (_customExpanded ? _customCount : 0);
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
        int flatIdx = index;

        // Built-in group
        if (_builtinCount > 0)
        {
            if (flatIdx == 0)
            {
                item->SetIsCategory(true);
                item->SetCurrentlyPlaying(false);
                buf[pos++] = _builtinExpanded ? u'-' : u'+';
                buf[pos++] = u' ';
                const char16_t* t = Localization::Translate("theme_builtin");
                for (int i = 0; t[i] && pos < 62; i++) buf[pos++] = t[i];
                buf[pos] = 0;
                item->SetText(buf);
                return;
            }
            flatIdx--;

            if (_builtinExpanded)
            {
                if (flatIdx < _builtinCount)
                {
                    int realIdx = flatIdx; // builtin at start of _allNames
                    bool isCurrent = (realIdx == _currentIndex);
                    item->SetIndentLevel(1);
                    item->SetCurrentlyPlaying(isCurrent);
                    if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
                    const char* name = _allNames[realIdx].GetString();
                    const char16_t* zhName = FindThemeChineseTranslation(name);
                    if (zhName)
                    { for (int i = 0; zhName[i] && pos < 62; i++) buf[pos++] = zhName[i]; }
                    else
                    { while (*name && pos < 62) { buf[pos++] = (char16_t)(unsigned char)*name; name++; } }
                    buf[pos] = 0;
                    item->SetText(buf);
                    return;
                }
                flatIdx -= _builtinCount;
            }
        }

        // Custom group
        if (_customCount > 0)
        {
            if (flatIdx == 0)
            {
                item->SetIsCategory(true);
                item->SetCurrentlyPlaying(false);
                buf[pos++] = _customExpanded ? u'-' : u'+';
                buf[pos++] = u' ';
                const char16_t* t = Localization::Translate("theme_custom");
                for (int i = 0; t[i] && pos < 62; i++) buf[pos++] = t[i];
                buf[pos] = 0;
                item->SetText(buf);
                return;
            }
            flatIdx--;

            if (_customExpanded)
            {
                if (flatIdx < _customCount)
                {
                    int realIdx = _builtinCount + flatIdx;
                    bool isCurrent = (realIdx == _currentIndex);
                    item->SetIndentLevel(1);
                    item->SetCurrentlyPlaying(isCurrent);
                    if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }
                    const char* name = _allNames[realIdx].GetString();
                    const char16_t* zhName = FindThemeChineseTranslation(name);
                    if (zhName)
                    { for (int i = 0; zhName[i] && pos < 62; i++) buf[pos++] = zhName[i]; }
                    else
                    { while (*name && pos < 62) { buf[pos++] = (char16_t)(unsigned char)*name; name++; } }
                    buf[pos] = 0;
                    item->SetText(buf);
                    return;
                }
            }
        }

        buf[0] = 0;
        item->SetText(buf);
    }

    void ReleaseView(View* view, int index) const override { }

    bool IsCategoryItem(int index) const
    {
        int flatIdx = index;
        if (_builtinCount > 0)
        {
            if (flatIdx == 0) return true;
            flatIdx--;
            if (_builtinExpanded) flatIdx -= _builtinCount;
            if (flatIdx < 0) return false;
        }
        if (_customCount > 0)
        {
            if (flatIdx == 0) return true;
        }
        return false;
    }

    void ToggleCategory(int index)
    {
        int flatIdx = index;
        if (_builtinCount > 0)
        {
            if (flatIdx == 0) { _builtinExpanded = !_builtinExpanded; return; }
            flatIdx--;
            if (_builtinExpanded) flatIdx -= _builtinCount;
            if (flatIdx < 0) return;
        }
        if (_customCount > 0)
        {
            if (flatIdx == 0) { _customExpanded = !_customExpanded; return; }
        }
    }

    const char* GetThemeName(int index) const
    {
        int realIdx = GetRealIndex(index);
        if (realIdx >= 0 && realIdx < _builtinCount + _customCount)
            return _allNames[realIdx].GetString();
        return "material";
    }

    int GetCurrentIndex() const { return _currentIndex; }

    // Convert flat list index to the initial focus index
    int GetFlatIndex(int realIdx) const
    {
        if (realIdx < 0) return 0;
        int flat = 0;
        if (_builtinCount > 0)
        {
            flat++; // header
            if (realIdx < _builtinCount)
                return _builtinExpanded ? flat + realIdx : flat - 1;
            if (_builtinExpanded) flat += _builtinCount;
        }
        if (_customCount > 0)
        {
            flat++; // header
            int customIdx = realIdx - _builtinCount;
            if (customIdx >= 0 && customIdx < _customCount)
                return _customExpanded ? flat + customIdx : flat - 1;
        }
        return 0;
    }

private:
    String<char, 64> _allNames[kMaxThemes]; // builtin first, then custom
    int _builtinCount;
    int _customCount;
    int _currentIndex;
    bool _builtinExpanded;
    bool _customExpanded;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;

    int GetRealIndex(int flatIndex) const
    {
        int flatIdx = flatIndex;
        if (_builtinCount > 0)
        {
            if (flatIdx == 0) return -1; // category
            flatIdx--;
            if (_builtinExpanded)
            {
                if (flatIdx < _builtinCount) return flatIdx;
                flatIdx -= _builtinCount;
            }
        }
        if (_customCount > 0)
        {
            if (flatIdx == 0) return -1; // category
            flatIdx--;
            if (_customExpanded)
            {
                if (flatIdx < _customCount) return _builtinCount + flatIdx;
            }
        }
        return -1;
    }

    void ScanThemes()
    {
        // Known built-in themes
        static const char* builtins[] = { "material", "raspberry" };
        static constexpr int numBuiltins = 2;

        // Scan directory
        String<char, 64> allDirs[kMaxThemes];
        int dirCount = 0;

        Directory dir;
        if (dir.Open("/_pico/themes") == FR_OK)
        {
            FILINFO fi;
            while (dirCount < kMaxThemes)
            {
                if (dir.Read(&fi) != FR_OK || fi.fname[0] == 0)
                    break;
                if (!(fi.fattrib & AM_DIR) || fi.fname[0] == '.' || fi.fname[0] == '_')
                    continue;
                allDirs[dirCount++] = fi.fname;
            }
        }

        // Separate into builtin and custom
        _builtinCount = 0;
        _customCount = 0;

        // Add builtins first (in order)
        for (int b = 0; b < numBuiltins; b++)
        {
            for (int d = 0; d < dirCount; d++)
            {
                if (!strcasecmp(allDirs[d].GetString(), builtins[b]))
                {
                    _allNames[_builtinCount++] = allDirs[d].GetString();
                    break;
                }
            }
        }

        // Add custom (everything that's not builtin)
        for (int d = 0; d < dirCount; d++)
        {
            bool isBuiltin = false;
            for (int b = 0; b < numBuiltins; b++)
            {
                if (!strcasecmp(allDirs[d].GetString(), builtins[b]))
                { isBuiltin = true; break; }
            }
            if (!isBuiltin && _builtinCount + _customCount < kMaxThemes)
            {
                _allNames[_builtinCount + _customCount] = allDirs[d].GetString();
                _customCount++;
            }
        }
    }
};

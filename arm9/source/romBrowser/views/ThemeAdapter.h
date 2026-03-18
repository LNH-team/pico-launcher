#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "core/String.h"
#include "fat/Directory.h"
#include "BgmListItemView.h"

class ThemeAdapter : public RecyclerAdapter
{
public:
    static constexpr int kMaxThemes = 16;

    ThemeAdapter(const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository,
        const char* currentThemeName)
        : _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository)
        , _count(0)
        , _currentIndex(-1)
    {
        ScanThemes();
        if (currentThemeName)
        {
            for (int i = 0; i < _count; i++)
            {
                if (!strcasecmp(_names[i].GetString(), currentThemeName))
                {
                    _currentIndex = i;
                    break;
                }
            }
        }
    }

    u32 GetItemCount() const override { return (u32)_count; }

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

        bool isCurrent = (index == _currentIndex);
        item->SetCurrentlyPlaying(isCurrent);

        char16_t buf[64];
        int pos = 0;
        if (isCurrent) { buf[pos++] = u'\u00B7'; buf[pos++] = u' '; }

        const char* name = _names[index].GetString();
        while (*name && pos < 62)
        {
            buf[pos++] = (char16_t)(unsigned char)*name;
            name++;
        }
        buf[pos] = 0;
        item->SetText(buf);
    }

    void ReleaseView(View* view, int index) const override { }

    const char* GetThemeName(int index) const
    {
        if (index >= 0 && index < _count)
            return _names[index].GetString();
        return "material";
    }

    int GetCurrentIndex() const { return _currentIndex; }

private:
    String<char, 64> _names[kMaxThemes];
    int _count;
    int _currentIndex;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;

    void ScanThemes()
    {
        Directory dir;
        if (dir.Open("/_pico/themes") != FR_OK)
            return;

        FILINFO fi;
        while (_count < kMaxThemes)
        {
            if (dir.Read(&fi) != FR_OK || fi.fname[0] == 0)
                break;
            if (!(fi.fattrib & AM_DIR))
                continue;
            if (fi.fname[0] == '.' || fi.fname[0] == '_')
                continue;
            _names[_count++] = fi.fname;
        }
    }
};

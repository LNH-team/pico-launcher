#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "core/String.h"
#include "BgmListItemView.h"

class BgmAdapter : public RecyclerAdapter
{
public:
    BgmAdapter(const String<char, 128>* bgmFileNames, int bgmFileCount,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        int currentBgmIndex = -1)
        : _bgmFileNames(bgmFileNames)
        , _bgmFileCount(bgmFileCount)
        , _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository)
        , _currentBgmIndex(currentBgmIndex)
    {
    }

    u32 GetItemCount() const override
    {
        return (u32)(_bgmFileCount + 1); // +1 for "Random"
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

        // _currentBgmIndex: -1 = Random is current
        // In the list: index 0 = Random, index 1+ = file
        // So "current" when: index==0 && _currentBgmIndex<0, or index-1 == _currentBgmIndex
        bool isCurrent = (index == 0 && _currentBgmIndex < 0)
            || (index > 0 && index - 1 == _currentBgmIndex);
        item->SetCurrentlyPlaying(isCurrent);

        char16_t buf[64];
        int pos = 0;

        // Add marker for currently playing
        if (isCurrent)
        {
            buf[pos++] = u'\u00B7'; // ·
            buf[pos++] = u' ';
        }

        if (index == 0)
        {
            const char16_t* r = u"Random";
            for (int i = 0; r[i] && pos < 62; i++)
                buf[pos++] = r[i];
        }
        else
        {
            int fileIdx = index - 1;
            if (fileIdx >= 0 && fileIdx < _bgmFileCount)
            {
                const char* name = _bgmFileNames[fileIdx].GetString();
                int j = 0;
                while (name[j] && name[j] != '.' && pos < 62)
                {
                    buf[pos++] = (char16_t)(unsigned char)name[j];
                    j++;
                }
            }
        }
        buf[pos] = 0;
        item->SetText(buf);
    }

    void ReleaseView(View* view, int index) const override
    {
    }

private:
    const String<char, 128>* _bgmFileNames;
    int _bgmFileCount;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    int _currentBgmIndex; // -1 = Random is current
};

#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "core/String.h"
#include "BgmListItemView.h"

class BgmAdapter : public RecyclerAdapter
{
public:
    BgmAdapter(const String<char, 128>* bgmFileNames, int bgmFileCount,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
        : _bgmFileNames(bgmFileNames)
        , _bgmFileCount(bgmFileCount)
        , _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository)
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
        if (index == 0)
        {
            item->SetText(u"Random");
            return;
        }

        // Strip .bcstm extension for display
        int fileIdx = index - 1;
        if (fileIdx < 0 || fileIdx >= _bgmFileCount)
        {
            item->SetText(u"");
            return;
        }

        const char* name = _bgmFileNames[fileIdx].GetString();
        char16_t buf[64];
        int pos = 0;
        while (name[pos] && name[pos] != '.' && pos < 62)
        {
            buf[pos] = (char16_t)(unsigned char)name[pos];
            pos++;
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
};

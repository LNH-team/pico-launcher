#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "cheats/CheatCategory.h"
#include "cheats/Cheat.h"
#include "CheatListItemView.h"

class CheatsAdapter : public RecyclerAdapter
{
public:
    CheatsAdapter(const ICheatCategory* cheatCategory, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository, const CheatListItemView::VramOffsets& vramOffsets)
        : _cheatCategory(cheatCategory), _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository), _vramOffsets(vramOffsets) { }

    // Constructor for selected-only mode
    CheatsAdapter(const Cheat* cheats, u32 numberOfCheats, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository, const CheatListItemView::VramOffsets& vramOffsets)
        : _flatCheats(cheats), _flatCheatCount(numberOfCheats), _materialColorScheme(materialColorScheme)
        , _fontRepository(fontRepository), _vramOffsets(vramOffsets) { }

    u32 GetItemCount() const override
    {
        if (_cheatCategory == nullptr)
        {
            return _flatCheatCount;
        }

        u32 numberOfCategories = 0;
        _cheatCategory->GetCategories(numberOfCategories);
        u32 numberOfCheats = 0;
        _cheatCategory->GetCheats(numberOfCheats);
        return numberOfCategories + numberOfCheats;
    }

    void GetViewSize(int& width, int& height) const override
    {
        width = 224;
        height = 24;
    }

    View* CreateView() const override
    {
        return new CheatListItemView(_vramOffsets, _materialColorScheme, _fontRepository);
    }

    void DestroyView(View* view) const override
    {
        delete (CheatListItemView*)view;
    }

    void BindView(View* view, int index) const override
    {
        auto listItemView = static_cast<CheatListItemView*>(view);
        if (_cheatCategory == nullptr)
        {
            listItemView->SetCheat(&_flatCheats[index]);
            return;
        }

        u32 numberOfCategories = 0;
        auto categories = _cheatCategory->GetCategories(numberOfCategories);
        if ((u32)index < numberOfCategories)
        {
            listItemView->SetCategory(&categories[index]);
        }
        else
        {
            index -= numberOfCategories;
            u32 numberOfCheats = 0;
            auto cheats = _cheatCategory->GetCheats(numberOfCheats);
            listItemView->SetCheat(&cheats[index]);
        }
    }

    void ReleaseView(View* view, int index) const override
    {
        // Nothing to do
    }

private:
    const ICheatCategory* _cheatCategory = nullptr;
    const Cheat* _flatCheats = nullptr;
    u32 _flatCheatCount = 0;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    CheatListItemView::VramOffsets _vramOffsets;
};

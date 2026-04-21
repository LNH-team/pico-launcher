#pragma once
#include "RomBrowserDisplayMode.h"

class RomBrowserHorizontalCoverFlowDisplayMode : public RomBrowserDisplayMode
{
public:
    static const RomBrowserHorizontalCoverFlowDisplayMode sInstance;

    bool IsVertical() const override { return false; }
    bool ShowCoverOnTopScreen() const override { return false; }

    SharedPtr<AppBarView> CreateAppBarView(const IRomBrowserViewFactory* romBrowserViewFactory,
        int startButtonCount, int endButtonCount) const override
    {
        return romBrowserViewFactory->CreateAppBarView(0, 0,
            AppBarView::Orientation::Horizontal, startButtonCount, endButtonCount);
    }

    SharedPtr<RecyclerViewBase> CreateRecyclerView(const IRomBrowserViewFactory* romBrowserViewFactory) const override
    {
        return romBrowserViewFactory->CreateCoverFlowRecyclerView();
    }

    SharedPtr<FileRecyclerAdapter> CreateRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory, VBlankTextureLoader* vblankTextureLoader) const override
    {
        return romBrowserViewFactory->CreateCoverFlowRecyclerAdapter(
            viewModel, themeFileIconFactory, vblankTextureLoader);
    }

private:
    constexpr RomBrowserHorizontalCoverFlowDisplayMode() { }
};

#pragma once
#include "gui/views/RecyclerView.h"
#include "RomBrowserDisplayMode.h"
#include "BannerListFileRecyclerAdapter.h"

class RomBrowserBannerListDisplayMode : public RomBrowserDisplayMode
{
public:
    static const RomBrowserBannerListDisplayMode sInstance;

    bool IsVertical() const override { return true; }

    SharedPtr<AppBarView> CreateAppBarView(const IRomBrowserViewFactory* romBrowserViewFactory,
        int startButtonCount, int endButtonCount) const override
    {
        return romBrowserViewFactory->CreateAppBarView(0, 0,
            AppBarView::Orientation::Vertical, startButtonCount, endButtonCount);
    }

    SharedPtr<RecyclerViewBase> CreateRecyclerView(const IRomBrowserViewFactory* romBrowserViewFactory) const override
    {
        auto recyclerView = RecyclerView::CreateShared(42, 0, 256 - 42, 192, RecyclerView::Mode::VerticalList);
        recyclerView->SetPadding(0, 3);
        recyclerView->SetItemSpacing(0, 3);
        return recyclerView;
    }

    SharedPtr<FileRecyclerAdapter> CreateRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory, VBlankTextureLoader* vblankTextureLoader) const override
    {
        return SharedPtr<BannerListFileRecyclerAdapter>::MakeShared(viewModel->GetRomBrowserController(),
            &viewModel->GetFileInfoManager(), viewModel->GetIoTaskQueue(), themeFileIconFactory,
            romBrowserViewFactory, vblankTextureLoader);
    }

private:
    constexpr RomBrowserBannerListDisplayMode() { }
};

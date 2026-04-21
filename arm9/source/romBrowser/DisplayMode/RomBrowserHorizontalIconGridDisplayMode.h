#pragma once
#include "gui/views/RecyclerView.h"
#include "RomBrowserDisplayMode.h"
#include "IconGridFileRecyclerAdapter.h"

class RomBrowserHorizontalIconGridDisplayMode : public RomBrowserDisplayMode
{
public:
    static const RomBrowserHorizontalIconGridDisplayMode sInstance;

    bool IsVertical() const override { return false; }

    SharedPtr<AppBarView> CreateAppBarView(const IRomBrowserViewFactory* romBrowserViewFactory,
        int startButtonCount, int endButtonCount) const override
    {
        return romBrowserViewFactory->CreateAppBarView(0, 0,
            AppBarView::Orientation::Horizontal, startButtonCount, endButtonCount);
    }

    SharedPtr<RecyclerViewBase> CreateRecyclerView(const IRomBrowserViewFactory* romBrowserViewFactory) const override
    {
        auto recyclerView = RecyclerView::CreateShared(0, 42, 256, 192 - 42, RecyclerView::Mode::HorizontalGrid);
        recyclerView->SetPadding(10, 0);
        recyclerView->SetItemSpacing(4, 4);
        return recyclerView;
    }

    SharedPtr<FileRecyclerAdapter> CreateRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory, VBlankTextureLoader* vblankTextureLoader) const override
    {
        return SharedPtr<IconGridFileRecyclerAdapter>::MakeShared(viewModel->GetRomBrowserController(),
            &viewModel->GetFileInfoManager(), viewModel->GetIoTaskQueue(),
            themeFileIconFactory, romBrowserViewFactory);
    }

private:
    constexpr RomBrowserHorizontalIconGridDisplayMode() { }
};

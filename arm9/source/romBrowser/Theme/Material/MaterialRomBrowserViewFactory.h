#pragma once
#include "MaterialIconGridItemView.h"
#include "MaterialBannerListItemView.h"
#include "MaterialFileInfoCardView.h"
#include "../IRomBrowserViewFactory.h"
#include "romBrowser/viewModels/RomBrowserViewModel.h"
#include "CarouselRecyclerView.h"
#include "MaterialCoverFlowFileRecyclerAdapter.h"
#include "MaterialAppBarView.h"

class MaterialColorScheme;
class IFontRepository;

class MaterialRomBrowserViewFactory : public IRomBrowserViewFactory
{
public:
    MaterialRomBrowserViewFactory(const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository)
        : _materialColorScheme(materialColorScheme), _fontRepository(fontRepository) { }

    SharedPtr<IconGridItemView> CreateIconGridItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel) const override
    {
        return MaterialIconGridItemView::CreateShared(std::move(viewModel), _materialColorScheme);
    }

    IconGridItemView::VramToken UploadIconGridItemViewGraphics(
        const VramContext& vramContext) const override
    {
        return MaterialIconGridItemView::UploadGraphics(vramContext);
    }

    SharedPtr<BannerListItemView> CreateBannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel,
        VBlankTextureLoader* vblankTextureLoader) const override
    {
        return MaterialBannerListItemView::CreateShared(std::move(viewModel), _materialColorScheme, _fontRepository);
    }

    BannerListItemView::VramToken UploadBannerListItemViewGraphics(
        const VramContext& vramContext) const override
    {
        return MaterialBannerListItemView::UploadGraphics(vramContext);
    }

    SharedPtr<AppBarView> CreateAppBarView(int x, int y, AppBarView::Orientation orientation,
        int startButtonCount, int endButtonCount) const override
    {
        return MaterialAppBarView::CreateShared(x, y, orientation, startButtonCount, endButtonCount, _materialColorScheme);
    }

    SharedPtr<BannerView> CreateFileInfoView() const override
    {
        return MaterialFileInfoCardView::CreateShared(_materialColorScheme, _fontRepository);
    }

    SharedPtr<RecyclerViewBase> CreateCoverFlowRecyclerView() const override
    {
        return CarouselRecyclerView::CreateShared(_materialColorScheme);
    }

    SharedPtr<FileRecyclerAdapter> CreateCoverFlowRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader) const override
    {
        return SharedPtr<MaterialCoverFlowFileRecyclerAdapter>::MakeShared(viewModel->GetRomBrowserController(),
            &viewModel->GetFileInfoManager(), viewModel->GetIoTaskQueue(),
            themeFileIconFactory, this, vblankTextureLoader, &viewModel->GetCoverRepository());
    }

    Point GetTopCoverPosition() const override
    {
        return Point(75, 18);
    }

private:
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
};

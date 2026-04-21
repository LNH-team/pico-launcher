#pragma once
#include "CustomIconGridItemView.h"
#include "CustomAppBarView.h"
#include "CustomFileInfoView.h"
#include "../IRomBrowserViewFactory.h"
#include "CustomBannerListItemView.h"
#include "romBrowser/views/CoverFlowRecyclerView.h"
#include "romBrowser/viewModels/RomBrowserViewModel.h"
#include "romBrowser/DisplayMode/CoverFlowFileRecyclerAdapter.h"
#include "themes/custom/CustomThemeInfo.h"

class MaterialColorScheme;
class ITheme;
class VramContext;
class IFontRepository;

class CustomRomBrowserViewFactory : public IRomBrowserViewFactory
{
public:
    CustomRomBrowserViewFactory(const CustomThemeInfo* customThemeInfo, const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository)
        : _customThemeInfo(customThemeInfo), _materialColorScheme(materialColorScheme), _fontRepository(fontRepository) { }

    SharedPtr<IconGridItemView> CreateIconGridItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel) const override
    {
        return CustomIconGridItemView::CreateShared(std::move(viewModel), _customThemeInfo, _gridCellTexVramOffset, _gridCellPlttVramOffset,
            _gridCellSelectedTexVramOffset, _gridCellSelectedPlttVramOffset);
    }

    SharedPtr<BannerListItemView> CreateBannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel,
        VBlankTextureLoader* vblankTextureLoader) const override
    {
        return CustomBannerListItemView::CreateShared(std::move(viewModel), _customThemeInfo, _materialColorScheme, _fontRepository,
            _bannerListCellTexVramOffset, _bannerListCellPlttVramOffset,
            _bannerListCellSelectedTexVramOffset, _bannerListCellSelectedPlttVramOffset, vblankTextureLoader);
    }

    BannerListItemView::VramToken UploadBannerListItemViewGraphics(const VramContext& vramContext) const override
    {
        return BannerListItemView::VramToken(0);
    }

    SharedPtr<AppBarView> CreateAppBarView(int x, int y, AppBarView::Orientation orientation,
        int startButtonCount, int endButtonCount) const override
    {
        return CustomAppBarView::CreateShared(x, y, orientation, startButtonCount, endButtonCount, _materialColorScheme,
            _scrimTexVramOffset, _scrimPlttVramOffset);
    }

    SharedPtr<BannerView> CreateFileInfoView() const override
    {
        return CustomFileInfoView::CreateShared(_customThemeInfo, _fontRepository);
    }

    SharedPtr<RecyclerViewBase> CreateCoverFlowRecyclerView() const override
    {
        return CoverFlowRecyclerView::CreateShared();
    }

    SharedPtr<FileRecyclerAdapter> CreateCoverFlowRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader) const override
    {
        return SharedPtr<CoverFlowFileRecyclerAdapter>::MakeShared(viewModel->GetRomBrowserController(),
            &viewModel->GetFileInfoManager(), viewModel->GetIoTaskQueue(),
            themeFileIconFactory, this, vblankTextureLoader, &viewModel->GetCoverRepository());
    }

    Point GetTopCoverPosition() const override
    {
        return _customThemeInfo->topCoverInfo.GetPosition();
    }

    void LoadResources(const ITheme& theme, const VramContext& mainVramContext);

private:
    u32 _gridCellTexVramOffset;
    u32 _gridCellPlttVramOffset;
    u32 _gridCellSelectedTexVramOffset;
    u32 _gridCellSelectedPlttVramOffset;
    u32 _bannerListCellTexVramOffset;
    u32 _bannerListCellPlttVramOffset;
    u32 _bannerListCellSelectedTexVramOffset;
    u32 _bannerListCellSelectedPlttVramOffset;
    u32 _scrimTexVramOffset;
    u32 _scrimPlttVramOffset;
    const CustomThemeInfo* _customThemeInfo;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
};

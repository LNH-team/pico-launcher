#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "../views/IconGridItemView.h"
#include "../views/BannerListItemView.h"
#include "../views/AppBarView.h"
#include "../views/BannerView.h"
#include "gui/views/RecyclerViewBase.h"

class VramContext;
class VBlankTextureLoader;
class RomBrowserViewModel;
class IThemeFileIconFactory;
class FileRecyclerAdapter;
class RomBrowserItemViewModel;

class IRomBrowserViewFactory
{
public:
    virtual ~IRomBrowserViewFactory() = 0;

    virtual SharedPtr<IconGridItemView> CreateIconGridItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel) const = 0;
    virtual IconGridItemView::VramToken UploadIconGridItemViewGraphics(
        const VramContext& vramContext) const { return IconGridItemView::VramToken(0); }

    virtual SharedPtr<BannerListItemView> CreateBannerListItemView(std::unique_ptr<RomBrowserItemViewModel> viewModel,
        VBlankTextureLoader* vblankTextureLoader) const = 0;
    virtual BannerListItemView::VramToken UploadBannerListItemViewGraphics(
        const VramContext& vramContext) const { return BannerListItemView::VramToken(0); }

    virtual SharedPtr<AppBarView> CreateAppBarView(int x, int y, AppBarView::Orientation orientation,
        int startButtonCount, int endButtonCount) const = 0;

    virtual SharedPtr<BannerView> CreateFileInfoView() const = 0;

    virtual SharedPtr<RecyclerViewBase> CreateCoverFlowRecyclerView() const = 0;

    virtual SharedPtr<FileRecyclerAdapter> CreateCoverFlowRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader) const = 0;

    virtual Point GetTopCoverPosition() const = 0;
};

inline IRomBrowserViewFactory::~IRomBrowserViewFactory() { }

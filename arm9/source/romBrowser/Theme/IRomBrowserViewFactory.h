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
class IRomBrowserItemViewModel;
class StatusViewModel;
class View;

class IRomBrowserViewFactory
{
public:
    virtual ~IRomBrowserViewFactory() = 0;

    virtual SharedPtr<IconGridItemView> CreateIconGridItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel) const = 0;
    virtual IconGridItemView::VramToken UploadIconGridItemViewGraphics(
        const VramContext& vramContext) const { return IconGridItemView::VramToken(0); }

    virtual SharedPtr<BannerListItemView> CreateBannerListItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
        VBlankTextureLoader* vblankTextureLoader) const = 0;
    virtual BannerListItemView::VramToken UploadBannerListItemViewGraphics(
        const VramContext& vramContext) const { return BannerListItemView::VramToken(0); }

    virtual SharedPtr<AppBarView> CreateAppBarView(int x, int y, AppBarView::Orientation orientation,
        int startButtonCount, int endButtonCount) const = 0;

    virtual SharedPtr<BannerView> CreateFileInfoView() const = 0;

    /// @brief Creates the top-screen status overlay (clock + battery), or nullptr to render nothing.
    /// @param viewModel The status view model driving clock/battery state (owned by RomBrowserViewModel).
    virtual SharedPtr<View> CreateStatusView(StatusViewModel* viewModel) const = 0;

    virtual SharedPtr<RecyclerViewBase> CreateCoverFlowRecyclerView() const = 0;

    virtual SharedPtr<FileRecyclerAdapter> CreateCoverFlowRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader) const = 0;

    virtual Point GetTopCoverPosition() const = 0;
};

inline IRomBrowserViewFactory::~IRomBrowserViewFactory() { }

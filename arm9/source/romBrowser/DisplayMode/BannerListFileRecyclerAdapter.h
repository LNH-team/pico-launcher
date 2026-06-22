#pragma once
#include "../FileRecyclerAdapter.h"
#include "../views/BannerListItemView.h"

class TaskQueueBase;
class IRomBrowserViewFactory;

class BannerListFileRecyclerAdapter : public FileRecyclerAdapter
{
public:
    BannerListFileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        VBlankTextureLoader* vblankTextureLoader)
        : FileRecyclerAdapter(romBrowserController, fileInfoManager, taskQueue, themeFileIconFactory)
        , _romBrowserViewFactory(romBrowserViewFactory)
        , _vblankTextureLoader(vblankTextureLoader) { }

    void GetViewSize(int& width, int& height) const override;
    SharedPtr<View> CreateView() const override;
    void BindView(SharedPtr<View> view, int index) const override;
    void ReleaseView(SharedPtr<View> view, int index) const override;

    void InitVram(const VramContext& vramContext) override;

private:
    const IRomBrowserViewFactory* _romBrowserViewFactory;
    BannerListItemView::VramToken _bannerListItemViewGraphics;
    BannerListItemView::FavoriteBadgeVramToken _favoriteBadgeVramToken;
    VBlankTextureLoader* _vblankTextureLoader;

    TaskResult<void> BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const override;
    void SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const override;
};

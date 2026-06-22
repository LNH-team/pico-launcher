#pragma once
#include "../FileRecyclerAdapter.h"
#include "../views/IconGridItemView.h"

class IRomBrowserViewFactory;

class IconGridFileRecyclerAdapter : public FileRecyclerAdapter
{
public:
    IconGridFileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory)
        : FileRecyclerAdapter(romBrowserController, fileInfoManager, taskQueue, themeFileIconFactory)
        , _romBrowserViewFactory(romBrowserViewFactory) { }

    void GetViewSize(int& width, int& height) const override;
    SharedPtr<View> CreateView() const override;
    void BindView(SharedPtr<View> view, int index) const override;
    void ReleaseView(SharedPtr<View> view, int index) const override;

    void InitVram(const VramContext& vramContext) override;

private:
    const IRomBrowserViewFactory* _romBrowserViewFactory;
    IconGridItemView::VramToken _iconGridItemViewGraphics;
    IconGridItemView::FavoriteBadgeVramToken _favoriteBadgeVramToken;

    TaskResult<void> BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const override;
    void SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const override;
};

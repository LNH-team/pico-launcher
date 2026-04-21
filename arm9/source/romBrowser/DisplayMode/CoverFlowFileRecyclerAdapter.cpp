#include "common.h"
#include "../FileInfoManager.h"
#include "core/task/TaskQueue.h"
#include "../views/CoverView.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "../FileType/UnknownFileCover.h"
#include "romBrowser/ICoverRepository.h"
#include "CoverFlowFileRecyclerAdapter.h"

void CoverFlowFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = 44;
    height = 44;
}

SharedPtr<View> CoverFlowFileRecyclerAdapter::CreateView() const
{
    return CoverView::CreateShared(
        std::make_unique<RomBrowserItemViewModel>(_romBrowserController), _vblankTextureLoader);
}

TaskResult<void> CoverFlowFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const
{
    auto coverView = static_cast<CoverView*>(view.GetPointer());
    coverView->GetViewModel().SetIndex(index);
    auto cover = _fileInfoManager->GetFileCover(index);
    if (cancelRequested)
    {
        _fileInfoManager->ReleaseFileInfo(index);
        return TaskResult<void>::Canceled();
    }
    coverView->SetCover(std::move(cover));
    coverView->UploadCoverGraphics();
    if (cancelRequested)
    {
        coverView->ClearCover();
        _fileInfoManager->ReleaseFileInfo(index);
        return TaskResult<void>::Canceled();
    }
    return TaskResult<void>::Completed();
}

void CoverFlowFileRecyclerAdapter::SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const
{
    auto coverView = static_cast<CoverView*>(view.GetPointer());
    coverView->GetViewModel().SetQueueTask(std::move(queueTask));
}

void CoverFlowFileRecyclerAdapter::ReleaseView(SharedPtr<View> view, int index) const
{
    LOG_DEBUG("Releasing %d\n", index);
    auto coverView = static_cast<CoverView*>(view.GetPointer());
    coverView->ClearCover();
    coverView->GetViewModel().SetIndex(-1);
    coverView->GetViewModel().CancelQueueTask();
    _fileInfoManager->ReleaseFileInfo(index);
}

void CoverFlowFileRecyclerAdapter::InitVram(const VramContext& vramContext)
{
    // _iconGridItemViewGraphics = _romBrowserViewFactory->UploadIconGridItemViewGraphics(vramManager);
}

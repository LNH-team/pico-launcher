#include "common.h"
#include "romBrowser/FileInfoManager.h"
#include "core/task/TaskQueue.h"
#include "MaterialCoverView.h"
#include "romBrowser/Theme/IRomBrowserViewFactory.h"
#include "romBrowser/FileType/UnknownFileCover.h"
#include "romBrowser/ICoverRepository.h"
#include "MaterialCoverFlowFileRecyclerAdapter.h"

void MaterialCoverFlowFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = COVER_WIDTH;
    height = COVER_HEIGHT;
}

SharedPtr<View> MaterialCoverFlowFileRecyclerAdapter::CreateView() const
{
    return MaterialCoverView::CreateShared(
        std::make_unique<RomBrowserItemViewModel>(_romBrowserController), _vblankTextureLoader);
}

TaskResult<void> MaterialCoverFlowFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const
{
    auto coverView = static_cast<MaterialCoverView*>(view.GetPointer());
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

void MaterialCoverFlowFileRecyclerAdapter::SetQueueTask(const SharedPtr<View>& view, QueueTask<void> queueTask) const
{
    auto coverView = static_cast<MaterialCoverView*>(view.GetPointer());
    coverView->GetViewModel().SetQueueTask(std::move(queueTask));
}

void MaterialCoverFlowFileRecyclerAdapter::ReleaseView(SharedPtr<View> view, int index) const
{
    LOG_DEBUG("Releasing %d\n", index);
    auto coverView = static_cast<MaterialCoverView*>(view.GetPointer());
    coverView->ClearCover();
    coverView->GetViewModel().SetIndex(-1);
    coverView->GetViewModel().CancelQueueTask();
    _fileInfoManager->ReleaseFileInfo(index);
}
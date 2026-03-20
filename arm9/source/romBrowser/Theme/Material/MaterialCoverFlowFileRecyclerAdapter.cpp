#include "common.h"
#include "romBrowser/FileInfoManager.h"
#include "core/task/TaskQueue.h"
#include "romBrowser/views/CoverView.h"
#include "romBrowser/Theme/IRomBrowserViewFactory.h"
#include "romBrowser/FileType/UnknownFileCover.h"
#include "romBrowser/ICoverRepository.h"
#include "MaterialCoverFlowFileRecyclerAdapter.h"

void MaterialCoverFlowFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = COVER_WIDTH;
    height = COVER_HEIGHT;
}

View* MaterialCoverFlowFileRecyclerAdapter::CreateView() const
{
    return new CoverView(_vblankTextureLoader);
}

void MaterialCoverFlowFileRecyclerAdapter::DestroyView(View* view) const
{
    auto coverView = static_cast<CoverView*>(view);
    delete coverView;
}

TaskResult<void> MaterialCoverFlowFileRecyclerAdapter::BindView(View* view, int index,
    const InternalFileInfo* internalFileInfo, const vu8& cancelRequested) const
{
    auto coverView = static_cast<CoverView*>(view);
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

void MaterialCoverFlowFileRecyclerAdapter::ReleaseView(View* view, int index) const
{
    LOG_DEBUG("Releasing %d\n", index);
    auto coverView = static_cast<CoverView*>(view);
    coverView->ClearCover();
    _fileInfoManager->ReleaseFileInfo(index);
}
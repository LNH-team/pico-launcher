#include "common.h"
#include "core/task/TaskQueue.h"
#include "FileInfoManager.h"
#include "FileRecyclerAdapter.h"

u32 FileRecyclerAdapter::GetItemCount() const
{
    return _fileInfoManager->GetItemCount();
}

void FileRecyclerAdapter::BindView(SharedPtr<View> view, int index) const
{
    LOG_DEBUG("Binding %d\n", index);
    auto queueTask = _taskQueue->Enqueue([=, this] (const vu8& cancelRequested)
    {
        if (cancelRequested)
        {
            LOG_DEBUG("Task to load %d was canceled\n", index);
            return TaskResult<void>::Canceled();
        }

        LOG_DEBUG("Started task to load %d\n", index);
        _fileInfoManager->LoadFileInfo(index);
        auto internalFileInfo = _fileInfoManager->GetInternalFileInfo(index);
        if (cancelRequested)
        {
            _fileInfoManager->ReleaseFileInfo(index);
            return TaskResult<void>::Canceled();
        }
        return BindView(view, index, internalFileInfo, cancelRequested);
    });
    SetQueueTask(view, std::move(queueTask));
}

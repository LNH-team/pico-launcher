#pragma once
#include "core/task/TaskQueue.h"

class IRomBrowserItemViewModel
{
public:
    virtual ~IRomBrowserItemViewModel() = default;

    virtual void Activate() = 0;
    virtual void ShowGameInfo() = 0;
    virtual void SetIndex(int index) = 0;
    virtual void SetQueueTask(QueueTask<void> queueTask) = 0;
    virtual void CancelQueueTask() = 0;
    virtual void DisposeQueueTaskWhenComplete() = 0;
    /// @brief Whether the background task that loads this row's title/icon is
    ///        still running (or hasn't started yet). While true, that task's
    ///        completion will still call SetGameTitle()/SetFileName() from the
    ///        IO thread, so anything driven from the main thread must not
    ///        touch the same labels at the same time.
    virtual bool IsQueueTaskPending() const = 0;

protected:
    IRomBrowserItemViewModel() = default;
};

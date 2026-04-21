#pragma once
#include "core/task/TaskQueue.h"

class IRomBrowserController;

class RomBrowserItemViewModel
{
public:
    explicit RomBrowserItemViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void Activate();
    void ShowGameInfo();

    void SetIndex(int index)
    {
        _index = index;
    }

    void SetQueueTask(QueueTask<void> queueTask)
    {
        _queueTask = std::move(queueTask);
    }

    void CancelQueueTask()
    {
        _queueTask.CancelTask();
    }

    void DisposeQueueTaskWhenComplete()
    {
        if (_queueTask.GetTask().IsCompleted())
        {
            _queueTask.Dispose();
        }
    }

private:
    int _index = -1;
    QueueTask<void> _queueTask;

    IRomBrowserController* _romBrowserController;
};

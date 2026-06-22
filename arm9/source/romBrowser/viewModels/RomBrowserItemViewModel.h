#pragma once
#include <atomic>
#include "core/task/TaskQueue.h"

class IRomBrowserController;

class RomBrowserItemViewModel
{
public:
    explicit RomBrowserItemViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void Activate();
    void ShowGameInfo();
    void ToggleFavorite();

    // Cached on SetIndex() (runs on _ioTaskQueue, same as icon loading) so Draw(),
    // which runs on the render thread every frame, never touches the SD card.
    bool IsFavorite() const { return _isFavorite.load(std::memory_order_acquire); }

    void SetIndex(int index);

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
    std::atomic<bool> _isFavorite { false };
    QueueTask<void> _queueTask;

    IRomBrowserController* _romBrowserController;
};

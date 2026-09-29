#pragma once
#include "IRomBrowserItemViewModel.h"

class IRomBrowserController;

class RomBrowserItemViewModel : public IRomBrowserItemViewModel
{
public:
    explicit RomBrowserItemViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void Activate() override;
    void ShowGameInfo() override;
    void ToggleFavorite() override;

    // Cached on SetIndex() (runs on _ioTaskQueue, same as icon loading) so Draw(),
    // which runs on the render thread every frame, never touches the SD card.
    bool IsFavorite() const override;

    void SetIndex(int index) override;

    void SetQueueTask(QueueTask<void> queueTask) override
    {
        _queueTask = std::move(queueTask);
    }

    void CancelQueueTask() override
    {
        _queueTask.CancelTask();
    }

    void DisposeQueueTaskWhenComplete() override
    {
        if (_queueTask.GetTask().IsCompleted())
        {
            _queueTask.Dispose();
        }
    }

private:
    int _index = -1;
    volatile bool _isFavorite = false;
    QueueTask<void> _queueTask;

    IRomBrowserController* _romBrowserController;
};

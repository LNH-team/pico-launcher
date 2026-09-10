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

    void SetIndex(int index) override
    {
        _index = index;
    }

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
        if (_queueTask.IsValid() && _queueTask.GetTask().IsCompleted())
        {
            _queueTask.Dispose();
        }
    }

    bool IsQueueTaskPending() const override
    {
        return _queueTask.IsValid() && !_queueTask.GetTask().IsCompleted();
    }

private:
    int _index = -1;
    QueueTask<void> _queueTask;

    IRomBrowserController* _romBrowserController;
};

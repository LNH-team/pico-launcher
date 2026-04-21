#include "common.h"
#include "TaskQueue.h"

void TaskQueueBase::ThreadMain()
{
    while (true)
    {
        _idle = false;
        while (true)
        {
            u32 irqs = rtos_disableIrqs();
            auto task = _taskList.GetHead();
            if (!task)
            {
                rtos_restoreIrqs(irqs);
                break;
            }
            _taskList.Remove(task);
            task->Execute(irqs);
            if (task->GetDestroyWhenComplete())
            {
                // this will destroy the task
                ReturnOwnership(task);
            }
        }
        if (_endThreadWhenDone)
            return;
        _idle = true;
        rtos_waitEvent(&_event, false, true);
    }
}

void QueueTaskBase::Dispose()
{
    if (_task)
    {
        TaskBase* task = _task;
        _task = nullptr;
        _taskQueue->ReturnOwnership(task);
        _taskQueue = nullptr;
    }
}
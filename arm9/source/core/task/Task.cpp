#include "common.h"
#include "Task.h"

void TaskBase::RequestCancel()
{
    u32 irqs = rtos_disableIrqs();
    _cancelRequested = true;
    if (_state == TaskState::NotStarted)
    {
        _state = TaskState::Canceled;
        rtos_wakeupQueue(&_threadQueue);
    }
    rtos_restoreIrqs(irqs);
}

void TaskBase::Execute(u32 irqs)
{
    if (_state == TaskState::NotStarted)
    {
        _state = TaskState::Running;
        rtos_restoreIrqs(irqs);
        TaskState finalState = ExecuteDirect();
        SetFinalState(finalState);
    }
    else
    {
        rtos_restoreIrqs(irqs);
    }
}

void TaskBase::SetFinalState(TaskState finalState)
{
    u32 irqs = rtos_disableIrqs();
    if (_state == TaskState::Running)
    {
        _state = finalState;
        rtos_wakeupQueue(&_threadQueue);
    }
    rtos_restoreIrqs(irqs);
}

bool TaskBase::Wait()
{
    u32 irq = rtos_disableIrqs();

    if (!IsCompleted())
        rtos_queueThread(rtos_getCurThread(), &_threadQueue);

    rtos_restoreIrqs(irq);

    return IsCompletedSuccessfully();
}
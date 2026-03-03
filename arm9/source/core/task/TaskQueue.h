#pragma once
#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/rtos/rtosThread.h>
#include "core/BitVector.h"
#include "Task.h"

class TaskQueueBase;

class QueueTaskBase
{
public:
    // forbid copies
    QueueTaskBase(const QueueTaskBase&) = delete;
    QueueTaskBase& operator=(const QueueTaskBase&) = delete;

    // move assignment
    QueueTaskBase& operator=(QueueTaskBase&& other)
    {
        _taskQueue = other._taskQueue;
        _task = other._task;
        other._taskQueue = nullptr;
        other._task = nullptr;
        return *this;
    }

    ~QueueTaskBase()
    {
        // extra check here for optimizing out the dispose call after move assignment
        if (_task)
            Dispose();
    }

    void Dispose();

    void CancelTask()
    {
        if (_task)
        {
            _task->RequestCancel();
            Dispose();
        }
    }

    constexpr bool IsValid() const { return _task != nullptr; }

protected:
    TaskBase* _task;

    QueueTaskBase(TaskBase* task, TaskQueueBase* taskQueue)
        : _task(task), _taskQueue(taskQueue) { }

private:
    TaskQueueBase* _taskQueue;
};

template <typename T>
class QueueTask : public QueueTaskBase
{
public:
    QueueTask()
        : QueueTaskBase(nullptr, nullptr) { }

    QueueTask(Task<T>* task, TaskQueueBase* taskQueue)
        : QueueTaskBase(task, taskQueue) { }

    const Task<T>& GetTask() const { return (const Task<T>&)*_task; }
};

class TaskQueueBase
{
protected:
    template <typename T>
    static T TaskResultToResultType(TaskResult<T> arg);

public:
    virtual void ReturnOwnership(TaskBase* task) = 0;

    template <typename FuncType>
    [[gnu::noinline]]
    auto Enqueue(FuncType&& function) -> QueueTask<decltype(TaskResultToResultType(function(*new vu8())))>
    {
        using TaskType = FuncTask<decltype(TaskResultToResultType(function(*new vu8()))), FuncType>;
        // static_assert(sizeof(TaskType) <= MaxTaskSize, "Task is too big for this pool");
        void* slot = GetSlot();
        auto task = new (slot) TaskType(std::move(function));
        Enqueue(task);
        return QueueTask(task, this);
    }

protected:
    rtos_event_t _event;
    vu32 _queueReadPtr = 0;
    vu32 _queueWritePtr = 0;
    volatile bool _endThreadWhenDone = false;
    volatile bool _idle = true;

    void ThreadMain(TaskBase** queue, u32 queueLength);

    TaskQueueBase()
    {
        rtos_createEvent(&_event);
    }

    virtual void* GetSlot() = 0;
    virtual void Enqueue(TaskBase* task) = 0;
};

template <u32 QueueLength, u32 MaxTaskSize>
class TaskQueue : public TaskQueueBase
{
public:
    using TaskQueueBase::Enqueue;

    ~TaskQueue()
    {
        StopThread();
    }

    [[gnu::noinline]]
    void ReturnOwnership(TaskBase* task) override
    {
        u32 irqs = rtos_disableIrqs();
        if (task->IsCompleted())
        {
            task->~TaskBase();
            u32 slot = ((u32)task - (u32)_taskPool) / ((MaxTaskSize + 3) & ~3);
            _poolOccupation.Set(slot, 0);
        }
        else
        {
            task->SetDestroyWhenComplete();
        }
        rtos_restoreIrqs(irqs);
    }

    [[gnu::noinline]]
    void StartThread(u8 threadPriority, u32* stack, u32 stackSize)
    {
        if (_threadStarted)
            return;
        _endThreadWhenDone = false;
        _threadStarted = true;
        rtos_createThread(&_thread, threadPriority, ThreadMain, this, stack, stackSize);
        rtos_wakeupThread(&_thread);
    }

    [[gnu::noinline]]
    void StopThread()
    {
        if (!_threadStarted)
            return;
        _endThreadWhenDone = true;
        rtos_signalEvent(&_event);
        rtos_joinThread(&_thread);
        _threadStarted = false;
    }

    bool IsIdle() const
    {
        return _queueReadPtr == _queueWritePtr && _idle;
    }

private:
    u32 _taskPool[QueueLength][(MaxTaskSize + 3) / 4];
    BitVector<QueueLength> _poolOccupation;
    TaskBase* _queue[QueueLength];
    rtos_thread_t _thread;
    bool _threadStarted = false;

    [[gnu::noinline]]
    u32 AcquireSlot()
    {
        u32 slot;
        u32 irqs = rtos_disableIrqs();
        {
            slot = _poolOccupation.FindFirstZero();
            _poolOccupation.Set(slot, 1);
        }
        rtos_restoreIrqs(irqs);
        LOG_DEBUG("AcquireSlot: %d\n", slot);
        return slot;
    }

    void* GetSlot() override
    {
        return _taskPool[AcquireSlot()];
    }

    [[gnu::noinline]]
    void Enqueue(TaskBase* task) override
    {
        u32 writePtr = _queueWritePtr;
        _queue[writePtr] = task;
        if (writePtr == QueueLength - 1)
            _queueWritePtr = 0;
        else
            _queueWritePtr = writePtr + 1;
        rtos_signalEvent(&_event);
    }

    static void ThreadMain(void* arg)
    {
        ((TaskQueue*)arg)->ThreadMain();
    }

    void ThreadMain()
    {
        TaskQueueBase::ThreadMain(&_queue[0], QueueLength);
    }
};
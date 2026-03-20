#pragma once
#include <type_traits>
#include <memory>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include "TaskResult.h"

class TaskBase
{
public:
    virtual ~TaskBase() { }

    void Execute();

    TaskState GetState() const { return _state; }

    bool IsCompleted() const { return _state >= TaskState::Completed; }
    bool IsCanceled() const { return _state == TaskState::Canceled; }
    bool IsFailed() const { return _state == TaskState::Failed; }
    bool IsCompletedSuccessfully() const { return _state == TaskState::Completed; }

    bool Wait();

    bool GetDestroyWhenComplete() const { return _destroyWhenComplete; }
    void SetDestroyWhenComplete() { _destroyWhenComplete = true; }

    void RequestCancel() { _cancelRequested = true; }
    bool IsCancelRequested() const { return _cancelRequested; }

protected:
    volatile u8 _cancelRequested = false;

    virtual TaskState ExecuteDirect() = 0;

private:
    rtos_thread_queue_t _threadQueue = { NULL };
    volatile TaskState _state = TaskState::NotStarted;
    volatile u8 _destroyWhenComplete = false;

    void SetFinalState(TaskState finalState);
};

template <class T>
class Task : public TaskBase
{
public:
    const T& GetResult() const { return this->_result; }

protected:
    virtual TaskResult<T> ExecuteFunc() const = 0;

private:
    T _result;

    TaskState ExecuteDirect()
    {
        auto result = ExecuteFunc();
        if (result.GetFinalState() == TaskState::Completed)
            this->_result = result.GetResult();
        return result.GetFinalState();
    }
};

template <>
class Task<void> : public TaskBase
{
protected:
    virtual TaskResult<void> ExecuteFunc() = 0;

private:
    TaskState ExecuteDirect() override
    {
        auto result = ExecuteFunc();
        return result.GetFinalState();
    }
};

template <class T, typename FuncType>
class FuncTask : public Task<T>
{
public:
    FuncTask(FuncType&& function)
        : _function(std::move(function)) { }

private:
    FuncType _function;

    TaskResult<T> ExecuteFunc() override { return _function((const volatile u8&)this->_cancelRequested); }
};

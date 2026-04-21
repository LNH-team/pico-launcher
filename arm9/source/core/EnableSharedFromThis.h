#pragma once
#include "SharedPtr.h"
#include "WeakPtr.h"

class EnableSharedFromThisBase
{
    template <class Y>
    friend class SharedPtr;

    template <class Y>
    friend class EnableSharedFromThis;

private:
    EnableSharedFromThisBase() = default;
};

template <class T>
class EnableSharedFromThis : public EnableSharedFromThisBase
{
    template <class Y>
    friend class SharedPtr;

public:
    SharedPtr<T> SharedFromThis()
    {
        return __sharedFromThisWeakPtr.Lock();
    }

    WeakPtr<T> WeakFromThis()
    {
        return __sharedFromThisWeakPtr;
    }

private:
    WeakPtr<T> __sharedFromThisWeakPtr;

    template <class Y>
    void __SetSharedFromThisWeakPtr(const SharedPtr<Y>& sharedPtr)
    {
        __sharedFromThisWeakPtr = WeakPtr<Y>(sharedPtr.GetPointer(), sharedPtr._refCount);
    }
};

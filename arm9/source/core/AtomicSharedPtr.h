#pragma once
#include <type_traits>
#include <libtwl/rtos/rtosIrq.h>
#include "SharedPtr.h"

class AtomicSharedPtrBase
{
public:
    void Reset()
    {
        Reset(nullptr, nullptr, false);
    }

protected:
    void* volatile _object;
    RefCount* volatile _refCount;

    AtomicSharedPtrBase()
        : _object(nullptr), _refCount(nullptr) { }

    AtomicSharedPtrBase(void* object, RefCount* refCount)
        : _object(object), _refCount(refCount) { }

    ~AtomicSharedPtrBase()
    {
        Reset(nullptr, nullptr, false);
    }

    void Reset(void* newObject, RefCount* newRefCount, bool increaseNewRefCount);
};

template <class T>
class AtomicSharedPtr : public AtomicSharedPtrBase
{
public:
    AtomicSharedPtr() { }

    AtomicSharedPtr(std::nullptr_t) { }

    AtomicSharedPtr(const SharedPtr<T>& sharedPtr)
        : AtomicSharedPtrBase(sharedPtr._object, sharedPtr._refCount)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    AtomicSharedPtr(const SharedPtr<Y>& sharedPtr)
        : AtomicSharedPtrBase(static_cast<T*>(sharedPtr.GetPointer()), sharedPtr._refCount)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
    }

    template <class Y>
    explicit AtomicSharedPtr(const SharedPtr<Y>& sharedPtr)
        : AtomicSharedPtrBase(static_cast<T*>(sharedPtr.GetPointer()), sharedPtr._refCount)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
    }

    AtomicSharedPtr(SharedPtr<T>&& sharedPtr)
        : AtomicSharedPtrBase(sharedPtr._object, sharedPtr._refCount)
    {
        sharedPtr._object = nullptr;
        sharedPtr._refCount = nullptr;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    AtomicSharedPtr(SharedPtr<Y>&& sharedPtr)
        : AtomicSharedPtrBase(static_cast<T*>(sharedPtr.GetPointer()), sharedPtr._refCount)
    {
        sharedPtr._object = nullptr;
        sharedPtr._refCount = nullptr;
    }

    template <class Y>
    explicit AtomicSharedPtr(SharedPtr<Y>&& sharedPtr)
        : AtomicSharedPtrBase(static_cast<T*>(sharedPtr.GetPointer()), sharedPtr._refCount)
    {
        sharedPtr._object = nullptr;
        sharedPtr._refCount = nullptr;
    }

    AtomicSharedPtr& operator=(const SharedPtr<T>& sharedPtr)
    {
        Reset(sharedPtr._object, sharedPtr._refCount, true);
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    AtomicSharedPtr<T>& operator=(const SharedPtr<Y>& sharedPtr)
    {
        Reset(static_cast<T*>(sharedPtr.GetPointer()), sharedPtr._refCount, true);
        return *this;
    }

    AtomicSharedPtr& operator=(SharedPtr<T>&& sharedPtr)
    {
        Reset(sharedPtr._object, sharedPtr._refCount, false);
        sharedPtr._object = nullptr;
        sharedPtr._refCount = nullptr;
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    AtomicSharedPtr<T>& operator=(SharedPtr<Y>&& sharedPtr)
    {
        Reset(static_cast<T*>(sharedPtr.GetPointer()), sharedPtr._refCount, false);
        sharedPtr._object = nullptr;
        sharedPtr._refCount = nullptr;
        return *this;
    }

    SharedPtr<T> Lock() const
    {
        u32 irq = rtos_disableIrqs();
        auto object = static_cast<T*>(_object);
        auto refCount = _refCount;
        if (refCount)
        {
            refCount->refCount++;
        }
        rtos_restoreIrqs(irq);
        return SharedPtr<T>(object, refCount);
    }
};

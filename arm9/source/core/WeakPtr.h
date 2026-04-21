#pragma once
#include <type_traits>
#include "RefCount.h"
#include "SharedPtr.h"

class WeakPtrBase
{
public:
    ~WeakPtrBase()
    {
        Reset();
    }

    void Reset()
    {
        if (_refCount)
        {
            Reset();
        }
    }

protected:
    RefCount* _refCount;

    WeakPtrBase()
        : _refCount(nullptr) { }

    explicit WeakPtrBase(RefCount* refCount)
        : _refCount(refCount) { }

    void ResetIntern();
    bool LockIntern() const;
};

template <class T>
class WeakPtr : public WeakPtrBase
{
    template <class Y> friend class WeakPtr;
    template <class Y> friend class EnableSharedFromThis;

public:
    WeakPtr() { }

    WeakPtr(std::nullptr_t) { }

    WeakPtr(const WeakPtr& other)
        : WeakPtrBase(other._refCount), _object(other._object)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->weakRefCount);
        }
    }

    WeakPtr(WeakPtr&& other)
        : WeakPtrBase(other._refCount), _object(other._object)
    {
        other._refCount = nullptr;
        other._object = nullptr;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    WeakPtr(const SharedPtr<Y>& sharedPtr)
        : WeakPtrBase(sharedPtr._refCount), _object(static_cast<T*>(sharedPtr.GetPointer()))
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->weakRefCount);
        }
    }

    template <class Y>
    explicit WeakPtr(const SharedPtr<Y>& sharedPtr)
        : WeakPtrBase(sharedPtr._refCount), _object(static_cast<T*>(sharedPtr.GetPointer()))
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->weakRefCount);
        }
    }

    WeakPtr& operator=(const WeakPtr& other)
    {
        Reset();
        _object = other._object;
        _refCount = other._refCount;
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->weakRefCount);
        }
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    WeakPtr<T>& operator=(const WeakPtr<Y>& other)
    {
        Reset();
        _object = static_cast<T*>(static_cast<Y*>(other._object));
        _refCount = other._refCount;
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->weakRefCount);
        }
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    WeakPtr<T>& operator=(const SharedPtr<Y>& sharedPtr)
    {
        Reset();
        _object = static_cast<T*>(sharedPtr.GetPointer());
        _refCount = sharedPtr._refCount;
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->weakRefCount);
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other)
    {
        Reset();
        _object = other._object;
        _refCount = other._refCount;
        other._object = nullptr;
        other._refCount = nullptr;
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    WeakPtr<T>& operator=(WeakPtr<Y>&& other)
    {
        Reset();
        _object = static_cast<T*>(static_cast<Y*>(other._object));
        _refCount = other._refCount;
        other._object = nullptr;
        other._refCount = nullptr;
        return *this;
    }

    SharedPtr<T> Lock() const
    {
        if (_refCount && LockIntern())
        {
            return SharedPtr<T>(_object, _refCount);
        }
        else
        {
            return SharedPtr<T>();
        }
    }

private:
    T* _object;

    WeakPtr(T* object, RefCount* refCount)
        : WeakPtrBase(refCount), _object(object) { }
};

#pragma once
#include <type_traits>
#include "RefCount.h"

class EnableSharedFromThisBase;

template <class T>
class EnableSharedFromThis;

class SharedPtrBase
{
public:
    void Reset()
    {
        if (_refCount != nullptr)
        {
            ResetIntern();
        }
    }

protected:
    void* _object;
    RefCount* _refCount;

    SharedPtrBase()
        : _object(nullptr), _refCount(nullptr) { }

    SharedPtrBase(void* object, RefCount* refCount)
        : _object(object), _refCount(refCount) { }

    SharedPtrBase(const void* object, RefCount* refCount)
        : _object((void*)object), _refCount(refCount) { }

    ~SharedPtrBase()
    {
        Reset();
    }

    void ResetIntern();
};

template <class T>
class SharedPtr : public SharedPtrBase
{
    template <class Y> friend class WeakPtr;
    template <class Y> friend class SharedPtr;
    template <class Y> friend class AtomicSharedPtr;
    template <class Y> friend class EnableSharedFromThis;

public:
    SharedPtr() { }

    SharedPtr(std::nullptr_t) { }

    explicit SharedPtr(T* object)
        : SharedPtrBase(object, object == nullptr ? nullptr : new StandaloneRefCount<T>(object))
    {
        if (_object != nullptr)
        {
            if constexpr (std::is_convertible<T*, EnableSharedFromThisBase*>::value)
            {
                _refCount->weakRefCount = 1;
                GetPointer()->__SetSharedFromThisWeakPtr(*this);
            }
        }
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    explicit SharedPtr(Y* object)
        : SharedPtrBase(static_cast<T*>(object), object == nullptr ? nullptr : new StandaloneRefCount<Y>(object))
    {
        if (object != nullptr)
        {
            if constexpr (std::is_convertible<Y*, EnableSharedFromThisBase*>::value)
            {
                _refCount->weakRefCount = 1;
                GetPointer()->__SetSharedFromThisWeakPtr(*this);
            }
        }
    }

    SharedPtr(const SharedPtr& other)
        : SharedPtrBase(other._object, other._refCount)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    SharedPtr(const SharedPtr<Y>& other)
        : SharedPtrBase(static_cast<T*>(other.GetPointer()), other._refCount)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
    }

    template <class Y>
    explicit SharedPtr(const SharedPtr<Y>& other)
        : SharedPtrBase(static_cast<T*>(other.GetPointer()), other._refCount)
    {
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
    }

    SharedPtr(SharedPtr&& other)
        : SharedPtrBase(other._object, other._refCount)
    {
        other._object = nullptr;
        other._refCount = nullptr;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    SharedPtr(SharedPtr<Y>&& other)
        : SharedPtrBase(static_cast<T*>(other.GetPointer()), other._refCount)
    {
        other._object = nullptr;
        other._refCount = nullptr;
    }

    template <class Y>
    explicit SharedPtr(SharedPtr<Y>&& other)
        : SharedPtrBase(static_cast<T*>(other.GetPointer()), other._refCount)
    {
        other._object = nullptr;
        other._refCount = nullptr;
    }

    static SharedPtr<T> MakeShared(auto&&... args)
    {
        auto refCount = new MakeSharedRefCount<T>(std::forward<decltype(args)>(args)...);
        return SharedPtr<T>(refCount->GetObject(), refCount, true);
    }

    SharedPtr& operator=(const SharedPtr& other)
    {
        Reset();
        _object = other._object;
        _refCount = other._refCount;
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    SharedPtr<T>& operator=(const SharedPtr<Y>& other)
    {
        Reset();
        _object = static_cast<T*>(other.GetPointer());
        _refCount = other._refCount;
        if (_refCount)
        {
            shared_ptr_increase_ref_count(_refCount->refCount);
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other)
    {
        Reset();
        _object = other._object;
        _refCount = other._refCount;
        other._object = nullptr;
        other._refCount = nullptr;
        return *this;
    }

    template <class Y> requires std::assignable_from<T*&, Y*>
    SharedPtr<T>& operator=(SharedPtr<Y>&& other)
    {
        Reset();
        _object = static_cast<T*>(other.GetPointer());
        _refCount = other._refCount;
        other._object = nullptr;
        other._refCount = nullptr;
        return *this;
    }

    T& operator*() const { return *static_cast<T*>(_object); }
    T* operator->() const { return static_cast<T*>(_object); }
    T* GetPointer() const { return static_cast<T*>(_object); }

    bool IsValid() const { return _object != nullptr; }
    operator bool() const { return _object != nullptr; }

private:
    SharedPtr(T* object, RefCount* refCount)
        : SharedPtrBase(object, refCount) { }

    SharedPtr(T* object, RefCount* refCount, bool doSharedFromThis)
        : SharedPtrBase(object, refCount)
    {
        if (doSharedFromThis)
        {
            if constexpr (std::is_convertible<T*, EnableSharedFromThisBase*>::value)
            {
                _refCount->weakRefCount = 1;
                GetPointer()->__SetSharedFromThisWeakPtr(*this);
            }
        }
    }
};

#define SHARED_ONLY(className) \
    friend class MakeSharedRefCount<className>; \
public: \
    static SharedPtr<className> CreateShared(auto&&... args) \
    { return SharedPtr<className>::MakeShared(std::forward<decltype(args)>(args)...); } \
private:

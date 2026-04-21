#pragma once
#include <array>

extern "C" void shared_ptr_increase_ref_count(vu32& refCount);

class RefCount
{
public:
    vu32 refCount;
    vu32 weakRefCount;

    virtual ~RefCount() = default;

    virtual void DestructObject() = 0;

protected:
    explicit RefCount()
        : refCount(1), weakRefCount(0) { }
};

template <class T>
class StandaloneRefCount : public RefCount
{
public:
    explicit StandaloneRefCount(T* object)
        : _object(object) { }

    void DestructObject() final
    {
        delete _object;
    }

private:
    T* _object;
};

template <class T>
class alignas(T) MakeSharedRefCount : public RefCount
{
public:
    explicit MakeSharedRefCount(auto&&... args)
    {
        new (_object.data()) T(std::forward<decltype(args)>(args)...);
    }

    T* GetObject()
    {
        return reinterpret_cast<T*>(_object.data());
    }

    void DestructObject() final
    {
        reinterpret_cast<T*>(_object.data())->~T();
    }

private:
    std::array<u8, sizeof(T)> _object alignas(T);
};

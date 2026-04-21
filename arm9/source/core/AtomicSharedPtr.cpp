#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include "AtomicSharedPtr.h"

void AtomicSharedPtrBase::Reset(void* newObject, RefCount* newRefCount, bool increaseNewRefCount)
{
    u32 irq = rtos_disableIrqs(); // 1
    auto refCount = _refCount;
    _object = newObject;
    _refCount = newRefCount;
    if (increaseNewRefCount && _refCount)
    {
        _refCount->refCount++;
    }
    if (refCount && --refCount->refCount == 0) [[gnu::unlikely]]
    {
        refCount->weakRefCount++; // ensure the ref count is not destructed elsewhere
        rtos_restoreIrqs(irq); // 1
        refCount->DestructObject();
        irq = rtos_disableIrqs(); // 2
        if (--refCount->weakRefCount == 0) [[gnu::unlikely]]
        {
            rtos_restoreIrqs(irq); // 2
            delete refCount;
        }
        else
        {
            rtos_restoreIrqs(irq); // 2
        }
    }
    else
    {
        rtos_restoreIrqs(irq); // 1
    }
}

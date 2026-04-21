#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include "SharedPtr.h"

void SharedPtrBase::ResetIntern()
{
    auto refCount = _refCount;
    _object = nullptr;
    _refCount = nullptr;
    u32 irq = rtos_disableIrqs(); // 1
    if (--refCount->refCount == 0) [[gnu::unlikely]]
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

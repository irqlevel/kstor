#include "task.h"
#include "vector.h"

namespace Core
{

Task::Task(int pid, Error& err)
    : TaskPtr(nullptr)
{
    if (!err.Ok())
        return;

    TaskPtr = get_kapi()->task_lookup(pid);
    if (TaskPtr == nullptr)
    {
        err = MakeError(Error::NotFound);
        return;
    }
}

Error Task::DumpStack(AString& result)
{
    Vector<byte_t> stackBuf;
    if (!stackBuf.ReserveAndUse(4 * 4096))
        return MakeError(Error::NoMemory);

    Error err;
    AString endLine("\n", err);
    if (!err.Ok())
        return err;

    unsigned long sp_delta;
    size_t stackSize = get_kapi()->task_stack_read(TaskPtr, stackBuf.GetBuf(), stackBuf.GetSize(), &sp_delta);
    if (stackSize == 0) 
    {
        return MakeError(Error::InvalidValue);
    }

    if (!stackBuf.Truncate(stackSize))
        return MakeError(Error::InvalidState);

    unsigned long *stackPos = reinterpret_cast<unsigned long *>(stackBuf.GetBuf());
    for (size_t i = 0, off = 0;
        i < stackBuf.GetSize() / sizeof(unsigned long);
        i++, stackPos++, off += sizeof(unsigned long))
    {
        if (sp_delta != 0 && off < sp_delta)
            continue;

        AString symbol;

        if (!symbol.ReserveAndUse(4096))
            return MakeError(Error::NoMemory);

        size_t len = get_kapi()->sprint_symbol(symbol.GetBuf(), *stackPos);
        if (!symbol.Truncate(len))
            return MakeError(Error::InvalidState);

        err = result.Append(symbol);
        if (!err.Ok())
            return err;

        err = result.Append(endLine);
        if (!err.Ok())
            return err;
    }

    return MakeError(Error::Success);
}

Task::~Task()
{
    if (TaskPtr != nullptr)
    {
        get_kapi()->task_put(TaskPtr);
        TaskPtr = nullptr;
    }
}
}
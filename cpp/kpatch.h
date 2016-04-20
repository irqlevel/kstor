#pragma once

#include "main.h"
#include "astring.h"
#include "vector.h"

class KPatch
{
public:
    KPatch(int err);
    virtual ~KPatch();

    static bool KernelWrite(void *dst, const void *src, size_t size);
    static bool KernelRead(void *dst, const void *src, size_t size);

    static unsigned long GetSymbolAddress(const AString& symbol);

    int GetCallers(unsigned long addr, Vector<unsigned long>& callers);
    int GetCallers(const AString& symbol, Vector<unsigned long>& callers);

private:
    unsigned long KernelStart;
    unsigned long KernelEnd;

    KPatch(const KPatch& kpatch) = delete;
    KPatch(KPatch&& kpatch) = delete;
    KPatch& operator=(const KPatch& kpatch) = delete;
    KPatch& operator=(KPatch&& kpatch) = delete;
};

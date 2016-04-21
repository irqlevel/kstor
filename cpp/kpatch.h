#pragma once

#include "main.h"
#include "astring.h"
#include "vector.h"
#include "hash_table.h"

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

    int PatchCall(const AString& symbol, void *pFunc);
    int UnpatchCall(const AString& symbol);

private:
#pragma pack(push, 1)
    struct RelativeCall
    {
        unsigned char OpCode;
        int Offset;
    };
#pragma pack(pop)

    class PatchCallCtx
    {
    private:
        PatchCallCtx();
        virtual ~PatchCallCtx();
    private:
        AString& Symbol;
        unsigned long OrigAddr;
        unsigned long PatchAddr;
        Vector<unsigned long> Callers;
    };

    typedef shared_ptr<PatchCallCtx> PatchCallCtxRef;

    HashTable<AStringRef, PatchCallCtxRef> Patches;

    unsigned long KernelStart;
    unsigned long KernelEnd;

    KPatch(const KPatch& kpatch) = delete;
    KPatch(KPatch&& kpatch) = delete;
    KPatch& operator=(const KPatch& kpatch) = delete;
    KPatch& operator=(KPatch&& kpatch) = delete;
};

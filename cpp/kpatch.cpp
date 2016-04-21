#include "kpatch.h"

KPatch::KPatch(int err)
    : Patches(256, MemType::Kernel, err
{
    if (err)
        return;

    AString startSymbol("startup_64", MemType::Kernel, err);
    if (err)
        return;

    KernelStart = GetSymbolAddress(startSymbol);
    if (!KernelStart)
    {
        err = E_NOT_FOUND;
        return;
    }

    AString endSymbol("_etext", MemType::Kernel, err);
    if (err)
        return;

    KernelEnd = GetSymbolAddress(endSymbol);
    if (!KernelEnd)
    {
        err = E_NOT_FOUND;
        return;
    }
    err = E_OK;
    trace(1, "KernelStart 0x%lx KernelEnd 0x%lx", KernelStart, KernelEnd);
}

KPatch::~KPatch()
{
}

int KPatch::GetCallers(unsigned long addr, Vector<unsigned long>& callers)
{
    struct RelativeCall call;
    unsigned char *pCurrByte, *pEndByte;

    pCurrByte = reinterpret_cast<unsigned char *>(KernelStart);
    pEndByte = reinterpret_cast<unsigned char *>(KernelEnd);

    while (pCurrByte != pEndByte)
    {
        if (!KernelRead(&call, pCurrByte, sizeof(call)))
        {
            pCurrByte++;
            continue;
        }

        if (call.OpCode == 0xE8)
        {
            unsigned long target;

            target = reinterpret_cast<unsigned long>
                    (reinterpret_cast<unsigned long>(pCurrByte + sizeof(call))
                    + call.Offset);
            if (target == addr)
            {
                if (!callers.PushBack(reinterpret_cast<unsigned long>
                                      (pCurrByte)))
                {
                    return E_NO_MEM;
                }
            }
        }
        pCurrByte++;
    }
    return E_OK;
}

int KPatch::GetCallers(const AString& symbol, Vector<unsigned long>& callers)
{
    unsigned long addr = GetSymbolAddress(symbol);

    return GetCallers(addr, callers);
}

bool KPatch::KernelWrite(void *dst, const void *src, size_t size)
{
    int err;

    err = get_kapi()->probe_kernel_write(dst, src, size);
    return (err) ? false : true;
}

bool KPatch::KernelRead(void *dst, const void *src, size_t size)
{
    int err;

    err = get_kapi()->probe_kernel_read(dst, src, size);
    return (err) ? false : true;
}

unsigned long KPatch::GetSymbolAddress(const AString& symbol)
{
    return get_kapi()->get_symbol_address(symbol.GetBuf());
}

PatchCallCtx::PatchCallCtx()
{
}

PatchCallCtx::~PatchCallCtx()
{
}

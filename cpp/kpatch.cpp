#include "kpatch.h"
#include "smp.h"
#include "auto_lock.h"

KPatch::KPatch(int err)
    : Patches(MemType::Kernel, 256, err, &AString::Compare, &AString::Hash),
      Lock(err, MemType::Kernel)
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

            target = reinterpret_cast<unsigned long>(pCurrByte + sizeof(call))
                        + call.Offset;

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

KPatch::PatchCallCtx::PatchCallCtx(KPatch *kp,
                                   const AString& symbol,
                                   unsigned long patchAddress,
                                   int& err)
    : Err(E_OK), Installed(false), Symbol(symbol, err),
      PatchAddress(patchAddress),
      Callers(MemType::Kernel), Owner(kp)
{
    if (err)
        return;

    OrigAddress = GetSymbolAddress(Symbol);
    if (!OrigAddress)
    {
        err = E_NOT_FOUND;
        return;
    }

    err = Owner->GetCallers(OrigAddress, Callers);
    if (err)
        return;

}

KPatch::PatchCallCtxRef
KPatch::PatchCallCtx::Make(KPatch *kp,
                           const AString& symbol,
                           unsigned long patchAddress)
{
    int err = E_OK;

    PatchCallCtxRef ref = PatchCallCtxRef(
                                        new (MemType::Kernel)PatchCallCtx(kp,
                                                symbol, patchAddress, err));
    if (!ref.get() || err)
        return PatchCallCtxRef();
    return ref;
}

void KPatch::PatchCallCtx::PatchInternal()
{
    int err;

    if (Installed)
    {
        Err = E_OK;
        return;
    }

    for (size_t i = 0; i < Callers.GetSize(); i++)
    {
        unsigned long caller = Callers[i];
        struct RelativeCall *pCall =
            reinterpret_cast<struct RelativeCall *>(caller);

        pCall->Offset = PatchAddress -
                            reinterpret_cast<unsigned long>(pCall + 1);

    }
    err = E_OK;
    if (!err)
        Installed = true;
    Err = err;
}

void KPatch::PatchCallCtx::RestoreInternal()
{
    int err;

    if (!Installed)
    {
        Err = E_OK;
        return;
    }

    for (size_t i = 0; i < Callers.GetSize(); i++)
    {
        unsigned long caller = Callers[i];
        struct RelativeCall *pCall =
            reinterpret_cast<struct RelativeCall *>(caller);

        pCall->Offset = OrigAddress -
                            reinterpret_cast<unsigned long>(pCall + 1);
    }
    err = E_OK;
    if (!err)
        Installed = false;
    Err = err;
}

void KPatch::PatchCallCtx::PatchClb(void* data)
{
    KPatch::PatchCallCtx* patchCtx = static_cast<KPatch::PatchCallCtx*>(data);

    patchCtx->PatchInternal();
}

int KPatch::PatchCallCtx::Patch()
{
    int err;

    if (Installed)
        return E_OK;

    err = Smp::CallFunctionCurrCpuOnly(&KPatch::PatchCallCtx::PatchClb, this);
    if (err)
        return err;
    err = Err;
    return err;
}

void KPatch::PatchCallCtx::RestoreClb(void* data)
{
    KPatch::PatchCallCtx* patchCtx = static_cast<KPatch::PatchCallCtx*>(data);

    patchCtx->RestoreInternal();
}

int KPatch::PatchCallCtx::Restore()
{
    int err;

    if (!Installed)
        return E_OK;

    err = Smp::CallFunctionCurrCpuOnly(&KPatch::PatchCallCtx::RestoreClb, this);
    if (err)
        return err;
    err = Err;
    return err;
}

KPatch::PatchCallCtx::~PatchCallCtx()
{
    Restore();
}

int KPatch::PatchCall(const AString& symbol, void *pFunc)
{
    AutoLock lock(Lock);

    KPatch::PatchCallCtxRef patch = Patches.Get(symbol);
    if (patch.get())
        return E_EXISTS;

    patch = PatchCallCtx::Make(this, symbol,
                               reinterpret_cast<unsigned long>(pFunc));
    if (!patch.get())
        return E_NO_MEM;

    int err = E_OK;
    if (!Patches.Insert(symbol, patch, err))
        return E_STATE;

    err = patch->Patch();
    if (err)
    {
        Patches.Remove(symbol);
        return err;
    }

    return E_OK;
}

int KPatch::UnpatchCall(const AString& symbol)
{
    AutoLock lock(Lock);

    KPatch::PatchCallCtxRef patch = Patches.Get(symbol);
    if (!patch.get())
        return E_OK;

    int err = patch->Restore();
    Patches.Remove(symbol);

    return err;
}

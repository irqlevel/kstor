#include "bug.h"

#ifdef __cplusplus
extern "C"
{
#endif


void __cxa_pure_virtual(void)
{
    panic(true);
}

#ifdef __cplusplus
}
#endif

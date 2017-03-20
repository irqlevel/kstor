#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "../system/kapi.h"

int CoreInit(struct kernel_api *api);

void CoreDeinit(void);

#ifdef __cplusplus
}
#endif

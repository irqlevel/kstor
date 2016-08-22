#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "../lkm/kapi.h"

int kstorage_init(struct kernel_api *kapi);

void kstorage_deinit(void);

#ifdef __cplusplus
}
#endif

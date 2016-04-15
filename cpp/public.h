#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "../core/kapi.h"

int cpp_init(struct kernel_api *kapi);
void cpp_deinit(void);

#ifdef __cplusplus
}
#endif

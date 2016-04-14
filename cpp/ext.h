#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

int cpp_init(struct kernel_api *kapi);
void cpp_deinit(void);

#ifdef __cplusplus
}
#endif

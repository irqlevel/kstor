#pragma once

#include "kapi.h"

int kapi_init(void);
void kapi_deinit(void);
struct kernel_api *kapi_get(void);

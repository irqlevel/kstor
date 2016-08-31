#pragma once

#include "kapi.h"

int kapi_init(void);
void kapi_deinit(void);
struct kernel_api *kapi_get(void);

void *kapi_kmalloc_gfp(size_t size, gfp_t flags);
void kapi_kfree(void *ptr);

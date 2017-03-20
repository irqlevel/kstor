#pragma once

#include <linux/slab.h>

void unique_key_init(void);
void unique_key_deinit(void);

int unique_key_register(void *key, void *value, gfp_t flags);
int unique_key_unregister(void *key, void *value);
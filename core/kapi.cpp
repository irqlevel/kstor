#include "kapi.h"

struct kernel_api g_kapi;

void kapi_init(struct kernel_api* api)
{
    g_kapi = *api;
}

struct kernel_api *get_kapi(void)
{
    return &g_kapi;
}

#include "main.h"

#include <memory.h>

struct kernel_api g_kapi;

int cpp_init(struct kernel_api *kapi)
{
	memcpy(&g_kapi, kapi, sizeof(*kapi));
	g_kapi.printf("kcpp: cpp_init\n");
	return 0;
}

void cpp_deinit(void)
{
	g_kapi.printf("kcpp: cpp_deinit\n");
}

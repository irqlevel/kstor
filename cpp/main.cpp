#include "main.h"
#include "kobj.h"
#include "shared_ptr.h"

#include <memory.h>

struct kernel_api g_kapi;

struct kernel_api *get_kapi(void)
{
	return &g_kapi;
}

int cpp_init(struct kernel_api *kapi)
{
	memcpy(&g_kapi, kapi, sizeof(*kapi));
	PRINTF("cpp_init\n");

	KObj obj(2);

	KObjRef pobj = KObjRef(new KObj(3));
	PRINTF("pobj %p val %d\n", pobj.get(), pobj->GetValue());

	return 0;
}

void cpp_deinit(void)
{
	PRINTF("cpp_deinit\n");
}

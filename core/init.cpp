#include "init.h"
#include "kapi.h"
#include "trace.h"

int core_init(struct kernel_api *api)
{
    kapi_init(api);
    trace(1, "core_init completed");
    return 0;
}

void core_deinit(void)
{
    trace(1,"core_deinit");
}

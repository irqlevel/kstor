#include "init.h"
#include "kapi.h"
#include "trace.h"

int CoreInit(struct kernel_api *api)
{
    kapi_init(api);
    trace(1, "inited");
    return 0;
}

void CoreDeinit(void)
{
    trace(1,"deinited");
}

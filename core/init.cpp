#include "init.h"
#include "kapi.h"
#include "trace.h"
#include "test.h"

int core_init(struct kernel_api *api)
{
    kapi_init(api);
    trace(1, "kstorage_init");

    run_tests();

    trace(1, "kstorage_init completed");
    return 0;
}

void core_deinit(void)
{
    trace(1,"kstorage_deinit");
}

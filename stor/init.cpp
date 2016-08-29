#include "init.h"
#include "test.h"
#include "../core/trace.h"

int stor_init(void)
{
    trace(1, "storage_init");

    run_tests();

    trace(1, "storage_init completed");
    return 0;
}

void stor_deinit(void)
{
    trace(1,"storage_deinit");
}

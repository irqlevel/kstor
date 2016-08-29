#include "init.h"
#include "test.h"
#include "../core/trace.h"

int stor_init(void)
{
    trace(1, "initing");

    run_tests();

    trace(1, "inited");
    return 0;
}

void stor_deinit(void)
{
    trace(1,"deinited");
}

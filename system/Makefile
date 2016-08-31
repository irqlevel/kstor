include $(PROJ_ROOT)/Makefile.inc

KMOD = kstor
KMOD_KO = $(KMOD).ko

obj-m = $(KMOD).o
$(KMOD)-objs := module.o kapi.o malloc_checker.o page_checker.o ../lib/stor.a ../lib/core.a

ccflags-y := -I$(src) -I.. $(COMMON_CFLAGS) -fno-inline    \
            -D __KERNEL__ -D __MALLOC_CHECKER__ \
            -D __MALLOC_CHECKER_STACK_TRACE__   \
            -D __MALLOC_CHECKER_FILL_CC__       \
            -D __PAGE_CHECKER__                 \
            -D __PAGE_CHECKER_STACK_TRACE__     \
            -D __PAGE_CHECKER_FILL_CC__         \

ifndef KERNEL_PATH
KERNEL_PATH=/lib/modules/$(shell uname -r)/build
endif

all:
	$(MAKE) -C $(KERNEL_PATH) M=$(PROJ_ROOT)/system modules
	cp $(KMOD_KO) ../bin/$(KMOD_KO)
clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(PROJ_ROOT)/system clean
	rm -r -f *.o

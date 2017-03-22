include $(PROJ_ROOT)/Makefile.inc

KMOD = kstor
KMOD_KO = $(KMOD).ko

obj-m = $(KMOD).o
$(KMOD)-objs := module.o ksocket.o kapi.o malloc_checker.o page_checker.o   \
                trace.o unique_key.o ../lib/kstor.a ../lib/core.a

DEBUG_CFLAGS = -O1 -g3 -ggdb3 -fno-inline -D __DEBUG__ -D __MALLOC_CHECKER__ \
            -D __MALLOC_CHECKER_STACK_TRACE__   \
            -D __MALLOC_CHECKER_FILL_CC__       \
            -D __PAGE_CHECKER__                 \
            -D __PAGE_CHECKER_STACK_TRACE__     \
            -D __PAGE_CHECKER_FILL_CC__         \
            -D __UNIQUE_KEY__         \

ccflags-y := -D __KERNEL__ -I$(src) -I.. $(COMMON_CFLAGS)

ifeq ($(DEBUG), ON)
ccflags-y += $(DEBUG_CFLAGS)
else
ccflags-y += -O2
endif

ifndef KERNEL_PATH
KERNEL_PATH=/lib/modules/$(shell uname -r)/build
endif

KBUILD_EXTRA_SYMBOLS = $(KERNEL_PATH)/Module.symvers

all:
	$(MAKE) -C $(KERNEL_PATH) M=$(PROJ_ROOT)/system modules
	cp $(KMOD_KO) ../bin/$(KMOD_KO)
clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(PROJ_ROOT)/system clean
	rm -r -f *.o

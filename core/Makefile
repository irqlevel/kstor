include $(PROJ_ROOT)/Makefile.inc

KMOD = kcpp
KMOD_KO = $(KMOD).ko

obj-m = $(KMOD).o
$(KMOD)-objs := module.o ../lib/cpp.a

ccflags-y := -I$(src) -I.. -D __KERNEL__ $(COMMON_CFLAGS)

ifndef KERNEL_PATH
KERNEL_PATH=/lib/modules/$(shell uname -r)/build
endif

all:
	$(MAKE) -C $(KERNEL_PATH) M=$(PROJ_ROOT)/core modules
	cp $(KMOD_KO) ../bin/$(KMOD_KO)
clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(PROJ_ROOT)/core clean
	rm -r -f *.o

include $(PROJ_ROOT)/Makefile.inc

CFLAGS= $(COMMON_CFLAGS) -std=c++11 -mcmodel=kernel -D__CPP__ -ffreestanding -fno-inline -fno-exceptions -fno-rtti
LIB_OUT = core.a

LIB_SRC = kapi.cpp init.cpp new.cpp atomic.cpp thread.cpp \
          event.cpp worker.cpp spinlock.cpp pure_virtual.cpp trace.cpp \
          list_entry.cpp smp.cpp rwsem.cpp error.cpp page.cpp \
		  block_device.cpp vfs_file.cpp random.cpp misc_device.cpp \
		  bitops.cpp bitmap.cpp socket.cpp sha256.cpp xxhash.cpp task.cpp \

all:
	rm -rf *.o *.a
	$(CPP) $(CFLAGS) $(LIB_SRC)
	$(AR) -cvq $(LIB_OUT) *.o
	cp $(LIB_OUT) $(LIB)/$(LIB_OUT)
	rm -rf $(LIB_OUT)
clean:
	rm -rf *.o *.a

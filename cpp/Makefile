include $(PROJ_ROOT)/Makefile.inc

CFLAGS= $(COMMON_CFLAGS) -std=c++11 -mcmodel=kernel -D__CPP__ -ffreestanding -fno-exceptions -fno-rtti
LIB_OUT = cpp.a

LIB_SRC = main.cpp new_delete.cpp kobj.cpp atomic.cpp thread.cpp event.cpp\
          worker.cpp spinlock.cpp pure_virtual.cpp

OBJS = $(LIBS_OBJ) $(LIBS_KOBJ)

all:
	rm -rf *.o *.a
	$(CPP) $(CFLAGS) $(LIB_SRC)
	$(AR) -cvq $(LIB_OUT) *.o
	cp $(LIB_OUT) $(LIB)/$(LIB_OUT)
	rm -rf $(LIB_OUT)
clean:
	rm -rf *.o *.a

include $(PROJ_ROOT)/Makefile.inc

CFLAGS= $(COMMON_CFLAGS) $(EXTRA_CFLAGS) -std=c++11 -mcmodel=kernel -D__CPP__ -ffreestanding -fno-exceptions -fno-rtti
LIB_OUT = kstor.a

LIB_SRC = init.cpp control_device.cpp volume.cpp server.cpp guid.cpp journal.cpp \
	block_allocator.cpp

all:
	rm -rf *.o *.a
	$(CPP) $(CFLAGS) $(LIB_SRC)
	$(AR) -cvq $(LIB_OUT) *.o
	cp $(LIB_OUT) $(LIB)/$(LIB_OUT)
	rm -rf $(LIB_OUT)
clean:
	rm -rf *.o *.a

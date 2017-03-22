include $(PROJ_ROOT)/Makefile.inc

CFLAGS= $(COMMON_CFLAGS) $(EXTRA_CFLAGS) -std=c++11 -D__CPP__

BIN_OUT = kstor-ctl

all: ctl
	cp $(BIN_OUT) $(BIN)/$(BIN_OUT)
	rm -rf *.o $(BIN_OUT)

ctl: ctl.o main.o
	$(CPP) ctl.o main.o -o $(BIN_OUT)

ctl.o: ctl.cpp
	$(CPP) -c $(CFLAGS) ctl.cpp

main.o: ctl.cpp
	$(CPP) -c $(CFLAGS) main.cpp

clean:
	rm -rf *.o $(BIN_OUT)

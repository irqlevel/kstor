BIN=$(PROJ_ROOT)/bin
LIB=$(PROJ_ROOT)/lib
CC=gcc
CPP=c++
AR=ar
COMMON_CFLAGS=-c -Wall -Werror -I$(PROJ_ROOT) -D__ARCH_BITS__=$(ARCH_BITS) -D__MODULE_NAME__=\"kstor\"
ifndef NKFS_KERNEL_PATH
NKFS_KERNEL_PATH=/lib/modules/$(shell uname -r)/build
endif

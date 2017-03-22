export PROJ_ROOT=$(CURDIR)
export ARCH_BITS=$(shell getconf LONG_BIT)

SOURCE_DIRS = system core kstor ctl
BUILD_DIRS = bin lib obj

SOURCE_DIRS_CLEAN = $(addsuffix .clean,$(SOURCE_DIRS))
BUILD_DIRS_CLEAN = $(addsuffix .clean,$(BUILD_DIRS))

.PHONY: all check debug clean $(BUILD_DIRS) $(BUILD_DIRS_CLEAN) $(SOURCE_DIRS) $(SOURCE_DIRS_CLEAN)

all: export EXTRA_CFLAGS = -D__RELEASE__ -O2
all: export DEBUG = OFF
all: check $(BUILD_DIRS) $(SOURCE_DIRS)

debug: export EXTRA_CFLAGS = -D__DEBUG__ -O1 -g3 -ggdb3 -fno-inline
debug: export DEBUG = ON
debug: check $(BUILD_DIRS) $(SOURCE_DIRS)

clean: $(BUILD_DIRS_CLEAN) $(SOURCE_DIRS_CLEAN)

check:
	cppcheck --error-exitcode=22 -q . || exit 1

$(SOURCE_DIRS):
	$(MAKE) -C $@

$(BUILD_DIRS):
	mkdir -p $@

$(SOURCE_DIRS_CLEAN): %.clean:
	$(MAKE) -C $* clean

$(BUILD_DIRS_CLEAN): %.clean:
	rm -rf $*

core: $(BUILD_DIRS)

kstor: core $(BUILD_DIRS)

system: core kstor $(BUILD_DIRS)

ctl: system $(BUILD_DIRS)

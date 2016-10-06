export PROJ_ROOT=$(CURDIR)
export ARCH_BITS=$(shell getconf LONG_BIT)

SOURCE_DIRS = system core kstor ctl
BUILD_DIRS = bin lib obj

SOURCE_DIRS_CLEAN = $(addsuffix .clean,$(SOURCE_DIRS))
BUILD_DIRS_CLEAN = $(addsuffix .clean,$(BUILD_DIRS))

.PHONY: all clean $(BUILD_DIRS) $(BUILD_DIRS_CLEAN) $(SOURCE_DIRS) $(SOURCE_DIRS_CLEAN)

all: $(BUILD_DIRS) $(SOURCE_DIRS)

clean: $(BUILD_DIRS_CLEAN) $(SOURCE_DIRS_CLEAN)

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
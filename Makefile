
## BLYSK-specific variables
IPATH := $(CURDIR)/include 
ARCHPATH := $(CURDIR)/include_priv
FLAGS += -D_GNU_SOURCE -O3 -ggdb3 -pipe -I$(IPATH)
FLAGS += -DNDEBUG 
#FLAGS += -flto -fno-fat-lto-objects
#FLAGS += -v -Wl,-v
#~ FLAGS += -DBLYSK_ENABLE_ADAPT=0
LDFLAGS += -ldl -pthread
DYN := -fPIC
TEST_PATH := ../tests

## These variables specify which files to build
LIBS += libblysk.a lib_sched_central.so lib_sched_worksteal.so
LIBS += lib_sched_worksteal_stack.so
ifneq ($(ENABLE_SPEC),)
FLAGS += -DBLYSK_ENABLE_SLR=1
FLAGS += -DBLYSK_ENABLE_SPEC=1
#~ LIBS += lib_sched_central_spec.so
endif

## Check if Grain-Graph debugging is online
FLAGS += -D__STAT_TASK


BLYSK_SRC += system/bootup.c \
system/blysk_thread.c \
system/blysk_icv.c system/blysk_task.c \
system/blysk_scheduler.c \
api/blysk_api.c \
system/blysk_for.c \
gcc-wrapper/wrapper.c \
system/blysk_malloc.c \
system/circular_queue.c \
gg/gg_collect.c
#  system/blysk_memory.c
# arch/arch.c



all: $(LIBS)

## Use one bash shell per recipe
SHELL := bash
.SHELLFLAGS := -ce
.ONESHELL:

## Do not remove intermediate files
.SECONDARY: 
.SUFFIXES:

## Support cross compiling
CROSS?=
export CC := $(CROSS)gcc
export CXX := $(CROSS)g++

## These are nice to have flags
FLAGS += -Wcast-align -Wno-variadic-macros -W -Wpedantic -Wextra -Wall -Wcast-align -Wcast-qual  -Wstrict-aliasing=2 -Wstack-usage=32768 -Wframe-larger-than=32768 -Wstrict-overflow=1 -Wsync-nand -Wtrampolines  -Wabi -Wsign-compare  -Werror=float-equal -Werror=missing-braces -Werror=init-self -Werror=logical-op  -Werror=write-strings -Werror=address -Werror=array-bounds -Werror=char-subscripts -Werror=enum-compare -Werror=implicit-int -Werror=main  -Werror=aggressive-loop-optimizations -Werror=nonnull -Werror=parentheses -Werror=pointer-sign -Werror=return-type -Werror=sequence-point -Werror=uninitialized -Werror=volatile-register-var -Werror=ignored-qualifiers -Werror=missing-parameter-type -Werror=old-style-declaration -Wno-error=maybe-uninitialized -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers
FLAGS += -fgcse-sm -fgcse-las -fgcse-after-reload -ftree-partial-pre -fweb -frename-registers -ftracer -fsched-pressure --param sched-pressure-algorithm=2  -fasynchronous-unwind-tables -fomit-frame-pointer -fstack-check=no -fno-strict-volatile-bitfields -ffunction-sections
FLAGS += -fno-fat-lto-objects -fno-common -fpeel-loops -ftree-loop-distribute-patterns -ggdb3 -U_FORTIFY_SOURCE -fno-stack-protector -fno-exceptions
#  -fipa-pta  
FLAGS += -Wl,--eh-frame-hdr -Wl,--hash-style=sysv -Wl,--enable-new-dtags -Wl,-z -Wl,relro -Wl,-z -Wl,combreloc -Wl,--no-omagic -Wl,-O -Wl,9 -Wl,--relax -Wl,--gc-sections -Wl,--warn-shared-textrel -Wl,--as-needed

## These are not nice to have flags, that suppress some benign warnings
## We should find a way to avoid these flags
FLAGS += -Wno-error=float-equal

## These are nice to have flags that require good linker support
hasGold := $(shell which $(CROSS)ld.gold 2> /dev/null)
#~ 
hasPlugin := $(shell $(CROSS)gcc-ar --version 2> /dev/null)

ifeq (,$(hasGold))
else
	FLAGS += -fuse-ld=gold -Wl,--icf -Wl,safe -Wl,--detect-odr-violations -Wl,--warn-search-mismatch
endif

ifeq (,$(hasPlugin))
	export RANLIB := $(CROSS)ranlib
	export AR := $(CROSS)ar
else
	FLAGS += -fuse-linker-plugin
	export RANLIB := $(CROSS)gcc-ranlib
	export AR := $(CROSS)gcc-ar
endif

## Debug related flags
#FLAGS += -UNDEBUG # Enable assertions
#FLAGS += -O0 # Disable optimizations
#~ BLYSK_SRC += system/debugalloc.c # Allocate with mmap rather than malloc (slower, but often catches use after free)
#~ DYN +=  -Wl,-symbolic # Disable interpositioning (better code, worse gdb support)

## Export flags
export CFLAGS := -std=gnu11 $(FLAGS) -Wc++-compat -Wno-c++-compat $(CFLAGS)
export CXXFLAGS := -std=gnu++1y -fno-enforce-eh-specs -fno-rtti -fno-threadsafe-statics -fvisibility-inlines-hidden $(FLAGS) $(CXXFLAGS)
export OPT_FLAGS := $(CFLAGS)
export LDFLAGS

## Interesting flags
## -frepo -fno-pretty-templates

## Handle header dependencies automatically
SRCS := $(shell find . -name '*.c')
DEPS := $(addprefix build/, $(SRCS:%.c=%.d))
-include $(DEPS)

## Actual recipies
.PHONY: all distclean clean test install

ifeq (,$(WHOLE))
BLYSK_OBJ := $(addprefix build/, $(BLYSK_SRC:%.c=%.o))
libblysk.a:	$(BLYSK_OBJ)
	rm -rf $@
	#$(AR) cr $@ $^
	$(AR) crsv $@ $^
	#ar crsv --plugin /home/lfbo/overrides/libexec/gcc/x86_64-unknown-linux-gnu/4.9.1/liblto_plugin.so $@ $^
	#$(AR) rscv $@ $^
	$(RANLIB) $@

else
#~ add := -flto
ifneq ($(BLYSK_FIXED_SCHEDULER),)
add += -DBLYSK_FIXED_SCHEDULER=$(BLYSK_FIXED_SCHEDULER)
endif
export FLAGS += $(add)
export CFLAGS += $(add)
export CXXFLAGS += $(add)
export OPT_FLAGS += $(add)

libblysk.a:	$(BLYSK_SRC)
	rm -rf $@
	mkdir -p build
	cat $(BLYSK_FIXED_SCHEDULER) $^ > build/cat.c
	$(CC) -c $(CFLAGS) -I$(ARCHPATH) -Isystem -o $@ build/cat.c
endif

install: all
	mkdir -p $(PREFIX)/usr/lib/ $(PREFIX)/usr/include/
	rsync -ra *.so libblysk.a $(PREFIX)/usr/lib/
	rsync -ra include/* $(PREFIX)/usr/include/

clean:
	find . -name '*.so' -o -name '*.a' -o -name '*.so' -o -name '*.d' -o -name '*.o' -o -name '*.lo' | xargs rm -f
	(test -e build && rm -r build) || true

distclean: clean

test: setupTests.t all
	./runTests.sh $(TEST_PATH)

setupTests.t:
	svn co https://svn.sics.se/PaPP/Code/WP4/turboBLYSK/tests $(TEST_PATH)
	svn co https://svn.sics.se/PaPP/Code/WP2/UC_RadarDet/omp_tasks $(TEST_PATH)/UC_RadarDet
	touch $@

## Recipies for building normal object files
build/%.o: %.c Makefile
	@mkdir -p $(dir $@)/
	@echo CC $@ $<
	@$(CC) -c -MMD $(CFLAGS) -I$(ARCHPATH) -o $@ $<

build/%.o: %.cpp Makefile
	@mkdir -p $(dir $@)/
	@echo CXX $@ $<
	@$(CXX) -c -MMD $(CXXFLAGS) -I$(ARCHPATH) -o $@ $<

## Recipies for building object files for shared objects
build/%.lo: %.c Makefile
	@mkdir -p $(dir $@)/
	@echo CC $@ $<
	@$(CC) -c -MMD $(CFLAGS) -I$(ARCHPATH) $(DYN) -o $@ $<

build/%.lo: %.cpp Makefile
	@mkdir -p $(dir $@)/
	@echo CXX $@ $<
	@$(CXX) -c -MMD $(CXXFLAGS) -I$(ARCHPATH) $(DYN) -o $@ $<

## Recipies for the libraries
lib_sched_%.so: build/schedulers/%.lo Makefile
	@echo LD $@ $<
	@$(CC) -shared  -Wl,-soname,$@ $(CFLAGS) $(DYN) -o $@ $< $(LDFLAGS)

lib_adapt_HB.so: build/adapt/adapt_HB.lo Makefile
	@echo LD $@ $<
	@$(CC) -shared -Wl,-soname,$@ $(CFLAGS) $(DYN) -o $@ $< $(LDFLAGS)


## Debug stuff
ltoTest/test.o: ltoTest/test.c
	@$(CC) -fopenmp -c -MMD $(CFLAGS) -o $@ $<

test-lto: ltoTest/test.o libblysk.a
	$(CC) $(CFLAGS) -o ltoTest/test $^ $(LDFLAGS)


#	$(CC) $(CFLAGS) -o ltoTest/test -Wl,--start-group $^ -Wl,--end-group $(LDFLAGS)

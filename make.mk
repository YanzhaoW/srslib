ifeq (,$(QUIET))
QUIET:=@
endif

ifeq (,$(SRSCLI_CORE))
  SRSCLI_CORE:=.
endif

ifeq (,$(SRSCLI_COMPILE_MODE))
  SRSCLI_COMPILE_MODE := debug
endif
ifeq (valgrind,$(MODE))
  SRSCLI_COMPILE_MODE := valgrind
endif
ifeq (gcov,$(MODE))
  SRSCLI_COMPILE_MODE := gcov
endif
ifeq (release,$(MODE))
  SRSCLI_COMPILE_MODE := release
endif

CCNAME := $(notdir $(CC))
THIS_MACHINE := $(shell $(CC) -dumpmachine)
BUILD_DIR := build_$(CCNAME)_$(THIS_MACHINE)_$(shell $(CC) -dumpversion)

### Quirks
# march=native triggers compiler bug (gcc crash) on arm-linux-gnueabi*
# asan either is broken with some compilers or not included
ifeq (gcc,$(CC))
  ifeq (arm-linux-gnueabihf,$(THIS_MACHINE))
    NO_MARCH_NATIVE := yes
    NO_ASAN := yes
  endif
  ifeq (arm-linux-gnueabi,$(THIS_MACHINE))
    NO_MARCH_NATIVE := yes
    NO_ASAN := yes
  endif
endif
ifeq (clang,$(CC))
  ifeq (arm-unknown-linux-gnueabihf,$(THIS_MACHINE))
    NO_ASAN := yes
  endif
endif
###

CFLAGS := -std=iso9899:1990
CFLAGS += -Wall -Wextra -Werror -ggdb -Wfatal-errors \
       -Wmissing-prototypes -Wshadow -Wstrict-prototypes \
       -Wconversion -Wlong-long
ifneq (emcc,$(CC))
  CFLAGS += -Wcast-qual
endif
ifneq (tcc,$(CC))
  CFLAGS += -pedantic-errors
endif
ifeq (,$(NO_MARCH_NATIVE))
  CFLAGS += -march=native
endif

CFLAGS += -fdiagnostics-color=auto
CFLAGS += -fPIC
LIBS := -lm

# debug mode
ifeq (debug,$(SRSCLI_COMPILE_MODE))
  ifeq (,$(NO_ASAN))
    ASAN_FLAGS := -fsanitize=address -fno-omit-frame-pointer
  endif
  CFLAGS += -O
  ifneq (tcc,$(CC))
    CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE
  endif
  CFLAGS += $(ASAN_FLAGS)
  LDFLAGS += $(ASAN_FLAGS)
endif
ifeq (gcov,$(SRSCLI_COMPILE_MODE))
  GCOV_FLAGS := -fprofile-arcs -ftest-coverage -g -O0
  CFLAGS += $(GCOV_FLAGS)
  LDFLAGS += $(GCOV_FLAGS)
  BUILD_DIR := $(BUILD_DIR)_gcov
endif
ifeq (release,$(SRSCLI_COMPILE_MODE))
  CFLAGS += -O3
  BUILD_DIR := $(BUILD_DIR)_release
endif
ifeq (valgrind,$(SRSCLI_COMPILE_MODE))
  BUILD_DIR := $(BUILD_DIR)_valgrind
  CFLAGS += -O
endif

BUILD_DIR_PURE := $(BUILD_DIR)

####
# exceptions to c90 ISO standard
CFLAGS+=-Wno-variadic-macros -Wno-long-long
####


-include make_site.mk

include make.mk

CFLAGS+=-I. -Iinclude
CFLAGS+=-I$(BUILD_DIR)
CFLAGS+=-I/usr/include

SOURCES:=$(wildcard src/*.c)
OBJECTS:=$(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

ifneq (tcc,$(CC))
  CFLAGS += -iquote ./include
  DEPS := $(patsubst src/%.c,$(BUILD_DIR)/%.d,$(SOURCES))
endif

#CFLAGS+=-Iinclude_extra
#SOURCES_EXTRA:=$(wildcard src_extra/*.c)
#OBJECTS_EXTRA:=$(patsubst src_extra/%.c,$(BUILD_DIR)/%.o,$(SOURCES_EXTRA))
#DEPS_EXTRA:=$(patsubst src_extra/%.c,$(BUILD_DIR)/%.d,$(SOURCES_EXTRA))

STEM:=srscli

ifeq (,$(shell which git))
	HAS_GIT=no
else
	HAS_GIT=yes
endif

CFLAGS+=-D_X_PROGRAM_NAME=\"$(STEM)\"
CFLAGS+=-D_X_COMPILE_DATE=\"$(shell date "+%Y%m%d")\"
ifeq (yes,$(HAS_GIT))
CFLAGS+=-D_X_GIT_COMMIT=\"$(shell git describe --tags --abbrev=8 --long HEAD)\"
CFLAGS+=-D_X_VERSION=\"$(shell git describe 2> /dev/null)\"
CFLAGS+=-D_X_GIT_COMMIT_DATE=\"$(shell git log -1 --format=%cd --date=format:"%Y%m%d")\"
else
CFLAGS+=-D_X_GIT_COMMIT=\"nogit\"
CFLAGS+=-D_X_VERSION=\"nogit\"
CFLAGS+=-D_X_GIT_COMMIT_DATE=\"nogit\"
endif

# link time optimisation
# CFLAGS+=-flto -Wl,-flto
# LDFLAGS:=-Wl,-flto

# use ccache, if available
ifneq (,$(shell which ccache 2> /dev/null))
  CC_PREFIX ?= ccache
endif

TARGETS := $(BUILD_DIR)/lib$(STEM).a
TARGETS += $(BUILD_DIR)/lib$(STEM).so

APPS:=$(shell ls apps | grep -v "\.")

define include_app =
  $(dir)_NAME := $(dir)
  include apps/rules.mk
endef
include apps/make.mk

$(foreach dir,$(APPS),$(eval $(call include_app,$(dir))))

include $(DEPS)

TARGET_LIST:=$(subst $(BUILD_DIR),,$(subst $(BUILD_DIR)/,,$(TARGETS)))

.DEFAULT_GOAL :=

.PHONY: all clean gdb valgrind test doc run
all: $(TARGETS)
	@echo "Built targets: $(shell echo "${TARGET_LIST}" | sed -e 's/ /\n/g')"
	@echo ""
	@echo " --- Build successful ---"
	@echo ""
	@echo "What else:"
	@echo "  make doc      -- Build documentation"
	@echo "  make test     -- Run unit tests"
	@echo "  make valgrind -- Run with valgrind"
	@echo "  make gdb      -- Run with gdb"

$(BUILD_DIR):
	@test -d $@ || \
		echo "MKDR" $@ && \
		mkdir -p $(BUILD_DIR) && \
		mkdir -p $(BUILD_DIR)/bin

$(BUILD_DIR)/lib$(STEM).so: $(OBJECTS)
	@echo " SO " $@
	$(QUIET) $(CC) -shared $^ -o $@

$(BUILD_DIR)/lib$(STEM).a: $(OBJECTS)
	@echo " AR " $@
	$(QUIET)$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: src/%.c Makefile
	@echo " CC " $@
	$(QUIET)$(CC_PREFIX) $(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.d: src/%.c | $(BUILD_DIR)
	@echo "DEP " $@; \
         set -e; rm -f $@; \
         $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$

clean:
	rm -rf $(BUILD_DIR)

gdb: $(BUILD_DIR)/bin/random_source
	$(QUIET)gdb --args $(BUILD_DIR)/bin/random_source

## Checking

include tests/rules.mk
test: $(BUILD_DIR)/bin/tests
	ASAN_OPTIONS=symbolize=1 $(BUILD_DIR)/bin/tests

valgrind: $(BUILD_DIR)/bin/random_source
	$(QUIET)valgrind --leak-check=full $(BUILD_DIR)/bin/random_source

cppcheck:
	$(QUIET)cppcheck . --enable=all

## Documentation

doc:
	$(QUIET)make -C docs html

README.pdf: README.md Makefile
	$(QUIET)pandoc --variable urlcolor=blue -tlatex -o $@ $<

## Code coverage

gcov: | test
	$(QUIET)gcov $(shell find $(BUILD_DIR) -name "*gcno")
	$(QUIET)mkdir -p $(BUILD_DIR)/gcov
	$(QUIET)mv *.gcov $(BUILD_DIR)/gcov

gcovr: | test
	$(QUIET)gcovr -s $(shell find $(BUILD_DIR) -name "*gcno" -exec dirname {} \; | sort -u) -o $(BUILD_DIR)/gcovr.out
	$(QUIET)gcovr --html $(BUILD_DIR)/gcovr.html $(shell find $(BUILD_DIR) -name "*gcno" -exec dirname {} \; | sort -u)
	$(QUIET)echo "Open coverage report with: xdg-open $(BUILD_DIR)/gcovr.html"

lcov:
	$(QUIET)echo "Lcov works only with gcc < v9.0"
	$(QUIET)lcov -b . --capture \
		--directory $(BUILD_DIR) \
		--output-file $(BUILD_DIR)/lcov.out

watch:
	while true; do \
		make -j4 && make -j test; \
		inotifywait -qre close_write . --exclude .git; \
	done

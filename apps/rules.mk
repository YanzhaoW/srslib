TARGETS:=$(TARGETS) $(BUILD_DIR)/bin/$($(dir)_NAME)
TARGETS:=$(TARGETS) $(BUILD_DIR)/libapp_$($(dir)_NAME).a

-include apps/$($(dir)_NAME)/rules.mk

APP_OBJS := $($(dir)_OBJS) \
	   $(BUILD_DIR)/$($(dir)_NAME)/main.o

APP_OBJS_ALL := $(BUILD_DIR)/$($(dir)_NAME)/app_main.o

APP_LIBS := $(BUILD_DIR)/libapp_$($(dir)_NAME).a \
	$(BUILD_DIR)/lib$(STEM)_extra.a $(BUILD_DIR)/lib$(STEM).a

define app_link_step =
.PRECIOUS: $(BUILD_DIR)/bin/$($(dir)_NAME)
$(BUILD_DIR)/bin/$($(dir)_NAME): $(APP_OBJS_ALL) $(APP_LIBS) $($(dir)_DEPS)
	@echo " LD " $$@
	$(QUIET) $(CC) -o $$@ $(LDFLAGS) $$^ $(LIBS) $($(dir)_LIBS)
endef

$(eval $(call app_link_step,a))

$(BUILD_DIR)/libapp_$($(dir)_NAME).a: $(APP_OBJS)
	@echo " AR " $@
	$(QUIET)$(AR) rcs $@ $^

define app_main_step =
$(BUILD_DIR)/$($(dir)_NAME)/app_main.o: apps/app_main.c apps/rules.mk | $(BUILD_DIR)/$($(dir)_NAME)
	@echo " CC " $$@
	$(QUIET)$(CC_PREFIX) $(CC) $(CFLAGS) -DSRS_APP_NAME=$($(dir)_NAME) -c -o $$@ $$<
endef

$(eval $(call app_main_step,a))

$(BUILD_DIR)/$($(dir)_NAME)/%.o: apps/$($(dir)_NAME)/%.c apps/rules.mk | $(BUILD_DIR)/$($(dir)_NAME)
	@echo " CC " $@
	$(QUIET)$(CC_PREFIX) $(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$($(dir)_NAME):
	@test -d $@ || \
		echo "MKDR" $@ && \
		mkdir -p $@

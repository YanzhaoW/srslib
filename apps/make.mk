#$(BUILD_DIR)/box_main.o: boxes/box_main.c boxes/rules.mk boxes/make.mk
#	@mkdir -p $(BUILD_DIR)
#	@echo " CC " $@
#	$(QUIET)$(CC) $(CFLAGS) -c -o $@ $<
#

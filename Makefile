CC ?= cc
CFLAGS ?= -O2
# `override` so command-line CFLAGS/CPPFLAGS (e.g. -D option overrides) add
# to these instead of replacing them. -I. finds config.h at the repo root.
override CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
override CPPFLAGS += -D_POSIX_C_SOURCE=200809L -I. -MMD -MP
override LDLIBS += -lm

SRC_DIR := src
BUILD_DIR ?= target/release
BIN := $(BUILD_DIR)/matrixclear

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/obj/%.o)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

sanitize:
	$(MAKE) BUILD_DIR=target/debug CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' \
		LDFLAGS='-fsanitize=address,undefined'

clean:
	rm -rf target

.PHONY: sanitize clean

-include $(OBJS:.o=.d)

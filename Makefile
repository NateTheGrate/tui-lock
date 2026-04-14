CC = clang
PKG_CONFIG = pkg-config
CFLAGS  += -fPIC -Wall -Wextra -I./gtklock/include -I./include $(shell $(PKG_CONFIG) --cflags gtk+-3.0 vte-2.91, gtk-session-lock-0)
LDFLAGS += -shared $(shell $(PKG_CONFIG) --libs gtk+-3.0 vte-2.91, gtk-session-lock-0)
DEBUG_CFLAGS = -g -O0 -DDEBUG

BUILD_DIR = build

$(BUILD_DIR)/libtui-module.so: ./src/tui-module.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

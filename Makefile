CC      ?= gcc
WARNFLAGS = -std=c11 -Wall -Wextra -Werror
CFLAGS  ?= $(WARNFLAGS) -g $(shell pkg-config --cflags ncurses)
LIBS    ?= $(shell pkg-config --libs ncurses)

SRCDIR  = src
SRCS    = $(SRCDIR)/main.c \
          $(SRCDIR)/ui.c \
          $(SRCDIR)/menu.c \
          $(SRCDIR)/exec.c \
          $(SRCDIR)/conf.c \
          $(SRCDIR)/leds.c \
          $(SRCDIR)/overlays.c \
          $(SRCDIR)/console.c \
          $(SRCDIR)/usb.c \
          $(SRCDIR)/system.c
OBJS    = $(SRCS:.c=.o)
TARGET  = calculinux-config
PREFIX  = /usr
SYSTEMD_DIR ?= $(PREFIX)/lib/systemd/system

CLANG_TIDY ?= clang-tidy
CPPCHECK   ?= cppcheck
SHELLCHECK ?= shellcheck

.PHONY: all clean install uninstall check lint tidy cppcheck shellcheck test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	ln -sf $(TARGET) $(DESTDIR)$(PREFIX)/bin/ccfg
	install -Dm755 scripts/calculinux-leds $(DESTDIR)$(PREFIX)/sbin/calculinux-leds
	install -Dm644 config/leds.default $(DESTDIR)/etc/default/leds
	install -Dm644 systemd/calculinux-leds.service \
		$(DESTDIR)$(SYSTEMD_DIR)/calculinux-leds.service

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/bin/ccfg
	rm -f $(DESTDIR)$(PREFIX)/sbin/calculinux-leds
	rm -f $(DESTDIR)/etc/default/leds
	rm -f $(DESTDIR)$(SYSTEMD_DIR)/calculinux-leds.service

# Compiler-as-linter: catch warnings as errors without linking.
lint:
	$(CC) -fsyntax-only $(WARNFLAGS) -g $(shell pkg-config --cflags ncurses) -I$(SRCDIR) $(SRCS)

tidy:
	$(CLANG_TIDY) $(SRCS) -- $(WARNFLAGS) -g $(shell pkg-config --cflags ncurses) -I$(SRCDIR)

cppcheck:
	$(CPPCHECK) --error-exitcode=1 --std=c11 --enable=warning,style,performance \
		--suppress=missingIncludeSystem --inline-suppr -I$(SRCDIR) $(SRCS)

shellcheck:
	$(SHELLCHECK) -x scripts/calculinux-leds tests/check.sh

test check:
	bash tests/check.sh

clean:
	rm -f $(OBJS) $(TARGET)

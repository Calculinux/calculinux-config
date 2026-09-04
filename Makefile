CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g \
           $(shell pkg-config --cflags ncurses)
LIBS    = $(shell pkg-config --libs ncurses)

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

.PHONY: all clean install uninstall check

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
		$(DESTDIR)$(PREFIX)/lib/systemd/system/calculinux-leds.service

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/bin/ccfg
	rm -f $(DESTDIR)$(PREFIX)/sbin/calculinux-leds
	rm -f $(DESTDIR)/etc/default/leds
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/calculinux-leds.service

check:
	bash tests/check.sh

clean:
	rm -f $(OBJS) $(TARGET)

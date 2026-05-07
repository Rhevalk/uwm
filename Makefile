PREFIX = /usr
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--gc-sections
LIBS = -lxcb -lxcb-keysyms

all:
	$(CC) $(CFLAGS) uwm.c $(LDFLAGS) $(LIBS) -o uwm
	strip --strip-all uwm

clean:
	rm -f uwm

install: all
	install -Dm755 uwm $(DESTDIR)$(PREFIX)/bin/uwm

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/uwm

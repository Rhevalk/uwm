PREFIX = /usr
CFLAGS = -Os -Wall
LIBS = -lxcb -lxcb-keysyms

all:
	$(CC) $(CFLAGS) uwm.c $(LIBS) -o uwm
	strip --strip-all uwm

clean:
	rm -f uwm

install: all
	install -Dm755 uwm $(DESTDIR)$(PREFIX)/bin/uwm

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/uwm

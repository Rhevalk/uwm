PREFIX  = /usr
CC      = cc

# -Os        													: optimize for size 
# -flto      													: link-time optimization, removes dead code across files
# -ffunction-sections -fdata-sections : place each function/data into its own section
# -fno-asynchronous-unwind-tables     : remove .eh_frame (exception table), not used in pure C
# -fno-ident                          : remove .comment section (compiler version info)
# -fomit-frame-pointer                : omit frame pointer, saves 1 register + slightly reduces size
CFLAGS  = -Os -flto \
           -ffunction-sections \
           -fdata-sections \
           -fno-asynchronous-unwind-tables \
           -fno-ident \
		   -fno-plt \
		   -fmerge-all-constants \
           -fomit-frame-pointer \
           -Wall -fvisibility=hidden

# --gc-sections : remove unused sections (pairs with -ffunction-sections)
# -s            : strip symbol table during linking (more efficient than post-build strip)
# --as-needed   : only link libraries that are actually used
LDFLAGS = -Wl,--gc-sections \
          -Wl,-s \
          -Wl,--as-needed \
          -Wl,-O1

LIBS    = -lxcb -lxcb-keysyms

all: uwm

uwm: uwm.c config.h
	$(CC) $(CFLAGS) $(LDFLAGS) uwm.c $(LIBS) -o uwm

clean:
	rm -f uwm

install: all
	install -Dm755 uwm $(DESTDIR)$(PREFIX)/bin/uwm

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/uwm

.PHONY: all clean install uninstall

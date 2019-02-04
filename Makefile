CFLAGS+=-std=c99 -Wall -Wextra -Werror -Wformat -Wpointer-arith -pedantic-errors -Os

INSTALL_PROGRAM=/usr/bin/install
PREFIX=/usr/local

OBJ=tree.o dirent-list.o

tree: $(OBJ)

clean:
	$(RM) -v $(OBJ) tree

install: tree
	$(INSTALL_PROGRAM) -s tree $(DESTDIR)$(PREFIX)/bin/tree

all: tree

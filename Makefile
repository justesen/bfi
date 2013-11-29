# Edit prefixes as you prefer (defaults should be alright)
PREFIX = /usr/local
BINPREFIX = ${PREFIX}/bin
MANPREFIX  = ${PREFIX}/share/man

VERSION = 0.2

CFLAGS = -Wall -std=c90 -pedantic -O3
CC = cc

all: bfi

bfi: bfi.o
	cc bfi.o -o bfi

bfi.o: bfi.c

clean:
	@echo cleaning
	@rm -f bfi bfi.o bfi-${VERSION}.tar.gz

dist: clean
	@echo creating distribution tarball
	@mkdir -p bfi-${VERSION}
	@cp -r bfi.c bfi.1 Makefile README LICENSE bfi-${VERSION}
	@tar -cf bfi-${VERSION}.tar bfi-${VERSION}
	@gzip bfi-${VERSION}.tar
	@rm -rf bfi-${VERSION}

install:
	@echo installing executable file to ${PREFIX}/bin
	@mkdir -p ${BINPREFIX}
	@cp -f bfi ${BINPREFIX}
	@chmod 755 ${BINPREFIX}/bfi
	@echo installing manual page to ${MANPREFIX}/man1
	@mkdir -p ${MANPREFIX}/man1
	@cp -f bfi.1 ${MANPREFIX}/man1/bfi.1
	@chmod 644 ${MANPREFIX}/man1/bfi.1

uninstall:
	@echo removing executable file from ${BINPREFIX}
	@rm -f ${BINPREFIX}/bfi

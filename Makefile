# Copyright (c) 2025 impurify <have@anonymous.sex>.
#
# [REDACTED]

CFLAGS+=-I./vendor/quickjs
LDFLAGS+=-L./vendor
LDADD=-lquickjs -lm -ldl -lpthread

PREFIX?=/usr/local

# BSD sed:
#SED=sed -E
#SHA=sha256sum -q
# GNU/Linux sed:
SED=sed -r
SHA=sha256sum --quiet

SRC=katex.c katex.h katex_internal.h
LOADSRC=katex_loader_nomod.c katex_loader.h

AUTOSRC=_auto_katex_nomod.c _auto_katex_nomod_unsplit.c
AUTOOBJ=katex_loader_nomod.o

all: libkatex.a libkatex.so katex

libkatex.a libkatex.so: ${SRC} ${AUTOOBJ}

katex_loader_nomod.o: ${LOADSRC} _auto_katex_nomod.c
	cc ${CFLAGS} -c -o $@ katex_loader_nomod.c

_auto_katex_nomod.c: _auto_katex_nomod_unsplit.c
	csplit _auto_katex_nomod_unsplit.c \
		'/^static JSContext \*JS_NewCustomContext/' \
		'/^int main/'
	${SED} -e 's/^(#include )"quickjs-libc.h"/\1<stdint.h>/' \
		-e 's/^const .+/static \0/' \
		< xx00 > $@
	cat < xx01 >> $@
	rm xx[0-9][0-9]

_auto_katex_nomod_unsplit.c: katex/katex.js katex/contrib/mhchem.js vendor/quickjs/qjsc
	cd vendor && \
	quickjs/qjsc -e -o ../$@ \
		-flto \
		-p _qjs_bytecode_ \
		katex/katex.js \
		katex/contrib/mhchem.js

vendor/katex/katex.js vendor/katex/contrib/mhchem.js: vendor/katex

vendor/quickjs/qjsc: vendor/quickjs

vendor/quickjs:
	grep ^quickjs tarballs.sha256 | sha256sum -c -
	mkdir -p $@
	tar --strip-components 1 -xf vendor/`grep -Eo '^quickjs[^ ]+' tarballs.sha256` -C $@
	(cd vendor && ${SHA} -c ../quickjs.sha256;)
	make -C $@ libquickjs.lto.a qjsc
	ln -s vendor/quickjs/libquickjs.lto.a vendor/quickjs/libquickjs.a

vendor/katex:
	grep ^katex tarballs.sha256 | sha256sum -c -
	mkdir -p $@
	tar --strip-components 1 -xf vendor/`grep -Eo '^katex[^ ]+' tarballs.sha256` -C $@
	(cd vendor && ${SHA} -c ../katex.sha256;)

# Linux install(1) doesn't seem to have a safety mode (copy to
# temporary file and rename()).  BSD does.  I must hack around
# anyway, in case anyone (including myself) runs this on a busy
# server where a file could be non-atomically overwritten while
# it is being downloaded... or if a shared library is in use?
install:
	mkdir -p -m0755 \
		${PREFIX}/lib ${PREFIX}/bin \
		${PREFIX}/share/man/man1 ${PREFIX}/share/man/man3 ${PREFIX}/share/man/man5 \
		${PREFIX}/share/katex/fonts
	for f in libkatex.a libkatex.so; do \
		install -m0644 $$f ${PREFIX}/lib/$$f.new && \
			mv ${PREFIX}/lib/$$f.new ${PREFIX}/lib/$$f ; \
	done
	install -m0755 -t ${PREFIX}/bin katex
	install -m0644 katex.1 ${PREFIX}/share/man/man1
	install -m0644 katex.3 ${PREFIX}/share/man/man3
	install -m0644 katex.5 ${PREFIX}/share/man/man5
	for f in katex/fonts/*; do \
		install -p -m0644 $$f ${PREFIX}/share/katex/fonts/`basename $$f`.new && \
			mv ${PREFIX}/share/katex/fonts/`basename $$f`.new ${PREFIX}/share/katex/fonts/`basename $$f` ; \
	done
	for f in katex/*.css; do \
		install -p -m0644 $$f ${PREFIX}/share/katex/`basename $$f`.new && \
			mv ${PREFIX}/share/katex/`basename $$f`.new ${PREFIX}/share/katex/`basename $$f` ; \
	done
	install -p -m0644 -t ${PREFIX}/share/katex katex/README.md

clean:
	rm -f ${AUTOSRC} *.o
	rm -rf vendor

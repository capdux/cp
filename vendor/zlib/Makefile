# Makefile for zlib
# Copyright (C) 1995-2024 Jean-loup Gailly, Mark Adler
# For conditions of distribution and use, see copyright notice in zlib.h

# To compile and test, type:
#    ./configure; make test
# Normally configure builds both a static and a shared library.
# If you want to build just a static library, use: ./configure --static

# To install /usr/local/lib/libz.* and /usr/local/include/zlib.h, type:
#    make install
# To install in $HOME instead of /usr/local, use:
#    make install prefix=$HOME

CC=gcc

CFLAGS=-O2  -std=c11 -Wall -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
#CFLAGS=-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7
#CFLAGS=-g -DZLIB_DEBUG
#CFLAGS=-O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
#           -Wstrict-prototypes -Wmissing-prototypes

SFLAGS=-O2  -std=c11 -Wall -fPIC -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -fno-semantic-interposition -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
LDFLAGS=
LIBNAME1=libz
LIBNAME2=zlib
SUFFIX=
TEST_LIBS=$(LIBNAME1).a
LDSHARED=gcc
LDSHAREDFLAGS=
LDVERSIONSCRIPT=

VER=1.3.1.zlib-ng
VER1=1

STATICLIB=libz.a
SHAREDLIB=
SHAREDLIBV=
SHAREDLIBM=
IMPORTLIB=
SHAREDTARGET=
PKGFILE=$(LIBNAME2).pc

LIBS=$(STATICLIB) $(SHAREDTARGET)

AR=ar
ARFLAGS=rc
DEFFILE=
RC=
RCFLAGS=
RCOBJS=
STRIP=
RANLIB=ranlib
LDCONFIG=ldconfig
LDSHAREDLIBC=
EXE=

SRCDIR=/data/dev/cp/vendor/zlib
INCLUDES=-I/data/dev/cp/vendor/zlib

BUILDDIR=.

ARCHDIR=arch/x86
ARCH_STATIC_OBJS= x86_features.o chunkset_sse2.o chorba_sse2.o compare256_sse2.o slide_hash_sse2.o adler32_ssse3.o chunkset_ssse3.o chorba_sse41.o adler32_sse42.o crc32_pclmulqdq.o slide_hash_avx2.o chunkset_avx2.o compare256_avx2.o adler32_avx2.o adler32_avx512.o chunkset_avx512.o compare256_avx512.o adler32_avx512_vnni.o crc32_vpclmulqdq.o
ARCH_SHARED_OBJS= x86_features.lo chunkset_sse2.lo chorba_sse2.lo compare256_sse2.lo slide_hash_sse2.lo adler32_ssse3.lo chunkset_ssse3.lo chorba_sse41.lo adler32_sse42.lo crc32_pclmulqdq.lo slide_hash_avx2.lo chunkset_avx2.lo compare256_avx2.lo adler32_avx2.lo adler32_avx512.lo chunkset_avx512.lo compare256_avx512.lo adler32_avx512_vnni.lo crc32_vpclmulqdq.lo

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
sharedlibdir = ${libdir}
includedir = ${prefix}/include
mandir = ${prefix}/share/man
man3dir = ${mandir}/man3
pkgconfigdir = ${libdir}/pkgconfig

OBJZ = \
	arch/generic/adler32_c.o \
	arch/generic/adler32_fold_c.o \
	arch/generic/chunkset_c.o \
	arch/generic/compare256_c.o \
	arch/generic/crc32_braid_c.o \
	arch/generic/crc32_fold_c.o \
	arch/generic/slide_hash_c.o \
	adler32.o \
	compress.o \
	crc32.o \
	crc32_braid_comb.o \
	deflate.o \
	deflate_fast.o \
	deflate_huff.o \
	deflate_medium.o \
	deflate_quick.o \
	deflate_rle.o \
	deflate_slow.o \
	deflate_stored.o \
	functable.o \
	infback.o \
	inflate.o \
	inftrees.o \
	insert_string.o \
	insert_string_roll.o \
	trees.o \
	uncompr.o \
	zutil.o \
	arch/generic/crc32_chorba_c.o \
	cpu_features.o \
	$(ARCH_STATIC_OBJS)

OBJG = \
	gzlib.o \
	gzread.o \
	gzwrite.o

TESTOBJG = 
OBJC = $(OBJZ) $(OBJG)

PIC_OBJZ = \
	arch/generic/adler32_c.lo \
	arch/generic/adler32_fold_c.lo \
	arch/generic/chunkset_c.lo \
	arch/generic/compare256_c.lo \
	arch/generic/crc32_braid_c.lo \
	arch/generic/crc32_fold_c.lo \
	arch/generic/slide_hash_c.lo \
	adler32.lo \
	compress.lo \
	crc32.lo \
	crc32_braid_comb.lo \
	deflate.lo \
	deflate_fast.lo \
	deflate_huff.lo \
	deflate_medium.lo \
	deflate_quick.lo \
	deflate_rle.lo \
	deflate_slow.lo \
	deflate_stored.lo \
	functable.lo \
	infback.lo \
	inflate.lo \
	inftrees.lo \
	insert_string.lo \
	insert_string_roll.lo \
	trees.lo \
	uncompr.lo \
	zutil.lo \
	arch/generic/crc32_chorba_c.lo \
	cpu_features.lo \
	$(ARCH_SHARED_OBJS)

PIC_OBJG = \
	gzlib.lo \
	gzread.lo \
	gzwrite.lo

PIC_TESTOBJG = 
PIC_OBJC = $(PIC_OBJZ) $(PIC_OBJG)

OBJS = $(OBJC)

PIC_OBJS = $(PIC_OBJC)

all: static

static: example$(EXE) minigzip$(EXE) makefixed$(EXE) maketrees$(EXE) makecrct$(EXE)

shared: examplesh$(EXE) minigzipsh$(EXE)

check: test

.SECONDARY:

$(ARCHDIR)/%.o: $(SRCDIR)/$(ARCHDIR)/%.c
	$(MAKE) -C $(ARCHDIR) $(notdir $@)

$(ARCHDIR)/%.lo: $(SRCDIR)/$(ARCHDIR)/%.c
	$(MAKE) -C $(ARCHDIR) $(notdir $@)

arch/generic/%.o: $(SRCDIR)/arch/generic/%.c
	$(MAKE) -C arch/generic $(notdir $@)

arch/generic/%.lo: $(SRCDIR)/arch/generic/%.c
	$(MAKE) -C arch/generic $(notdir $@)

%.o: $(ARCHDIR)/%.o
	-cp $< $@

%.lo: $(ARCHDIR)/%.lo
	-cp $< $@

test: all
	$(MAKE) -C test

infcover.o: $(SRCDIR)/test/infcover.c  $(SRCDIR)/inflate.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/test/infcover.c

infcover$(EXE): infcover.o $(STATICLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ infcover.o $(STATICLIB)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

cover: infcover$(EXE)
	rm -f *.gcda
	./infcover
	gcov inf*.c

$(STATICLIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

example.o: $(SRCDIR)/test/example.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
	$(CC) $(CFLAGS) -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $(SRCDIR)/test/example.c

minigzip.o: $(SRCDIR)/test/minigzip.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
	$(CC) $(CFLAGS) -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $(SRCDIR)/test/minigzip.c

makefixed.o: $(SRCDIR)/tools/makefixed.c  $(SRCDIR)/inflate.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/tools/makefixed.c

maketrees.o: $(SRCDIR)/tools/maketrees.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/trees.h $(SRCDIR)/zbuild.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/tools/maketrees.c

makecrct.o: $(SRCDIR)/tools/makecrct.c  $(SRCDIR)/crc32_braid_comb_p.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/tools/makecrct.c

zlibrc.o: $(SRCDIR)/win32/zlib$(SUFFIX)1.rc
	$(RC) $(RCFLAGS) -o $@ $(SRCDIR)/win32/zlib$(SUFFIX)1.rc

.SUFFIXES: .lo

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.lo: $(SRCDIR)/%.c
	$(CC) $(SFLAGS) -DPIC $(INCLUDES) -c -o $@ $<

gzlib.o: $(SRCDIR)/gzlib.c  $(SRCDIR)/gzguts.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h $(SRCDIR)/zutil_p.h
	$(CC) $(CFLAGS) -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $<

gzlib.lo: $(SRCDIR)/gzlib.c  $(SRCDIR)/gzguts.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h $(SRCDIR)/zutil_p.h
	$(CC) $(SFLAGS) -DPIC -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $<

gzread.o: $(SRCDIR)/gzread.c  $(SRCDIR)/gzguts.h $(SRCDIR)/gzread_mangle.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil_p.h
	$(CC) $(CFLAGS) -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $<

gzread.lo: $(SRCDIR)/gzread.c  $(SRCDIR)/gzguts.h $(SRCDIR)/gzread_mangle.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil_p.h
	$(CC) $(SFLAGS) -DPIC -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $<

gzwrite.o: $(SRCDIR)/gzwrite.c  $(SRCDIR)/gzguts.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil_p.h
	$(CC) $(CFLAGS) -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $<

gzwrite.lo: $(SRCDIR)/gzwrite.c  $(SRCDIR)/gzguts.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil_p.h
	$(CC) $(SFLAGS) -DPIC -DWITH_GZFILEOP $(INCLUDES) -c -o $@ $<

$(SHAREDTARGET): $(PIC_OBJS) $(DEFFILE) $(RCOBJS)
ifneq ($(SHAREDTARGET),)
	$(LDSHARED) $(CFLAGS) $(LDSHAREDFLAGS) $(LDVERSIONSCRIPT) $(LDFLAGS) -o $@ $(DEFFILE) $(PIC_OBJS) $(RCOBJS) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif
ifneq ($(SHAREDLIB),$(SHAREDTARGET))
	rm -f $(SHAREDLIB) $(SHAREDLIBM)
	ln -s $@ $(SHAREDLIB)
	ln -s $@ $(SHAREDLIBM)
endif
endif

example$(EXE): example.o $(TESTOBJG) $(STATICLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ example.o $(TESTOBJG) $(TEST_LIBS) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

minigzip$(EXE): minigzip.o $(TESTOBJG) $(STATICLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ minigzip.o $(TESTOBJG) $(TEST_LIBS) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

minigzipsh$(EXE): minigzip.o $(PIC_TESTOBJG) $(SHAREDTARGET)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ minigzip.o $(PIC_TESTOBJG) $(SHAREDLIB) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif


examplesh$(EXE): example.o $(PIC_TESTOBJG) $(SHAREDTARGET)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ example.o $(PIC_TESTOBJG) $(SHAREDLIB) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

makefixed$(EXE): makefixed.o $(STATICLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ makefixed.o $(TEST_LIBS) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

maketrees$(EXE): maketrees.o $(STATICLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ maketrees.o $(TEST_LIBS) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

makecrct$(EXE): makecrct.o $(STATICLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ makecrct.o $(TEST_LIBS) $(LDSHAREDLIBC)
ifneq ($(STRIP),)
	$(STRIP) $@
endif

install-shared: $(SHAREDTARGET)
ifneq ($(SHAREDTARGET),)
	-@if [ ! -d $(DESTDIR)$(sharedlibdir) ]; then mkdir -p $(DESTDIR)$(sharedlibdir); fi
	rm -f $(DESTDIR)$(sharedlibdir)/$(SHAREDTARGET)
	cp $(SHAREDTARGET) $(DESTDIR)$(sharedlibdir)
	chmod 755 $(DESTDIR)$(sharedlibdir)/$(SHAREDTARGET)
ifneq ($(SHAREDLIB),$(SHAREDTARGET))
	rm -f $(DESTDIR)$(sharedlibdir)/$(SHAREDLIB) $(DESTDIR)$(sharedlibdir)/$(SHAREDLIBM)
	ln -s $(SHAREDLIBV) $(DESTDIR)$(sharedlibdir)/$(SHAREDLIB)
	ln -s $(SHAREDLIBV) $(DESTDIR)$(sharedlibdir)/$(SHAREDLIBM)
	($(LDCONFIG) || true)  >/dev/null 2>&1
# ldconfig is for Linux
endif
ifneq ($(IMPORTLIB),)
	cp $(IMPORTLIB) $(DESTDIR)$(sharedlibdir)
	chmod 644 $(DESTDIR)$(sharedlibdir)/$(IMPORTLIB)
endif
endif

install-static: $(STATICLIB)
	-@if [ ! -d $(DESTDIR)$(libdir)       ]; then mkdir -p $(DESTDIR)$(libdir); fi
	rm -f $(DESTDIR)$(libdir)/$(STATICLIB)
	cp $(STATICLIB) $(DESTDIR)$(libdir)
	chmod 644 $(DESTDIR)$(libdir)/$(STATICLIB)
	-@($(RANLIB) $(DESTDIR)$(libdir)/$(STATICLIB) || true) >/dev/null 2>&1
# The ranlib in install-static is needed on NeXTSTEP which checks file times

install-libs: install-static
	-@if [ ! -d $(DESTDIR)$(man3dir)      ]; then mkdir -p $(DESTDIR)$(man3dir); fi
	-@if [ ! -d $(DESTDIR)$(pkgconfigdir) ]; then mkdir -p $(DESTDIR)$(pkgconfigdir); fi
	rm -f $(DESTDIR)$(pkgconfigdir)/$(PKGFILE)
	cp $(PKGFILE) $(DESTDIR)$(pkgconfigdir)
	chmod 644 $(DESTDIR)$(pkgconfigdir)/$(PKGFILE)

install: install-libs
	-@if [ ! -d $(DESTDIR)$(includedir)   ]; then mkdir -p $(DESTDIR)$(includedir); fi
	rm -f $(DESTDIR)$(includedir)/zlib$(SUFFIX).h $(DESTDIR)$(includedir)/zconf$(SUFFIX).h $(DESTDIR)$(includedir)/zlib_name_mangling$(SUFFIX).h
	cp zlib$(SUFFIX).h $(DESTDIR)$(includedir)/zlib$(SUFFIX).h
	cp zconf$(SUFFIX).h $(DESTDIR)$(includedir)/zconf$(SUFFIX).h
	cp zlib_name_mangling$(SUFFIX).h $(DESTDIR)$(includedir)/zlib_name_mangling$(SUFFIX).h
	chmod 644 $(DESTDIR)$(includedir)/zlib$(SUFFIX).h $(DESTDIR)$(includedir)/zconf$(SUFFIX).h $(DESTDIR)$(includedir)/zlib_name_mangling$(SUFFIX).h

uninstall-static:
	cd $(DESTDIR)$(libdir) && rm -f $(STATICLIB)

uninstall-shared:
ifneq ($(SHAREDLIB),)
	cd $(DESTDIR)$(sharedlibdir) && rm -f $(SHAREDLIBV) $(SHAREDLIB) $(SHAREDLIBM)
endif
ifneq ($(IMPORTLIB),)
	cd $(DESTDIR)$(sharedlibdir) && rm -f $(IMPORTLIB)
endif

uninstall: uninstall-static uninstall-shared
	cd $(DESTDIR)$(includedir) && rm -f zlib$(SUFFIX).h zconf$(SUFFIX).h zlib_name_mangling$(SUFFIX).h
	cd $(DESTDIR)$(pkgconfigdir) && rm -f $(PKGFILE)

mostlyclean: clean
clean:
	@if [ -f $(ARCHDIR)/Makefile ]; then $(MAKE) -C $(ARCHDIR) clean; fi
	@if [ -f arch/generic/Makefile ]; then $(MAKE) -C arch/generic clean; fi
	@if [ -f test/Makefile ]; then $(MAKE) -C test clean; fi
	rm -f *.o *.lo *~ \
	   example$(EXE) minigzip$(EXE) minigzipsh$(EXE) \
	   infcover makefixed$(EXE) maketrees$(EXE) makecrct$(EXE) \
	   $(STATICLIB) $(IMPORTLIB) $(SHAREDLIB) $(SHAREDLIBV) $(SHAREDLIBM) \
	   foo.gz so_locations \
	   _match.s maketree
	rm -rf objs
	rm -f *.gcda *.gcno *.gcov
	rm -f a.out a.exe
	rm -f *._h
	rm -rf btmp1 btmp2 pkgtmp1 pkgtmp2

maintainer-clean: distclean
distclean: clean
	@if [ -f $(ARCHDIR)/Makefile ]; then $(MAKE) -C $(ARCHDIR) distclean; fi
	@if [ -f test/Makefile ]; then $(MAKE) -C test distclean; fi
	rm -f $(PKGFILE) configure.log zconf.h zconf.h.cmakein zlib$(SUFFIX).h zlib_name_mangling$(SUFFIX).h *.pc gzread_mangle.h zlib${SUFFIX}.map
	-@rm -f .DS_Store
# Reset Makefile if building inside source tree
	@if [ -f Makefile.in ]; then \
	printf 'all:\n\t-@echo "Please use ./configure first.  Thank you."\n' > Makefile ; \
	printf '\ndistclean:\n\t$(MAKE) -f Makefile.in distclean\n' >> Makefile ; \
	touch -r $(SRCDIR)/Makefile.in Makefile ; fi
# Reset zconf.h and zconf.h.cmakein if building inside source tree
	@if [ -f zconf.h.in ]; then \
	cp -p $(SRCDIR)/zconf.h.in zconf.h ; \
	grep -v '^#cmakedefine' $(SRCDIR)/zconf.h.in > zconf.h.cmakein &&\
	touch -r $(SRCDIR)/zconf.h.in zconf.h.cmakein ; fi
# Cleanup these files if building outside source tree
	@if [ ! -f README.md ]; then rm -f Makefile; fi
# Remove arch and test directory if building outside source tree
	@if [ ! -f $(ARCHDIR)/Makefile.in ]; then rm -rf arch; fi
	@if [ ! -f test/Makefile.in ]; then rm -rf test; fi

tags:
	etags $(SRCDIR)/*.[ch]
adler32.o: $(SRCDIR)/adler32.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
adler32.lo: $(SRCDIR)/adler32.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
compress.o: $(SRCDIR)/compress.c  $(SRCDIR)/arch/s390/dfltcc_common.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
compress.lo: $(SRCDIR)/compress.c  $(SRCDIR)/arch/s390/dfltcc_common.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
cpu_features.o: $(SRCDIR)/cpu_features.c  $(SRCDIR)/cpu_features.h $(SRCDIR)/zbuild.h
cpu_features.lo: $(SRCDIR)/cpu_features.c  $(SRCDIR)/cpu_features.h $(SRCDIR)/zbuild.h
crc32_braid_comb.o: $(SRCDIR)/crc32_braid_comb.c  $(SRCDIR)/crc32_braid_comb_p.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/zutil.h
crc32_braid_comb.lo: $(SRCDIR)/crc32_braid_comb.c  $(SRCDIR)/crc32_braid_comb_p.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/zutil.h
crc32.o: $(SRCDIR)/crc32.c  $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
crc32.lo: $(SRCDIR)/crc32.c  $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate.o: $(SRCDIR)/deflate.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate.lo: $(SRCDIR)/deflate.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_fast.o: $(SRCDIR)/deflate_fast.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_fast.lo: $(SRCDIR)/deflate_fast.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_huff.o: $(SRCDIR)/deflate_huff.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_huff.lo: $(SRCDIR)/deflate_huff.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_medium.o: $(SRCDIR)/deflate_medium.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_medium.lo: $(SRCDIR)/deflate_medium.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_quick.o: $(SRCDIR)/deflate_quick.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/trees_emit.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
deflate_quick.lo: $(SRCDIR)/deflate_quick.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/trees_emit.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
deflate_rle.o: $(SRCDIR)/deflate_rle.c  $(SRCDIR)/compare256_rle.h $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_rle.lo: $(SRCDIR)/deflate_rle.c  $(SRCDIR)/compare256_rle.h $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_slow.o: $(SRCDIR)/deflate_slow.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_slow.lo: $(SRCDIR)/deflate_slow.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_stored.o: $(SRCDIR)/deflate_stored.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
deflate_stored.lo: $(SRCDIR)/deflate_stored.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
functable.o: $(SRCDIR)/functable.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/cpu_features.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
functable.lo: $(SRCDIR)/functable.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/cpu_features.h $(SRCDIR)/functable.h $(SRCDIR)/zbuild.h
infback.o: $(SRCDIR)/infback.c  $(SRCDIR)/functable.h $(SRCDIR)/inflate.h $(SRCDIR)/inflate_p.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
infback.lo: $(SRCDIR)/infback.c  $(SRCDIR)/functable.h $(SRCDIR)/inflate.h $(SRCDIR)/inflate_p.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
inflate.o: $(SRCDIR)/inflate.c  $(SRCDIR)/functable.h $(SRCDIR)/inffixed_tbl.h $(SRCDIR)/inflate.h $(SRCDIR)/inflate_p.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
inflate.lo: $(SRCDIR)/inflate.c  $(SRCDIR)/functable.h $(SRCDIR)/inffixed_tbl.h $(SRCDIR)/inflate.h $(SRCDIR)/inflate_p.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
inftrees.o: $(SRCDIR)/inftrees.c  $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
inftrees.lo: $(SRCDIR)/inftrees.c  $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
insert_string.o: $(SRCDIR)/insert_string.c  $(SRCDIR)/deflate.h $(SRCDIR)/insert_string_tpl.h $(SRCDIR)/zbuild.h
insert_string.lo: $(SRCDIR)/insert_string.c  $(SRCDIR)/deflate.h $(SRCDIR)/insert_string_tpl.h $(SRCDIR)/zbuild.h
insert_string_roll.o: $(SRCDIR)/insert_string_roll.c  $(SRCDIR)/deflate.h $(SRCDIR)/insert_string_tpl.h $(SRCDIR)/zbuild.h
insert_string_roll.lo: $(SRCDIR)/insert_string_roll.c  $(SRCDIR)/deflate.h $(SRCDIR)/insert_string_tpl.h $(SRCDIR)/zbuild.h
test.o: $(SRCDIR)/test.c 
test.lo: $(SRCDIR)/test.c 
trees.o: $(SRCDIR)/trees.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/trees_emit.h $(SRCDIR)/trees.h $(SRCDIR)/trees_tbl.h $(SRCDIR)/zbuild.h
trees.lo: $(SRCDIR)/trees.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/trees_emit.h $(SRCDIR)/trees.h $(SRCDIR)/trees_tbl.h $(SRCDIR)/zbuild.h
uncompr.o: $(SRCDIR)/uncompr.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
uncompr.lo: $(SRCDIR)/uncompr.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
ztest2837218.o: $(SRCDIR)/ztest2837218.c 
ztest2837218.lo: $(SRCDIR)/ztest2837218.c 
zutil.o: $(SRCDIR)/zutil.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h $(SRCDIR)/zutil_p.h
zutil.lo: $(SRCDIR)/zutil.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h $(SRCDIR)/zutil_p.h
example.lo: $(SRCDIR)/test/example.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
gh1235.o: $(SRCDIR)/test/gh1235.c  $(SRCDIR)/zutil.h
gh1235.lo: $(SRCDIR)/test/gh1235.c  $(SRCDIR)/zutil.h
infcover.lo: $(SRCDIR)/test/infcover.c  $(SRCDIR)/inflate.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
minideflate.o: $(SRCDIR)/test/minideflate.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
minideflate.lo: $(SRCDIR)/test/minideflate.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
minigzip.lo: $(SRCDIR)/test/minigzip.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
switchlevels.o: $(SRCDIR)/test/switchlevels.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
switchlevels.lo: $(SRCDIR)/test/switchlevels.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_checksum.o: $(SRCDIR)/test/fuzz/fuzzer_checksum.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_checksum.lo: $(SRCDIR)/test/fuzz/fuzzer_checksum.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_compress.o: $(SRCDIR)/test/fuzz/fuzzer_compress.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_compress.lo: $(SRCDIR)/test/fuzz/fuzzer_compress.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_dict.o: $(SRCDIR)/test/fuzz/fuzzer_example_dict.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_dict.lo: $(SRCDIR)/test/fuzz/fuzzer_example_dict.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_flush.o: $(SRCDIR)/test/fuzz/fuzzer_example_flush.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_flush.lo: $(SRCDIR)/test/fuzz/fuzzer_example_flush.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_large.o: $(SRCDIR)/test/fuzz/fuzzer_example_large.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_large.lo: $(SRCDIR)/test/fuzz/fuzzer_example_large.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_small.o: $(SRCDIR)/test/fuzz/fuzzer_example_small.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_example_small.lo: $(SRCDIR)/test/fuzz/fuzzer_example_small.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_minigzip.o: $(SRCDIR)/test/fuzz/fuzzer_minigzip.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
fuzzer_minigzip.lo: $(SRCDIR)/test/fuzz/fuzzer_minigzip.c  $(SRCDIR)/zbuild.h $(SRCDIR)/zlib.h
standalone_fuzz_target_runner.o: $(SRCDIR)/test/fuzz/standalone_fuzz_target_runner.c  $(SRCDIR)/zbuild.h
standalone_fuzz_target_runner.lo: $(SRCDIR)/test/fuzz/standalone_fuzz_target_runner.c  $(SRCDIR)/zbuild.h
adler32_avx2.o: $(SRCDIR)/arch/x86/adler32_avx2.c  $(SRCDIR)/arch/x86/adler32_avx2_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
arch/x86/adler32_avx2.o: $(SRCDIR)/arch/x86/adler32_avx2.c  $(SRCDIR)/arch/x86/adler32_avx2_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
adler32_avx2.lo: $(SRCDIR)/arch/x86/adler32_avx2.c  $(SRCDIR)/arch/x86/adler32_avx2_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
adler32_avx512.o: $(SRCDIR)/arch/x86/adler32_avx512.c  $(SRCDIR)/arch/x86/adler32_avx512_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
arch/x86/adler32_avx512.o: $(SRCDIR)/arch/x86/adler32_avx512.c  $(SRCDIR)/arch/x86/adler32_avx512_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
adler32_avx512.lo: $(SRCDIR)/arch/x86/adler32_avx512.c  $(SRCDIR)/arch/x86/adler32_avx512_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
adler32_avx512_vnni.o: $(SRCDIR)/arch/x86/adler32_avx512_vnni.c  $(SRCDIR)/arch/x86/adler32_avx2_p.h $(SRCDIR)/arch/x86/adler32_avx512_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
arch/x86/adler32_avx512_vnni.o: $(SRCDIR)/arch/x86/adler32_avx512_vnni.c  $(SRCDIR)/arch/x86/adler32_avx2_p.h $(SRCDIR)/arch/x86/adler32_avx512_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
adler32_avx512_vnni.lo: $(SRCDIR)/arch/x86/adler32_avx512_vnni.c  $(SRCDIR)/arch/x86/adler32_avx2_p.h $(SRCDIR)/arch/x86/adler32_avx512_p.h $(SRCDIR)/adler32_p.h $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h
adler32_sse42.o: $(SRCDIR)/arch/x86/adler32_sse42.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/adler32_ssse3_p.h $(SRCDIR)/zbuild.h
arch/x86/adler32_sse42.o: $(SRCDIR)/arch/x86/adler32_sse42.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/adler32_ssse3_p.h $(SRCDIR)/zbuild.h
adler32_sse42.lo: $(SRCDIR)/arch/x86/adler32_sse42.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/adler32_ssse3_p.h $(SRCDIR)/zbuild.h
adler32_ssse3.o: $(SRCDIR)/arch/x86/adler32_ssse3.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/adler32_ssse3_p.h $(SRCDIR)/zbuild.h
arch/x86/adler32_ssse3.o: $(SRCDIR)/arch/x86/adler32_ssse3.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/adler32_ssse3_p.h $(SRCDIR)/zbuild.h
adler32_ssse3.lo: $(SRCDIR)/arch/x86/adler32_ssse3.c  $(SRCDIR)/adler32_p.h $(SRCDIR)/arch/x86/adler32_ssse3_p.h $(SRCDIR)/zbuild.h
chorba_sse2.o: $(SRCDIR)/arch/x86/chorba_sse2.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/crc32.h $(SRCDIR)/zbuild.h
arch/x86/chorba_sse2.o: $(SRCDIR)/arch/x86/chorba_sse2.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/crc32.h $(SRCDIR)/zbuild.h
chorba_sse2.lo: $(SRCDIR)/arch/x86/chorba_sse2.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/crc32.h $(SRCDIR)/zbuild.h
chorba_sse41.o: $(SRCDIR)/arch/x86/chorba_sse41.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/crc32.h $(SRCDIR)/zbuild.h
arch/x86/chorba_sse41.o: $(SRCDIR)/arch/x86/chorba_sse41.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/crc32.h $(SRCDIR)/zbuild.h
chorba_sse41.lo: $(SRCDIR)/arch/x86/chorba_sse41.c  $(SRCDIR)/arch_functions.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/crc32_braid_p.h $(SRCDIR)/crc32_braid_tbl.h $(SRCDIR)/crc32.h $(SRCDIR)/zbuild.h
chunkset_avx2.o: $(SRCDIR)/arch/x86/chunkset_avx2.c  $(SRCDIR)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/chunkset_avx2.o: $(SRCDIR)/arch/x86/chunkset_avx2.c  $(SRCDIR)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_avx2.lo: $(SRCDIR)/arch/x86/chunkset_avx2.c  $(SRCDIR)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_avx512.o: $(SRCDIR)/arch/x86/chunkset_avx512.c  $(SRCDIR)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/chunkset_avx512.o: $(SRCDIR)/arch/x86/chunkset_avx512.c  $(SRCDIR)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_avx512.lo: $(SRCDIR)/arch/x86/chunkset_avx512.c  $(SRCDIR)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/arch/x86/x86_intrins.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_sse2.o: $(SRCDIR)/arch/x86/chunkset_sse2.c  $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/chunkset_sse2.o: $(SRCDIR)/arch/x86/chunkset_sse2.c  $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_sse2.lo: $(SRCDIR)/arch/x86/chunkset_sse2.c  $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_ssse3.o: $(SRCDIR)/arch/x86/chunkset_ssse3.c  $(SRCDIR)/arch/generic/chunk_128bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/chunkset_ssse3.o: $(SRCDIR)/arch/x86/chunkset_ssse3.c  $(SRCDIR)/arch/generic/chunk_128bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
chunkset_ssse3.lo: $(SRCDIR)/arch/x86/chunkset_ssse3.c  $(SRCDIR)/arch/generic/chunk_128bit_perm_idx_lut.h $(SRCDIR)/chunkset_tpl.h $(SRCDIR)/inffast_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
compare256_avx2.o: $(SRCDIR)/arch/x86/compare256_avx2.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/compare256_avx2.o: $(SRCDIR)/arch/x86/compare256_avx2.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
compare256_avx2.lo: $(SRCDIR)/arch/x86/compare256_avx2.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
compare256_avx512.o: $(SRCDIR)/arch/x86/compare256_avx512.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/compare256_avx512.o: $(SRCDIR)/arch/x86/compare256_avx512.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
compare256_avx512.lo: $(SRCDIR)/arch/x86/compare256_avx512.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
compare256_sse2.o: $(SRCDIR)/arch/x86/compare256_sse2.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
arch/x86/compare256_sse2.o: $(SRCDIR)/arch/x86/compare256_sse2.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
compare256_sse2.lo: $(SRCDIR)/arch/x86/compare256_sse2.c  $(SRCDIR)/deflate.h $(SRCDIR)/fallback_builtins.h $(SRCDIR)/match_tpl.h $(SRCDIR)/zbuild.h $(SRCDIR)/zmemory.h
crc32_pclmulqdq.o: $(SRCDIR)/arch/x86/crc32_pclmulqdq.c  $(SRCDIR)/arch/x86/crc32_pclmulqdq_tpl.h
arch/x86/crc32_pclmulqdq.o: $(SRCDIR)/arch/x86/crc32_pclmulqdq.c  $(SRCDIR)/arch/x86/crc32_pclmulqdq_tpl.h
crc32_pclmulqdq.lo: $(SRCDIR)/arch/x86/crc32_pclmulqdq.c  $(SRCDIR)/arch/x86/crc32_pclmulqdq_tpl.h
crc32_vpclmulqdq.o: $(SRCDIR)/arch/x86/crc32_vpclmulqdq.c  $(SRCDIR)/arch/x86/crc32_pclmulqdq_tpl.h
arch/x86/crc32_vpclmulqdq.o: $(SRCDIR)/arch/x86/crc32_vpclmulqdq.c  $(SRCDIR)/arch/x86/crc32_pclmulqdq_tpl.h
crc32_vpclmulqdq.lo: $(SRCDIR)/arch/x86/crc32_vpclmulqdq.c  $(SRCDIR)/arch/x86/crc32_pclmulqdq_tpl.h
slide_hash_avx2.o: $(SRCDIR)/arch/x86/slide_hash_avx2.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h
arch/x86/slide_hash_avx2.o: $(SRCDIR)/arch/x86/slide_hash_avx2.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h
slide_hash_avx2.lo: $(SRCDIR)/arch/x86/slide_hash_avx2.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h
slide_hash_sse2.o: $(SRCDIR)/arch/x86/slide_hash_sse2.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h
arch/x86/slide_hash_sse2.o: $(SRCDIR)/arch/x86/slide_hash_sse2.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h
slide_hash_sse2.lo: $(SRCDIR)/arch/x86/slide_hash_sse2.c  $(SRCDIR)/deflate.h $(SRCDIR)/zbuild.h
x86_features.o: $(SRCDIR)/arch/x86/x86_features.c  $(SRCDIR)/arch/x86/x86_features.h $(SRCDIR)/zbuild.h
arch/x86/x86_features.o: $(SRCDIR)/arch/x86/x86_features.c  $(SRCDIR)/arch/x86/x86_features.h $(SRCDIR)/zbuild.h
x86_features.lo: $(SRCDIR)/arch/x86/x86_features.c  $(SRCDIR)/arch/x86/x86_features.h $(SRCDIR)/zbuild.h
makecrct.lo: $(SRCDIR)/tools/makecrct.c  $(SRCDIR)/crc32_braid_comb_p.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
makefixed.lo: $(SRCDIR)/tools/makefixed.c  $(SRCDIR)/inflate.h $(SRCDIR)/inftrees.h $(SRCDIR)/zbuild.h $(SRCDIR)/zutil.h
maketrees.lo: $(SRCDIR)/tools/maketrees.c  $(SRCDIR)/deflate.h $(SRCDIR)/deflate_p.h $(SRCDIR)/trees.h $(SRCDIR)/zbuild.h

# Copyright (C) 1995-2024 Jean-loup Gailly and Mark Adler
# Copyright 2015, Daniel Axtens, IBM Corporation
# zlib license, see zlib.h

CC=gcc
CFLAGS=-O2  -std=c11 -Wall -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
EXE=
SRCDIR=/data/dev/cp/vendor/zlib/test
SRCTOP=/data/dev/cp/vendor/zlib
LIBNAME=libz
TEST_LDFLAGS=-L.. ../$(LIBNAME).a

EMU_RUN=

all: alltests

alltests: teststatic ghtests
check_cross_dep:
ifneq (,$(findstring qemu,$(EMU_RUN)))
QEMU_VER:=$(shell command -v $(EMU_RUN) --version 2> /dev/null)
ifeq (,$(QEMU_VER))
$(error You need QEMU to run tests on non-native platform)
endif
endif

ALL_SRC_FILES := $(wildcard ../*)

teststatic: check_cross_dep
	@TMPST=tmpst_$$$$; \
	HELLOST=tmphellost_$$$$; \
	if echo hello world | ${EMU_RUN} ../minigzip$(EXE) > $$HELLOST && ${EMU_RUN} ../minigzip$(EXE) -d < $$HELLOST && ${EMU_RUN} ../example$(EXE) $$TMPST; then \
	  echo '		*** zlib test OK ***'; \
	else \
	  echo '		*** zlib test FAILED ***'; exit 1; \
	fi; \
	rm -f $$TMPST $$HELLOST

testshared: check_cross_dep
	@LD_LIBRARY_PATH=`pwd`/..:$(LD_LIBRARY_PATH) ; export LD_LIBRARY_PATH; \
	LD_LIBRARYN32_PATH=`pwd`/..:$(LD_LIBRARYN32_PATH) ; export LD_LIBRARYN32_PATH; \
	DYLD_LIBRARY_PATH=`pwd`/..:$(DYLD_LIBRARY_PATH) ; export DYLD_LIBRARY_PATH; \
	SHLIB_PATH=`pwd`/..:$(SHLIB_PATH) ; export SHLIB_PATH; \
	TMPSH=tmpsh_$$$$; \
	HELLOSH=tmphellosh_$$$$; \
	if echo hello world | ${EMU_RUN} ../minigzipsh$(EXE) > $$HELLOSH && ${EMU_RUN} ../minigzipsh$(EXE) -d < $$HELLOSH && ${EMU_RUN} ../examplesh$(EXE) $$TMPSH; then \
	  echo '		*** zlib shared test OK ***'; \
	else \
	  echo '		*** zlib shared test FAILED ***'; exit 1; \
	fi; \
	rm -f $$TMPSH $$HELLOSH

.PHONY: ghtests
ghtests: testGH-361 testGH-364 testGH-751 testGH-1235

.PHONY: testGH-361
testGH-361:
	$(EMU_RUN) ../minigzip$(EXE) -4 <$(SRCDIR)/GH-361/test.txt >/dev/null

switchlevels$(EXE): $(SRCDIR)/switchlevels.c
	$(CC) $(CFLAGS) -I.. -I$(SRCTOP) -o $@ $< $(TEST_LDFLAGS)

.PHONY: testGH-364
testGH-364: switchlevels$(EXE)
	$(EMU_RUN) ./switchlevels$(EXE) 1 5 9 3 <$(SRCDIR)/GH-364/test.bin >/dev/null

.PHONY: testGH-751
testGH-751:
	$(EMU_RUN) ../minigzip$(EXE) <$(SRCDIR)/GH-751/test.txt | $(EMU_RUN) ../minigzip$(EXE) -d >/dev/null

gh1235$(EXE): $(SRCDIR)/gh1235.c
	$(CC) $(CFLAGS) -I.. -I$(SRCTOP) -o $@ $< $(TEST_LDFLAGS)

.PHONY: testGH-1235
testGH-1235: gh1235$(EXE)
	$(EMU_RUN) ./gh1235$(EXE)

clean:
	rm -f *.o *.gcda *.gcno *.gcov
	rm -f switchlevels$(EXE) gh1235$(EXE)

distclean: clean
	rm -f Makefile

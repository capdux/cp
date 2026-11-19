# Makefile for zlib-ng
# Copyright (C) 1995-2013 Jean-loup Gailly, Mark Adler
# Copyright (C) 2024 Hans Kristian Rosbach
# For conditions of distribution and use, see copyright notice in zlib.h

CC=gcc
CFLAGS=-O2  -std=c11 -Wall -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
SFLAGS=-O2  -std=c11 -Wall -fPIC -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -fno-semantic-interposition -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
INCLUDES=-I/data/dev/cp/vendor/zlib

SRCDIR=/data/dev/cp/vendor/zlib/arch/generic
SRCTOP=/data/dev/cp/vendor/zlib
TOPDIR=$(SRCTOP)

all: \
 adler32_c.o adler32_c.lo \
 adler32_fold_c.o adler32_fold_c.lo \
 chunkset_c.o chunkset_c.lo \
 compare256_c.o compare256_c.lo \
 crc32_braid_c.o crc32_braid_c.lo \
 crc32_chorba_c.o crc32_chorba_c.lo \
 crc32_fold_c.o crc32_fold_c.lo \
 slide_hash_c.o slide_hash_c.lo


adler32_c.o: $(SRCDIR)/adler32_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/adler32_p.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_c.c

adler32_c.lo: $(SRCDIR)/adler32_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/adler32_p.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_c.c

adler32_fold_c.o: $(SRCDIR)/adler32_fold_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/functable.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_fold_c.c

adler32_fold_c.lo: $(SRCDIR)/adler32_fold_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/functable.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_fold_c.c

chunkset_c.o: $(SRCDIR)/chunkset_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_c.c

chunkset_c.lo: $(SRCDIR)/chunkset_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_c.c

compare256_c.o: $(SRCDIR)/compare256_c.c  $(SRCTOP)/zbuild.h $(SRCDIR)/compare256_p.h $(SRCTOP)/zmemory.h $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_c.c

compare256_c.lo: $(SRCDIR)/compare256_c.c  $(SRCTOP)/zbuild.h $(SRCDIR)/compare256_p.h $(SRCTOP)/zmemory.h $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_c.c

crc32_braid_c.o: $(SRCDIR)/crc32_braid_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_braid_c.c

crc32_braid_c.lo: $(SRCDIR)/crc32_braid_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_braid_c.c

crc32_chorba_c.o: $(SRCDIR)/crc32_chorba_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_chorba_c.c

crc32_chorba_c.lo: $(SRCDIR)/crc32_chorba_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_chorba_c.c

crc32_fold_c.o: $(SRCDIR)/crc32_fold_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/functable.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_fold_c.c

crc32_fold_c.lo: $(SRCDIR)/crc32_fold_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/functable.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_fold_c.c

slide_hash_c.o: $(SRCDIR)/slide_hash_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/deflate.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/slide_hash_c.c

slide_hash_c.lo: $(SRCDIR)/slide_hash_c.c  $(SRCTOP)/zbuild.h $(SRCTOP)/deflate.h
	$(CC) $(SFLAGS) $(INCLUDES) -c -o $@ $(SRCDIR)/slide_hash_c.c


mostlyclean: clean
clean:
	rm -f *.o *.lo *~
	rm -rf objs
	rm -f *.gcda *.gcno *.gcov

distclean: clean
	rm -f Makefile

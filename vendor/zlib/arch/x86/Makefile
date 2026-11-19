# Makefile for zlib
# Copyright (C) 1995-2013 Jean-loup Gailly, Mark Adler
# For conditions of distribution and use, see copyright notice in zlib.h

CC=gcc
CFLAGS=-O2  -std=c11 -Wall -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
SFLAGS=-O2  -std=c11 -Wall -fPIC -DNDEBUG -D_LARGEFILE64_SOURCE=1 -DHAVE_CPUID_GNU -DHAVE_SYS_AUXV_H -DHAVE_LINUX_AUXVEC_H -DZLIB_COMPAT -DWITH_GZFILEOP -fno-semantic-interposition -DHAVE_VISIBILITY_HIDDEN -DHAVE_VISIBILITY_INTERNAL -DHAVE_ATTRIBUTE_ALIGNED -DHAVE_BUILTIN_ASSUME_ALIGNED -DHAVE_BUILTIN_CTZ -DHAVE_BUILTIN_CTZLL -DWITH_ALL_FALLBACKS -DWITH_OPTIM -DX86_FEATURES -DX86_HAVE_XSAVE_INTRIN -DX86_SSE2 -DX86_SSSE3 -DX86_SSE41 -DX86_SSE42 -DX86_PCLMULQDQ_CRC -DX86_AVX2 -DX86_AVX512 -DX86_AVX512VNNI -DX86_VPCLMULQDQ_CRC
INCLUDES=-I/data/dev/cp/vendor/zlib
SUFFIX=

AVX512FLAG=-mavx512f -mavx512dq -mavx512bw -mavx512vl -mbmi2 -mtune=cascadelake
AVX512VNNIFLAG=-mavx512f -mavx512dq -mavx512bw -mavx512vl -mbmi2 -mavx512vnni -mtune=cascadelake
AVX2FLAG=-mavx2 -mbmi2
SSE2FLAG=-msse2
SSSE3FLAG=-mssse3
SSE41FLAG=-msse4.1
SSE42FLAG=-msse4.2
PCLMULFLAG=-mpclmul
VPCLMULFLAG=-mvpclmulqdq -mavx512f
XSAVEFLAG=-mxsave
NOLTOFLAG=-fno-lto

SRCDIR=/data/dev/cp/vendor/zlib/arch/x86
SRCTOP=/data/dev/cp/vendor/zlib
TOPDIR=$(SRCTOP)

all: \
	x86_features.o x86_features.lo \
	adler32_avx2.o adler32_avx2.lo \
	adler32_avx512.o adler32_avx512.lo \
	adler32_avx512_vnni.o adler32_avx512_vnni.lo \
	adler32_sse42.o adler32_sse42.lo \
	adler32_ssse3.o adler32_ssse3.lo \
	chunkset_avx2.o chunkset_avx2.lo \
	chunkset_avx512.o chunkset_avx512.lo \
	chunkset_sse2.o chunkset_sse2.lo \
	chunkset_ssse3.o chunkset_ssse3.lo \
	chorba_sse2.o chorba_sse2.lo \
	chorba_sse41.o chorba_sse41.lo \
	compare256_avx2.o compare256_avx2.lo \
	compare256_avx512.o compare256_avx512.lo \
	compare256_sse2.o compare256_sse2.lo \
	crc32_pclmulqdq.o crc32_pclmulqdq.lo \
	crc32_vpclmulqdq.o crc32_vpclmulqdq.lo \
	slide_hash_avx2.o slide_hash_avx2.lo \
	slide_hash_sse2.o slide_hash_sse2.lo

x86_features.o: $(SRCDIR)/x86_features.c  $(SRCDIR)/x86_features.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(XSAVEFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/x86_features.c

x86_features.lo: $(SRCDIR)/x86_features.c  $(SRCDIR)/x86_features.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(XSAVEFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/x86_features.c

chunkset_avx2.o: $(SRCDIR)/chunkset_avx2.c  $(SRCTOP)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(AVX2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_avx2.c

chunkset_avx2.lo: $(SRCDIR)/chunkset_avx2.c  $(SRCTOP)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(AVX2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_avx2.c

chunkset_avx512.o: $(SRCDIR)/chunkset_avx512.c  $(SRCTOP)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(AVX512FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_avx512.c

chunkset_avx512.lo: $(SRCDIR)/chunkset_avx512.c  $(SRCTOP)/arch/generic/chunk_256bit_perm_idx_lut.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(AVX512FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_avx512.c

chunkset_sse2.o: $(SRCDIR)/chunkset_sse2.c  $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(SSE2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_sse2.c

chunkset_sse2.lo: $(SRCDIR)/chunkset_sse2.c  $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(SSE2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_sse2.c

chunkset_ssse3.o: $(SRCDIR)/chunkset_ssse3.c  $(SRCTOP)/arch/generic/chunk_128bit_perm_idx_lut.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(SSSE3FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_ssse3.c

chunkset_ssse3.lo: $(SRCDIR)/chunkset_ssse3.c  $(SRCTOP)/arch/generic/chunk_128bit_perm_idx_lut.h $(SRCTOP)/chunkset_tpl.h $(SRCTOP)/inffast_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(SSSE3FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/chunkset_ssse3.c

chorba_sse2.o: $(SRCDIR)/chorba_sse2.c  $(SRCTOP)/arch_functions.h $(SRCTOP)/arch/x86/x86_intrins.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h $(SRCTOP)/crc32.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(SSE2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/chorba_sse2.c

chorba_sse2.lo: $(SRCDIR)/chorba_sse2.c  $(SRCTOP)/arch_functions.h $(SRCTOP)/arch/x86/x86_intrins.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h $(SRCTOP)/crc32.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(SSE2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/chorba_sse2.c

chorba_sse41.o: $(SRCDIR)/chorba_sse41.c  $(SRCTOP)/arch_functions.h $(SRCTOP)/arch/x86/x86_intrins.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h $(SRCTOP)/crc32.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(SSE41FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/chorba_sse41.c

chorba_sse41.lo: $(SRCDIR)/chorba_sse41.c  $(SRCTOP)/arch_functions.h $(SRCTOP)/arch/x86/x86_intrins.h $(SRCTOP)/crc32_braid_p.h $(SRCTOP)/crc32_braid_tbl.h $(SRCTOP)/crc32.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(SSE41FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/chorba_sse41.c

compare256_avx2.o: $(SRCDIR)/compare256_avx2.c  $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h $(SRCTOP)/match_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(AVX2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_avx2.c

compare256_avx2.lo: $(SRCDIR)/compare256_avx2.c  $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h $(SRCTOP)/match_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(AVX2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_avx2.c

compare256_avx512.o: $(SRCDIR)/compare256_avx512.c  $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h $(SRCTOP)/match_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(AVX512FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_avx512.c

compare256_avx512.lo: $(SRCDIR)/compare256_avx512.c  $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h $(SRCTOP)/match_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(AVX512FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_avx512.c

compare256_sse2.o: $(SRCDIR)/compare256_sse2.c  $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h $(SRCTOP)/match_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(CFLAGS) $(SSE2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_sse2.c

compare256_sse2.lo: $(SRCDIR)/compare256_sse2.c  $(SRCTOP)/deflate.h $(SRCTOP)/fallback_builtins.h $(SRCTOP)/match_tpl.h $(SRCTOP)/zbuild.h $(SRCTOP)/zmemory.h
	$(CC) $(SFLAGS) $(SSE2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/compare256_sse2.c

crc32_pclmulqdq.o: $(SRCDIR)/crc32_pclmulqdq.c  $(SRCDIR)/crc32_pclmulqdq_tpl.h
	$(CC) $(CFLAGS) $(PCLMULFLAG) $(SSE42FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_pclmulqdq.c

crc32_pclmulqdq.lo: $(SRCDIR)/crc32_pclmulqdq.c  $(SRCDIR)/crc32_pclmulqdq_tpl.h
	$(CC) $(SFLAGS) $(PCLMULFLAG) $(SSE42FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_pclmulqdq.c

crc32_vpclmulqdq.o: $(SRCDIR)/crc32_vpclmulqdq.c  $(SRCDIR)/crc32_pclmulqdq_tpl.h
	$(CC) $(CFLAGS) $(PCLMULFLAG) $(VPCLMULFLAG) $(AVX512FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_vpclmulqdq.c

crc32_vpclmulqdq.lo: $(SRCDIR)/crc32_vpclmulqdq.c  $(SRCDIR)/crc32_pclmulqdq_tpl.h
	$(CC) $(SFLAGS) $(PCLMULFLAG) $(VPCLMULFLAG) $(AVX512FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/crc32_vpclmulqdq.c

slide_hash_avx2.o: $(SRCDIR)/slide_hash_avx2.c  $(SRCTOP)/deflate.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(AVX2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/slide_hash_avx2.c

slide_hash_avx2.lo: $(SRCDIR)/slide_hash_avx2.c  $(SRCTOP)/deflate.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(AVX2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/slide_hash_avx2.c

slide_hash_sse2.o: $(SRCDIR)/slide_hash_sse2.c  $(SRCTOP)/deflate.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(SSE2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/slide_hash_sse2.c

slide_hash_sse2.lo: $(SRCDIR)/slide_hash_sse2.c  $(SRCTOP)/deflate.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(SSE2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/slide_hash_sse2.c

adler32_avx2.o: $(SRCDIR)/adler32_avx2.c  $(SRCDIR)/adler32_avx2_p.h $(SRCTOP)/adler32_p.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(AVX2FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_avx2.c

adler32_avx2.lo: $(SRCDIR)/adler32_avx2.c  $(SRCDIR)/adler32_avx2_p.h $(SRCTOP)/adler32_p.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(AVX2FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_avx2.c

adler32_avx512.o: $(SRCDIR)/adler32_avx512.c  $(SRCDIR)/adler32_avx512_p.h $(SRCTOP)/adler32_p.h $(SRCTOP)/arch_functions.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(AVX512FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_avx512.c

adler32_avx512.lo: $(SRCDIR)/adler32_avx512.c  $(SRCDIR)/adler32_avx512_p.h $(SRCTOP)/adler32_p.h $(SRCTOP)/arch_functions.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(AVX512FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_avx512.c

adler32_avx512_vnni.o: $(SRCDIR)/adler32_avx512_vnni.c  $(SRCDIR)/adler32_avx2_p.h $(SRCDIR)/adler32_avx512_p.h $(SRCTOP)/adler32_p.h $(SRCTOP)/arch_functions.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(AVX512VNNIFLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_avx512_vnni.c

adler32_avx512_vnni.lo: $(SRCDIR)/adler32_avx512_vnni.c  $(SRCDIR)/adler32_avx2_p.h $(SRCDIR)/adler32_avx512_p.h $(SRCTOP)/adler32_p.h $(SRCTOP)/arch_functions.h $(SRCDIR)/x86_intrins.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(AVX512VNNIFLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_avx512_vnni.c

adler32_ssse3.o: $(SRCDIR)/adler32_ssse3.c  $(SRCTOP)/adler32_p.h $(SRCDIR)/adler32_ssse3_p.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(SSSE3FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_ssse3.c

adler32_ssse3.lo: $(SRCDIR)/adler32_ssse3.c  $(SRCTOP)/adler32_p.h $(SRCDIR)/adler32_ssse3_p.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(SSSE3FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_ssse3.c

adler32_sse42.o: $(SRCDIR)/adler32_sse42.c  $(SRCTOP)/adler32_p.h $(SRCDIR)/adler32_ssse3_p.h $(SRCTOP)/zbuild.h
	$(CC) $(CFLAGS) $(SSE42FLAG) $(NOLTOFLAG) $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_sse42.c

adler32_sse42.lo: $(SRCDIR)/adler32_sse42.c  $(SRCTOP)/adler32_p.h $(SRCDIR)/adler32_ssse3_p.h $(SRCTOP)/zbuild.h
	$(CC) $(SFLAGS) $(SSE42FLAG) $(NOLTOFLAG) -DPIC $(INCLUDES) -c -o $@ $(SRCDIR)/adler32_sse42.c

mostlyclean: clean
clean:
	rm -f *.o *.lo *~
	rm -rf objs
	rm -f *.gcda *.gcno *.gcov

distclean: clean
	rm -f Makefile

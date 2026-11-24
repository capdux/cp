#ifndef LIBSRTP_CONFIG_H
#define LIBSRTP_CONFIG_H

#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_UNISTD_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_SOCKET_H 1

#define HAVE_UINT8_T 1
#define HAVE_UINT16_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT64_T 1
#define HAVE_INT32_T 1
#define HAVE_INT16_T 1
#define HAVE_INT8_T 1

#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_UNSIGNED_LONG_LONG 8

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define CPU_CISC 1
#else
#define CPU_RISC 1
#endif

#define PACKAGE_NAME "libsrtp2"
#define PACKAGE_VERSION "2.6.0"
#define PACKAGE_STRING "libsrtp2 2.6.0"

#endif

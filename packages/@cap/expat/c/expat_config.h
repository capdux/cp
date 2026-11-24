/* Minimal portable expat_config.h for @cap/expat (libexpat 2.6.4) */
#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BYTEORDER 4321
#else
#define BYTEORDER 1234
#endif

#ifndef XML_CONTEXT_BYTES
#define XML_CONTEXT_BYTES 1024
#endif

#ifndef XML_GE
#define XML_GE 1
#endif

#ifndef XML_DTD
#define XML_DTD 1
#endif

#ifndef XML_NS
#define XML_NS 1
#endif

#ifndef HAVE_MEMMOVE
#define HAVE_MEMMOVE 1
#endif

#ifndef XML_DEV_URANDOM
#define XML_DEV_URANDOM 1
#endif

#endif /* EXPAT_CONFIG_H */

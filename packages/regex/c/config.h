/* Minimal config.h for @cap/pcre2 (PCRE2 10.44, 8-bit, Unicode, SLJIT) */
#ifndef CP_PCRE2_CONFIG_H
#define CP_PCRE2_CONFIG_H

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MEMMOVE 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRERROR 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1

#define PACKAGE "pcre2"
#define PACKAGE_NAME "PCRE2"
#define PACKAGE_VERSION "10.44"
#define PACKAGE_STRING "PCRE2 10.44"
#define PACKAGE_TARNAME "pcre2"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_URL ""

#ifndef PCRE2_STATIC
#define PCRE2_STATIC 1
#endif

#ifndef PCRE2_EXPORT
#define PCRE2_EXPORT
#endif

#ifndef SUPPORT_PCRE2_8
#define SUPPORT_PCRE2_8 1
#endif

#ifndef SUPPORT_UNICODE
#define SUPPORT_UNICODE 1
#endif

#ifndef SUPPORT_JIT
#define SUPPORT_JIT 1
#endif

#ifndef LINK_SIZE
#define LINK_SIZE 2
#endif

#ifndef HEAP_LIMIT
#define HEAP_LIMIT 20000000
#endif

#ifndef MATCH_LIMIT
#define MATCH_LIMIT 10000000
#endif

#ifndef MATCH_LIMIT_DEPTH
#define MATCH_LIMIT_DEPTH MATCH_LIMIT
#endif

#ifndef MAX_NAME_COUNT
#define MAX_NAME_COUNT 10000
#endif

#ifndef MAX_NAME_SIZE
#define MAX_NAME_SIZE 128
#endif

#ifndef MAX_VARLOOKBEHIND
#define MAX_VARLOOKBEHIND 255
#endif

#ifndef NEWLINE_DEFAULT
#define NEWLINE_DEFAULT 2
#endif

#ifndef PARENS_NEST_LIMIT
#define PARENS_NEST_LIMIT 250
#endif

#endif /* CP_PCRE2_CONFIG_H */

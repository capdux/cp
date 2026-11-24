// Generated minimal snappy-stubs-public.h for @cap/snappy (Google Snappy 1.2.1)
// License: BSD-3-Clause

#ifndef THIRD_PARTY_SNAPPY_OPENSOURCE_SNAPPY_STUBS_PUBLIC_H_
#define THIRD_PARTY_SNAPPY_OPENSOURCE_SNAPPY_STUBS_PUBLIC_H_

#include <stddef.h>
#include <stdint.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/uio.h>
#define SNAPPY_HAVE_SYS_UIO_H 1
#else
#define SNAPPY_HAVE_SYS_UIO_H 0
#endif

#define SNAPPY_MAJOR 1
#define SNAPPY_MINOR 2
#define SNAPPY_PATCHLEVEL 1
#define SNAPPY_VERSION \
    ((SNAPPY_MAJOR << 16) | (SNAPPY_MINOR << 8) | SNAPPY_PATCHLEVEL)

namespace snappy {

#if !SNAPPY_HAVE_SYS_UIO_H
struct iovec {
  void* iov_base;
  size_t iov_len;
};
#endif

}  // namespace snappy

#endif  // THIRD_PARTY_SNAPPY_OPENSOURCE_SNAPPY_STUBS_PUBLIC_H_

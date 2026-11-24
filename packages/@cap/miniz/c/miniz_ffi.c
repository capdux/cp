// Thin FFI shim for miniz ZIP in-memory creation & extraction
#include "miniz.h"
#include <stdlib.h>
#include <string.h>

const char *cp_miniz_version(void) {
    return MZ_VERSION;
}

void *cp_miniz_zip_create_single(
    const char *entry_name,
    const void *data,
    size_t data_len,
    size_t *out_zip_len
) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_heap(&zip, 0, 0)) return NULL;
    if (!mz_zip_writer_add_mem(&zip, entry_name, data, data_len, MZ_DEFAULT_COMPRESSION)) {
        mz_zip_writer_end(&zip);
        return NULL;
    }
    void *buf = NULL;
    size_t sz = 0;
    if (!mz_zip_writer_finalize_heap_archive(&zip, &buf, &sz)) {
        mz_zip_writer_end(&zip);
        return NULL;
    }
    mz_zip_writer_end(&zip);
    if (out_zip_len) *out_zip_len = sz;
    return buf;
}

void *cp_miniz_zip_extract_entry(
    const void *zip_bytes,
    size_t zip_len,
    const char *entry_name,
    size_t *out_len
) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_mem(&zip, zip_bytes, zip_len, 0)) return NULL;
    size_t sz = 0;
    void *extracted = mz_zip_reader_extract_file_to_heap(&zip, entry_name, &sz, 0);
    mz_zip_reader_end(&zip);
    if (!extracted) return NULL;
    if (out_len) *out_len = sz;
    return extracted;
}

void cp_miniz_free(void *ptr) {
    if (ptr) mz_free(ptr);
}

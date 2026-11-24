/* Thin C FFI bridge for @cap/pcre2 (PCRE2 10.44, 8-bit + SLJIT) */
#define PCRE2_CODE_UNIT_WIDTH 8
#define PCRE2_STATIC 1
#include "pcre2.h"
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    pcre2_code *code;
    pcre2_match_data *mdata;
    int jit_ready;
} cp_pcre2_handle_t;

void *cp_pcre2_compile(const uint8_t *pattern, size_t pat_len, uint32_t opts, int jit) {
    int errcode = 0;
    PCRE2_SIZE erroff = 0;
    pcre2_code *code = pcre2_compile(
        (PCRE2_SPTR)pattern, (PCRE2_SIZE)pat_len, opts, &errcode, &erroff, NULL
    );
    if (!code) return NULL;
    int jit_ready = 0;
    if (jit) {
        if (pcre2_jit_compile(code, PCRE2_JIT_COMPLETE) == 0) jit_ready = 1;
    }
    pcre2_match_data *mdata = pcre2_match_data_create_from_pattern(code, NULL);
    if (!mdata) {
        pcre2_code_free(code);
        return NULL;
    }
    cp_pcre2_handle_t *h = (cp_pcre2_handle_t *)malloc(sizeof(cp_pcre2_handle_t));
    if (!h) {
        pcre2_match_data_free(mdata);
        pcre2_code_free(code);
        return NULL;
    }
    h->code = code;
    h->mdata = mdata;
    h->jit_ready = jit_ready;
    return (void *)h;
}

int cp_pcre2_match(void *re, const uint8_t *subj, size_t subj_len, size_t start_off,
                   size_t *out_start, size_t *out_end) {
    if (!re) return -1;
    cp_pcre2_handle_t *h = (cp_pcre2_handle_t *)re;
    int rc = pcre2_match(
        h->code, (PCRE2_SPTR)subj, (PCRE2_SIZE)subj_len, (PCRE2_SIZE)start_off, 0, h->mdata, NULL
    );
    if (rc == PCRE2_ERROR_NOMATCH) return 0;
    if (rc < 0) return rc;
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(h->mdata);
    if (out_start) *out_start = (size_t)ov[0];
    if (out_end) *out_end = (size_t)ov[1];
    return 1;
}

void cp_pcre2_free(void *re) {
    if (!re) return;
    cp_pcre2_handle_t *h = (cp_pcre2_handle_t *)re;
    if (h->mdata) pcre2_match_data_free(h->mdata);
    if (h->code) pcre2_code_free(h->code);
    free(h);
}

int cp_pcre2_is_jit(void *re) {
    if (!re) return 0;
    return ((cp_pcre2_handle_t *)re)->jit_ready;
}

const char *cp_pcre2_version(void) {
    static char buf[64];
    if (buf[0] == '\0') {
        pcre2_config(PCRE2_CONFIG_VERSION, buf);
    }
    return buf;
}

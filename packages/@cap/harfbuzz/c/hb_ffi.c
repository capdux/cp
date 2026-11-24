#include "hb.h"

const char *cp_hb_version_string(void) {
    return hb_version_string();
}

void *cp_hb_buffer_create(void) {
    return (void *)hb_buffer_create();
}

void cp_hb_buffer_destroy(void *buf) {
    if (buf) {
        hb_buffer_destroy((hb_buffer_t *)buf);
    }
}

void cp_hb_buffer_reset(void *buf) {
    if (buf) {
        hb_buffer_reset((hb_buffer_t *)buf);
    }
}

void cp_hb_buffer_add_utf8(void *buf, const char *text, int len) {
    if (buf && text) {
        hb_buffer_add_utf8((hb_buffer_t *)buf, text, len, 0, len);
    }
}

void cp_hb_buffer_guess_segment_properties(void *buf) {
    if (buf) {
        hb_buffer_guess_segment_properties((hb_buffer_t *)buf);
    }
}

unsigned int cp_hb_buffer_get_length(const void *buf) {
    if (!buf) {
        return 0;
    }
    return hb_buffer_get_length((const hb_buffer_t *)buf);
}

int cp_hb_buffer_get_direction(const void *buf) {
    if (!buf) {
        return 0;
    }
    return (int)hb_buffer_get_direction((const hb_buffer_t *)buf);
}

#include <stddef.h>
#include "dav1d/dav1d.h"

const char *cp_dav1d_version(void) {
    return dav1d_version();
}

unsigned int cp_dav1d_version_api(void) {
    return dav1d_version_api();
}

void *cp_dav1d_open(int n_threads, int max_frame_delay) {
    Dav1dSettings settings;
    dav1d_default_settings(&settings);
    if (n_threads >= 0 && n_threads <= DAV1D_MAX_THREADS) {
        settings.n_threads = n_threads;
    }
    if (max_frame_delay >= 0 && max_frame_delay <= DAV1D_MAX_FRAME_DELAY) {
        settings.max_frame_delay = max_frame_delay;
    }
    Dav1dContext *ctx = NULL;
    if (dav1d_open(&ctx, &settings) != 0) {
        return NULL;
    }
    return (void *)ctx;
}

void cp_dav1d_flush(void *ctx) {
    if (ctx) {
        dav1d_flush((Dav1dContext *)ctx);
    }
}

void cp_dav1d_close(void *ctx) {
    if (ctx) {
        Dav1dContext *c = (Dav1dContext *)ctx;
        dav1d_close(&c);
    }
}

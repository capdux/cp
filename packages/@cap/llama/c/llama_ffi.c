// @cap/llama — Thin C ABI bridge for llama.cpp (b12676) (~45 lines)
// Isolates CP from version-sensitive llama_model_params struct layout.
#if !defined(_WIN32)
#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "llama.h"

void cp_llama_backend_init(void) {
#if !defined(_WIN32)
    typedef void (*load_from_path_fn)(const char *);
    load_from_path_fn fn = (load_from_path_fn)dlsym(RTLD_DEFAULT, "ggml_backend_load_all_from_path");
    Dl_info info;
    if (fn && dladdr((const void *)llama_backend_init, &info) && info.dli_fname) {
        const char *slash = strrchr(info.dli_fname, '/');
        if (slash && (size_t)(slash - info.dli_fname) < 1023) {
            char dir[1024];
            size_t len = (size_t)(slash - info.dli_fname);
            memcpy(dir, info.dli_fname, len);
            dir[len] = '\0';
            fn(dir);
        }
    }
#endif
    llama_backend_init();
}

void cp_llama_backend_free(void) {
    llama_backend_free();
}

size_t cp_llama_max_devices(void) {
    return llama_max_devices();
}

int32_t cp_llama_supports_mmap(void) {
    return llama_supports_mmap() ? 1 : 0;
}

int32_t cp_llama_supports_mlock(void) {
    return llama_supports_mlock() ? 1 : 0;
}

int32_t cp_llama_supports_gpu_offload(void) {
    return llama_supports_gpu_offload() ? 1 : 0;
}

int64_t cp_llama_time_us(void) {
    return llama_time_us();
}

const char *cp_llama_print_system_info(void) {
    const char *info = llama_print_system_info();
    return info ? info : "";
}

void *cp_llama_load_model(const char *path, int32_t n_gpu_layers) {
    if (!path) return NULL;
    struct llama_model_params params = llama_model_default_params();
    params.n_gpu_layers = n_gpu_layers;
    return (void *)llama_model_load_from_file(path, params);
}

void cp_llama_free_model(void *m) {
    if (m) llama_model_free((struct llama_model *)m);
}

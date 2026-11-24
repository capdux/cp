// @cap/whisper — Thin C ABI bridge for whisper.cpp (v1.9.6)
// Isolates CP from version-sensitive whisper_context_params struct layout.
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "whisper.h"

const char *cp_whisper_version(void) {
    const char *ver = whisper_version();
    return ver ? ver : "";
}

const char *cp_whisper_print_system_info(void) {
    const char *info = whisper_print_system_info();
    return info ? info : "";
}

int32_t cp_whisper_lang_max_id(void) {
    return (int32_t)whisper_lang_max_id();
}

int32_t cp_whisper_lang_id(const char *lang) {
    if (!lang) return -1;
    return (int32_t)whisper_lang_id(lang);
}

const char *cp_whisper_lang_str(int32_t id) {
    const char *s = whisper_lang_str((int)id);
    return s ? s : "";
}

void *cp_whisper_init_from_file(const char *path) {
    if (!path) return NULL;
    struct whisper_context_params params = whisper_context_default_params();
    return (void *)whisper_init_from_file_with_params(path, params);
}

void cp_whisper_free(void *ctx) {
    if (ctx) {
        whisper_free((struct whisper_context *)ctx);
    }
}

int32_t cp_whisper_n_vocab(void *ctx) {
    if (!ctx) return 0;
    return (int32_t)whisper_n_vocab((struct whisper_context *)ctx);
}

// @cap/onnx — Thin C ABI bridge for ONNX Runtime C API (~45 lines)
// Isolates CP from large version-sensitive OrtApi function-table struct layout.
#include <stdint.h>
#include <stddef.h>
#include "onnxruntime_c_api.h"

static const OrtApi *cp_ort_api(void) {
    const OrtApiBase *base = OrtGetApiBase();
    return base ? base->GetApi(ORT_API_VERSION) : NULL;
}

const char *cp_ort_version(void) {
    const OrtApiBase *base = OrtGetApiBase();
    return base ? base->GetVersionString() : "";
}

void *cp_ort_create_env(const char *log_id) {
    const OrtApi *api = cp_ort_api();
    if (!api) return NULL;
    OrtEnv *env = NULL;
    OrtStatus *st = api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, log_id ? log_id : "cp_onnx", &env);
    if (st) {
        api->ReleaseStatus(st);
        return NULL;
    }
    return (void *)env;
}

void cp_ort_release_env(void *env) {
    const OrtApi *api = cp_ort_api();
    if (api && env) api->ReleaseEnv((OrtEnv *)env);
}

void *cp_ort_create_session_options(void) {
    const OrtApi *api = cp_ort_api();
    if (!api) return NULL;
    OrtSessionOptions *opts = NULL;
    OrtStatus *st = api->CreateSessionOptions(&opts);
    if (st) {
        api->ReleaseStatus(st);
        return NULL;
    }
    return (void *)opts;
}

int32_t cp_ort_set_intra_op_threads(void *opts, int32_t n) {
    const OrtApi *api = cp_ort_api();
    if (!api || !opts) return -1;
    OrtStatus *st = api->SetIntraOpNumThreads((OrtSessionOptions *)opts, (int)n);
    if (st) {
        api->ReleaseStatus(st);
        return -1;
    }
    return 0;
}

void cp_ort_release_session_options(void *opts) {
    const OrtApi *api = cp_ort_api();
    if (api && opts) api->ReleaseSessionOptions((OrtSessionOptions *)opts);
}

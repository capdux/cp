#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

const char* cp_ma_version_string(void) {
    return ma_version_string();
}

float* cp_ma_decoder_from_memory(
    const unsigned char* data,
    size_t size,
    uint32_t* out_channels,
    uint32_t* out_sample_rate,
    uint64_t* out_frames
) {
    if (data == NULL || size == 0) return NULL;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_uint64 frames = 0;
    void* pcm = NULL;
    ma_result res = ma_decode_memory(data, size, &config, &frames, &pcm);
    if (res != MA_SUCCESS || pcm == NULL) {
        return NULL;
    }
    if (out_channels != NULL) *out_channels = config.channels;
    if (out_sample_rate != NULL) *out_sample_rate = config.sampleRate;
    if (out_frames != NULL) *out_frames = (uint64_t)frames;
    return (float*)pcm;
}

void cp_ma_free(void* ptr) {
    if (ptr != NULL) {
        ma_free(ptr, NULL);
    }
}

void* cp_ma_resampler_create(uint32_t in_rate, uint32_t out_rate, uint32_t channels) {
    if (in_rate == 0 || out_rate == 0 || channels == 0) return NULL;
    ma_resampler* resampler = (ma_resampler*)malloc(sizeof(ma_resampler));
    if (resampler == NULL) return NULL;
    ma_resampler_config config = ma_resampler_config_init(
        ma_format_f32,
        channels,
        in_rate,
        out_rate,
        ma_resample_algorithm_linear
    );
    if (ma_resampler_init(&config, NULL, resampler) != MA_SUCCESS) {
        free(resampler);
        return NULL;
    }
    return (void*)resampler;
}

int32_t cp_ma_resampler_process(
    void* resampler,
    const float* frames_in,
    uint64_t* frame_count_in,
    float* frames_out,
    uint64_t* frame_count_out
) {
    if (resampler == NULL || frame_count_in == NULL || frame_count_out == NULL) {
        return (int32_t)MA_INVALID_ARGS;
    }
    ma_uint64 in_cnt = (ma_uint64)(*frame_count_in);
    ma_uint64 out_cnt = (ma_uint64)(*frame_count_out);
    ma_result res = ma_resampler_process_pcm_frames(
        (ma_resampler*)resampler,
        frames_in,
        &in_cnt,
        frames_out,
        &out_cnt
    );
    *frame_count_in = (uint64_t)in_cnt;
    *frame_count_out = (uint64_t)out_cnt;
    return (int32_t)res;
}

void cp_ma_resampler_free(void* resampler) {
    if (resampler != NULL) {
        ma_resampler_uninit((ma_resampler*)resampler, NULL);
        free(resampler);
    }
}

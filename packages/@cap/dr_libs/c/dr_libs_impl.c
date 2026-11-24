// Thin implementation + FFI shim for dr_wav / dr_mp3 / dr_flac
#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION
#include <stdint.h>
#include <stdlib.h>
#include "dr_wav.h"
#include "dr_mp3.h"
#include "dr_flac.h"
#include <stdlib.h>

void *cp_drwav_encode_pcm16_mem(
    const int16_t *pcm,
    uint64_t frame_count,
    uint32_t channels,
    uint32_t sample_rate,
    size_t *out_size
) {
    drwav wav;
    drwav_data_format fmt;
    fmt.container = drwav_container_riff;
    fmt.format = DR_WAVE_FORMAT_PCM;
    fmt.channels = channels;
    fmt.sampleRate = sample_rate;
    fmt.bitsPerSample = 16;

    void *buf = NULL;
    size_t sz = 0;
    if (!drwav_init_memory_write(&wav, &buf, &sz, &fmt, NULL)) {
        return NULL;
    }
    drwav_write_pcm_frames(&wav, frame_count, pcm);
    drwav_uninit(&wav);
    if (out_size) *out_size = sz;
    return buf;
}

int16_t *cp_drwav_decode_pcm16_mem(
    const void *data,
    size_t data_size,
    uint32_t *out_channels,
    uint32_t *out_sample_rate,
    uint64_t *out_frames
) {
    unsigned int ch = 0;
    unsigned int sr = 0;
    drwav_uint64 frames = 0;
    int16_t *pcm = drwav_open_memory_and_read_pcm_frames_s16(data, data_size, &ch, &sr, &frames, NULL);
    if (!pcm) return NULL;
    if (out_channels) *out_channels = ch;
    if (out_sample_rate) *out_sample_rate = sr;
    if (out_frames) *out_frames = (uint64_t)frames;
    return pcm;
}

int16_t *cp_drflac_decode_pcm16_mem(
    const void *data,
    size_t data_size,
    uint32_t *out_channels,
    uint32_t *out_sample_rate,
    uint64_t *out_frames
) {
    unsigned int ch = 0;
    unsigned int sr = 0;
    drflac_uint64 frames = 0;
    int16_t *pcm = drflac_open_memory_and_read_pcm_frames_s16(data, data_size, &ch, &sr, &frames, NULL);
    if (!pcm) return NULL;
    if (out_channels) *out_channels = ch;
    if (out_sample_rate) *out_sample_rate = sr;
    if (out_frames) *out_frames = (uint64_t)frames;
    return pcm;
}

void cp_dr_free(void *ptr) {
    if (ptr) drwav_free(ptr, NULL);
}

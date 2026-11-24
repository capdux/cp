#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "FLAC/format.h"
#include "FLAC/stream_encoder.h"

const char *cp_flac_version(void) {
    return FLAC__VERSION_STRING;
}

const char *cp_flac_vendor(void) {
    return FLAC__VENDOR_STRING;
}

static FLAC__StreamEncoderWriteStatus cp_flac_mem_write_cb(
    const FLAC__StreamEncoder *encoder,
    const FLAC__byte buffer[],
    size_t bytes,
    uint32_t samples,
    uint32_t current_frame,
    void *client_data
) {
    (void)encoder; (void)buffer; (void)samples; (void)current_frame;
    uint64_t *total = (uint64_t *)client_data;
    if (total) *total += (uint64_t)bytes;
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

int32_t cp_flac_encode_pcm16_test(const int16_t *pcm, uint32_t samples, uint32_t sample_rate, uint64_t *out_bytes_written) {
    if (!pcm || samples == 0 || sample_rate == 0 || !out_bytes_written) return -1;
    *out_bytes_written = 0;
    FLAC__StreamEncoder *enc = FLAC__stream_encoder_new();
    if (!enc) return -2;

    FLAC__stream_encoder_set_channels(enc, 1);
    FLAC__stream_encoder_set_bits_per_sample(enc, 16);
    FLAC__stream_encoder_set_sample_rate(enc, sample_rate);
    FLAC__stream_encoder_set_compression_level(enc, 5);

    uint64_t total_bytes = 0;
    if (FLAC__stream_encoder_init_stream(enc, cp_flac_mem_write_cb, NULL, NULL, NULL, &total_bytes) != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
        FLAC__stream_encoder_delete(enc);
        return -3;
    }

    FLAC__int32 *buf = (FLAC__int32 *)malloc((size_t)samples * sizeof(FLAC__int32));
    if (!buf) {
        FLAC__stream_encoder_delete(enc);
        return -4;
    }
    for (uint32_t i = 0; i < samples; i++) buf[i] = (FLAC__int32)pcm[i];

    FLAC__bool ok = FLAC__stream_encoder_process_interleaved(enc, buf, samples);
    free(buf);
    if (!ok || !FLAC__stream_encoder_finish(enc)) {
        FLAC__stream_encoder_delete(enc);
        return -5;
    }
    FLAC__stream_encoder_delete(enc);
    *out_bytes_written = total_bytes;
    return 0;
}

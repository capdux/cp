#include "vpx/vpx_codec.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vp8dx.h"
#include <stddef.h>

const char *cp_vpx_version_str(void) {
    const char *v = vpx_codec_version_str();
    return v ? v : "unknown";
}

int cp_vpx_vp8_enc_cfg_default(unsigned int *out_w, unsigned int *out_h) {
    vpx_codec_enc_cfg_t cfg;
    vpx_codec_err_t err = vpx_codec_enc_config_default(vpx_codec_vp8_cx(), &cfg, 0);
    if (err != VPX_CODEC_OK) {
        return (int)err;
    }
    if (out_w) *out_w = cfg.g_w;
    if (out_h) *out_h = cfg.g_h;
    return 0;
}

int cp_vpx_vp9_dec_open_close(int threads) {
    vpx_codec_ctx_t ctx;
    vpx_codec_dec_cfg_t cfg;
    cfg.threads = (unsigned int)(threads > 0 ? threads : 1);
    cfg.w = 0;
    cfg.h = 0;
    vpx_codec_err_t err = vpx_codec_dec_init(&ctx, vpx_codec_vp9_dx(), &cfg, 0);
    if (err != VPX_CODEC_OK) {
        return (int)err;
    }
    err = vpx_codec_destroy(&ctx);
    return (int)err;
}

// @cap/libvpx — Google libvpx 1.15.0 Royalty-Free VP8/VP9 Video Codec for CP
// License: BSD-3-Clause (Upstream: webmproject/libvpx v1.15.0)

import std from "std";

pub @c {
    fn cp_vpx_version_str(): [*:0]const u8;
    fn cp_vpx_vp8_enc_cfg_default(out_w: *u32!, out_h: *u32!): i32;
    fn cp_vpx_vp9_dec_open_close(threads: i32): i32;
}

pub err VpxError {
    EncConfigFailed,
    DecoderInitFailed,
}

pub struct Vp8DefaultSize {
    pub width: u32 = 0,
    pub height: u32 = 0,
}

/// Returns the linked libvpx version string (e.g. `"v1.15.0"`).
pub fn versionStr(): string {
    unsafe {
        return std.mem.span(cp_vpx_version_str());
    }
}

/// Queries the default VP8 encoder configuration dimensions via `vpx_codec_enc_config_default`.
pub fn vp8EncCfgDefault(): Vp8DefaultSize! {
    let w: u32 = 0;
    let h: u32 = 0;
    let rc: i32 = 0;
    unsafe {
        rc = cp_vpx_vp8_enc_cfg_default(&w, &h);
    }
    if (rc != 0) {
        return VpxError.EncConfigFailed;
    }
    return Vp8DefaultSize{ .width = w, .height = h };
}

/// Opens and cleanly destroys a real VP9 decoder context (`vpx_codec_dec_init` + `vpx_codec_destroy`).
pub fn vp9DecOpenClose(threads: i32): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_vpx_vp9_dec_open_close(threads);
    }
    if (rc != 0) {
        return VpxError.DecoderInitFailed;
    }
}

// @cap/vorbis — Xiph libvorbis 1.3.7 royalty-free audio codec for CP
// License: BSD-3-Clause (Upstream: xiph/vorbis v1.3.7)

import std from "std";

pub @c {
    fn cp_vorbis_version_string(): [*:0]const u8;
    fn cp_vorbis_encoder_test_init(channels: i32, rate: isize, quality: f32): i32;
}

pub err VorbisError {
    InvalidParams,
    InitFailed,
}

/// Returns the linked Xiph libvorbis version string (e.g. `"Xiph.Org libVorbis 1.3.7"`).
pub fn versionString(): string {
    unsafe {
        return std.mem.span(cp_vorbis_version_string());
    }
}

/// Validates and initializes a Vorbis VBR encoder pipeline (`vorbis_info_init` +
/// `vorbis_encode_init_vbr` + `vorbis_analysis_init` + `vorbis_block_init`) and
/// cleanly releases all resources.
pub fn encoderTestInit(channels: i32, rate: isize, quality: f32): void! {
    if (channels <= 0 || rate <= 0) {
        return VorbisError.InvalidParams;
    }
    let rc: i32 = 0;
    unsafe {
        rc = cp_vorbis_encoder_test_init(channels, rate, quality);
    }
    if (rc != 0) {
        return VorbisError.InitFailed;
    }
}

// @cap/flac — Xiph libFLAC 1.4.3 lossless audio codec engine for CP
// License: BSD-3-Clause (Upstream: xiph/flac 1.4.3)

import std from "std";

pub @c {
    fn cp_flac_version(): [*:0]const u8?;
    fn cp_flac_vendor(): [*:0]const u8?;
    fn cp_flac_encode_pcm16_test(pcm: [*]const i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32;
}

pub err FlacError {
    InvalidInput,
    InitFailed,
    EncodeFailed,
}

/// Returns the linked libFLAC version string (e.g. "1.4.3").
pub fn version(): string {
    unsafe {
        if (ver := cp_flac_version()) {
            return std.mem.span(ver);
        }
    }
    return "1.4.3";
}

/// Returns the linked libFLAC vendor string.
pub fn vendor(): string {
    unsafe {
        if (v := cp_flac_vendor()) {
            return std.mem.span(v);
        }
    }
    return "";
}

/// Encodes mono 16-bit PCM samples in memory as a FLAC stream and returns the compressed byte count.
pub fn encodePcm16Test(pcm: []const i16, sample_rate: u32): u64! {
    if (pcm.len == 0 || sample_rate == 0) return FlacError.InvalidInput;
    let written: u64 = 0;
    let rc: i32 = 0;
    unsafe {
        rc = cp_flac_encode_pcm16_test(pcm.ptr, @intCast(pcm.len), sample_rate, &written);
    }
    if (rc == -1) return FlacError.InvalidInput;
    if (rc == -2 || rc == -3) return FlacError.InitFailed;
    if (rc != 0) return FlacError.EncodeFailed;
    return written;
}

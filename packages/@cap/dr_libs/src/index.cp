// @cap/dr_libs — Zero-dependency WAV / MP3 / FLAC audio codecs for CP
// License: MIT-0 / Public Domain (Upstream: David Reid / dr_libs)

import std from "std";

pub @c {
    fn cp_drwav_encode_pcm16_mem(
        pcm: [*]const i16!,
        frame_count: u64,
        channels: u32,
        sample_rate: u32,
        out_size: *usize!
    ): [*]u8?!;

    fn cp_drwav_decode_pcm16_mem(
        data: [*]const u8!,
        data_size: usize,
        out_channels: *u32!,
        out_sample_rate: *u32!,
        out_frames: *u64!
    ): [*]i16?!;

    fn cp_drflac_decode_pcm16_mem(
        data: [*]const u8!,
        data_size: usize,
        out_channels: *u32!,
        out_sample_rate: *u32!,
        out_frames: *u64!
    ): [*]i16?!;

    fn cp_dr_free(ptr: *!?);
}

pub err AudioCodecError {
    EncodeFailed,
    DecodeFailed,
}

/// Owned RAII encoded WAV byte buffer.
pub struct WavBytes {
    raw: [*]u8?! = null,
    pub len: usize = 0,

    pub static fn encodePcm16(pcm: []const i16, channels: u32, sample_rate: u32): WavBytes! {
        if (channels == 0 or pcm.len == 0) return AudioCodecError.EncodeFailed;
        const frames: u64 = @intCast(pcm.len / @as(usize, channels));
        let sz: usize = 0;
        let ptr: [*]u8?! = null;
        unsafe {
            ptr = cp_drwav_encode_pcm16_mem(pcm.ptr, frames, channels, sample_rate, &sz);
        }
        if (ptr == null or sz == 0) return AudioCodecError.EncodeFailed;
        return WavBytes{ .raw = ptr, .len = sz };
    }

    pub fn slice(): []const u8 {
        if (p := _.raw) return p[0.._.len];
        return "";
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            _.len = 0;
            unsafe { cp_dr_free(@ptrCast(p)); }
        }
    }
}

/// Owned RAII decoded 16-bit interleaved PCM audio buffer.
pub struct DecodedPcm16 {
    raw: [*]i16?! = null,
    pub channels: u32 = 0,
    pub sample_rate: u32 = 0,
    pub frame_count: u64 = 0,

    pub static fn decodeWav(bytes: []const u8): DecodedPcm16! {
        let ch: u32 = 0;
        let sr: u32 = 0;
        let frames: u64 = 0;
        let ptr: [*]i16?! = null;
        unsafe {
            ptr = cp_drwav_decode_pcm16_mem(bytes.ptr, bytes.len, &ch, &sr, &frames);
        }
        if (ptr == null) return AudioCodecError.DecodeFailed;
        return DecodedPcm16{ .raw = ptr, .channels = ch, .sample_rate = sr, .frame_count = frames };
    }

    pub static fn decodeFlac(bytes: []const u8): DecodedPcm16! {
        let ch: u32 = 0;
        let sr: u32 = 0;
        let frames: u64 = 0;
        let ptr: [*]i16?! = null;
        unsafe {
            ptr = cp_drflac_decode_pcm16_mem(bytes.ptr, bytes.len, &ch, &sr, &frames);
        }
        if (ptr == null) return AudioCodecError.DecodeFailed;
        return DecodedPcm16{ .raw = ptr, .channels = ch, .sample_rate = sr, .frame_count = frames };
    }

    pub fn samples(): []const i16 {
        if (p := _.raw) {
            const total: usize = @intCast(_.frame_count * @as(u64, _.channels));
            return p[0..total];
        }
        return &.{};
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            _.frame_count = 0;
            unsafe { cp_dr_free(@ptrCast(p)); }
        }
    }
}

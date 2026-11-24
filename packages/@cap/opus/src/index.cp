// @cap/opus — Xiph libopus 1.5.2 audio codec native C binding for CP
// License: BSD-3-Clause (Upstream: Xiph.Org Foundation / opus)

import std from "std";

pub @c {
    fn opus_get_version_string(): [*:0]const u8?;
    fn opus_encoder_create(fs: i32, channels: i32, application: i32, err_out: *i32?!): *!?;
    fn opus_encoder_destroy(st: *!?);
    fn opus_encode(
        st: *!?,
        pcm: [*]const i16!,
        frame_size: i32,
        data: [*]u8!,
        max_data_bytes: i32
    ): i32;
    fn opus_decoder_create(fs: i32, channels: i32, err_out: *i32?!): *!?;
    fn opus_decoder_destroy(st: *!?);
    fn opus_decode(
        st: *!?,
        data: [*]const u8!,
        len: i32,
        pcm: [*]i16!,
        frame_size: i32,
        decode_fec: i32
    ): i32;
}

pub const OPUS_APPLICATION_VOIP: i32 = 2048;
pub const OPUS_APPLICATION_AUDIO: i32 = 2049;
pub const OPUS_APPLICATION_RESTRICTED_LOWDELAY: i32 = 2051;

pub err OpusError {
    BadArg,
    BufferTooSmall,
    InternalError,
    InvalidPacket,
    Unimplemented,
    InvalidState,
    AllocFail,
    EncodeFailed,
    DecodeFailed,
}

fn mapOpusError(code: i32): OpusError {
    if (code == -1) return OpusError.BadArg;
    if (code == -2) return OpusError.BufferTooSmall;
    if (code == -3) return OpusError.InternalError;
    if (code == -4) return OpusError.InvalidPacket;
    if (code == -5) return OpusError.Unimplemented;
    if (code == -6) return OpusError.InvalidState;
    if (code == -7) return OpusError.AllocFail;
    return OpusError.InternalError;
}

/// Returns the linked libopus version string (e.g. "libopus 1.5.2").
pub fn versionString(): string {
    unsafe {
        if (ver := opus_get_version_string()) {
            return std.mem.span(ver);
        }
    }
    return "libopus 1.5.2";
}

/// Owned RAII Opus encoder. Automatically destroys the native encoder on scope exit.
pub struct OpusEncoder {
    raw: *!? = null,
    pub sample_rate: i32 = 48000,
    pub channels: i32 = 1,

    pub static fn create(sample_rate: i32, channels: i32, app: i32): OpusEncoder! {
        let err_code: i32 = 0;
        let enc: *!? = null;
        unsafe {
            enc = opus_encoder_create(sample_rate, channels, app, &err_code);
        }
        if (err_code != 0) return mapOpusError(err_code);
        if (enc == null) return OpusError.AllocFail;
        return OpusEncoder{
            .raw = enc,
            .sample_rate = sample_rate,
            .channels = channels,
        };
    }

    /// Encodes 16-bit signed PCM samples (`frame_size * channels` samples) into `out`.
    /// Returns the encoded Opus packet slice view into `out`.
    pub fn encode(pcm: []const i16, frame_size: i32, out: []u8): []const u8! {
        if (_.raw == null) return OpusError.InvalidState;
        if (frame_size <= 0) return OpusError.BadArg;
        const needed: usize = @intCast(frame_size * _.channels);
        if (pcm.len < needed) return OpusError.BadArg;
        if (out.len == 0 or out.len > 0x7FFFFFFF) return OpusError.BufferTooSmall;
        let written: i32 = 0;
        unsafe {
            written = opus_encode(_.raw, pcm.ptr, frame_size, out.ptr, @intCast(out.len));
        }
        if (written < 0) return mapOpusError(written);
        return out[0..@intCast(written)];
    }

    ~init() {
        if (p := _.raw) {
            unsafe { opus_encoder_destroy(p); }
            _.raw = null;
        }
    }
}

/// Owned RAII Opus decoder. Automatically destroys the native decoder on scope exit.
pub struct OpusDecoder {
    raw: *!? = null,
    pub sample_rate: i32 = 48000,
    pub channels: i32 = 1,

    pub static fn create(sample_rate: i32, channels: i32): OpusDecoder! {
        let err_code: i32 = 0;
        let decoder: *!? = null;
        unsafe {
            decoder = opus_decoder_create(sample_rate, channels, &err_code);
        }
        if (err_code != 0) return mapOpusError(err_code);
        if (decoder == null) return OpusError.AllocFail;
        return OpusDecoder{
            .raw = decoder,
            .sample_rate = sample_rate,
            .channels = channels,
        };
    }

    /// Decodes an Opus packet into `out_pcm` (capacity at least `frame_size * channels` i16 samples).
    /// Returns the number of decoded samples per channel.
    pub fn decode(packet: []const u8, out_pcm: []i16, frame_size: i32): usize! {
        if (_.raw == null) return OpusError.InvalidState;
        if (frame_size <= 0) return OpusError.BadArg;
        if (packet.len > 0x7FFFFFFF) return OpusError.InvalidPacket;
        const needed: usize = @intCast(frame_size * _.channels);
        if (out_pcm.len < needed) return OpusError.BufferTooSmall;
        let decoded: i32 = 0;
        unsafe {
            decoded = opus_decode(_.raw, packet.ptr, @intCast(packet.len), out_pcm.ptr, frame_size, 0);
        }
        if (decoded < 0) return mapOpusError(decoded);
        return @intCast(decoded);
    }

    ~init() {
        if (p := _.raw) {
            unsafe { opus_decoder_destroy(p); }
            _.raw = null;
        }
    }
}

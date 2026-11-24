// @cap/miniaudio — Cross-platform audio decoding & PCM resampling (miniaudio 0.11.21)
// License: MIT-0 (Upstream: David Reid / mackron/miniaudio)

import std from "std";

pub @c {
    fn cp_ma_version_string(): [*:0]const u8?;
    fn cp_ma_decoder_from_memory(
        data: [*]const u8!,
        size: usize,
        out_channels: *u32?!,
        out_sample_rate: *u32?!,
        out_frames: *u64?!
    ): [*]f32?;
    fn cp_ma_free(ptr: *!?);
    fn cp_ma_resampler_create(in_rate: u32, out_rate: u32, channels: u32): *!?;
    fn cp_ma_resampler_process(
        resampler: *!?,
        frames_in: [*]const f32!,
        frame_count_in: *u64?!,
        frames_out: [*]f32!,
        frame_count_out: *u64?!
    ): i32;
    fn cp_ma_resampler_free(resampler: *!?);
}

pub err MiniaudioError {
    DecodeFailed,
    ResamplerCreateFailed,
    ResampleFailed,
    InvalidArg,
    InvalidState,
}

pub struct ResampleResult {
    pub in_frames: u64,
    pub out_frames: u64,
}

/// Returns the linked miniaudio version string (e.g. "0.11.21").
pub fn versionString(): string {
    unsafe {
        if (ver := cp_ma_version_string()) {
            return std.mem.span(ver);
        }
    }
    return "0.11.21";
}

/// Owned RAII decoded f32 PCM audio buffer. Automatically freed on scope exit.
pub struct DecodedAudio {
    raw: [*]f32? = null,
    pub channels: u32 = 0,
    pub sample_rate: u32 = 0,
    pub frames: u64 = 0,

    pub static fn fromMemory(data: []const u8): DecodedAudio! {
        if (data.len == 0) return MiniaudioError.InvalidArg;
        let ch: u32 = 0;
        let sr: u32 = 0;
        let fr: u64 = 0;
        let ptr: [*]f32? = null;
        unsafe {
            ptr = cp_ma_decoder_from_memory(data.ptr, data.len, &ch, &sr, &fr);
        }
        if (ptr == null) return MiniaudioError.DecodeFailed;
        return DecodedAudio{
            .raw = ptr,
            .channels = ch,
            .sample_rate = sr,
            .frames = fr,
        };
    }

    pub fn samples(): []const f32 {
        if (p := _.raw) {
            const total: usize = @intCast(_.frames * @as(u64, _.channels));
            return p[0..total];
        }
        return &.{};
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_ma_free(@ptrCast(p)); }
            _.raw = null;
            _.channels = 0;
            _.sample_rate = 0;
            _.frames = 0;
        }
    }
}

/// Owned RAII f32 PCM resampler. Automatically destroyed on scope exit.
pub struct Resampler {
    raw: *!? = null,
    pub in_rate: u32 = 0,
    pub out_rate: u32 = 0,
    pub channels: u32 = 0,

    pub static fn create(in_rate: u32, out_rate: u32, channels: u32): Resampler! {
        if (in_rate == 0 or out_rate == 0 or channels == 0) {
            return MiniaudioError.InvalidArg;
        }
        let handle: *!? = null;
        unsafe {
            handle = cp_ma_resampler_create(in_rate, out_rate, channels);
        }
        if (handle == null) return MiniaudioError.ResamplerCreateFailed;
        return Resampler{
            .raw = handle,
            .in_rate = in_rate,
            .out_rate = out_rate,
            .channels = channels,
        };
    }

    /// Resamples interleaved f32 PCM frames from `input` into `output`.
    pub fn process(input: []const f32, output: []f32): ResampleResult! {
        if (_.raw == null) return MiniaudioError.InvalidState;
        if (_.channels == 0) return MiniaudioError.InvalidState;
        const ch: usize = @intCast(_.channels);
        if (input.len == 0 or output.len == 0) return MiniaudioError.InvalidArg;
        let in_frames: u64 = @intCast(input.len / ch);
        let out_frames: u64 = @intCast(output.len / ch);
        if (in_frames == 0 or out_frames == 0) return MiniaudioError.InvalidArg;

        let rc: i32 = 0;
        unsafe {
            rc = cp_ma_resampler_process(_.raw, input.ptr, &in_frames, output.ptr, &out_frames);
        }
        if (rc != 0) return MiniaudioError.ResampleFailed;
        return ResampleResult{
            .in_frames = in_frames,
            .out_frames = out_frames,
        };
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_ma_resampler_free(p); }
            _.raw = null;
        }
    }
}

// Type definitions for CP

import {  } from "std";
export fn cp_ma_version_string(): [*:0]&u8!?;
export fn cp_ma_decoder_from_memory(data: [*]&u8!, size: usize, out_channels: *u32!?, out_sample_rate: *u32!?, out_frames: *u64!?): [*]f32!?;
export fn cp_ma_free(ptr: *auto!?);
export fn cp_ma_resampler_create(in_rate: u32, out_rate: u32, channels: u32): *auto!?;
export fn cp_ma_resampler_process(resampler: *auto!?, frames_in: [*]&f32!, frame_count_in: *u64!?, frames_out: [*]f32!, frame_count_out: *u64!?): i32;
export fn cp_ma_resampler_free(resampler: *auto!?);
export error MiniaudioError { DecodeFailed, ResamplerCreateFailed, ResampleFailed, InvalidArg, InvalidState }

export class ResampleResult {
    in_frames: u64,
    out_frames: u64,
}

export fn versionString(): string;
export class DecodedAudio {
    raw: [*]f32!?,
    channels: u32,
    sample_rate: u32,
    frames: u64,
    fn fromMemory(data: &[]u8): !DecodedAudio;
    fn samples(): &[]f32;
    fn __cp_deinit(_: &mut DecodedAudio);
}

export class Resampler {
    raw: *auto!?,
    in_rate: u32,
    out_rate: u32,
    channels: u32,
    fn create(in_rate: u32, out_rate: u32, channels: u32): !Resampler;
    fn process(input: &[]f32, output: []f32): !ResampleResult;
    fn __cp_deinit(_: &mut Resampler);
}


// Type definitions for CP

import {  } from "std";
export fn cp_drwav_encode_pcm16_mem(pcm: [*]&i16!, frame_count: u64, channels: u32, sample_rate: u32, out_size: *usize!): [*]u8!?;
export fn cp_drwav_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;
export fn cp_drflac_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;
export fn cp_dr_free(ptr: *auto!?);
export error AudioCodecError { EncodeFailed, DecodeFailed }

export class WavBytes {
    raw: [*]u8!?,
    len: usize,
    fn encodePcm16(pcm: &[]i16, channels: u32, sample_rate: u32): !WavBytes;
    fn slice(): &[]u8;
    fn __cp_deinit(_: &mut WavBytes);
}

export class DecodedPcm16 {
    raw: [*]i16!?,
    channels: u32,
    sample_rate: u32,
    frame_count: u64,
    fn decodeWav(bytes: &[]u8): !DecodedPcm16;
    fn decodeFlac(bytes: &[]u8): !DecodedPcm16;
    fn samples(): &[]i16;
    fn __cp_deinit(_: &mut DecodedPcm16);
}


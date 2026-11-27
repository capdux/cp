// Type definitions for CP

import {  } from "std";
export fn opus_get_version_string(): [*:0]&u8!?;
export fn opus_encoder_create(fs: i32, channels: i32, application: i32, err_out: *i32!?): *auto!?;
export fn opus_encoder_destroy(st: *auto!?);
export fn opus_encode(st: *auto!?, pcm: [*]&i16!, frame_size: i32, data: [*]u8!, max_data_bytes: i32): i32;
export fn opus_decoder_create(fs: i32, channels: i32, err_out: *i32!?): *auto!?;
export fn opus_decoder_destroy(st: *auto!?);
export fn opus_decode(st: *auto!?, data: [*]&u8!, len: i32, pcm: [*]i16!, frame_size: i32, decode_fec: i32): i32;
export error OpusError { BadArg, BufferTooSmall, InternalError, InvalidPacket, Unimplemented, InvalidState, AllocFail, EncodeFailed, DecodeFailed }

export fn mapOpusError(code: i32): OpusError;
export fn versionString(): string;
export class OpusEncoder {
    raw: *auto!?,
    sample_rate: i32,
    channels: i32,
    fn create(sample_rate: i32, channels: i32, app: i32): !OpusEncoder;
    fn encode(pcm: &[]i16, frame_size: i32, out: []u8): !&[]u8;
    fn __cp_deinit(_: &mut OpusEncoder);
}

export class OpusDecoder {
    raw: *auto!?,
    sample_rate: i32,
    channels: i32,
    fn create(sample_rate: i32, channels: i32): !OpusDecoder;
    fn decode(packet: &[]u8, out_pcm: []i16, frame_size: i32): !usize;
    fn __cp_deinit(_: &mut OpusDecoder);
}


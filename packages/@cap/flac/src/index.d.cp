// Type definitions for CP

import {  } from "std";
export fn cp_flac_version(): [*:0]&u8!?;
export fn cp_flac_vendor(): [*:0]&u8!?;
export fn cp_flac_encode_pcm16_test(pcm: [*]&i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32;
export error FlacError { InvalidInput, InitFailed, EncodeFailed }

export fn version(): string;
export fn vendor(): string;
export fn encodePcm16Test(pcm: &[]i16, sample_rate: u32): !u64;

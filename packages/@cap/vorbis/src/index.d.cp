// Type definitions for CP

import {  } from "std";
export fn cp_vorbis_version_string(): [*:0]&u8!;
export fn cp_vorbis_encoder_test_init(channels: i32, rate: isize, quality: f32): i32;
export error VorbisError { InvalidParams, InitFailed }

export fn versionString(): string;
export fn encoderTestInit(channels: i32, rate: isize, quality: f32): !void;

// Type definitions for CP

import {  } from "std";
export fn cp_vpx_version_str(): [*:0]&u8!;
export fn cp_vpx_vp8_enc_cfg_default(out_w: *u32!, out_h: *u32!): i32;
export fn cp_vpx_vp9_dec_open_close(threads: i32): i32;
export error VpxError { EncConfigFailed, DecoderInitFailed }

export class Vp8DefaultSize {
    width: u32,
    height: u32,
}

export fn versionStr(): string;
export fn vp8EncCfgDefault(): !Vp8DefaultSize;
export fn vp9DecOpenClose(threads: i32): !void;

# @cap/libvpx@1.15.0 (Agent Quickref)

> Google libvpx 1.15.0 royalty-free VP8/VP9 video codec for CP via prebuilt dynamic library | License: BSD-3-Clause
> Import: `import { ... } from "@cap/libvpx";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_vpx_version_str(): [*:0]&u8!;`
- `export fn cp_vpx_vp8_enc_cfg_default(out_w: *u32!, out_h: *u32!): i32;`
- `export fn cp_vpx_vp9_dec_open_close(threads: i32): i32;`
- `export fn versionStr(): string;`
- `export fn vp8EncCfgDefault(): !Vp8DefaultSize;`
- `export fn vp9DecOpenClose(threads: i32): !void;`

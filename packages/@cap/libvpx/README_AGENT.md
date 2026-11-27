# @cap/libvpx — AI Agent Quick Ref

Google libvpx 1.15.0 royalty-free VP8/VP9 video codec for CP via prebuilt dynamic library (`v1.15.0`, `BSD-3-Clause`)

## Import

```cp
import {
    VpxError,
    Vp8DefaultSize,
    cp_vpx_version_str,
    cp_vpx_vp8_enc_cfg_default,
    cp_vpx_vp9_dec_open_close,
    versionStr,
    vp8EncCfgDefault,
    vp9DecOpenClose,
} from "@cap/libvpx";
```

## Core Types & Constants

```cp
error VpxError { EncConfigFailed, DecoderInitFailed }
class Vp8DefaultSize { width: u32, height: u32 }
```

## API Signatures

```cp
fn cp_vpx_version_str(): [*:0]&u8!;
fn cp_vpx_vp8_enc_cfg_default(out_w: *u32!, out_h: *u32!): i32;
fn cp_vpx_vp9_dec_open_close(threads: i32): i32;
fn versionStr(): string;
fn vp8EncCfgDefault(): !Vp8DefaultSize;
fn vp9DecOpenClose(threads: i32): !void;
```

## Minimal Usage

```cp
import { versionStr } from "@cap/libvpx";

pub fn main() {
    let result = versionStr();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/libvpx"`; install via `cap add @cap/libvpx`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

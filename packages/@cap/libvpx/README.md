[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/libvpx

Google libvpx 1.15.0 royalty-free VP8/VP9 video codec for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`libvpx`](https://github.com/webmproject/libvpx)（`BSD-3-Clause`）（包名 `@cap/libvpx`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/libvpx` |
| Version | `1.15.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/libvpx
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `VpxError` | `EncConfigFailed, DecoderInitFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Vp8DefaultSize` | `class` | Fields: `width: u32, height: u32` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_vpx_version_str` | `(): [*:0]&u8!` | Invokes `cp_vpx_version_str` and returns `[*:0]&u8!`. |
| `cp_vpx_vp8_enc_cfg_default` | `(out_w: *u32!, out_h: *u32!): i32` | Invokes `cp_vpx_vp8_enc_cfg_default` and returns `i32`. |
| `cp_vpx_vp9_dec_open_close` | `(threads: i32): i32` | Invokes `cp_vpx_vp9_dec_open_close` and returns `i32`. |
| `versionStr` | `(): string` | Invokes `versionStr` and returns `string`. |
| `vp8EncCfgDefault` | `(): !Vp8DefaultSize` | Invokes `vp8EncCfgDefault` and returns `!Vp8DefaultSize`. |
| `vp9DecOpenClose` | `(threads: i32): !void` | Invokes `vp9DecOpenClose` and returns `!void`. |

## Minimal Example

```cp
import { versionStr } from "@cap/libvpx";

pub fn main() {
    let result = versionStr();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

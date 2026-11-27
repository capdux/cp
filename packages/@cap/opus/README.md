[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/opus

Xiph libopus 1.5.2 low-latency audio codec for voice AI agents and WebRTC in CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/opus` |
| Version | `1.5.2` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/opus
```

```cp
import {
    OpusError,
    OpusEncoder,
    OpusDecoder,
    opus_get_version_string,
    opus_encoder_create,
    opus_encoder_destroy,
    opus_encode,
    opus_decoder_create,
} from "@cap/opus";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `OpusError` | `BadArg, BufferTooSmall, InternalError, InvalidPacket, Unimplemented, InvalidState, AllocFail, EncodeFailed ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OpusEncoder` | `class` | Fields: `raw: *auto!?, sample_rate: i32, channels: i32` · Methods: `create, encode, __cp_deinit` |
| `OpusDecoder` | `class` | Fields: `raw: *auto!?, sample_rate: i32, channels: i32` · Methods: `create, decode, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `opus_get_version_string` | `(): [*:0]&u8!?` | Invokes `opus_get_version_string` and returns `[*:0]&u8!?`. |
| `opus_encoder_create` | `(fs: i32, channels: i32, application: i32, err_out: *i32!?): *auto!?` | Invokes `opus_encoder_create` and returns `*auto!?`. |
| `opus_encoder_destroy` | `(st: *auto!?)` | Executes `opus_encoder_destroy` in-place. |
| `opus_encode` | `(st: *auto!?, pcm: [*]&i16!, frame_size: i32, data: [*]u8!, max_data_bytes: i32): i32` | Invokes `opus_encode` and returns `i32`. |
| `opus_decoder_create` | `(fs: i32, channels: i32, err_out: *i32!?): *auto!?` | Invokes `opus_decoder_create` and returns `*auto!?`. |
| `opus_decoder_destroy` | `(st: *auto!?)` | Executes `opus_decoder_destroy` in-place. |
| `opus_decode` | `(st: *auto!?, data: [*]&u8!, len: i32, pcm: [*]i16!, frame_size: i32, decode_fec: i32): i32` | Invokes `opus_decode` and returns `i32`. |
| `mapOpusError` | `(code: i32): OpusError` | Invokes `mapOpusError` and returns `OpusError`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/opus";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

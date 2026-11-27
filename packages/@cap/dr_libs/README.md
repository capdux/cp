[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/dr_libs

Single-header zero-dependency WAV, MP3, and FLAC audio decoders/encoders (dr_libs) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/dr_libs` |
| Version | `0.13.17` |
| License | `MIT-0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/dr_libs
```

```cp
import {
    AudioCodecError,
    WavBytes,
    DecodedPcm16,
    cp_drwav_encode_pcm16_mem,
    cp_drwav_decode_pcm16_mem,
    cp_drflac_decode_pcm16_mem,
    cp_dr_free,
} from "@cap/dr_libs";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `AudioCodecError` | `EncodeFailed, DecodeFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WavBytes` | `class` | Fields: `raw: [*]u8!?, len: usize` · Methods: `encodePcm16, slice, __cp_deinit` |
| `DecodedPcm16` | `class` | Fields: `raw: [*]i16!?, channels: u32, sample_rate: u32, frame_count: u64` · Methods: `decodeWav, decodeFlac, samples, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_drwav_encode_pcm16_mem` | `(pcm: [*]&i16!, frame_count: u64, channels: u32, sample_rate: u32, out_size: *usize!): [*]u8!?` | Invokes `cp_drwav_encode_pcm16_mem` and returns `[*]u8!?`. |
| `cp_drwav_decode_pcm16_mem` | `(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?` | Invokes `cp_drwav_decode_pcm16_mem` and returns `[*]i16!?`. |
| `cp_drflac_decode_pcm16_mem` | `(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?` | Invokes `cp_drflac_decode_pcm16_mem` and returns `[*]i16!?`. |
| `cp_dr_free` | `(ptr: *auto!?)` | Executes `cp_dr_free` in-place. |

## Minimal Example

```cp
import { cp_drwav_encode_pcm16_mem } from "@cap/dr_libs";

pub fn main() {
    let input = "example";
    let result = cp_drwav_encode_pcm16_mem(input.ptr, 0, 0, 0, /* out_size: *usize! */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

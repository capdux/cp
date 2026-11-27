[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/miniaudio

Cross-platform audio playback, capture, decoding (WAV/FLAC/MP3) and PCM resampling (miniaudio 0.11.21) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/miniaudio` |
| Version | `0.11.21` |
| License | `MIT-0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/miniaudio
```

```cp
import {
    MiniaudioError,
    ResampleResult,
    DecodedAudio,
    Resampler,
    cp_ma_version_string,
    cp_ma_decoder_from_memory,
    cp_ma_free,
    cp_ma_resampler_create,
} from "@cap/miniaudio";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `MiniaudioError` | `DecodeFailed, ResamplerCreateFailed, ResampleFailed, InvalidArg, InvalidState` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ResampleResult` | `class` | Fields: `in_frames: u64, out_frames: u64` |
| `DecodedAudio` | `class` | Fields: `raw: [*]f32!?, channels: u32, sample_rate: u32, frames: u64` · Methods: `fromMemory, samples, __cp_deinit` |
| `Resampler` | `class` | Fields: `raw: *auto!?, in_rate: u32, out_rate: u32, channels: u32` · Methods: `create, process, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_ma_version_string` | `(): [*:0]&u8!?` | Invokes `cp_ma_version_string` and returns `[*:0]&u8!?`. |
| `cp_ma_decoder_from_memory` | `(data: [*]&u8!, size: usize, out_channels: *u32!?, out_sample_rate: *u32!?, out_frames: *u64!?): [*]f32!?` | Invokes `cp_ma_decoder_from_memory` and returns `[*]f32!?`. |
| `cp_ma_free` | `(ptr: *auto!?)` | Executes `cp_ma_free` in-place. |
| `cp_ma_resampler_create` | `(in_rate: u32, out_rate: u32, channels: u32): *auto!?` | Invokes `cp_ma_resampler_create` and returns `*auto!?`. |
| `cp_ma_resampler_process` | `(resampler: *auto!?, frames_in: [*]&f32!, frame_count_in: *u64!?, frames_out: [*]f32!, frame_count_out: *u64!?): i32` | Invokes `cp_ma_resampler_process` and returns `i32`. |
| `cp_ma_resampler_free` | `(resampler: *auto!?)` | Executes `cp_ma_resampler_free` in-place. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/miniaudio";

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

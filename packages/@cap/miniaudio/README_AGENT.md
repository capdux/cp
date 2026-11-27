# @cap/miniaudio — AI Agent Quick Ref

Cross-platform audio playback, capture, decoding (WAV/FLAC/MP3) and PCM resampling (miniaudio 0.11.21) for CP (`v0.11.21`, `MIT-0`)

## Import

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

## Core Types & Constants

```cp
error MiniaudioError { DecodeFailed, ResamplerCreateFailed, ResampleFailed, InvalidArg, InvalidState }
class ResampleResult { in_frames: u64, out_frames: u64 }
class DecodedAudio { raw: [*]f32!?, channels: u32, sample_rate: u32, frames: u64 } // methods: fromMemory(...) +2
class Resampler { raw: *auto!?, in_rate: u32, out_rate: u32, channels: u32 } // methods: create(...) +2
```

## API Signatures

```cp
fn cp_ma_version_string(): [*:0]&u8!?;
fn cp_ma_decoder_from_memory(data: [*]&u8!, size: usize, out_channels: *u32!?, out_sample_rate: *u32!?, out_frames: *u64!?): [*]f32!?;
fn cp_ma_free(ptr: *auto!?);
fn cp_ma_resampler_create(in_rate: u32, out_rate: u32, channels: u32): *auto!?;
fn cp_ma_resampler_process(resampler: *auto!?, frames_in: [*]&f32!, frame_count_in: *u64!?, frames_out: [*]f32!, frame_count_out: *u64!?): i32;
fn cp_ma_resampler_free(resampler: *auto!?);
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/miniaudio";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/miniaudio"`; install via `cap add @cap/miniaudio`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

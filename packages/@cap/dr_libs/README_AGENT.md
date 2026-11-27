# @cap/dr_libs — AI Agent Quick Ref

Single-header zero-dependency WAV, MP3, and FLAC audio decoders/encoders (dr_libs) for CP (`v0.13.17`, `MIT-0`)

## Import

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

## Core Types & Constants

```cp
error AudioCodecError { EncodeFailed, DecodeFailed }
class WavBytes { raw: [*]u8!?, len: usize } // methods: encodePcm16(...) +2
class DecodedPcm16 { raw: [*]i16!?, channels: u32, sample_rate: u32, frame_count: u64 } // methods: decodeWav(...) +3
```

## API Signatures

```cp
fn cp_drwav_encode_pcm16_mem(pcm: [*]&i16!, frame_count: u64, channels: u32, sample_rate: u32, out_size: *usize!): [*]u8!?;
fn cp_drwav_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;
fn cp_drflac_decode_pcm16_mem(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?;
fn cp_dr_free(ptr: *auto!?);
```

## Minimal Usage

```cp
import { cp_drwav_encode_pcm16_mem } from "@cap/dr_libs";

pub fn main() {
    let input = "example";
    let result = cp_drwav_encode_pcm16_mem(input.ptr, 0, 0, 0, /* out_size: *usize! */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/dr_libs"`; install via `cap add @cap/dr_libs`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

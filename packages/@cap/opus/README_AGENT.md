# @cap/opus — AI Agent Quick Ref

Xiph libopus 1.5.2 low-latency audio codec for voice AI agents and WebRTC in CP (`v1.5.2`, `BSD-3-Clause`)

## Import

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

## Core Types & Constants

```cp
error OpusError { BadArg, BufferTooSmall, InternalError, InvalidPacket, Unimplemented, InvalidState }
class OpusEncoder { raw: *auto!?, sample_rate: i32, channels: i32 } // methods: create(...) +2
class OpusDecoder { raw: *auto!?, sample_rate: i32, channels: i32 } // methods: create(...) +2
```

## API Signatures

```cp
fn opus_get_version_string(): [*:0]&u8!?;
fn opus_encoder_create(fs: i32, channels: i32, application: i32, err_out: *i32!?): *auto!?;
fn opus_encoder_destroy(st: *auto!?);
fn opus_encode(st: *auto!?, pcm: [*]&i16!, frame_size: i32, data: [*]u8!, max_data_bytes: i32): i32;
fn opus_decoder_create(fs: i32, channels: i32, err_out: *i32!?): *auto!?;
fn opus_decoder_destroy(st: *auto!?);
fn opus_decode(st: *auto!?, data: [*]&u8!, len: i32, pcm: [*]i16!, frame_size: i32, decode_fec: i32): i32;
fn mapOpusError(code: i32): OpusError;
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/opus";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/opus"`; install via `cap add @cap/opus`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

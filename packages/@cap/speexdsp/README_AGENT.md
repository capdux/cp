# @cap/speexdsp — AI Agent Quick Ref

Xiph SpeexDSP 1.2.1 real-time acoustic echo cancellation (AEC), noise suppression, AGC, and VAD for CP (`v1.2.1`, `BSD-3-Clause`)

## Import

```cp
import {
    SpeexDspError,
    SpeexEchoState,
    SpeexPreprocessState,
    speex_echo_state_init,
    speex_echo_state_destroy,
    speex_echo_cancellation,
    speex_echo_state_reset,
    speex_echo_ctl,
} from "@cap/speexdsp";
```

## Core Types & Constants

```cp
error SpeexDspError { InvalidArg, AllocFail, InvalidState, BufferTooSmall, CtlFailed }
class SpeexEchoState { raw: *auto!?, frame_size: i32, filter_length: i32 } // methods: create(...) +5
class SpeexPreprocessState { raw: *auto!?, frame_size: i32, sample_rate: i32 } // methods: create(...) +6
```

## API Signatures

```cp
fn speex_echo_state_init(frame_size: i32, filter_length: i32): *auto!?;
fn speex_echo_state_destroy(st: *auto!?);
fn speex_echo_cancellation(st: *auto!?, rec: [*]&i16!, play: [*]&i16!, out: [*]i16!);
fn speex_echo_state_reset(st: *auto!?);
fn speex_echo_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;
fn speex_preprocess_state_init(frame_size: i32, sampling_rate: i32): *auto!?;
fn speex_preprocess_state_destroy(st: *auto!?);
fn speex_preprocess_run(st: *auto!?, x: [*]i16!): i32;
fn speex_preprocess_ctl(st: *auto!?, request: i32, ptr: *auto!?): i32;
```

## Minimal Usage

```cp
import { speex_echo_state_init } from "@cap/speexdsp";

pub fn main() {
    let result = speex_echo_state_init(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/speexdsp"`; install via `cap add @cap/speexdsp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

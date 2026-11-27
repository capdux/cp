# @cap/rnnoise — AI Agent Quick Ref

Xiph RNNoise recurrent neural network speech denoising and VAD for CP voice AI agents (`v0.1.1`, `BSD-3-Clause`)

## Import

```cp
import {
    RnnoiseError,
    DenoiseState,
    rnnoise_get_size,
    rnnoise_get_frame_size,
    rnnoise_create,
    rnnoise_destroy,
    rnnoise_process_frame,
    queryFrameSize,
} from "@cap/rnnoise";
```

## Core Types & Constants

```cp
error RnnoiseError { AllocFail, InvalidState, BufferTooSmall }
class DenoiseState { raw: *auto!? } // methods: frameSize(...) +3
```

## API Signatures

```cp
fn rnnoise_get_size(): i32;
fn rnnoise_get_frame_size(): i32;
fn rnnoise_create(model: *auto!?): *auto!?;
fn rnnoise_destroy(st: *auto!?);
fn rnnoise_process_frame(st: *auto!?, out: [*]f32!, input: [*]&f32!): f32;
fn queryFrameSize(): usize;
fn frameSize(): usize;
```

## Minimal Usage

```cp
import { rnnoise_get_size } from "@cap/rnnoise";

pub fn main() {
    let result = rnnoise_get_size();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/rnnoise"`; install via `cap add @cap/rnnoise`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

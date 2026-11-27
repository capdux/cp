[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/speexdsp

Xiph SpeexDSP 1.2.1 real-time acoustic echo cancellation (AEC), noise suppression, AGC, and VAD for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/speexdsp` |
| Version | `1.2.1` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/speexdsp
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `SpeexDspError` | `InvalidArg, AllocFail, InvalidState, BufferTooSmall, CtlFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SpeexEchoState` | `class` | Fields: `raw: *auto!?, frame_size: i32, filter_length: i32` · Methods: `create, setSamplingRate, cancel, reset, rawPtr, __cp_deinit` |
| `SpeexPreprocessState` | `class` | Fields: `raw: *auto!?, frame_size: i32, sample_rate: i32` · Methods: `create, setDenoise, setAgc, setVad, setEchoState, run, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `speex_echo_state_init` | `(frame_size: i32, filter_length: i32): *auto!?` | Invokes `speex_echo_state_init` and returns `*auto!?`. |
| `speex_echo_state_destroy` | `(st: *auto!?)` | Executes `speex_echo_state_destroy` in-place. |
| `speex_echo_cancellation` | `(st: *auto!?, rec: [*]&i16!, play: [*]&i16!, out: [*]i16!)` | Executes `speex_echo_cancellation` in-place. |
| `speex_echo_state_reset` | `(st: *auto!?)` | Executes `speex_echo_state_reset` in-place. |
| `speex_echo_ctl` | `(st: *auto!?, request: i32, ptr: *auto!?): i32` | Invokes `speex_echo_ctl` and returns `i32`. |
| `speex_preprocess_state_init` | `(frame_size: i32, sampling_rate: i32): *auto!?` | Invokes `speex_preprocess_state_init` and returns `*auto!?`. |
| `speex_preprocess_state_destroy` | `(st: *auto!?)` | Executes `speex_preprocess_state_destroy` in-place. |
| `speex_preprocess_run` | `(st: *auto!?, x: [*]i16!): i32` | Invokes `speex_preprocess_run` and returns `i32`. |
| `speex_preprocess_ctl` | `(st: *auto!?, request: i32, ptr: *auto!?): i32` | Invokes `speex_preprocess_ctl` and returns `i32`. |

## Minimal Example

```cp
import { speex_echo_state_init } from "@cap/speexdsp";

pub fn main() {
    let result = speex_echo_state_init(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

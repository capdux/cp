[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/rnnoise

Xiph RNNoise recurrent neural network speech denoising and VAD for CP voice AI agents

| Metadata | Value |
| --- | --- |
| Package | `@cap/rnnoise` |
| Version | `0.1.1` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/rnnoise
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `RnnoiseError` | `AllocFail, InvalidState, BufferTooSmall` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DenoiseState` | `class` | Fields: `raw: *auto!?` · Methods: `frameSize, create, processFrame, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rnnoise_get_size` | `(): i32` | Invokes `rnnoise_get_size` and returns `i32`. |
| `rnnoise_get_frame_size` | `(): i32` | Invokes `rnnoise_get_frame_size` and returns `i32`. |
| `rnnoise_create` | `(model: *auto!?): *auto!?` | Invokes `rnnoise_create` and returns `*auto!?`. |
| `rnnoise_destroy` | `(st: *auto!?)` | Executes `rnnoise_destroy` in-place. |
| `rnnoise_process_frame` | `(st: *auto!?, out: [*]f32!, input: [*]&f32!): f32` | Invokes `rnnoise_process_frame` and returns `f32`. |
| `queryFrameSize` | `(): usize` | Invokes `queryFrameSize` and returns `usize`. |
| `frameSize` | `(): usize` | Invokes `frameSize` and returns `usize`. |

## Minimal Example

```cp
import { rnnoise_get_size } from "@cap/rnnoise";

pub fn main() {
    let result = rnnoise_get_size();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

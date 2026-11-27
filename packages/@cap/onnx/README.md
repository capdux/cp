[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/onnx

ONNX Runtime (v1.32.0) high-performance ML inference engine for CP via official prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`onnxruntime`](https://github.com/microsoft/onnxruntime)（`MIT`）（原库名 `onnxruntime`，CP 包名 `@cap/onnx`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/onnx` |
| Version | `1.32.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/onnx
```

```cp
import {
    OrtError,
    OrtEnv,
    OrtSessionOptions,
    cp_ort_version,
    cp_ort_create_env,
    cp_ort_release_env,
    cp_ort_create_session_options,
    cp_ort_set_intra_op_threads,
} from "@cap/onnx";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `OrtError` | `CreateEnvFailed, CreateSessionOptionsFailed, SetThreadsFailed, ClosedHandle, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OrtEnv` | `class` | Fields: `env_ptr: *auto!?` · Methods: `create, isValid, __cp_deinit` |
| `OrtSessionOptions` | `class` | Fields: `opts_ptr: *auto!?` · Methods: `create, setIntraOpThreads, isValid, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_ort_version` | `(): [*:0]&u8!` | Invokes `cp_ort_version` and returns `[*:0]&u8!`. |
| `cp_ort_create_env` | `(log_id: [*:0]&u8!): *auto!?` | Invokes `cp_ort_create_env` and returns `*auto!?`. |
| `cp_ort_release_env` | `(env: *auto!?)` | Executes `cp_ort_release_env` in-place. |
| `cp_ort_create_session_options` | `(): *auto!?` | Invokes `cp_ort_create_session_options` and returns `*auto!?`. |
| `cp_ort_set_intra_op_threads` | `(opts: *auto!?, n: i32): i32` | Invokes `cp_ort_set_intra_op_threads` and returns `i32`. |
| `cp_ort_release_session_options` | `(opts: *auto!?)` | Executes `cp_ort_release_session_options` in-place. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/onnx";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

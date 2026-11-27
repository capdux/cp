[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/llama

llama.cpp (b12676) local LLM inference engine for CP via official prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`llama.cpp`](https://github.com/ggml-org/llama.cpp)（`MIT`）（原库名 `llama.cpp`，CP 包名 `@cap/llama`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/llama` |
| Version | `1.0.12676` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/llama
```

```cp
import {
    LlamaError,
    LlamaBackend,
    LlamaModel,
    cp_llama_backend_init,
    cp_llama_backend_free,
    cp_llama_max_devices,
    cp_llama_supports_mmap,
    cp_llama_supports_mlock,
} from "@cap/llama";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `LlamaError` | `LoadModelFailed, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LlamaBackend` | `class` | Fields: `active: bool` · Methods: `init, isActive, __cp_deinit` |
| `LlamaModel` | `class` | Fields: `model_ptr: *auto!?` · Methods: `load, isValid, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_llama_backend_init` | `()` | Executes `cp_llama_backend_init` in-place. |
| `cp_llama_backend_free` | `()` | Executes `cp_llama_backend_free` in-place. |
| `cp_llama_max_devices` | `(): usize` | Invokes `cp_llama_max_devices` and returns `usize`. |
| `cp_llama_supports_mmap` | `(): i32` | Invokes `cp_llama_supports_mmap` and returns `i32`. |
| `cp_llama_supports_mlock` | `(): i32` | Invokes `cp_llama_supports_mlock` and returns `i32`. |
| `cp_llama_supports_gpu_offload` | `(): i32` | Invokes `cp_llama_supports_gpu_offload` and returns `i32`. |
| `cp_llama_time_us` | `(): i64` | Invokes `cp_llama_time_us` and returns `i64`. |
| `cp_llama_print_system_info` | `(): [*:0]&u8!` | Invokes `cp_llama_print_system_info` and returns `[*:0]&u8!`. |
| `cp_llama_load_model` | `(path: [*:0]&u8!, n_gpu_layers: i32): *auto!?` | Invokes `cp_llama_load_model` and returns `*auto!?`. |
| `cp_llama_free_model` | `(m: *auto!?)` | Executes `cp_llama_free_model` in-place. |
| `systemInfo` | `(): string` | Invokes `systemInfo` and returns `string`. |
| `supportsMmap` | `(): bool` | Invokes `supportsMmap` and returns `bool`. |
| `supportsMlock` | `(): bool` | Invokes `supportsMlock` and returns `bool`. |
| `supportsGpuOffload` | `(): bool` | Invokes `supportsGpuOffload` and returns `bool`. |
| `maxDevices` | `(): usize` | Invokes `maxDevices` and returns `usize`. |
| `timeUs` | `(): i64` | Invokes `timeUs` and returns `i64`. |

## Minimal Example

```cp
import { cp_llama_max_devices } from "@cap/llama";

pub fn main() {
    let result = cp_llama_max_devices();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

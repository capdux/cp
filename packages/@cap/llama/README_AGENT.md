# @cap/llama — AI Agent Quick Ref

llama.cpp (b12676) local LLM inference engine for CP via official prebuilt dynamic library (`v1.0.12676`, `MIT`)

## Import

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

## Core Types & Constants

```cp
error LlamaError { LoadModelFailed, OutOfMemory }
class LlamaBackend { active: bool } // methods: init(...) +2
class LlamaModel { model_ptr: *auto!? } // methods: load(...) +2
```

## API Signatures

```cp
fn cp_llama_backend_init();
fn cp_llama_backend_free();
fn cp_llama_max_devices(): usize;
fn cp_llama_supports_mmap(): i32;
fn cp_llama_supports_mlock(): i32;
fn cp_llama_supports_gpu_offload(): i32;
fn cp_llama_time_us(): i64;
fn cp_llama_print_system_info(): [*:0]&u8!;
fn cp_llama_load_model(path: [*:0]&u8!, n_gpu_layers: i32): *auto!?;
fn cp_llama_free_model(m: *auto!?);
fn systemInfo(): string;
fn supportsMmap(): bool;
fn supportsMlock(): bool;
fn supportsGpuOffload(): bool;
fn maxDevices(): usize;
fn timeUs(): i64;
```

## Minimal Usage

```cp
import { cp_llama_max_devices } from "@cap/llama";

pub fn main() {
    let result = cp_llama_max_devices();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/llama"`; install via `cap add @cap/llama`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

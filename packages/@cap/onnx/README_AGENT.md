# @cap/onnx — AI Agent Quick Ref

ONNX Runtime (v1.32.0) high-performance ML inference engine for CP via official prebuilt dynamic library (`v1.32.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
error OrtError { CreateEnvFailed, CreateSessionOptionsFailed, SetThreadsFailed, ClosedHandle, OutOfMemory }
class OrtEnv { env_ptr: *auto!? } // methods: create(...) +2
class OrtSessionOptions { opts_ptr: *auto!? } // methods: create(...) +3
```

## API Signatures

```cp
fn cp_ort_version(): [*:0]&u8!;
fn cp_ort_create_env(log_id: [*:0]&u8!): *auto!?;
fn cp_ort_release_env(env: *auto!?);
fn cp_ort_create_session_options(): *auto!?;
fn cp_ort_set_intra_op_threads(opts: *auto!?, n: i32): i32;
fn cp_ort_release_session_options(opts: *auto!?);
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/onnx";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/onnx"`; install via `cap add @cap/onnx`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

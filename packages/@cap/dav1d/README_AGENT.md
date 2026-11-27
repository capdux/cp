# @cap/dav1d — AI Agent Quick Ref

VideoLAN dav1d (v1.5.4) high-performance royalty-free AV1 video decoder for CP via prebuilt dynamic library (`v1.5.4`, `BSD-2-Clause`)

## Import

```cp
import {
    Dav1dError,
    Dav1dDecoder,
    cp_dav1d_version,
    cp_dav1d_version_api,
    cp_dav1d_open,
    cp_dav1d_flush,
    cp_dav1d_close,
    version,
} from "@cap/dav1d";
```

## Core Types & Constants

```cp
error Dav1dError { OpenFailed }
class Dav1dDecoder { raw: *auto!? } // methods: open(...) +3
```

## API Signatures

```cp
fn cp_dav1d_version(): [*:0]&u8!;
fn cp_dav1d_version_api(): u32;
fn cp_dav1d_open(n_threads: i32, max_frame_delay: i32): *auto!?;
fn cp_dav1d_flush(ctx: *auto!?);
fn cp_dav1d_close(ctx: *auto!?);
fn version(): string;
fn versionApi(): u32;
```

## Minimal Usage

```cp
import { version } from "@cap/dav1d";

pub fn main() {
    let result = version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/dav1d"`; install via `cap add @cap/dav1d`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

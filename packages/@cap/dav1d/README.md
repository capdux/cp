[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/dav1d

VideoLAN dav1d (v1.5.4) high-performance royalty-free AV1 video decoder for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`dav1d`](https://code.videolan.org/videolan/dav1d)（`BSD-2-Clause`）（包名 `@cap/dav1d`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/dav1d` |
| Version | `1.5.4` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/dav1d
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `Dav1dError` | `OpenFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Dav1dDecoder` | `class` | Fields: `raw: *auto!?` · Methods: `open, isOpen, flush, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_dav1d_version` | `(): [*:0]&u8!` | Invokes `cp_dav1d_version` and returns `[*:0]&u8!`. |
| `cp_dav1d_version_api` | `(): u32` | Invokes `cp_dav1d_version_api` and returns `u32`. |
| `cp_dav1d_open` | `(n_threads: i32, max_frame_delay: i32): *auto!?` | Invokes `cp_dav1d_open` and returns `*auto!?`. |
| `cp_dav1d_flush` | `(ctx: *auto!?)` | Executes `cp_dav1d_flush` in-place. |
| `cp_dav1d_close` | `(ctx: *auto!?)` | Executes `cp_dav1d_close` in-place. |
| `version` | `(): string` | Invokes `version` and returns `string`. |
| `versionApi` | `(): u32` | Invokes `versionApi` and returns `u32`. |

## Minimal Example

```cp
import { version } from "@cap/dav1d";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/harfbuzz

HarfBuzz (v15.2.0) industrial-standard OpenType text shaping engine (CJK/Arabic/Latin/Indic) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`harfbuzz`](https://github.com/harfbuzz/harfbuzz)（`MIT`）（包名 `@cap/harfbuzz`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/harfbuzz` |
| Version | `15.2.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/harfbuzz
```

```cp
import {
    HarfBuzzError,
    HbBuffer,
    cp_hb_version_string,
    cp_hb_buffer_create,
    cp_hb_buffer_destroy,
    cp_hb_buffer_reset,
    cp_hb_buffer_add_utf8,
    cp_hb_buffer_guess_segment_properties,
} from "@cap/harfbuzz";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `HarfBuzzError` | `BufferCreateFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HbBuffer` | `class` | Fields: `raw: *auto!?` · Methods: `create, addUtf8, guessSegmentProperties, length, direction, reset, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_hb_version_string` | `(): [*:0]&u8!` | Invokes `cp_hb_version_string` and returns `[*:0]&u8!`. |
| `cp_hb_buffer_create` | `(): *auto!?` | Invokes `cp_hb_buffer_create` and returns `*auto!?`. |
| `cp_hb_buffer_destroy` | `(buf: *auto!?)` | Executes `cp_hb_buffer_destroy` in-place. |
| `cp_hb_buffer_reset` | `(buf: *auto!?)` | Executes `cp_hb_buffer_reset` in-place. |
| `cp_hb_buffer_add_utf8` | `(buf: *auto!?, text: [*]&u8!, len: i32)` | Executes `cp_hb_buffer_add_utf8` in-place. |
| `cp_hb_buffer_guess_segment_properties` | `(buf: *auto!?)` | Executes `cp_hb_buffer_guess_segment_properties` in-place. |
| `cp_hb_buffer_get_length` | `(buf: *auto!?): u32` | Invokes `cp_hb_buffer_get_length` and returns `u32`. |
| `cp_hb_buffer_get_direction` | `(buf: *auto!?): i32` | Invokes `cp_hb_buffer_get_direction` and returns `i32`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/harfbuzz";

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

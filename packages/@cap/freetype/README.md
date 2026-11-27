[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/freetype

FreeType 2.13.3 industrial-standard TrueType/SFNT font rasterization engine for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/freetype` |
| Version | `2.13.3` |
| License | `FTL` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/freetype
```

```cp
import {
    FreeTypeError,
    FtVersion,
    FtFace,
    FtLibrary,
    cp_ft_init,
    cp_ft_done,
    cp_ft_version,
    cp_ft_new_memory_face,
} from "@cap/freetype";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `FreeTypeError` | `InitFailed, InvalidState, FaceLoadFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FtVersion` | `class` | Fields: `major: i32, minor: i32, patch: i32` |
| `FtFace` | `class` | Fields: `raw: *auto!?` · Methods: `numGlyphs, __cp_deinit` |
| `FtLibrary` | `class` | Fields: `raw: *auto!?` · Methods: `init, version, newMemoryFace, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_ft_init` | `(): *auto!?` | Invokes `cp_ft_init` and returns `*auto!?`. |
| `cp_ft_done` | `(lib: *auto!?)` | Executes `cp_ft_done` in-place. |
| `cp_ft_version` | `(lib: *auto!?, maj: *i32!, min: *i32!, pat: *i32!)` | Executes `cp_ft_version` in-place. |
| `cp_ft_new_memory_face` | `(lib: *auto!?, data: [*]&u8!, size: i64, face_idx: i64): *auto!?` | Invokes `cp_ft_new_memory_face` and returns `*auto!?`. |
| `cp_ft_done_face` | `(face: *auto!?)` | Executes `cp_ft_done_face` in-place. |
| `cp_ft_face_num_glyphs` | `(face: *auto!?): i64` | Invokes `cp_ft_face_num_glyphs` and returns `i64`. |

## Minimal Example

```cp
import { cp_ft_init } from "@cap/freetype";

pub fn main() {
    let result = cp_ft_init();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

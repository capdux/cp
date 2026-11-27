[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/libavif

AOMediaCodec libavif (v1.2.1) AVIF/HEIF image container & pixel format library for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/libavif` |
| Version | `1.2.1` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/libavif
```

```cp
import {
    AvifError,
    AvifImage,
    cp_avif_version,
    cp_avif_pixel_format_to_string,
    cp_avif_image_create,
    cp_avif_image_destroy,
    cp_avif_image_width,
    cp_avif_image_height,
} from "@cap/libavif";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `AvifError` | `InvalidArgument, AllocFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AvifImage` | `class` | Fields: `raw: *auto!?` · Methods: `create, width, height, depth, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_avif_version` | `(): [*:0]&u8!?` | Invokes `cp_avif_version` and returns `[*:0]&u8!?`. |
| `cp_avif_pixel_format_to_string` | `(fmt: i32): [*:0]&u8!?` | Invokes `cp_avif_pixel_format_to_string` and returns `[*:0]&u8!?`. |
| `cp_avif_image_create` | `(width: u32, height: u32, depth: u32, yuv_format: i32): *auto!?` | Invokes `cp_avif_image_create` and returns `*auto!?`. |
| `cp_avif_image_destroy` | `(img: *auto!?)` | Executes `cp_avif_image_destroy` in-place. |
| `cp_avif_image_width` | `(img: *auto!?): u32` | Invokes `cp_avif_image_width` and returns `u32`. |
| `cp_avif_image_height` | `(img: *auto!?): u32` | Invokes `cp_avif_image_height` and returns `u32`. |
| `cp_avif_image_depth` | `(img: *auto!?): u32` | Invokes `cp_avif_image_depth` and returns `u32`. |
| `cp_avif_image_create_and_destroy` | `(width: u32, height: u32, depth: u32): i32` | Invokes `cp_avif_image_create_and_destroy` and returns `i32`. |
| `version` | `(): string` | Invokes `version` and returns `string`. |
| `pixelFormatName` | `(fmt: i32): string` | Invokes `pixelFormatName` and returns `string`. |
| `imageCreateAndDestroy` | `(width: u32, height: u32, depth: u32): !void` | Invokes `imageCreateAndDestroy` and returns `!void`. |

## Minimal Example

```cp
import { version } from "@cap/libavif";

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

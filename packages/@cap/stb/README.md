[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/stb

Single-header image decoding (PNG/JPG/BMP/TGA/GIF), PNG encoding and SIMD image resizing (nothings/stb) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/stb` |
| Version | `2.30.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/stb
```

```cp
import {
    StbError,
    StbImage,
    PngBuffer,
    cp_stbi_load_from_memory,
    cp_stbi_image_free,
    cp_stbi_write_png_to_mem,
    cp_stbiw_free,
    cp_stbir_resize_rgba8,
} from "@cap/stb";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `StbError` | `DecodeFailed, EncodeFailed, ResizeFailed, InvalidArg, BufferTooSmall` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StbImage` | `class` | Fields: `raw: [*]u8!?, width: i32, height: i32, channels: i32` · Methods: `loadFromMemory, pixels, __cp_deinit` |
| `PngBuffer` | `class` | Fields: `raw: [*]u8!?, len: usize` · Methods: `encode, bytes, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_stbi_load_from_memory` | `(buffer: [*]&u8!, len: i32, x: *i32!?, y: *i32!?, channels_in_file: *i32!?, desired_channels: i32): [*]u8!?` | Invokes `cp_stbi_load_from_memory` and returns `[*]u8!?`. |
| `cp_stbi_image_free` | `(ptr: *auto!?)` | Executes `cp_stbi_image_free` in-place. |
| `cp_stbi_write_png_to_mem` | `(pixels: [*]&u8!, w: i32, h: i32, comp: i32, stride_bytes: i32, out_len: *i32!?): [*]u8!?` | Invokes `cp_stbi_write_png_to_mem` and returns `[*]u8!?`. |
| `cp_stbiw_free` | `(ptr: *auto!?)` | Executes `cp_stbiw_free` in-place. |
| `cp_stbir_resize_rgba8` | `(in_pixels: [*]&u8!, in_w: i32, in_h: i32, out_pixels: [*]u8!, out_w: i32, out_h: i32): i32` | Invokes `cp_stbir_resize_rgba8` and returns `i32`. |
| `resizeRgba8` | `(input: &[]u8, in_w: i32, in_h: i32, output: []u8, out_w: i32, out_h: i32): !void` | Invokes `resizeRgba8` and returns `!void`. |

## Minimal Example

```cp
import { resizeRgba8 } from "@cap/stb";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = resizeRgba8(out_buf[0..256], 0, 0, buf_1[0..256], 0, 0) catch return;
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

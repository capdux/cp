[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/libyuv

Google libyuv high-performance YUV/RGB pixel conversion, scaling, and rotation for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/libyuv` |
| Version | `1.9.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/libyuv
```

```cp
import {
    LibyuvError,
    I420ToARGB,
    ARGBToI420,
    ARGBScale,
    I420Rotate,
    absI32,
    i420ToArgb,
    argbToI420,
} from "@cap/libyuv";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `LibyuvError` | `InvalidDimensions, BufferTooSmall, InvalidRotation, ConvertFailed, ScaleFailed, RotateFailed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `I420ToARGB` | `(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_argb: [*]u8!, dst_stride_argb: i32, width: i32, height: i32): i32` | Invokes `I420ToARGB` and returns `i32`. |
| `ARGBToI420` | `(src_argb: [*]&u8!, src_stride_argb: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32): i32` | Invokes `ARGBToI420` and returns `i32`. |
| `ARGBScale` | `(src_argb: [*]&u8!, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: [*]u8!, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): i32` | Invokes `ARGBScale` and returns `i32`. |
| `I420Rotate` | `(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32, mode: i32): i32` | Invokes `I420Rotate` and returns `i32`. |
| `absI32` | `(v: i32): i32` | Invokes `absI32` and returns `i32`. |
| `i420ToArgb` | `(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_argb: []u8, dst_stride_argb: i32, width: i32, height: i32): !void` | Invokes `i420ToArgb` and returns `!void`. |
| `argbToI420` | `(src_argb: &[]u8, src_stride_argb: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32): !void` | Invokes `argbToI420` and returns `!void`. |
| `argbScale` | `(src_argb: &[]u8, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: []u8, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): !void` | Invokes `argbScale` and returns `!void`. |
| `i420Rotate` | `(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32, mode: i32): !void` | Invokes `i420Rotate` and returns `!void`. |

## Minimal Example

```cp
import { absI32 } from "@cap/libyuv";

pub fn main() {
    let result = absI32(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

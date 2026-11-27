[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/opencv

OpenCV (v4.10.0) core + imgproc computer vision library for CP via prebuilt dynamic libraries

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`opencv`](https://github.com/opencv/opencv)（`Apache-2.0`）（包名 `@cap/opencv`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/opencv` |
| Version | `4.10.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/opencv
```

```cp
import {
    OpenCvError,
    cp_cv_version,
    cp_cv_gaussian_blur_u8,
    cp_cv_canny_u8,
    cp_cv_resize_u8,
    version,
    gaussianBlurU8,
    cannyU8,
} from "@cap/opencv";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `OpenCvError` | `InvalidDimensions, InvalidKernelSize, InvalidThreshold, BufferTooSmall, OperationFailed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_cv_version` | `(): [*:0]&u8!` | Invokes `cp_cv_version` and returns `[*:0]&u8!`. |
| `cp_cv_gaussian_blur_u8` | `(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, ksize: i32): i32` | Invokes `cp_cv_gaussian_blur_u8` and returns `i32`. |
| `cp_cv_canny_u8` | `(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, t1: f64, t2: f64): i32` | Invokes `cp_cv_canny_u8` and returns `i32`. |
| `cp_cv_resize_u8` | `(src: [*]&u8!, sw: i32, sh: i32, dst: [*]u8!, dw: i32, dh: i32): i32` | Invokes `cp_cv_resize_u8` and returns `i32`. |
| `version` | `(): string` | Invokes `version` and returns `string`. |
| `gaussianBlurU8` | `(src: &[]u8, dst: []u8, w: i32, h: i32, ksize: i32): !void` | Invokes `gaussianBlurU8` and returns `!void`. |
| `cannyU8` | `(src: &[]u8, dst: []u8, w: i32, h: i32, t1: f64, t2: f64): !void` | Checks predicate `cannyU8` and returns `bool`. |
| `resizeU8` | `(src: &[]u8, sw: i32, sh: i32, dst: []u8, dw: i32, dh: i32): !void` | Invokes `resizeU8` and returns `!void`. |

## Minimal Example

```cp
import { version } from "@cap/opencv";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

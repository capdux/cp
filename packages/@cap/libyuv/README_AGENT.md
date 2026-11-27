# @cap/libyuv — AI Agent Quick Ref

Google libyuv high-performance YUV/RGB pixel conversion, scaling, and rotation for CP (`v1.9.0`, `BSD-3-Clause`)

## Import

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

## Core Types & Constants

```cp
error LibyuvError { InvalidDimensions, BufferTooSmall, InvalidRotation, ConvertFailed, ScaleFailed, RotateFailed }
```

## API Signatures

```cp
fn I420ToARGB(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_argb: [*]u8!, dst_stride_argb: i32, width: i32, height: i32): i32;
fn ARGBToI420(src_argb: [*]&u8!, src_stride_argb: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32): i32;
fn ARGBScale(src_argb: [*]&u8!, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: [*]u8!, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): i32;
fn I420Rotate(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32, mode: i32): i32;
fn absI32(v: i32): i32;
fn i420ToArgb(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_argb: []u8, dst_stride_argb: i32, width: i32, height: i32): !void;
fn argbToI420(src_argb: &[]u8, src_stride_argb: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32): !void;
fn argbScale(src_argb: &[]u8, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: []u8, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): !void;
fn i420Rotate(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32, mode: i32): !void;
```

## Minimal Usage

```cp
import { absI32 } from "@cap/libyuv";

pub fn main() {
    let result = absI32(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/libyuv"`; install via `cap add @cap/libyuv`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

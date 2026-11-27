# @cap/opencv — AI Agent Quick Ref

OpenCV (v4.10.0) core + imgproc computer vision library for CP via prebuilt dynamic libraries (`v4.10.0`, `Apache-2.0`)

## Import

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

## Core Types & Constants

```cp
error OpenCvError { InvalidDimensions, InvalidKernelSize, InvalidThreshold, BufferTooSmall, OperationFailed }
```

## API Signatures

```cp
fn cp_cv_version(): [*:0]&u8!;
fn cp_cv_gaussian_blur_u8(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, ksize: i32): i32;
fn cp_cv_canny_u8(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, t1: f64, t2: f64): i32;
fn cp_cv_resize_u8(src: [*]&u8!, sw: i32, sh: i32, dst: [*]u8!, dw: i32, dh: i32): i32;
fn version(): string;
fn gaussianBlurU8(src: &[]u8, dst: []u8, w: i32, h: i32, ksize: i32): !void;
fn cannyU8(src: &[]u8, dst: []u8, w: i32, h: i32, t1: f64, t2: f64): !void;
fn resizeU8(src: &[]u8, sw: i32, sh: i32, dst: []u8, dw: i32, dh: i32): !void;
```

## Minimal Usage

```cp
import { version } from "@cap/opencv";

pub fn main() {
    let result = version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/opencv"`; install via `cap add @cap/opencv`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

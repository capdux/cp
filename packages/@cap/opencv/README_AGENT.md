# @cap/opencv@4.10.0 (Agent Quickref)

> OpenCV (v4.10.0) core + imgproc computer vision library for CP via prebuilt dynamic libraries | License: Apache-2.0
> Import: `import { ... } from "@cap/opencv";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_cv_version(): [*:0]&u8!;`
- `export fn cp_cv_gaussian_blur_u8(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, ksize: i32): i32;`
- `export fn cp_cv_canny_u8(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, t1: f64, t2: f64): i32;`
- `export fn cp_cv_resize_u8(src: [*]&u8!, sw: i32, sh: i32, dst: [*]u8!, dw: i32, dh: i32): i32;`
- `export fn version(): string;`
- `export fn gaussianBlurU8(src: &[]u8, dst: []u8, w: i32, h: i32, ksize: i32): !void;`
- `export fn cannyU8(src: &[]u8, dst: []u8, w: i32, h: i32, t1: f64, t2: f64): !void;`
- `export fn resizeU8(src: &[]u8, sw: i32, sh: i32, dst: []u8, dw: i32, dh: i32): !void;`

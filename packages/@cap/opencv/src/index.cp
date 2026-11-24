// @cap/opencv — OpenCV (4.10.0) Core & Image Processing Binding for CP
// License: Apache-2.0 (Upstream: opencv/opencv)

import std from "std";

pub @c {
    fn cp_cv_version(): [*:0]const u8;
    fn cp_cv_gaussian_blur_u8(src: [*]const u8!, dst: [*]u8!, w: i32, h: i32, ksize: i32): i32;
    fn cp_cv_canny_u8(src: [*]const u8!, dst: [*]u8!, w: i32, h: i32, t1: f64, t2: f64): i32;
    fn cp_cv_resize_u8(src: [*]const u8!, sw: i32, sh: i32, dst: [*]u8!, dw: i32, dh: i32): i32;
}

pub err OpenCvError {
    InvalidDimensions,
    InvalidKernelSize,
    InvalidThreshold,
    BufferTooSmall,
    OperationFailed,
}

/// Returns the linked OpenCV version string (e.g. "4.10.0").
pub fn version(): string {
    unsafe {
        return std.mem.span(cp_cv_version());
    }
}

/// Applies Gaussian blur to a single-channel 8-bit grayscale image.
/// `ksize` must be a positive odd integer (e.g. 3, 5, 7).
pub fn gaussianBlurU8(src: []const u8, dst: []u8, w: i32, h: i32, ksize: i32): void! {
    if (w <= 0 or h <= 0) return OpenCvError.InvalidDimensions;
    if (ksize <= 0 or @rem(ksize, 2) == 0) return OpenCvError.InvalidKernelSize;
    const need: usize = @intCast(w * h);
    if (src.len < need or dst.len < need) return OpenCvError.BufferTooSmall;
    let rc: i32 = 0;
    unsafe {
        rc = cp_cv_gaussian_blur_u8(src.ptr, dst.ptr, w, h, ksize);
    }
    if (rc != 0) return OpenCvError.OperationFailed;
}

/// Runs Canny edge detection on a single-channel 8-bit grayscale image.
/// `t1` and `t2` are the hysteresis lower/upper thresholds (must be >= 0).
pub fn cannyU8(src: []const u8, dst: []u8, w: i32, h: i32, t1: f64, t2: f64): void! {
    if (w <= 0 or h <= 0) return OpenCvError.InvalidDimensions;
    if (t1 < 0.0 or t2 < 0.0) return OpenCvError.InvalidThreshold;
    const need: usize = @intCast(w * h);
    if (src.len < need or dst.len < need) return OpenCvError.BufferTooSmall;
    let rc: i32 = 0;
    unsafe {
        rc = cp_cv_canny_u8(src.ptr, dst.ptr, w, h, t1, t2);
    }
    if (rc != 0) return OpenCvError.OperationFailed;
}

/// Resizes a single-channel 8-bit grayscale image from `(sw, sh)` to `(dw, dh)` using bilinear interpolation.
pub fn resizeU8(src: []const u8, sw: i32, sh: i32, dst: []u8, dw: i32, dh: i32): void! {
    if (sw <= 0 or sh <= 0 or dw <= 0 or dh <= 0) return OpenCvError.InvalidDimensions;
    const need_src: usize = @intCast(sw * sh);
    const need_dst: usize = @intCast(dw * dh);
    if (src.len < need_src or dst.len < need_dst) return OpenCvError.BufferTooSmall;
    let rc: i32 = 0;
    unsafe {
        rc = cp_cv_resize_u8(src.ptr, sw, sh, dst.ptr, dw, dh);
    }
    if (rc != 0) return OpenCvError.OperationFailed;
}

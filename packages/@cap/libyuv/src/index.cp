// @cap/libyuv — Google libyuv YUV/RGB conversion, scaling, and rotation binding for CP
// License: BSD-3-Clause (Upstream: Google / Chromium libyuv)

pub @c {
    fn I420ToARGB(
        src_y: [*]const u8!,
        src_stride_y: i32,
        src_u: [*]const u8!,
        src_stride_u: i32,
        src_v: [*]const u8!,
        src_stride_v: i32,
        dst_argb: [*]u8!,
        dst_stride_argb: i32,
        width: i32,
        height: i32
    ): i32;

    fn ARGBToI420(
        src_argb: [*]const u8!,
        src_stride_argb: i32,
        dst_y: [*]u8!,
        dst_stride_y: i32,
        dst_u: [*]u8!,
        dst_stride_u: i32,
        dst_v: [*]u8!,
        dst_stride_v: i32,
        width: i32,
        height: i32
    ): i32;

    fn ARGBScale(
        src_argb: [*]const u8!,
        src_stride_argb: i32,
        src_width: i32,
        src_height: i32,
        dst_argb: [*]u8!,
        dst_stride_argb: i32,
        dst_width: i32,
        dst_height: i32,
        filtering: i32
    ): i32;

    fn I420Rotate(
        src_y: [*]const u8!,
        src_stride_y: i32,
        src_u: [*]const u8!,
        src_stride_u: i32,
        src_v: [*]const u8!,
        src_stride_v: i32,
        dst_y: [*]u8!,
        dst_stride_y: i32,
        dst_u: [*]u8!,
        dst_stride_u: i32,
        dst_v: [*]u8!,
        dst_stride_v: i32,
        width: i32,
        height: i32,
        mode: i32
    ): i32;
}

pub const FILTER_NONE: i32 = 0;
pub const FILTER_LINEAR: i32 = 1;
pub const FILTER_BILINEAR: i32 = 2;
pub const FILTER_BOX: i32 = 3;

pub const ROTATE_0: i32 = 0;
pub const ROTATE_90: i32 = 90;
pub const ROTATE_180: i32 = 180;
pub const ROTATE_270: i32 = 270;

pub err LibyuvError {
    InvalidDimensions,
    BufferTooSmall,
    InvalidRotation,
    ConvertFailed,
    ScaleFailed,
    RotateFailed,
}

fn absI32(v: i32): i32 {
    if (v < 0) return -v;
    return v;
}

/// Converts planar I420 (YUV 4:2:0) image planes into 32-bit ARGB (BGRA in little-endian memory).
pub fn i420ToArgb(
    src_y: []const u8,
    src_stride_y: i32,
    src_u: []const u8,
    src_stride_u: i32,
    src_v: []const u8,
    src_stride_v: i32,
    dst_argb: []u8,
    dst_stride_argb: i32,
    width: i32,
    height: i32
): void! {
    if (width <= 0 or height == 0) return LibyuvError.InvalidDimensions;
    if (src_stride_y <= 0 or src_stride_u <= 0 or src_stride_v <= 0 or dst_stride_argb <= 0) {
        return LibyuvError.InvalidDimensions;
    }
    const h: i32 = absI32(height);
    const half_h: i32 = @divTrunc(h + 1, 2);
    const need_y: usize = @intCast(src_stride_y * h);
    const need_u: usize = @intCast(src_stride_u * half_h);
    const need_v: usize = @intCast(src_stride_v * half_h);
    const need_dst: usize = @intCast(dst_stride_argb * h);
    if (src_y.len < need_y or src_u.len < need_u or src_v.len < need_v or dst_argb.len < need_dst) {
        return LibyuvError.BufferTooSmall;
    }
    let rc: i32 = 0;
    unsafe {
        rc = I420ToARGB(
            src_y.ptr,
            src_stride_y,
            src_u.ptr,
            src_stride_u,
            src_v.ptr,
            src_stride_v,
            dst_argb.ptr,
            dst_stride_argb,
            width,
            height
        );
    }
    if (rc != 0) return LibyuvError.ConvertFailed;
}

/// Converts 32-bit ARGB (BGRA in little-endian memory) into planar I420 (YUV 4:2:0).
pub fn argbToI420(
    src_argb: []const u8,
    src_stride_argb: i32,
    dst_y: []u8,
    dst_stride_y: i32,
    dst_u: []u8,
    dst_stride_u: i32,
    dst_v: []u8,
    dst_stride_v: i32,
    width: i32,
    height: i32
): void! {
    if (width <= 0 or height == 0) return LibyuvError.InvalidDimensions;
    if (src_stride_argb <= 0 or dst_stride_y <= 0 or dst_stride_u <= 0 or dst_stride_v <= 0) {
        return LibyuvError.InvalidDimensions;
    }
    const h: i32 = absI32(height);
    const half_h: i32 = @divTrunc(h + 1, 2);
    const need_src: usize = @intCast(src_stride_argb * h);
    const need_y: usize = @intCast(dst_stride_y * h);
    const need_u: usize = @intCast(dst_stride_u * half_h);
    const need_v: usize = @intCast(dst_stride_v * half_h);
    if (src_argb.len < need_src or dst_y.len < need_y or dst_u.len < need_u or dst_v.len < need_v) {
        return LibyuvError.BufferTooSmall;
    }
    let rc: i32 = 0;
    unsafe {
        rc = ARGBToI420(
            src_argb.ptr,
            src_stride_argb,
            dst_y.ptr,
            dst_stride_y,
            dst_u.ptr,
            dst_stride_u,
            dst_v.ptr,
            dst_stride_v,
            width,
            height
        );
    }
    if (rc != 0) return LibyuvError.ConvertFailed;
}

/// Scales an ARGB image from (src_width, src_height) to (dst_width, dst_height).
pub fn argbScale(
    src_argb: []const u8,
    src_stride_argb: i32,
    src_width: i32,
    src_height: i32,
    dst_argb: []u8,
    dst_stride_argb: i32,
    dst_width: i32,
    dst_height: i32,
    filtering: i32
): void! {
    if (src_width <= 0 or src_height == 0 or dst_width <= 0 or dst_height <= 0) {
        return LibyuvError.InvalidDimensions;
    }
    if (src_stride_argb <= 0 or dst_stride_argb <= 0) {
        return LibyuvError.InvalidDimensions;
    }
    const sh: i32 = absI32(src_height);
    const need_src: usize = @intCast(src_stride_argb * sh);
    const need_dst: usize = @intCast(dst_stride_argb * dst_height);
    if (src_argb.len < need_src or dst_argb.len < need_dst) {
        return LibyuvError.BufferTooSmall;
    }
    let rc: i32 = 0;
    unsafe {
        rc = ARGBScale(
            src_argb.ptr,
            src_stride_argb,
            src_width,
            src_height,
            dst_argb.ptr,
            dst_stride_argb,
            dst_width,
            dst_height,
            filtering
        );
    }
    if (rc != 0) return LibyuvError.ScaleFailed;
}

/// Rotates a planar I420 frame by 0, 90, 180, or 270 degrees clockwise.
pub fn i420Rotate(
    src_y: []const u8,
    src_stride_y: i32,
    src_u: []const u8,
    src_stride_u: i32,
    src_v: []const u8,
    src_stride_v: i32,
    dst_y: []u8,
    dst_stride_y: i32,
    dst_u: []u8,
    dst_stride_u: i32,
    dst_v: []u8,
    dst_stride_v: i32,
    width: i32,
    height: i32,
    mode: i32
): void! {
    if (width <= 0 or height == 0) return LibyuvError.InvalidDimensions;
    if (src_stride_y <= 0 or src_stride_u <= 0 or src_stride_v <= 0) {
        return LibyuvError.InvalidDimensions;
    }
    if (dst_stride_y <= 0 or dst_stride_u <= 0 or dst_stride_v <= 0) {
        return LibyuvError.InvalidDimensions;
    }
    if (mode != ROTATE_0 and mode != ROTATE_90 and mode != ROTATE_180 and mode != ROTATE_270) {
        return LibyuvError.InvalidRotation;
    }
    const h: i32 = absI32(height);
    const half_h: i32 = @divTrunc(h + 1, 2);
    let dst_h: i32 = h;
    if (mode == ROTATE_90 or mode == ROTATE_270) {
        dst_h = width;
    }
    const dst_half_h: i32 = @divTrunc(dst_h + 1, 2);
    const need_sy: usize = @intCast(src_stride_y * h);
    const need_su: usize = @intCast(src_stride_u * half_h);
    const need_sv: usize = @intCast(src_stride_v * half_h);
    const need_dy: usize = @intCast(dst_stride_y * dst_h);
    const need_du: usize = @intCast(dst_stride_u * dst_half_h);
    const need_dv: usize = @intCast(dst_stride_v * dst_half_h);
    if (src_y.len < need_sy or src_u.len < need_su or src_v.len < need_sv) {
        return LibyuvError.BufferTooSmall;
    }
    if (dst_y.len < need_dy or dst_u.len < need_du or dst_v.len < need_dv) {
        return LibyuvError.BufferTooSmall;
    }
    let rc: i32 = 0;
    unsafe {
        rc = I420Rotate(
            src_y.ptr,
            src_stride_y,
            src_u.ptr,
            src_stride_u,
            src_v.ptr,
            src_stride_v,
            dst_y.ptr,
            dst_stride_y,
            dst_u.ptr,
            dst_stride_u,
            dst_v.ptr,
            dst_stride_v,
            width,
            height,
            mode
        );
    }
    if (rc != 0) return LibyuvError.RotateFailed;
}

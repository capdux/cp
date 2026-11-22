// Origin: Pure-CP Stdlib (Unbounded-Slice Image Processing, YUV420p/NV12 Color Space, Separable Resize/Blur, Sobel & BMP/PPM Codec) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice image & video frame processing kernel:
//   - Video color space conversion: RGBA <-> Grayscale (BT.709), RGBA <-> YUV420p (I420) & NV12
//     with BT.601 / BT.709 limited & full range matrices and 2x2 chroma averaging
//   - Geometry transforms: `cropRgba`, `flipRgbaHorizontal`, `flipRgbaVertical`, `rotateRgba90Cw`
//   - Two-pass separable high-quality resizer (`nearest | bilinear | bicubic_catmull_rom | lanczos3`)
//     with anti-aliased downsampling kernel scaling and zero solid-color drift
//   - Separable Gaussian blur (`gaussianBlurRgba`) & 3x3 Sobel edge gradient magnitude (`sobelEdgeMagnitude`)
//   - Lossless container codecs: PPM P6 (with comment skipping) & BMP (24-bit BGR + 32-bit BGRA, top-down/bottom-up)
//   - Objective image quality metric: `computePsnrRgba`

const std = @import("std");

// ============================================================================
// 1. Error Set, Enums & Image View Metadata
// ============================================================================

const PI_F64: f64 = 3.14159265358979323846;
const MAX_GAUSS_RADIUS: usize = 64;

/// Error set returned by image color conversion, resizing, filtering, and codec operations.
pub err ImageOpsError {
    InvalidDimensions,
    OddDimensionsForYuv420,
    BufferTooSmall,
    InvalidFormat,
    UnsupportedBitsPerPixel,
    InvalidKernel,
    InvalidCrop,
}

/// Color matrix and range specification for RGB <-> YUV conversion.
pub enum ColorMatrixKind {
    bt601_limited,
    bt601_full,
    bt709_limited,
    bt709_full,
}

/// Resampling filter kernel for `resizeRgba`.
pub enum ResizeFilterKind {
    nearest,
    bilinear,
    bicubic_catmull_rom,
    lanczos3,
}

/// Decoded or external image plane metadata.
pub struct ImageViewMeta {
    width: usize = 0;
    height: usize = 0;
    channels: usize = 4;
    stride_bytes: usize = 0;

    /// Returns total tight byte count `width * height * channels`.
    fn requiredBytes(): usize {
        return width * height * channels;
    }

    /// Returns `true` iff dimensions, channel count (`1`, `3`, or `4`), and stride are valid.
    fn isValid(): bool {
        if (width == 0 || height == 0) return false;
        if (channels != 1 && channels != 3 && channels != 4) return false;
        if (stride_bytes < width * channels) return false;
        return true;
    }
}

/// Returns the minimum `scratch_f32.len` required by `resizeRgba` for non-nearest filters.
pub fn resizeRgbaScratchLen(src_h: usize, dst_w: usize): usize {
    return src_h * dst_w * 4;
}

// ============================================================================
// 2. Internal Numerical & Binary Helpers
// ============================================================================

fn clampU8FromF64(v: f64): u8 {
    if (v <= 0.0) return 0;
    if (v >= 255.0) return 255;
    return @as(u8, @intFromFloat(v + 0.5));
}

fn clampU8FromF32(v: f32): u8 {
    if (v <= 0.0) return 0;
    if (v >= 255.0) return 255;
    return @as(u8, @intFromFloat(v + 0.5));
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn clampIdx(v: i64, max_len: usize): usize {
    if (v <= 0) return 0;
    const lim: i64 = @as(i64, @intCast(max_len - 1));
    if (v >= lim) return max_len - 1;
    return @as(usize, @intCast(v));
}

fn readU16Le(buf: []const u8, off: usize): u16 {
    return @as(u16, buf[off]) | (@as(u16, buf[off + 1]) << 8);
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return @as(u32, buf[off]) |
        (@as(u32, buf[off + 1]) << 8) |
        (@as(u32, buf[off + 2]) << 16) |
        (@as(u32, buf[off + 3]) << 24);
}

fn readI32Le(buf: []const u8, off: usize): i32 {
    return @as(i32, @bitCast(readU32Le(buf, off)));
}

fn writeU16Le(dst: *[]u8, off: usize, v: u16) {
    dst[off] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
}

fn writeU32Le(dst: *[]u8, off: usize, v: u32) {
    dst[off] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    dst[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    dst[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

fn writeAsciiUsize(dst: *[]u8, pos: *usize, val_in: usize): bool {
    let digits: [24]u8 = [0; 24];
    let len: usize = 0;
    let v: usize = val_in;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @as(u8, @intCast('0' + (v % 10)));
            len += 1;
            v /= 10;
        }
    }
    if (pos.* + len > dst.len) return false;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        dst[pos.* + i] = digits[len - 1 - i];
    }
    pos.* += len;
    return true;
}

// ============================================================================
// 3. Video Color Space Conversions (Grayscale, YUV420p / I420, NV12)
// ============================================================================

struct YuvCoeffs {
    kr: f64 = 0.299;
    kg: f64 = 0.587;
    kb: f64 = 0.114;
    y_offset: f64 = 0.0;
    y_scale: f64 = 1.0;
    uv_scale: f64 = 1.0;
}

fn getYuvCoeffs(matrix: ColorMatrixKind): YuvCoeffs {
    if (matrix == ColorMatrixKind.bt601_full) {
        return YuvCoeffs{ kr: 0.299, kg: 0.587, kb: 0.114, y_offset: 0.0, y_scale: 1.0, uv_scale: 1.0 };
    }
    if (matrix == ColorMatrixKind.bt601_limited) {
        return YuvCoeffs{ kr: 0.299, kg: 0.587, kb: 0.114, y_offset: 16.0, y_scale: 219.0 / 255.0, uv_scale: 224.0 / 255.0 };
    }
    if (matrix == ColorMatrixKind.bt709_full) {
        return YuvCoeffs{ kr: 0.2126, kg: 0.7152, kb: 0.0722, y_offset: 0.0, y_scale: 1.0, uv_scale: 1.0 };
    }
    return YuvCoeffs{ kr: 0.2126, kg: 0.7152, kb: 0.0722, y_offset: 16.0, y_scale: 219.0 / 255.0, uv_scale: 224.0 / 255.0 };
}

fn rgbToY(r: f64, g: f64, b: f64, c: YuvCoeffs): u8 {
    const y_raw: f64 = c.kr * r + c.kg * g + c.kb * b;
    return clampU8FromF64(c.y_offset + c.y_scale * y_raw);
}

fn rgbToU(r: f64, g: f64, b: f64, c: YuvCoeffs): u8 {
    const y_raw: f64 = c.kr * r + c.kg * g + c.kb * b;
    const cb: f64 = 0.5 * (b - y_raw) / (1.0 - c.kb);
    return clampU8FromF64(128.0 + c.uv_scale * cb);
}

fn rgbToV(r: f64, g: f64, b: f64, c: YuvCoeffs): u8 {
    const y_raw: f64 = c.kr * r + c.kg * g + c.kb * b;
    const cr: f64 = 0.5 * (r - y_raw) / (1.0 - c.kr);
    return clampU8FromF64(128.0 + c.uv_scale * cr);
}

fn yuvPixelToRgba(y_u8: u8, u_u8: u8, v_u8: u8, c: YuvCoeffs, dst: *[]u8, off: usize) {
    const y: f64 = (@as(f64, @floatFromInt(y_u8)) - c.y_offset) / c.y_scale;
    const cb: f64 = (@as(f64, @floatFromInt(u_u8)) - 128.0) / c.uv_scale;
    const cr: f64 = (@as(f64, @floatFromInt(v_u8)) - 128.0) / c.uv_scale;

    const r: f64 = y + 2.0 * (1.0 - c.kr) * cr;
    const b: f64 = y + 2.0 * (1.0 - c.kb) * cb;
    const g: f64 = y - (2.0 * c.kb * (1.0 - c.kb) * cb + 2.0 * c.kr * (1.0 - c.kr) * cr) / c.kg;

    dst[off + 0] = clampU8FromF64(r);
    dst[off + 1] = clampU8FromF64(g);
    dst[off + 2] = clampU8FromF64(b);
    dst[off + 3] = 255;
}

/// Converts an RGBA frame to 8-bit grayscale using BT.709 perceptual luminance weights.
pub fn rgbaToGrayscale(
    rgba: []const u8,
    width: usize,
    height: usize,
    out_gray: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const npix: usize = width * height;
    if (rgba.len < npix * 4 || out_gray.len < npix) {
        return ImageOpsError.BufferTooSmall;
    }

    let i: usize = 0;
    while (i < npix) : (i += 1) {
        const off: usize = i * 4;
        const r: f64 = @as(f64, @floatFromInt(rgba[off + 0]));
        const g: f64 = @as(f64, @floatFromInt(rgba[off + 1]));
        const b: f64 = @as(f64, @floatFromInt(rgba[off + 2]));
        out_gray[i] = clampU8FromF64(0.2126 * r + 0.7152 * g + 0.0722 * b);
    }
}

/// Converts an RGBA frame (even width & height) into planar YUV420p (I420) with 2x2 chroma averaging.
pub fn rgbaToYuv420p(
    rgba: []const u8,
    width: usize,
    height: usize,
    matrix: ColorMatrixKind,
    out_y: *[]u8,
    out_u: *[]u8,
    out_v: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    if ((width % 2) != 0 || (height % 2) != 0) return ImageOpsError.OddDimensionsForYuv420;

    const npix: usize = width * height;
    const uv_w: usize = width / 2;
    const uv_h: usize = height / 2;
    const uv_len: usize = uv_w * uv_h;
    if (rgba.len < npix * 4 || out_y.len < npix || out_u.len < uv_len || out_v.len < uv_len) {
        return ImageOpsError.BufferTooSmall;
    }

    const c = getYuvCoeffs(matrix);

    // Luma plane Y
    let p: usize = 0;
    while (p < npix) : (p += 1) {
        const off: usize = p * 4;
        out_y[p] = rgbToY(
            @as(f64, @floatFromInt(rgba[off + 0])),
            @as(f64, @floatFromInt(rgba[off + 1])),
            @as(f64, @floatFromInt(rgba[off + 2])),
            c,
        );
    }

    // Subsampled 2x2 averaged U and V planes
    let cy: usize = 0;
    while (cy < uv_h) : (cy += 1) {
        const y0: usize = cy * 2;
        const y1: usize = y0 + 1;
        let cx: usize = 0;
        while (cx < uv_w) : (cx += 1) {
            const x0: usize = cx * 2;
            const x1: usize = x0 + 1;
            const o00: usize = (y0 * width + x0) * 4;
            const o01: usize = (y0 * width + x1) * 4;
            const o10: usize = (y1 * width + x0) * 4;
            const o11: usize = (y1 * width + x1) * 4;

            const r_avg: f64 = 0.25 * (@as(f64, @floatFromInt(rgba[o00 + 0])) +
                @as(f64, @floatFromInt(rgba[o01 + 0])) +
                @as(f64, @floatFromInt(rgba[o10 + 0])) +
                @as(f64, @floatFromInt(rgba[o11 + 0])));
            const g_avg: f64 = 0.25 * (@as(f64, @floatFromInt(rgba[o00 + 1])) +
                @as(f64, @floatFromInt(rgba[o01 + 1])) +
                @as(f64, @floatFromInt(rgba[o10 + 1])) +
                @as(f64, @floatFromInt(rgba[o11 + 1])));
            const b_avg: f64 = 0.25 * (@as(f64, @floatFromInt(rgba[o00 + 2])) +
                @as(f64, @floatFromInt(rgba[o01 + 2])) +
                @as(f64, @floatFromInt(rgba[o10 + 2])) +
                @as(f64, @floatFromInt(rgba[o11 + 2])));

            const uv_idx: usize = cy * uv_w + cx;
            out_u[uv_idx] = rgbToU(r_avg, g_avg, b_avg, c);
            out_v[uv_idx] = rgbToV(r_avg, g_avg, b_avg, c);
        }
    }
}

/// Converts planar YUV420p (I420) back to RGBA (`alpha = 255`).
pub fn yuv420pToRgba(
    y_plane: []const u8,
    u_plane: []const u8,
    v_plane: []const u8,
    width: usize,
    height: usize,
    matrix: ColorMatrixKind,
    out_rgba: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    if ((width % 2) != 0 || (height % 2) != 0) return ImageOpsError.OddDimensionsForYuv420;

    const npix: usize = width * height;
    const uv_w: usize = width / 2;
    const uv_len: usize = uv_w * (height / 2);
    if (y_plane.len < npix || u_plane.len < uv_len || v_plane.len < uv_len || out_rgba.len < npix * 4) {
        return ImageOpsError.BufferTooSmall;
    }

    const c = getYuvCoeffs(matrix);
    let y: usize = 0;
    while (y < height) : (y += 1) {
        const uv_row: usize = (y / 2) * uv_w;
        const y_row: usize = y * width;
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const uv_idx: usize = uv_row + (x / 2);
            const p_idx: usize = y_row + x;
            yuvPixelToRgba(y_plane[p_idx], u_plane[uv_idx], v_plane[uv_idx], c, out_rgba, p_idx * 4);
        }
    }
}

/// Converts an RGBA frame (even width & height) to semi-planar NV12 (`Y` + interleaved `UV`).
pub fn rgbaToNv12(
    rgba: []const u8,
    width: usize,
    height: usize,
    matrix: ColorMatrixKind,
    out_y: *[]u8,
    out_uv: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    if ((width % 2) != 0 || (height % 2) != 0) return ImageOpsError.OddDimensionsForYuv420;

    const npix: usize = width * height;
    const uv_w: usize = width / 2;
    const uv_h: usize = height / 2;
    const uv_bytes: usize = uv_w * uv_h * 2;
    if (rgba.len < npix * 4 || out_y.len < npix || out_uv.len < uv_bytes) {
        return ImageOpsError.BufferTooSmall;
    }

    const c = getYuvCoeffs(matrix);
    let p: usize = 0;
    while (p < npix) : (p += 1) {
        const off: usize = p * 4;
        out_y[p] = rgbToY(
            @as(f64, @floatFromInt(rgba[off + 0])),
            @as(f64, @floatFromInt(rgba[off + 1])),
            @as(f64, @floatFromInt(rgba[off + 2])),
            c,
        );
    }

    let cy: usize = 0;
    while (cy < uv_h) : (cy += 1) {
        const y0: usize = cy * 2;
        const y1: usize = y0 + 1;
        let cx: usize = 0;
        while (cx < uv_w) : (cx += 1) {
            const x0: usize = cx * 2;
            const x1: usize = x0 + 1;
            const o00: usize = (y0 * width + x0) * 4;
            const o01: usize = (y0 * width + x1) * 4;
            const o10: usize = (y1 * width + x0) * 4;
            const o11: usize = (y1 * width + x1) * 4;

            const r_avg: f64 = 0.25 * (@as(f64, @floatFromInt(rgba[o00 + 0])) +
                @as(f64, @floatFromInt(rgba[o01 + 0])) +
                @as(f64, @floatFromInt(rgba[o10 + 0])) +
                @as(f64, @floatFromInt(rgba[o11 + 0])));
            const g_avg: f64 = 0.25 * (@as(f64, @floatFromInt(rgba[o00 + 1])) +
                @as(f64, @floatFromInt(rgba[o01 + 1])) +
                @as(f64, @floatFromInt(rgba[o10 + 1])) +
                @as(f64, @floatFromInt(rgba[o11 + 1])));
            const b_avg: f64 = 0.25 * (@as(f64, @floatFromInt(rgba[o00 + 2])) +
                @as(f64, @floatFromInt(rgba[o01 + 2])) +
                @as(f64, @floatFromInt(rgba[o10 + 2])) +
                @as(f64, @floatFromInt(rgba[o11 + 2])));

            const uv_off: usize = (cy * uv_w + cx) * 2;
            out_uv[uv_off + 0] = rgbToU(r_avg, g_avg, b_avg, c);
            out_uv[uv_off + 1] = rgbToV(r_avg, g_avg, b_avg, c);
        }
    }
}

/// Converts semi-planar NV12 (`Y` + interleaved `UV`) back to RGBA (`alpha = 255`).
pub fn nv12ToRgba(
    y_plane: []const u8,
    uv_plane: []const u8,
    width: usize,
    height: usize,
    matrix: ColorMatrixKind,
    out_rgba: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    if ((width % 2) != 0 || (height % 2) != 0) return ImageOpsError.OddDimensionsForYuv420;

    const npix: usize = width * height;
    const uv_w: usize = width / 2;
    const uv_bytes: usize = uv_w * (height / 2) * 2;
    if (y_plane.len < npix || uv_plane.len < uv_bytes || out_rgba.len < npix * 4) {
        return ImageOpsError.BufferTooSmall;
    }

    const c = getYuvCoeffs(matrix);
    let y: usize = 0;
    while (y < height) : (y += 1) {
        const uv_row: usize = (y / 2) * uv_w;
        const y_row: usize = y * width;
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const uv_off: usize = (uv_row + (x / 2)) * 2;
            const p_idx: usize = y_row + x;
            yuvPixelToRgba(y_plane[p_idx], uv_plane[uv_off + 0], uv_plane[uv_off + 1], c, out_rgba, p_idx * 4);
        }
    }
}

// ============================================================================
// 4. Geometry Transforms & Two-Pass Separable Resizing
// ============================================================================

/// Extracts a sub-rectangle `[x..x+crop_w, y..y+crop_h]` from an RGBA frame into `dst`.
pub fn cropRgba(
    src: []const u8,
    src_w: usize,
    src_h: usize,
    x: usize,
    y: usize,
    crop_w: usize,
    crop_h: usize,
    dst: *[]u8,
): void ! ImageOpsError {
    if (src_w == 0 || src_h == 0 || crop_w == 0 || crop_h == 0) {
        return ImageOpsError.InvalidDimensions;
    }
    if (x + crop_w > src_w || y + crop_h > src_h) {
        return ImageOpsError.InvalidCrop;
    }
    if (src.len < src_w * src_h * 4 || dst.len < crop_w * crop_h * 4) {
        return ImageOpsError.BufferTooSmall;
    }

    let ry: usize = 0;
    while (ry < crop_h) : (ry += 1) {
        const src_off: usize = ((y + ry) * src_w + x) * 4;
        const dst_off: usize = (ry * crop_w) * 4;
        const row_bytes: usize = crop_w * 4;
        let b: usize = 0;
        while (b < row_bytes) : (b += 1) {
            dst[dst_off + b] = src[src_off + b];
        }
    }
}

/// Flips an RGBA frame horizontally (left-to-right mirror).
pub fn flipRgbaHorizontal(
    src: []const u8,
    width: usize,
    height: usize,
    dst: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const total: usize = width * height * 4;
    if (src.len < total || dst.len < total) return ImageOpsError.BufferTooSmall;

    let y: usize = 0;
    while (y < height) : (y += 1) {
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const s_off: usize = (y * width + x) * 4;
            const d_off: usize = (y * width + (width - 1 - x)) * 4;
            dst[d_off + 0] = src[s_off + 0];
            dst[d_off + 1] = src[s_off + 1];
            dst[d_off + 2] = src[s_off + 2];
            dst[d_off + 3] = src[s_off + 3];
        }
    }
}

/// Flips an RGBA frame vertically (top-to-bottom mirror).
pub fn flipRgbaVertical(
    src: []const u8,
    width: usize,
    height: usize,
    dst: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const total: usize = width * height * 4;
    if (src.len < total || dst.len < total) return ImageOpsError.BufferTooSmall;

    const row_bytes: usize = width * 4;
    let y: usize = 0;
    while (y < height) : (y += 1) {
        const s_off: usize = y * row_bytes;
        const d_off: usize = (height - 1 - y) * row_bytes;
        let b: usize = 0;
        while (b < row_bytes) : (b += 1) {
            dst[d_off + b] = src[s_off + b];
        }
    }
}

/// Rotates an RGBA frame 90 degrees clockwise (output dimensions are `dst_w = height, dst_h = width`).
pub fn rotateRgba90Cw(
    src: []const u8,
    width: usize,
    height: usize,
    dst: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const total: usize = width * height * 4;
    if (src.len < total || dst.len < total) return ImageOpsError.BufferTooSmall;

    let y: usize = 0;
    while (y < height) : (y += 1) {
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const s_off: usize = (y * width + x) * 4;
            const dx: usize = height - 1 - y;
            const dy: usize = x;
            const d_off: usize = (dy * height + dx) * 4;
            dst[d_off + 0] = src[s_off + 0];
            dst[d_off + 1] = src[s_off + 1];
            dst[d_off + 2] = src[s_off + 2];
            dst[d_off + 3] = src[s_off + 3];
        }
    }
}

fn filterSupportRadius(filter: ResizeFilterKind): f64 {
    if (filter == ResizeFilterKind.bilinear) return 1.0;
    if (filter == ResizeFilterKind.bicubic_catmull_rom) return 2.0;
    if (filter == ResizeFilterKind.lanczos3) return 3.0;
    return 0.5;
}

fn evalFilterKernel(filter: ResizeFilterKind, x_in: f64): f64 {
    const x: f64 = absF64(x_in);
    if (filter == ResizeFilterKind.bilinear) {
        return if (x < 1.0) 1.0 - x else 0.0;
    }
    if (filter == ResizeFilterKind.bicubic_catmull_rom) {
        // Catmull-Rom spline (Keys cubic with a = -0.5)
        if (x < 1.0) {
            return (1.5 * x - 2.5) * x * x + 1.0;
        }
        if (x < 2.0) {
            return ((-0.5 * x + 2.5) * x - 4.0) * x + 2.0;
        }
        return 0.0;
    }
    // Lanczos-3 kernel
    if (x < 1.0e-9) return 1.0;
    if (x >= 3.0) return 0.0;
    const px: f64 = PI_F64 * x;
    const px3: f64 = px / 3.0;
    return (std.math.sin(px) / px) * (std.math.sin(px3) / px3);
}

/// Resizes an RGBA frame from `src_w x src_h` to `dst_w x dst_h` using two-pass separable
/// horizontal + vertical filtering (`nearest | bilinear | bicubic_catmull_rom | lanczos3`).
/// For non-nearest modes, requires `scratch_f32.len >= src_h * dst_w * 4`.
pub fn resizeRgba(
    src: []const u8,
    src_w: usize,
    src_h: usize,
    dst_w: usize,
    dst_h: usize,
    filter: ResizeFilterKind,
    dst: *[]u8,
    scratch_f32: *[]f32,
): void ! ImageOpsError {
    if (src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0) {
        return ImageOpsError.InvalidDimensions;
    }
    if (src.len < src_w * src_h * 4 || dst.len < dst_w * dst_h * 4) {
        return ImageOpsError.BufferTooSmall;
    }

    if (filter == ResizeFilterKind.nearest) {
        let dy: usize = 0;
        while (dy < dst_h) : (dy += 1) {
            const sy: usize = (dy * src_h) / dst_h;
            let dx: usize = 0;
            while (dx < dst_w) : (dx += 1) {
                const sx: usize = (dx * src_w) / dst_w;
                const s_off: usize = (sy * src_w + sx) * 4;
                const d_off: usize = (dy * dst_w + dx) * 4;
                dst[d_off + 0] = src[s_off + 0];
                dst[d_off + 1] = src[s_off + 1];
                dst[d_off + 2] = src[s_off + 2];
                dst[d_off + 3] = src[s_off + 3];
            }
        }
        return;
    }

    if (scratch_f32.len < resizeRgbaScratchLen(src_h, dst_w)) {
        return ImageOpsError.BufferTooSmall;
    }

    const base_radius: f64 = filterSupportRadius(filter);

    // Pass 1: Horizontal resize (src_w -> dst_w) into scratch_f32[src_h * dst_w * 4]
    const scale_x: f64 = @as(f64, @floatFromInt(src_w)) / @as(f64, @floatFromInt(dst_w));
    const filter_scale_x: f64 = if (scale_x > 1.0) scale_x else 1.0;
    const support_x: f64 = base_radius * filter_scale_x;

    let sy: usize = 0;
    while (sy < src_h) : (sy += 1) {
        let dx: usize = 0;
        while (dx < dst_w) : (dx += 1) {
            const center_x: f64 = (@as(f64, @floatFromInt(dx)) + 0.5) * scale_x - 0.5;
            const left: i64 = @as(i64, @intFromFloat(std.math.floor(center_x - support_x)));
            const right: i64 = @as(i64, @intFromFloat(std.math.ceil(center_x + support_x)));

            let acc0: f64 = 0.0;
            let acc1: f64 = 0.0;
            let acc2: f64 = 0.0;
            let acc3: f64 = 0.0;
            let w_sum: f64 = 0.0;

            let kx: i64 = left;
            while (kx <= right) : (kx += 1) {
                const w: f64 = evalFilterKernel(filter, (@as(f64, @floatFromInt(kx)) - center_x) / filter_scale_x);
                if (absF64(w) <= 1.0e-12) continue;
                const sx: usize = clampIdx(kx, src_w);
                const s_off: usize = (sy * src_w + sx) * 4;
                acc0 += @as(f64, @floatFromInt(src[s_off + 0])) * w;
                acc1 += @as(f64, @floatFromInt(src[s_off + 1])) * w;
                acc2 += @as(f64, @floatFromInt(src[s_off + 2])) * w;
                acc3 += @as(f64, @floatFromInt(src[s_off + 3])) * w;
                w_sum += w;
            }

            const inv_w: f64 = if (absF64(w_sum) > 1.0e-12) (1.0 as f64) / w_sum else 0.0;
            const sc_off: usize = (sy * dst_w + dx) * 4;
            scratch_f32[sc_off + 0] = (acc0 * inv_w) as f32;
            scratch_f32[sc_off + 1] = (acc1 * inv_w) as f32;
            scratch_f32[sc_off + 2] = (acc2 * inv_w) as f32;
            scratch_f32[sc_off + 3] = (acc3 * inv_w) as f32;
        }
    }

    // Pass 2: Vertical resize (src_h -> dst_h) from scratch_f32 into dst[dst_h * dst_w * 4]
    const scale_y: f64 = @as(f64, @floatFromInt(src_h)) / @as(f64, @floatFromInt(dst_h));
    const filter_scale_y: f64 = if (scale_y > 1.0) scale_y else 1.0;
    const support_y: f64 = base_radius * filter_scale_y;

    let dy: usize = 0;
    while (dy < dst_h) : (dy += 1) {
        const center_y: f64 = (@as(f64, @floatFromInt(dy)) + 0.5) * scale_y - 0.5;
        const top: i64 = @as(i64, @intFromFloat(std.math.floor(center_y - support_y)));
        const bottom: i64 = @as(i64, @intFromFloat(std.math.ceil(center_y + support_y)));

        let dx: usize = 0;
        while (dx < dst_w) : (dx += 1) {
            let acc0: f64 = 0.0;
            let acc1: f64 = 0.0;
            let acc2: f64 = 0.0;
            let acc3: f64 = 0.0;
            let w_sum: f64 = 0.0;

            let ky: i64 = top;
            while (ky <= bottom) : (ky += 1) {
                const w: f64 = evalFilterKernel(filter, (@as(f64, @floatFromInt(ky)) - center_y) / filter_scale_y);
                if (absF64(w) <= 1.0e-12) continue;
                const sy_c: usize = clampIdx(ky, src_h);
                const sc_off: usize = (sy_c * dst_w + dx) * 4;
                acc0 += (scratch_f32[sc_off + 0] as f64) * w;
                acc1 += (scratch_f32[sc_off + 1] as f64) * w;
                acc2 += (scratch_f32[sc_off + 2] as f64) * w;
                acc3 += (scratch_f32[sc_off + 3] as f64) * w;
                w_sum += w;
            }

            const inv_w: f64 = if (absF64(w_sum) > 1.0e-12) (1.0 as f64) / w_sum else 0.0;
            const d_off: usize = (dy * dst_w + dx) * 4;
            dst[d_off + 0] = clampU8FromF64(acc0 * inv_w);
            dst[d_off + 1] = clampU8FromF64(acc1 * inv_w);
            dst[d_off + 2] = clampU8FromF64(acc2 * inv_w);
            dst[d_off + 3] = clampU8FromF64(acc3 * inv_w);
        }
    }
}

// ============================================================================
// 5. Separable Gaussian Blur & 3x3 Sobel Edge Gradient Magnitude
// ============================================================================

/// Applies a two-pass separable 2D Gaussian blur to an RGBA frame with zero heap allocation.
/// Requires `scratch_f32.len >= width * height * 4` and `1 <= radius <= 64`.
pub fn gaussianBlurRgba(
    src: []const u8,
    width: usize,
    height: usize,
    sigma: f32,
    radius: usize,
    dst: *[]u8,
    scratch_f32: *[]f32,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    if (radius == 0 || radius > MAX_GAUSS_RADIUS || sigma <= 0.0 || std.math.isNan(sigma) || std.math.isInf(sigma)) {
        return ImageOpsError.InvalidKernel;
    }
    const total: usize = width * height * 4;
    if (src.len < total || dst.len < total || scratch_f32.len < total) {
        return ImageOpsError.BufferTooSmall;
    }

    // Precompute normalized symmetric 1D Gaussian weights kernel[0..=radius].
    let kernel: [MAX_GAUSS_RADIUS + 1]f64 = [0.0; MAX_GAUSS_RADIUS + 1];
    const sig_f64: f64 = sigma as f64;
    const inv_2sig2: f64 = 1.0 / (2.0 * sig_f64 * sig_f64);
    kernel[0] = 1.0;
    let w_total: f64 = 1.0;
    let r_i: usize = 1;
    while (r_i <= radius) : (r_i += 1) {
        const d: f64 = @as(f64, @floatFromInt(r_i));
        const kv: f64 = std.math.exp(-(d * d) * inv_2sig2);
        kernel[r_i] = kv;
        w_total += 2.0 * kv;
    }
    let norm_i: usize = 0;
    while (norm_i <= radius) : (norm_i += 1) {
        kernel[norm_i] /= w_total;
    }

    // Pass 1: Horizontal convolution into scratch_f32
    let y: usize = 0;
    while (y < height) : (y += 1) {
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const c_off: usize = (y * width + x) * 4;
            const w0: f64 = kernel[0];
            let acc0: f64 = @as(f64, @floatFromInt(src[c_off + 0])) * w0;
            let acc1: f64 = @as(f64, @floatFromInt(src[c_off + 1])) * w0;
            let acc2: f64 = @as(f64, @floatFromInt(src[c_off + 2])) * w0;
            let acc3: f64 = @as(f64, @floatFromInt(src[c_off + 3])) * w0;

            let k: usize = 1;
            while (k <= radius) : (k += 1) {
                const wk: f64 = kernel[k];
                const lx: usize = clampIdx(@as(i64, @intCast(x)) - @as(i64, @intCast(k)), width);
                const rx: usize = clampIdx(@as(i64, @intCast(x)) + @as(i64, @intCast(k)), width);
                const loff: usize = (y * width + lx) * 4;
                const roff: usize = (y * width + rx) * 4;
                acc0 += (@as(f64, @floatFromInt(src[loff + 0])) + @as(f64, @floatFromInt(src[roff + 0]))) * wk;
                acc1 += (@as(f64, @floatFromInt(src[loff + 1])) + @as(f64, @floatFromInt(src[roff + 1]))) * wk;
                acc2 += (@as(f64, @floatFromInt(src[loff + 2])) + @as(f64, @floatFromInt(src[roff + 2]))) * wk;
                acc3 += (@as(f64, @floatFromInt(src[loff + 3])) + @as(f64, @floatFromInt(src[roff + 3]))) * wk;
            }

            scratch_f32[c_off + 0] = acc0 as f32;
            scratch_f32[c_off + 1] = acc1 as f32;
            scratch_f32[c_off + 2] = acc2 as f32;
            scratch_f32[c_off + 3] = acc3 as f32;
        }
    }

    // Pass 2: Vertical convolution from scratch_f32 into dst
    let y2: usize = 0;
    while (y2 < height) : (y2 += 1) {
        let x2: usize = 0;
        while (x2 < width) : (x2 += 1) {
            const c_off: usize = (y2 * width + x2) * 4;
            const w0: f64 = kernel[0];
            let acc0: f64 = (scratch_f32[c_off + 0] as f64) * w0;
            let acc1: f64 = (scratch_f32[c_off + 1] as f64) * w0;
            let acc2: f64 = (scratch_f32[c_off + 2] as f64) * w0;
            let acc3: f64 = (scratch_f32[c_off + 3] as f64) * w0;

            let k: usize = 1;
            while (k <= radius) : (k += 1) {
                const wk: f64 = kernel[k];
                const uy: usize = clampIdx(@as(i64, @intCast(y2)) - @as(i64, @intCast(k)), height);
                const dy: usize = clampIdx(@as(i64, @intCast(y2)) + @as(i64, @intCast(k)), height);
                const uoff: usize = (uy * width + x2) * 4;
                const doff: usize = (dy * width + x2) * 4;
                acc0 += ((scratch_f32[uoff + 0] as f64) + (scratch_f32[doff + 0] as f64)) * wk;
                acc1 += ((scratch_f32[uoff + 1] as f64) + (scratch_f32[doff + 1] as f64)) * wk;
                acc2 += ((scratch_f32[uoff + 2] as f64) + (scratch_f32[doff + 2] as f64)) * wk;
                acc3 += ((scratch_f32[uoff + 3] as f64) + (scratch_f32[doff + 3] as f64)) * wk;
            }

            dst[c_off + 0] = clampU8FromF64(acc0);
            dst[c_off + 1] = clampU8FromF64(acc1);
            dst[c_off + 2] = clampU8FromF64(acc2);
            dst[c_off + 3] = clampU8FromF64(acc3);
        }
    }
}

/// Computes 3x3 Sobel gradient magnitude `clamp(sqrt(Gx^2 + Gy^2), 0, 255)` on an 8-bit grayscale plane.
pub fn sobelEdgeMagnitude(
    gray: []const u8,
    width: usize,
    height: usize,
    out_mag: *[]u8,
): void ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const npix: usize = width * height;
    if (gray.len < npix || out_mag.len < npix) return ImageOpsError.BufferTooSmall;

    let y: usize = 0;
    while (y < height) : (y += 1) {
        const ym1: usize = clampIdx(@as(i64, @intCast(y)) - 1, height);
        const yp1: usize = clampIdx(@as(i64, @intCast(y)) + 1, height);
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const xm1: usize = clampIdx(@as(i64, @intCast(x)) - 1, width);
            const xp1: usize = clampIdx(@as(i64, @intCast(x)) + 1, width);

            const p00: i32 = @as(i32, gray[ym1 * width + xm1]);
            const p01: i32 = @as(i32, gray[ym1 * width + x]);
            const p02: i32 = @as(i32, gray[ym1 * width + xp1]);
            const p10: i32 = @as(i32, gray[y * width + xm1]);
            const p12: i32 = @as(i32, gray[y * width + xp1]);
            const p20: i32 = @as(i32, gray[yp1 * width + xm1]);
            const p21: i32 = @as(i32, gray[yp1 * width + x]);
            const p22: i32 = @as(i32, gray[yp1 * width + xp1]);

            const gx: f64 = @as(f64, @floatFromInt(-p00 + p02 - 2 * p10 + 2 * p12 - p20 + p22));
            const gy: f64 = @as(f64, @floatFromInt(-p00 - 2 * p01 - p02 + p20 + 2 * p21 + p22));
            out_mag[y * width + x] = clampU8FromF64(std.math.sqrt(gx * gx + gy * gy));
        }
    }
}

// ============================================================================
// 6. Lossless Image Container Codecs (PPM P6 & BMP 24/32-bit)
// ============================================================================

/// Encodes an RGBA frame as a binary Netpbm PPM P6 (`RGB888`) image into `out_buf`.
pub fn encodePpmP6(
    rgba: []const u8,
    width: usize,
    height: usize,
    out_buf: *[]u8,
): usize ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const npix: usize = width * height;
    if (rgba.len < npix * 4) return ImageOpsError.BufferTooSmall;
    if (out_buf.len < 16 + npix * 3) return ImageOpsError.BufferTooSmall;

    let pos: usize = 0;
    out_buf[0] = 'P';
    out_buf[1] = '6';
    out_buf[2] = '\n';
    pos = 3;
    if (!writeAsciiUsize(out_buf, *pos, width)) return ImageOpsError.BufferTooSmall;
    if (pos >= out_buf.len) return ImageOpsError.BufferTooSmall;
    out_buf[pos] = ' ';
    pos += 1;
    if (!writeAsciiUsize(out_buf, *pos, height)) return ImageOpsError.BufferTooSmall;
    if (pos + 5 > out_buf.len) return ImageOpsError.BufferTooSmall;
    out_buf[pos + 0] = '\n';
    out_buf[pos + 1] = '2';
    out_buf[pos + 2] = '5';
    out_buf[pos + 3] = '5';
    out_buf[pos + 4] = '\n';
    pos += 5;

    if (pos + npix * 3 > out_buf.len) return ImageOpsError.BufferTooSmall;
    let i: usize = 0;
    while (i < npix) : (i += 1) {
        const s_off: usize = i * 4;
        out_buf[pos + 0] = rgba[s_off + 0];
        out_buf[pos + 1] = rgba[s_off + 1];
        out_buf[pos + 2] = rgba[s_off + 2];
        pos += 3;
    }
    return pos;
}

fn isPpmSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn readPpmUint(raw: []const u8, pos: *usize): usize ! ImageOpsError {
    // Skip whitespace and '#' comments.
    while (pos.* < raw.len) {
        const ch: u8 = raw[pos.*];
        if (isPpmSpace(ch)) {
            pos.* += 1;
        } else if (ch == '#') {
            while (pos.* < raw.len && raw[pos.*] != '\n') {
                pos.* += 1;
            }
        } else {
            break;
        }
    }
    if (pos.* >= raw.len || raw[pos.*] < '0' || raw[pos.*] > '9') {
        return ImageOpsError.InvalidFormat;
    }
    let val: usize = 0;
    while (pos.* < raw.len && raw[pos.*] >= '0' && raw[pos.*] <= '9') {
        val = val * 10 + @as(usize, raw[pos.*] - '0');
        pos.* += 1;
    }
    return val;
}

/// Decodes a binary PPM P6 (`maxval == 255`) image into `out_rgba` (`alpha = 255`).
pub fn decodePpmP6(raw: []const u8, out_rgba: *[]u8): ImageViewMeta ! ImageOpsError {
    if (raw.len < 9 || raw[0] != 'P' || raw[1] != '6') {
        return ImageOpsError.InvalidFormat;
    }
    let pos: usize = 2;
    const width: usize = readPpmUint(raw, *pos)?;
    const height: usize = readPpmUint(raw, *pos)?;
    const maxval: usize = readPpmUint(raw, *pos)?;
    if (width == 0 || height == 0 || maxval != 255) {
        return ImageOpsError.InvalidFormat;
    }
    if (pos >= raw.len || !isPpmSpace(raw[pos])) {
        return ImageOpsError.InvalidFormat;
    }
    pos += 1; // Consume single whitespace separator after maxval

    const npix: usize = width * height;
    if (pos + npix * 3 > raw.len) {
        return ImageOpsError.InvalidFormat;
    }
    if (out_rgba.len < npix * 4) {
        return ImageOpsError.BufferTooSmall;
    }

    let i: usize = 0;
    while (i < npix) : (i += 1) {
        const s_off: usize = pos + i * 3;
        const d_off: usize = i * 4;
        out_rgba[d_off + 0] = raw[s_off + 0];
        out_rgba[d_off + 1] = raw[s_off + 1];
        out_rgba[d_off + 2] = raw[s_off + 2];
        out_rgba[d_off + 3] = 255;
    }

    return ImageViewMeta{
        width: width,
        height: height,
        channels: 4,
        stride_bytes: width * 4,
    };
}

/// Encodes an RGBA frame as a lossless 32-bit uncompressed BGRA BMP (bottom-up DIB) into `out_buf`.
pub fn encodeBmpRgba(
    rgba: []const u8,
    width: usize,
    height: usize,
    out_buf: *[]u8,
): usize ! ImageOpsError {
    if (width == 0 || height == 0) return ImageOpsError.InvalidDimensions;
    const npix: usize = width * height;
    const pixel_bytes: usize = npix * 4;
    const total_bytes: usize = pixel_bytes + 54;
    if (rgba.len < pixel_bytes || out_buf.len < total_bytes) {
        return ImageOpsError.BufferTooSmall;
    }

    // Zero 54-byte BITMAPFILEHEADER (14) + BITMAPINFOHEADER (40)
    let z: usize = 0;
    while (z < 54) : (z += 1) {
        out_buf[z] = 0;
    }

    out_buf[0] = 'B';
    out_buf[1] = 'M';
    writeU32Le(out_buf, 2, @as(u32, @intCast(total_bytes)));
    writeU32Le(out_buf, 10, 54); // pixel data offset

    writeU32Le(out_buf, 14, 40); // BITMAPINFOHEADER size
    writeU32Le(out_buf, 18, @as(u32, @intCast(width)));
    writeU32Le(out_buf, 22, @as(u32, @intCast(height))); // positive => bottom-up
    writeU16Le(out_buf, 26, 1);  // planes
    writeU16Le(out_buf, 28, 32); // 32-bit BGRA
    writeU32Le(out_buf, 30, 0);  // BI_RGB (uncompressed)
    writeU32Le(out_buf, 34, @as(u32, @intCast(pixel_bytes)));

    let y: usize = 0;
    while (y < height) : (y += 1) {
        const src_y: usize = height - 1 - y;
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const s_off: usize = (src_y * width + x) * 4;
            const d_off: usize = (y * width + x) * 4 + 54;
            out_buf[d_off + 0] = rgba[s_off + 2]; // B
            out_buf[d_off + 1] = rgba[s_off + 1]; // G
            out_buf[d_off + 2] = rgba[s_off + 0]; // R
            out_buf[d_off + 3] = rgba[s_off + 3]; // A
        }
    }

    return total_bytes;
}

/// Decodes a 24-bit BGR or 32-bit BGRA uncompressed BMP (supporting both bottom-up and top-down
/// heights and 4-byte row padding) into top-down RGBA in `out_rgba`.
pub fn decodeBmpToRgba(raw: []const u8, out_rgba: *[]u8): ImageViewMeta ! ImageOpsError {
    if (raw.len < 54 || raw[0] != 'B' || raw[1] != 'M') {
        return ImageOpsError.InvalidFormat;
    }

    const data_offset: usize = @as(usize, readU32Le(raw, 10));
    const dib_size: u32 = readU32Le(raw, 14);
    if (dib_size < 40 || data_offset < 54 || data_offset > raw.len) {
        return ImageOpsError.InvalidFormat;
    }

    const raw_w: i32 = readI32Le(raw, 18);
    const raw_h: i32 = readI32Le(raw, 22);
    if (raw_w <= 0 || raw_h == 0) {
        return ImageOpsError.InvalidDimensions;
    }
    const width: usize = @as(usize, @intCast(raw_w));
    const top_down: bool = raw_h < 0;
    const height: usize = @as(usize, @intCast(if (top_down) -raw_h else raw_h));

    const bpp: u16 = readU16Le(raw, 28);
    const compression: u32 = readU32Le(raw, 30);
    if (compression != 0) {
        return ImageOpsError.InvalidFormat;
    }
    if (bpp != 24 && bpp != 32) {
        return ImageOpsError.UnsupportedBitsPerPixel;
    }

    const bytes_per_px: usize = @as(usize, bpp / 8);
    const row_stride: usize = ((width * bytes_per_px) + 3) & (~@as(usize, 3));
    if (data_offset + row_stride * height > raw.len) {
        return ImageOpsError.InvalidFormat;
    }
    if (out_rgba.len < width * height * 4) {
        return ImageOpsError.BufferTooSmall;
    }

    let y: usize = 0;
    while (y < height) : (y += 1) {
        const src_y: usize = if (top_down) y else height - 1 - y;
        const row_off: usize = data_offset + src_y * row_stride;
        let x: usize = 0;
        while (x < width) : (x += 1) {
            const s_off: usize = row_off + x * bytes_per_px;
            const d_off: usize = (y * width + x) * 4;
            out_rgba[d_off + 0] = raw[s_off + 2]; // R
            out_rgba[d_off + 1] = raw[s_off + 1]; // G
            out_rgba[d_off + 2] = raw[s_off + 0]; // B
            out_rgba[d_off + 3] = if (bpp == 32) raw[s_off + 3] else 255;
        }
    }

    return ImageViewMeta{
        width: width,
        height: height,
        channels: 4,
        stride_bytes: width * 4,
    };
}

// ============================================================================
// 7. Objective Image Quality Metric (PSNR)
// ============================================================================

/// Computes Peak Signal-to-Noise Ratio (PSNR in dB) across RGB channels of two RGBA frames.
/// Returns `100.0` for identical (zero-MSE) images.
pub fn computePsnrRgba(
    a: []const u8,
    b: []const u8,
    width: usize,
    height: usize,
): f64 {
    const npix: usize = width * height;
    if (npix == 0 || a.len < npix * 4 || b.len < npix * 4) {
        return 0.0;
    }
    let mse_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < npix) : (i += 1) {
        const off: usize = i * 4;
        const dr: f64 = @as(f64, @floatFromInt(a[off + 0])) - @as(f64, @floatFromInt(b[off + 0]));
        const dg: f64 = @as(f64, @floatFromInt(a[off + 1])) - @as(f64, @floatFromInt(b[off + 1]));
        const db: f64 = @as(f64, @floatFromInt(a[off + 2])) - @as(f64, @floatFromInt(b[off + 2]));
        mse_sum += dr * dr + dg * dg + db * db;
    }
    const mse: f64 = mse_sum / (@as(f64, @floatFromInt(npix)) * 3.0);
    if (mse <= 1.0e-12) {
        return 100.0;
    }
    return 10.0 * std.math.log10((255.0 * 255.0) / mse);
}

// Reference-hardened against aldanor/rapid-qoi (MIT/Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc QOI v1.0 Lossless Image Codec) | Created: 2026-11-22

const std = @import("std");

pub const QOI_MAX_PIXELS: u64 = 64000000;

pub error QoiError {
    InvalidHeader,
    InvalidChannels,
    InvalidColorspace,
    ImageTooLarge,
    BufferTooSmall,
    TruncatedData,
    MissingEndMarker,
    InvalidChunkStream,
    OutOfMemory,
}

/// QOI 14 字节大端文件头（魔数 `"qoif"`）
pub struct QoiHeader {
    width: u32,
    height: u32,
    channels: u8,
    colorspace: u8,
}

/// 16-lane SIMD `@Vector(16, u8)` RGBA run fill (4 pixels per vector store).
pub fn qoiFillRunRgbaSimd(dst: []u8, count_pixels: usize, r: u8, g: u8, b: u8, a: u8) {
    const px4: @Vector(16, u8) = [ r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a ];
    let px_idx: usize = 0;
    let byte_off: usize = 0;
    while (px_idx + 4 <= count_pixels && byte_off + 16 <= dst.len) {
        dst[byte_off..][0..16].* = px4;
        px_idx += 4;
        byte_off += 16;
    }
    while (px_idx < count_pixels && byte_off + 4 <= dst.len) {
        dst[byte_off] = r;
        dst[byte_off + 1] = g;
        dst[byte_off + 2] = b;
        dst[byte_off + 3] = a;
        px_idx += 1;
        byte_off += 4;
    }
}

/// Scalar reference RGBA run fill for 0-diff parity testing against `qoiFillRunRgbaSimd`.
pub fn qoiFillRunRgbaScalar(dst: []u8, count_pixels: usize, r: u8, g: u8, b: u8, a: u8) {
    let px_idx: usize = 0;
    let byte_off: usize = 0;
    while (px_idx < count_pixels && byte_off + 4 <= dst.len) {
        dst[byte_off] = r;
        dst[byte_off + 1] = g;
        dst[byte_off + 2] = b;
        dst[byte_off + 3] = a;
        px_idx += 1;
        byte_off += 4;
    }
}

/// 16-lane SIMD `@Vector(16, u8)` RGB24 -> RGBA32 (`A=255`) conversion (4 pixels per step).
pub fn qoiRgbToRgbaSimd(dst_rgba: []u8, src_rgb: []const u8) {
    let si: usize = 0;
    let di: usize = 0;
    while (si + 12 <= src_rgb.len && di + 16 <= dst_rgba.len) {
        const v: @Vector(16, u8) = [
            src_rgb[si], src_rgb[si + 1], src_rgb[si + 2], 255,
            src_rgb[si + 3], src_rgb[si + 4], src_rgb[si + 5], 255,
            src_rgb[si + 6], src_rgb[si + 7], src_rgb[si + 8], 255,
            src_rgb[si + 9], src_rgb[si + 10], src_rgb[si + 11], 255,
        ];
        dst_rgba[di..][0..16].* = v;
        si += 12;
        di += 16;
    }
    while (si + 3 <= src_rgb.len && di + 4 <= dst_rgba.len) {
        dst_rgba[di] = src_rgb[si];
        dst_rgba[di + 1] = src_rgb[si + 1];
        dst_rgba[di + 2] = src_rgb[si + 2];
        dst_rgba[di + 3] = 255;
        si += 3;
        di += 4;
    }
}

/// Scalar reference RGB24 -> RGBA32 conversion for 0-diff parity testing.
pub fn qoiRgbToRgbaScalar(dst_rgba: []u8, src_rgb: []const u8) {
    let si: usize = 0;
    let di: usize = 0;
    while (si + 3 <= src_rgb.len && di + 4 <= dst_rgba.len) {
        dst_rgba[di] = src_rgb[si];
        dst_rgba[di + 1] = src_rgb[si + 1];
        dst_rgba[di + 2] = src_rgb[si + 2];
        dst_rgba[di + 3] = 255;
        si += 3;
        di += 4;
    }
}

/// 16-lane SIMD `@Vector(16, u8)` pixel buffer equality check.
pub fn qoiPixelsEqualSimd(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: @Vector(16, u8) = a[i..][0..16].*;
        const vb: @Vector(16, u8) = b[i..][0..16].*;
        if (!@reduce(.And, va == vb)) return false;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference pixel buffer equality check for 0-diff parity testing.
pub fn qoiPixelsEqualScalar(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Computes worst-case encoded QOI buffer size in bytes for `(width, height, channels)`.
pub fn qoiMaxEncodedSize(width: u32, height: u32, channels: u8): usize? {
    if (width == 0 || height == 0) return null;
    if (channels != 3 && channels != 4) return null;
    const px = (width as u64) * (height as u64);
    if (px > QOI_MAX_PIXELS) return null;
    return QOI_HEADER_SIZE + (px as usize) * ((channels as usize) + 1) + QOI_PADDING_SIZE;
}

struct Rgba {
    r: u8 = 0,
    g: u8 = 0,
    b: u8 = 0,
    a: u8 = 0,
}

const QOI_HEADER_SIZE: usize = 14;
const QOI_PADDING_SIZE: usize = 8;

const QOI_OP_INDEX: u8 = 0x00;
const QOI_OP_DIFF: u8 = 0x40;
const QOI_OP_LUMA: u8 = 0x80;
const QOI_OP_RUN: u8 = 0xC0;
const QOI_OP_RGB: u8 = 0xFE;
const QOI_OP_RGBA: u8 = 0xFF;
const QOI_MASK_2: u8 = 0xC0;

fn readU32Be(data: []const u8, pos: usize): u32 {
    return (u32(data[pos]) << 24)
        | (u32(data[pos + 1]) << 16)
        | (u32(data[pos + 2]) << 8)
        | u32(data[pos + 3]);
}

fn writeU32Be(out: []u8, pos: usize, v: u32) {
    out[pos] = u8((v >> 24) & 0xFF);
    out[pos + 1] = u8((v >> 16) & 0xFF);
    out[pos + 2] = u8((v >> 8) & 0xFF);
    out[pos + 3] = u8(v & 0xFF);
}

fn qoiColorHash(px: Rgba): usize {
    return (usize(px.r) * 3 + usize(px.g) * 5 + usize(px.b) * 7 + usize(px.a) * 11) % 64;
}

fn rgbaEq(a: Rgba, b: Rgba): bool {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

/// 8-bit 环绕有符号差值（范围 `-128..127`，无下溢）
fn wrapDiffI8(cur: u8, prev: u8): i16 {
    const diff_u8 = u8((u16(cur) + 256 - u16(prev)) & 0xFF);
    return if (diff_u8 < 128) i16(diff_u8) else i16(diff_u8) - 256;
}

/// 8-bit 环绕加法（无上溢/下溢）
fn wrapAddU8(base: u8, delta: i16): u8 {
    return u8((i32(base) + i32(delta) + 512) & 0xFF);
}

fn hasQoiEndMarker(data: []const u8): bool {
    if (data.len < QOI_PADDING_SIZE) return false;
    const base = data.len - QOI_PADDING_SIZE;
    return data[base] == 0
        && data[base + 1] == 0
        && data[base + 2] == 0
        && data[base + 3] == 0
        && data[base + 4] == 0
        && data[base + 5] == 0
        && data[base + 6] == 0
        && data[base + 7] == 1;
}

/// 解析 14 字节大端 QOI 文件头，严格校验 `"qoif"` 魔数与字段范围
pub fn parseQoiHeader(data: []const u8): QoiHeader? {
    if (data.len < QOI_HEADER_SIZE) {
        return null;
    }
    if (data[0] != 'q' || data[1] != 'o' || data[2] != 'i' || data[3] != 'f') {
        return null;
    }
    const width = readU32Be(data, 4);
    const height = readU32Be(data, 8);
    const channels = data[12];
    const colorspace = data[13];
    if (width == 0 || height == 0) {
        return null;
    }
    if (channels != 3 && channels != 4) {
        return null;
    }
    if (colorspace > 1) {
        return null;
    }
    return QoiHeader {
        width: width,
        height: height,
        channels: channels,
        colorspace: colorspace,
    };
}

/// 快速校验输入是否包含合法 QOI 文件头
pub fn isQoi(data: []const u8): bool {
    return parseQoiHeader(data) != null;
}

/// 零堆分配 QOI 无损编码器：将 RGB/RGBA 原始像素编码到 `out` 并返回实际写入切片；参数非法或缓冲不足返回 `null`
pub fn encodeQoi(
    pixels: []const u8,
    width: u32,
    height: u32,
    channels: u8,
    colorspace: u8,
    out: []u8,
): ([]const u8)? {
    if (width == 0 || height == 0) return null;
    if (channels != 3 && channels != 4) return null;
    if (colorspace > 1) return null;

    const pixel_count = u64(width) * u64(height);
    const expected_bytes = pixel_count * u64(channels);
    if (expected_bytes != u64(pixels.len)) {
        return null;
    }
    if (out.len < QOI_HEADER_SIZE + QOI_PADDING_SIZE) {
        return null;
    }

    // 写入 14 字节大端头
    out[0] = 'q';
    out[1] = 'o';
    out[2] = 'i';
    out[3] = 'f';
    writeU32Be(out, 4, width);
    writeU32Be(out, 8, height);
    out[12] = channels;
    out[13] = colorspace;

    let p: usize = QOI_HEADER_SIZE;
    let index: [64]Rgba = undefined;
    let k: usize = 0;
    while (k < 64) {
        index[k] = Rgba { r: 0, g: 0, b: 0, a: 0 };
        k += 1;
    }

    let prev = Rgba { r: 0, g: 0, b: 0, a: 255 };
    let run: u8 = 0;
    const ch = usize(channels);
    const last_px_offset = pixels.len - ch;
    let px_pos: usize = 0;

    while (px_pos < pixels.len) {
        const px = Rgba {
            r: pixels[px_pos],
            g: pixels[px_pos + 1],
            b: pixels[px_pos + 2],
            a: if (channels == 4) pixels[px_pos + 3] else prev.a,
        };

        if (rgbaEq(px, prev)) {
            run += 1;
            if (run == 62 || px_pos == last_px_offset) {
                if (p + 1 > out.len - QOI_PADDING_SIZE) return null;
                out[p] = QOI_OP_RUN | (run - 1);
                p += 1;
                run = 0;
            }
        } else {
            if (run > 0) {
                if (p + 1 > out.len - QOI_PADDING_SIZE) return null;
                out[p] = QOI_OP_RUN | (run - 1);
                p += 1;
                run = 0;
            }

            const idx = qoiColorHash(px);
            if (rgbaEq(index[idx], px)) {
                if (p + 1 > out.len - QOI_PADDING_SIZE) return null;
                out[p] = QOI_OP_INDEX | u8(idx);
                p += 1;
            } else {
                index[idx] = px;

                if (px.a == prev.a) {
                    const vr = wrapDiffI8(px.r, prev.r);
                    const vg = wrapDiffI8(px.g, prev.g);
                    const vb = wrapDiffI8(px.b, prev.b);

                    const vg_r = vr - vg;
                    const vg_b = vb - vg;

                    if (vr >= -2 && vr <= 1 && vg >= -2 && vg <= 1 && vb >= -2 && vb <= 1) {
                        if (p + 1 > out.len - QOI_PADDING_SIZE) return null;
                        out[p] = QOI_OP_DIFF
                            | (u8(vr + 2) << 4)
                            | (u8(vg + 2) << 2)
                            | u8(vb + 2);
                        p += 1;
                    } else if (vg >= -32 && vg <= 31
                        && vg_r >= -8 && vg_r <= 7
                        && vg_b >= -8 && vg_b <= 7) {
                        if (p + 2 > out.len - QOI_PADDING_SIZE) return null;
                        out[p] = QOI_OP_LUMA | u8(vg + 32);
                        out[p + 1] = (u8(vg_r + 8) << 4) | u8(vg_b + 8);
                        p += 2;
                    } else {
                        if (p + 4 > out.len - QOI_PADDING_SIZE) return null;
                        out[p] = QOI_OP_RGB;
                        out[p + 1] = px.r;
                        out[p + 2] = px.g;
                        out[p + 3] = px.b;
                        p += 4;
                    }
                } else {
                    if (p + 5 > out.len - QOI_PADDING_SIZE) return null;
                    out[p] = QOI_OP_RGBA;
                    out[p + 1] = px.r;
                    out[p + 2] = px.g;
                    out[p + 3] = px.b;
                    out[p + 4] = px.a;
                    p += 5;
                }
            }
        }

        prev = px;
        px_pos += ch;
    }

    // 写入 8 字节尾标记 `0x00 * 7, 0x01`
    if (p + QOI_PADDING_SIZE > out.len) {
        return null;
    }
    let pad: usize = 0;
    while (pad < 7) {
        out[p + pad] = 0;
        pad += 1;
    }
    out[p + 7] = 1;
    p += QOI_PADDING_SIZE;

    return out[0..p];
}

/// 零堆分配 QOI 无损解码器：解码至 `out_pixels` 并返回 `QoiHeader`；校验尾标记与缓冲越界
pub fn decodeQoi(data: []const u8, out_pixels: []u8): QoiHeader? {
    if (data.len < QOI_HEADER_SIZE + QOI_PADDING_SIZE) {
        return null;
    }
    let hdr = QoiHeader { width: 0, height: 0, channels: 0, colorspace: 0 };
    if (h := parseQoiHeader(data)) {
        hdr = h;
    } else {
        return null;
    }
    if (!hasQoiEndMarker(data)) {
        return null;
    }

    const pixel_count = u64(hdr.width) * u64(hdr.height);
    const total_bytes_u64 = pixel_count * u64(hdr.channels);
    if (total_bytes_u64 > u64(out_pixels.len)) {
        return null;
    }
    const total_bytes = usize(total_bytes_u64);
    const chunks_end = data.len - QOI_PADDING_SIZE;

    let index: [64]Rgba = undefined;
    let k: usize = 0;
    while (k < 64) {
        index[k] = Rgba { r: 0, g: 0, b: 0, a: 0 };
        k += 1;
    }

    let px = Rgba { r: 0, g: 0, b: 0, a: 255 };
    let run: u8 = 0;
    let p: usize = QOI_HEADER_SIZE;
    let out_pos: usize = 0;
    const ch = usize(hdr.channels);

    while (out_pos < total_bytes) {
        if (run > 0) {
            run -= 1;
        } else {
            if (p >= chunks_end) {
                return null;
            }
            const b1 = data[p];
            p += 1;

            if (b1 == QOI_OP_RGB) {
                if (p + 3 > chunks_end) return null;
                px.r = data[p];
                px.g = data[p + 1];
                px.b = data[p + 2];
                p += 3;
            } else if (b1 == QOI_OP_RGBA) {
                if (p + 4 > chunks_end) return null;
                px.r = data[p];
                px.g = data[p + 1];
                px.b = data[p + 2];
                px.a = data[p + 3];
                p += 4;
            } else if ((b1 & QOI_MASK_2) == QOI_OP_INDEX) {
                px = index[usize(b1 & 0x3F)];
            } else if ((b1 & QOI_MASK_2) == QOI_OP_DIFF) {
                const dr = i16((b1 >> 4) & 0x03) - 2;
                const dg = i16((b1 >> 2) & 0x03) - 2;
                const db = i16(b1 & 0x03) - 2;
                px.r = wrapAddU8(px.r, dr);
                px.g = wrapAddU8(px.g, dg);
                px.b = wrapAddU8(px.b, db);
            } else if ((b1 & QOI_MASK_2) == QOI_OP_LUMA) {
                if (p >= chunks_end) return null;
                const b2 = data[p];
                p += 1;
                const vg = i16(b1 & 0x3F) - 32;
                const vr = vg + (i16((b2 >> 4) & 0x0F) - 8);
                const vb = vg + (i16(b2 & 0x0F) - 8);
                px.r = wrapAddU8(px.r, vr);
                px.g = wrapAddU8(px.g, vg);
                px.b = wrapAddU8(px.b, vb);
            } else if ((b1 & QOI_MASK_2) == QOI_OP_RUN) {
                run = b1 & 0x3F;
            }

            if ((b1 & QOI_MASK_2) != QOI_OP_RUN || b1 == QOI_OP_RGB || b1 == QOI_OP_RGBA) {
                index[qoiColorHash(px)] = px;
            }
        }

        if (hdr.channels == 4 && run > 0) {
            const total_run = (run as usize) + 1;
            const avail_px = (total_bytes - out_pos) / 4;
            const fill_px = if (total_run > avail_px) avail_px else total_run;
            qoiFillRunRgbaSimd(out_pixels[out_pos..total_bytes], fill_px, px.r, px.g, px.b, px.a);
            out_pos += fill_px * 4;
            run -= (fill_px - 1) as u8;
            if (fill_px < total_run) {
                return null;
            }
            continue;
        }

        out_pixels[out_pos] = px.r;
        out_pixels[out_pos + 1] = px.g;
        out_pixels[out_pos + 2] = px.b;
        if (hdr.channels == 4) {
            out_pixels[out_pos + 3] = px.a;
        }
        out_pos += ch;
    }

    if (run > 0) {
        return null;
    }
    return hdr;
}

/// Strict error-returning QOI header parser (`QoiError`).
pub fn parseQoiHeaderStrict(data: []const u8): QoiHeader!QoiError {
    if (data.len < QOI_HEADER_SIZE) return QoiError.TruncatedData;
    if (data[0] != 'q' || data[1] != 'o' || data[2] != 'i' || data[3] != 'f') {
        return QoiError.InvalidHeader;
    }
    const width = readU32Be(data, 4);
    const height = readU32Be(data, 8);
    const channels = data[12];
    const colorspace = data[13];
    if (width == 0 || height == 0) return QoiError.InvalidHeader;
    if (channels != 3 && channels != 4) return QoiError.InvalidChannels;
    if (colorspace > 1) return QoiError.InvalidColorspace;
    const px = (width as u64) * (height as u64);
    if (px > QOI_MAX_PIXELS) return QoiError.ImageTooLarge;
    return QoiHeader {
        width: width,
        height: height,
        channels: channels,
        colorspace: colorspace,
    };
}

/// Strict error-returning QOI encoder (`QoiError`).
pub fn encodeQoiStrict(
    pixels: []const u8,
    width: u32,
    height: u32,
    channels: u8,
    colorspace: u8,
    out: []u8,
): ([]const u8)!QoiError {
    if (width == 0 || height == 0) return QoiError.InvalidHeader;
    if (channels != 3 && channels != 4) return QoiError.InvalidChannels;
    if (colorspace > 1) return QoiError.InvalidColorspace;
    const px = (width as u64) * (height as u64);
    if (px > QOI_MAX_PIXELS) return QoiError.ImageTooLarge;
    if (px * (channels as u64) != (pixels.len as u64)) return QoiError.TruncatedData;
    return encodeQoi(pixels, width, height, channels, colorspace, out) orelse return QoiError.BufferTooSmall;
}

/// Strict error-returning QOI decoder (`QoiError`), enforcing 8-byte end marker & `QOI_MAX_PIXELS`.
pub fn decodeQoiStrict(data: []const u8, out_pixels: []u8): QoiHeader!QoiError {
    if (data.len < QOI_HEADER_SIZE + QOI_PADDING_SIZE) return QoiError.TruncatedData;
    const hdr = try parseQoiHeaderStrict(data);
    if (!hasQoiEndMarker(data)) return QoiError.MissingEndMarker;
    const total_bytes = (hdr.width as u64) * (hdr.height as u64) * (hdr.channels as u64);
    if (total_bytes > (out_pixels.len as u64)) return QoiError.BufferTooSmall;
    return decodeQoi(data, out_pixels) orelse return QoiError.InvalidChunkStream;
}

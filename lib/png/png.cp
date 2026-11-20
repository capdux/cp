// Source: cp/lib/png/png.cp (Pure CP PNG Image Codec via cap:deflate)
// Zero external C dependency implementation using pure CP, std, and cap:deflate.

const std = @import("std");
import { compressZlib, decompressZlib, crc32 } from "cap:deflate";

const PNG_SIGNATURE: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
const DEFAULT_MAX_DIMENSION: u32 = 65535;

/// Error set for PNG encoding and decoding.
pub err PngError {
    InvalidSignature,
    InvalidHeader,
    ChecksumMismatch,
    UnsupportedFormat,
    InvalidFilterType,
    TruncatedData,
    MissingIend,
    ImageTooLarge,
    OutOfMemory,
}

/// Standard PNG color types (RFC 2083).
pub enum ColorType: u8 {
    Grayscale = 0,
    Rgb = 2,
    Indexed = 3,
    GrayscaleAlpha = 4,
    Rgba = 6,
}

/// Decoded 8-bit PNG image container.
pub struct PngImage {
    width: u32,
    height: u32,
    bit_depth: u8,
    color_type: ColorType,
    pixels: []u8,

    /// Returns the number of bytes per pixel for this image's color type.
    pub fn bytesPerPixel(): usize {
        return colorTypeBpp(_.color_type);
    }

    /// Frees the pixel buffer owned by this image.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        allocator.free(_.pixels);
    }
}

fn colorTypeBpp(ct: ColorType): usize {
    return switch (ct) {
        ColorType.Grayscale => 1,
        ColorType.Rgb => 3,
        ColorType.Indexed => 1,
        ColorType.GrayscaleAlpha => 2,
        ColorType.Rgba => 4,
    };
}

fn parseColorType(raw: u8): PngError!ColorType {
    return switch (raw) {
        0 => ColorType.Grayscale,
        2 => ColorType.Rgb,
        3 => ColorType.Indexed,
        4 => ColorType.GrayscaleAlpha,
        6 => ColorType.Rgba,
        else => PngError.UnsupportedFormat,
    };
}

fn readU32BE(buf: []const u8, pos: usize): u32 {
    const b0 = buf[pos] as u32;
    const b1 = buf[pos + 1] as u32;
    const b2 = buf[pos + 2] as u32;
    const b3 = buf[pos + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn appendU32BE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): !PngError {
    const bytes: [4]u8 = [
        ((v >> 24) & 0xff) as u8,
        ((v >> 16) & 0xff) as u8,
        ((v >> 8) & 0xff) as u8,
        (v & 0xff) as u8,
    ];
    out.appendSlice(alloc, bytes[0..4]) catch return PngError.OutOfMemory;
}

fn appendChunk(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    chunk_type: [4]u8,
    payload: []const u8,
): !PngError {
    appendU32BE(alloc, out, payload.len as u32)?;
    const crc_start = out.items.len;
    out.appendSlice(alloc, chunk_type[0..4]) catch return PngError.OutOfMemory;
    if (payload.len > 0) {
        out.appendSlice(alloc, payload) catch return PngError.OutOfMemory;
    }
    const checksum = crc32(out.items[crc_start..out.items.len]);
    appendU32BE(alloc, out, checksum)?;
}

fn paethPredictor(a: u8, b: u8, c: u8): u8 {
    const ia = a as i32;
    const ib = b as i32;
    const ic = c as i32;
    const p = ia + ib - ic;
    const pa = if (p >= ia) (p - ia) else (ia - p);
    const pb = if (p >= ib) (p - ib) else (ib - p);
    const pc = if (p >= ic) (p - ic) else (ic - p);
    if (pa <= pb && pa <= pc) {
        return a;
    }
    if (pb <= pc) {
        return b;
    }
    return c;
}

fn filterByteCost(b: u8): usize {
    return if (b <= 128) (b as usize) else ((256 - (b as usize)));
}

fn computeFilteredByte(
    filter_type: u8,
    cur: u8,
    a: u8,
    b: u8,
    c: u8,
): u8 {
    return switch (filter_type) {
        0 => cur,
        1 => cur -% a,
        2 => cur -% b,
        3 => cur -% ((((a as u16) + (b as u16)) >> 1) as u8),
        4 => cur -% paethPredictor(a, b, c),
        else => cur,
    };
}

fn selectBestFilter(
    pixels: []const u8,
    y: usize,
    stride: usize,
    bpp: usize,
): u8 {
    const row_off = y * stride;
    let best_filter: u8 = 0;
    let best_sad: usize = std.math.maxInt(usize);

    let f: u8 = 0;
    while (f <= 4) : (f += 1) {
        let sad: usize = 0;
        let x: usize = 0;
        while (x < stride) : (x += 1) {
            const cur = pixels[row_off + x];
            const a: u8 = if (x >= bpp) pixels[row_off + x - bpp] else 0;
            const b: u8 = if (y > 0) pixels[row_off - stride + x] else 0;
            const c: u8 = if (y > 0 && x >= bpp) pixels[row_off - stride + x - bpp] else 0;
            const fb = computeFilteredByte(f, cur, a, b, c);
            sad += filterByteCost(fb);
            if (sad >= best_sad) {
                break;
            }
        }
        if (sad < best_sad) {
            best_sad = sad;
            best_filter = f;
        }
    }
    return best_filter;
}

/// Encodes 8-bit raw pixels (`Grayscale`, `Rgb`, `GrayscaleAlpha`, `Rgba`) into a PNG byte stream.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn encode(
    allocator: std.mem.Allocator,
    width: u32,
    height: u32,
    color_type: ColorType,
    pixels: []const u8,
): ([]u8) ! PngError {
    if (width == 0 || height == 0) {
        return PngError.InvalidHeader;
    }
    if (color_type == ColorType.Indexed) {
        return PngError.UnsupportedFormat;
    }

    const bpp = colorTypeBpp(color_type);
    const expected_len: u64 = (width as u64) * (height as u64) * (bpp as u64);
    if (expected_len != (pixels.len as u64)) {
        return PngError.InvalidHeader;
    }

    const stride = (width as usize) * bpp;
    const h = height as usize;
    const raw_len = h * (1 + stride);
    let raw = allocator.alloc(u8, raw_len) catch return PngError.OutOfMemory;
    defer allocator.free(raw);

    let y: usize = 0;
    while (y < h) : (y += 1) {
        const best_f = selectBestFilter(pixels, y, stride, bpp);
        const dst_off = y * (1 + stride);
        const src_off = y * stride;
        raw[dst_off] = best_f;

        let x: usize = 0;
        while (x < stride) : (x += 1) {
            const cur = pixels[src_off + x];
            const a: u8 = if (x >= bpp) pixels[src_off + x - bpp] else 0;
            const b: u8 = if (y > 0) pixels[src_off - stride + x] else 0;
            const c: u8 = if (y > 0 && x >= bpp) pixels[src_off - stride + x - bpp] else 0;
            raw[dst_off + 1 + x] = computeFilteredByte(best_f, cur, a, b, c);
        }
    }

    const zlib_data = compressZlib(allocator, raw) catch |e| {
        return if (e == error.OutOfMemory) PngError.OutOfMemory else PngError.InvalidHeader;
    };
    defer allocator.free(zlib_data);

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    out.appendSlice(allocator, PNG_SIGNATURE[0..8]) catch return PngError.OutOfMemory;

    const ihdr: [13]u8 = [
        ((width >> 24) & 0xff) as u8,
        ((width >> 16) & 0xff) as u8,
        ((width >> 8) & 0xff) as u8,
        (width & 0xff) as u8,
        ((height >> 24) & 0xff) as u8,
        ((height >> 16) & 0xff) as u8,
        ((height >> 8) & 0xff) as u8,
        (height & 0xff) as u8,
        8,
        @intFromEnum(color_type),
        0,
        0,
        0,
    ];
    appendChunk(allocator, &out, ['I', 'H', 'D', 'R'], ihdr[0..13])?;
    appendChunk(allocator, &out, ['I', 'D', 'A', 'T'], zlib_data)?;
    const empty_iend: [0]u8 = [];
    appendChunk(allocator, &out, ['I', 'E', 'N', 'D'], empty_iend[0..0])?;

    return out.toOwnedSlice(allocator) catch return PngError.OutOfMemory;
}

/// Decodes a PNG byte stream into an 8-bit `PngImage`.
/// Caller owns the returned `PngImage.pixels` and must call `img.deinit(allocator)`.
pub fn decode(allocator: std.mem.Allocator, png_bytes: []const u8): PngImage ! PngError {
    return decodeWithMaxDimensions(allocator, png_bytes, DEFAULT_MAX_DIMENSION, DEFAULT_MAX_DIMENSION);
}

/// Decodes a PNG byte stream while enforcing upper bounds `max_w` and `max_h` on image dimensions.
/// Caller owns the returned `PngImage.pixels` and must call `img.deinit(allocator)`.
pub fn decodeWithMaxDimensions(
    allocator: std.mem.Allocator,
    png_bytes: []const u8,
    max_w: u32,
    max_h: u32,
): PngImage ! PngError {
    if (png_bytes.len < 8) {
        return PngError.TruncatedData;
    }
    if (!std.mem.eql(u8, png_bytes[0..8], PNG_SIGNATURE[0..8])) {
        return PngError.InvalidSignature;
    }

    let pos: usize = 8;
    let seen_ihdr = false;
    let seen_iend = false;
    let width: u32 = 0;
    let height: u32 = 0;
    let color_type: ColorType = ColorType.Rgba;

    let palette: []const u8 = png_bytes[0..0];
    let has_plte = false;
    let trns: []const u8 = png_bytes[0..0];
    let has_trns = false;

    let idat_buf: std.ArrayList(u8) = .empty;
    defer idat_buf.deinit(allocator);

    while (pos < png_bytes.len) {
        if (pos + 12 > png_bytes.len) {
            return PngError.TruncatedData;
        }
        const chunk_len = readU32BE(png_bytes, pos) as usize;
        if (chunk_len > png_bytes.len - pos - 12) {
            return PngError.TruncatedData;
        }

        const chunk_type = png_bytes[pos + 4 .. pos + 8];
        const payload = png_bytes[pos + 8 .. pos + 8 + chunk_len];
        const expected_crc = readU32BE(png_bytes, pos + 8 + chunk_len);
        const actual_crc = crc32(png_bytes[pos + 4 .. pos + 8 + chunk_len]);
        if (actual_crc != expected_crc) {
            return PngError.ChecksumMismatch;
        }

        if (!seen_ihdr) {
            if (!std.mem.eql(u8, chunk_type, "IHDR") || chunk_len != 13) {
                return PngError.InvalidHeader;
            }
            width = readU32BE(payload, 0);
            height = readU32BE(payload, 4);
            const bit_depth = payload[8];
            const ct_raw = payload[9];
            const comp_method = payload[10];
            const filter_method = payload[11];
            const interlace_method = payload[12];

            if (width == 0 || height == 0) {
                return PngError.InvalidHeader;
            }
            if (width > max_w || height > max_h) {
                return PngError.ImageTooLarge;
            }
            if (bit_depth != 8 || comp_method != 0 || filter_method != 0 || interlace_method != 0) {
                return PngError.UnsupportedFormat;
            }
            color_type = parseColorType(ct_raw)?;
            seen_ihdr = true;
        } else if (std.mem.eql(u8, chunk_type, "PLTE")) {
            if (chunk_len == 0 || (chunk_len % 3) != 0 || chunk_len > 768) {
                return PngError.InvalidHeader;
            }
            palette = payload;
            has_plte = true;
        } else if (std.mem.eql(u8, chunk_type, "tRNS")) {
            trns = payload;
            has_trns = true;
        } else if (std.mem.eql(u8, chunk_type, "IDAT")) {
            idat_buf.appendSlice(allocator, payload) catch return PngError.OutOfMemory;
        } else if (std.mem.eql(u8, chunk_type, "IEND")) {
            seen_iend = true;
            pos += 12 + chunk_len;
            break;
        }

        pos += 12 + chunk_len;
    }

    if (!seen_ihdr) {
        return PngError.InvalidHeader;
    }
    if (!seen_iend) {
        return PngError.MissingIend;
    }
    if (idat_buf.items.len == 0) {
        return PngError.TruncatedData;
    }
    if (color_type == ColorType.Indexed && !has_plte) {
        return PngError.InvalidHeader;
    }

    const raw = decompressZlib(allocator, idat_buf.items) catch |e| {
        return switch (e) {
            error.OutOfMemory => PngError.OutOfMemory,
            error.ChecksumMismatch => PngError.ChecksumMismatch,
            error.UnexpectedEof => PngError.TruncatedData,
            else => PngError.InvalidHeader,
        };
    };
    defer allocator.free(raw);

    const src_bpp = colorTypeBpp(color_type);
    const w = width as usize;
    const h = height as usize;
    const stride = w * src_bpp;
    const expected_raw_len = h * (1 + stride);
    if (raw.len < expected_raw_len) {
        return PngError.TruncatedData;
    }
    if (raw.len != expected_raw_len) {
        return PngError.InvalidHeader;
    }

    let unfiltered = allocator.alloc(u8, h * stride) catch return PngError.OutOfMemory;
    errdefer allocator.free(unfiltered);

    let y: usize = 0;
    while (y < h) : (y += 1) {
        const src_off = y * (1 + stride);
        const dst_off = y * stride;
        const filter_type = raw[src_off];
        if (filter_type > 4) {
            return PngError.InvalidFilterType;
        }

        let x: usize = 0;
        while (x < stride) : (x += 1) {
            const filt = raw[src_off + 1 + x];
            const a: u8 = if (x >= src_bpp) unfiltered[dst_off + x - src_bpp] else 0;
            const b: u8 = if (y > 0) unfiltered[dst_off - stride + x] else 0;
            const c: u8 = if (y > 0 && x >= src_bpp) unfiltered[dst_off - stride + x - src_bpp] else 0;

            const recon: u8 = switch (filter_type) {
                0 => filt,
                1 => filt +% a,
                2 => filt +% b,
                3 => filt +% ((((a as u16) + (b as u16)) >> 1) as u8),
                4 => filt +% paethPredictor(a, b, c),
                else => unreachable,
            };
            unfiltered[dst_off + x] = recon;
        }
    }

    if (color_type == ColorType.Indexed) {
        const out_bpp: usize = if (has_trns) 4 else 3;
        const out_ct = if (has_trns) ColorType.Rgba else ColorType.Rgb;
        const pixel_count = w * h;
        let expanded = allocator.alloc(u8, pixel_count * out_bpp) catch return PngError.OutOfMemory;
        errdefer allocator.free(expanded);

        let i: usize = 0;
        while (i < pixel_count) : (i += 1) {
            const idx = unfiltered[i] as usize;
            const p_off = idx * 3;
            if (p_off + 2 >= palette.len) {
                return PngError.InvalidHeader;
            }
            const o_off = i * out_bpp;
            expanded[o_off] = palette[p_off];
            expanded[o_off + 1] = palette[p_off + 1];
            expanded[o_off + 2] = palette[p_off + 2];
            if (has_trns) {
                expanded[o_off + 3] = if (idx < trns.len) trns[idx] else 255;
            }
        }
        allocator.free(unfiltered);
        return PngImage{
            width: width,
            height: height,
            bit_depth: 8,
            color_type: out_ct,
            pixels: expanded,
        };
    }

    return PngImage{
        width: width,
        height: height,
        bit_depth: 8,
        color_type: color_type,
        pixels: unfiltered,
    };
}

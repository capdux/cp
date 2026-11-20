// Source: cp/lib/bmp/bmp.cp (Pure CP BMP Bitmap Image Codec: 24/32-bit BGR/BGRA & 8-bit Palette)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");

const BMP_FILE_HEADER_SIZE: usize = 14;
const BMP_INFO_HEADER_SIZE: usize = 40;
const BMP_HEADERS_SIZE: usize = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
const DEFAULT_MAX_DIMENSION: u32 = 65535;

const BI_RGB: u32 = 0;
const BI_BITFIELDS: u32 = 3;

/// Error set for BMP encoding and decoding.
pub err BmpError {
    InvalidSignature,
    InvalidHeader,
    UnsupportedBitDepth,
    UnsupportedCompression,
    TruncatedData,
    ImageTooLarge,
    InvalidDimensions,
    OutOfMemory,
}

/// Output pixel format of a decoded `BmpImage` (always top-down RGB/RGBA).
pub enum BmpPixelFormat: u8 {
    Rgb24 = 3,
    Rgba32 = 4,
}

/// Decoded BMP image container stored in unified top-down RGB/RGBA byte order.
pub struct BmpImage {
    width: u32,
    height: u32,
    format: BmpPixelFormat,
    pixels: []u8,

    /// Returns the number of bytes per pixel for this image's pixel format.
    pub fn bytesPerPixel(): usize {
        return switch (_.format) {
            BmpPixelFormat.Rgb24 => 3,
            BmpPixelFormat.Rgba32 => 4,
        };
    }

    /// Frees the pixel buffer owned by this image.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        allocator.free(_.pixels);
    }
}

fn readU16LE(buf: []const u8, pos: usize): u16 {
    const b0 = buf[pos] as u16;
    const b1 = buf[pos + 1] as u16;
    return b0 | (b1 << 8);
}

fn readU32LE(buf: []const u8, pos: usize): u32 {
    const b0 = buf[pos] as u32;
    const b1 = buf[pos + 1] as u32;
    const b2 = buf[pos + 2] as u32;
    const b3 = buf[pos + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readI32LE(buf: []const u8, pos: usize): i32 {
    const u = readU32LE(buf, pos);
    return @bitCast(u);
}

fn writeU16LE(buf: []u8, pos: usize, val: u16) {
    buf[pos] = (val & 0xFF) as u8;
    buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, pos: usize, val: u32) {
    buf[pos] = (val & 0xFF) as u8;
    buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
    buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
    buf[pos + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeI32LE(buf: []u8, pos: usize, val: i32) {
    const u: u32 = @bitCast(val);
    writeU32LE(buf, pos, u);
}

fn encodeInternal(
    allocator: std.mem.Allocator,
    width: u32,
    height: u32,
    bpp: u16,
    pixels: []const u8,
): ([]u8) ! BmpError {
    if (width == 0 || height == 0 || width > 0x7FFF_FFFF || height > 0x7FFF_FFFF) {
        return BmpError.InvalidDimensions;
    }

    const src_bpp: u64 = (bpp / 8) as u64;
    const w64 = width as u64;
    const h64 = height as u64;
    const expected_input_len = w64 * h64 * src_bpp;
    if ((pixels.len as u64) != expected_input_len) {
        return BmpError.InvalidDimensions;
    }

    const row_bytes_unpadded = w64 * src_bpp;
    const dst_row_stride64 = ((row_bytes_unpadded + 3) / 4) * 4;
    const pixel_data_size64 = dst_row_stride64 * h64;
    const total_size64 = (BMP_HEADERS_SIZE as u64) + pixel_data_size64;
    if (total_size64 > (std.math.maxInt(u32) as u64)) {
        return BmpError.ImageTooLarge;
    }

    const total_size = total_size64 as usize;
    let out = allocator.alloc(u8, total_size) catch return BmpError.OutOfMemory;
    errdefer allocator.free(out);

    // Zero-initialize headers and row padding bytes.
    @memset(out, 0);

    // BITMAPFILEHEADER (14 bytes)
    out[0] = 'B';
    out[1] = 'M';
    writeU32LE(out, 2, total_size as u32);
    writeU32LE(out, 10, BMP_HEADERS_SIZE as u32);

    // BITMAPINFOHEADER (40 bytes)
    writeU32LE(out, 14, BMP_INFO_HEADER_SIZE as u32);
    writeI32LE(out, 18, width as i32);
    writeI32LE(out, 22, height as i32);
    writeU16LE(out, 26, 1);
    writeU16LE(out, 28, bpp);
    writeU32LE(out, 30, BI_RGB);
    writeU32LE(out, 34, pixel_data_size64 as u32);
    writeI32LE(out, 38, 2835);
    writeI32LE(out, 42, 2835);

    const w = width as usize;
    const h = height as usize;
    const src_stride = w * (src_bpp as usize);
    const dst_stride = dst_row_stride64 as usize;

    let y: usize = 0;
    while (y < h) : (y += 1) {
        const src_row_off = (h - 1 - y) * src_stride;
        const dst_row_off = BMP_HEADERS_SIZE + y * dst_stride;

        let x: usize = 0;
        while (x < w) : (x += 1) {
            const s_off = src_row_off + x * (src_bpp as usize);
            const d_off = dst_row_off + x * (src_bpp as usize);
            // Convert RGB/RGBA -> BGR/BGRA
            out[d_off] = pixels[s_off + 2];
            out[d_off + 1] = pixels[s_off + 1];
            out[d_off + 2] = pixels[s_off];
            if (bpp == 32) {
                out[d_off + 3] = pixels[s_off + 3];
            }
        }
    }

    return out;
}

/// Encodes top-down 24-bit RGB pixels into a standard bottom-up BMP byte stream
/// with 4-byte row padding and BGR channel ordering.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn encode24(
    allocator: std.mem.Allocator,
    width: u32,
    height: u32,
    rgb_pixels: []const u8,
): ([]u8) ! BmpError {
    return encodeInternal(allocator, width, height, 24, rgb_pixels);
}

/// Encodes top-down 32-bit RGBA pixels into a standard bottom-up BMP byte stream
/// with BGRA channel ordering.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn encode32(
    allocator: std.mem.Allocator,
    width: u32,
    height: u32,
    rgba_pixels: []const u8,
): ([]u8) ! BmpError {
    return encodeInternal(allocator, width, height, 32, rgba_pixels);
}

/// Decodes a BMP byte stream (24-bit BGR, 32-bit BGRA, or 8-bit palette) into a top-down `BmpImage`.
/// Caller owns the returned `BmpImage.pixels` and must call `img.deinit(allocator)`.
pub fn decode(allocator: std.mem.Allocator, bmp_bytes: []const u8): BmpImage ! BmpError {
    return decodeWithMaxDimensions(allocator, bmp_bytes, DEFAULT_MAX_DIMENSION, DEFAULT_MAX_DIMENSION);
}

/// Decodes a BMP byte stream while enforcing upper bounds `max_w` and `max_h` on image dimensions.
/// Caller owns the returned `BmpImage.pixels` and must call `img.deinit(allocator)`.
pub fn decodeWithMaxDimensions(
    allocator: std.mem.Allocator,
    bmp_bytes: []const u8,
    max_w: u32,
    max_h: u32,
): BmpImage ! BmpError {
    if (bmp_bytes.len < BMP_FILE_HEADER_SIZE) {
        return BmpError.TruncatedData;
    }
    if (bmp_bytes[0] != 'B' || bmp_bytes[1] != 'M') {
        return BmpError.InvalidSignature;
    }
    if (bmp_bytes.len < BMP_HEADERS_SIZE) {
        return BmpError.TruncatedData;
    }

    const pixel_offset = readU32LE(bmp_bytes, 10) as usize;
    const dib_size = readU32LE(bmp_bytes, 14) as usize;
    if (dib_size < BMP_INFO_HEADER_SIZE) {
        return BmpError.InvalidHeader;
    }
    if (dib_size > bmp_bytes.len - BMP_FILE_HEADER_SIZE) {
        return BmpError.TruncatedData;
    }

    const raw_width = readI32LE(bmp_bytes, 18);
    const raw_height = readI32LE(bmp_bytes, 22);
    if (raw_width <= 0 || raw_height == 0 || raw_height == std.math.minInt(i32)) {
        return BmpError.InvalidDimensions;
    }

    const top_down = raw_height < 0;
    const width: u32 = raw_width as u32;
    const height: u32 = (if (top_down) -raw_height else raw_height) as u32;
    if (width > max_w || height > max_h) {
        return BmpError.ImageTooLarge;
    }

    const planes = readU16LE(bmp_bytes, 26);
    if (planes != 1) {
        return BmpError.InvalidHeader;
    }

    const bpp = readU16LE(bmp_bytes, 28);
    if (bpp != 8 && bpp != 24 && bpp != 32) {
        return BmpError.UnsupportedBitDepth;
    }

    const compression = readU32LE(bmp_bytes, 30);
    if (compression == BI_RGB) {
        // Supported for 8, 24, 32-bit
    } else if (compression == BI_BITFIELDS) {
        if (bpp != 32) {
            return BmpError.UnsupportedCompression;
        }
    } else {
        return BmpError.UnsupportedCompression;
    }

    const colors_used = readU32LE(bmp_bytes, 46);
    const header_end = BMP_FILE_HEADER_SIZE + dib_size;

    let num_palette_colors: usize = 0;
    if (bpp == 8) {
        num_palette_colors = if (colors_used == 0) 256 else (colors_used as usize);
        if (num_palette_colors > 256) {
            return BmpError.InvalidHeader;
        }
        const palette_bytes = num_palette_colors * 4;
        if (header_end + palette_bytes > bmp_bytes.len) {
            return BmpError.TruncatedData;
        }
        if (pixel_offset < header_end + palette_bytes) {
            return BmpError.InvalidHeader;
        }
    } else {
        if (pixel_offset < header_end) {
            return BmpError.InvalidHeader;
        }
    }

    if (pixel_offset > bmp_bytes.len) {
        return BmpError.TruncatedData;
    }

    const w64 = width as u64;
    const h64 = height as u64;
    const src_row_stride64 = ((w64 * (bpp as u64) + 31) / 32) * 4;
    const src_total_bytes64 = src_row_stride64 * h64;
    if ((pixel_offset as u64) + src_total_bytes64 > (bmp_bytes.len as u64)) {
        return BmpError.TruncatedData;
    }

    const out_format = if (bpp == 32) BmpPixelFormat.Rgba32 else BmpPixelFormat.Rgb24;
    const out_bpp: usize = if (bpp == 32) 4 else 3;
    const out_len64 = w64 * h64 * (out_bpp as u64);
    if (out_len64 > (std.math.maxInt(usize) as u64)) {
        return BmpError.ImageTooLarge;
    }

    const out_len = out_len64 as usize;
    let pixels = allocator.alloc(u8, out_len) catch return BmpError.OutOfMemory;
    errdefer allocator.free(pixels);

    const w = width as usize;
    const h = height as usize;
    const src_stride = src_row_stride64 as usize;
    const dst_stride = w * out_bpp;

    let y: usize = 0;
    while (y < h) : (y += 1) {
        const src_y = if (top_down) y else (h - 1 - y);
        const src_row_off = pixel_offset + src_y * src_stride;
        const dst_row_off = y * dst_stride;

        if (bpp == 24) {
            let x: usize = 0;
            while (x < w) : (x += 1) {
                const s_off = src_row_off + x * 3;
                const d_off = dst_row_off + x * 3;
                pixels[d_off] = bmp_bytes[s_off + 2];
                pixels[d_off + 1] = bmp_bytes[s_off + 1];
                pixels[d_off + 2] = bmp_bytes[s_off];
            }
        } else if (bpp == 32) {
            let x: usize = 0;
            while (x < w) : (x += 1) {
                const s_off = src_row_off + x * 4;
                const d_off = dst_row_off + x * 4;
                pixels[d_off] = bmp_bytes[s_off + 2];
                pixels[d_off + 1] = bmp_bytes[s_off + 1];
                pixels[d_off + 2] = bmp_bytes[s_off];
                pixels[d_off + 3] = bmp_bytes[s_off + 3];
            }
        } else {
            // 8-bit palette indexed
            let x: usize = 0;
            while (x < w) : (x += 1) {
                const idx = bmp_bytes[src_row_off + x] as usize;
                if (idx >= num_palette_colors) {
                    return BmpError.InvalidHeader;
                }
                const p_off = header_end + idx * 4;
                const d_off = dst_row_off + x * 3;
                pixels[d_off] = bmp_bytes[p_off + 2];
                pixels[d_off + 1] = bmp_bytes[p_off + 1];
                pixels[d_off + 2] = bmp_bytes[p_off];
            }
        }
    }

    return BmpImage{
        width: width,
        height: height,
        format: out_format,
        pixels: pixels,
    };
}

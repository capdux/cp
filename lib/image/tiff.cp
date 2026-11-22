// Source: cp/lib/tiff/tiff.cp (Pure CP Baseline TIFF Codec: II/MM Endian, IFD Tags, None/PackBits/LZW/Deflate)
// Zero external C dependency implementation using pure CP, cap:lzw, and cap:deflate.

const std = @import("std");
import {
    lzwCompressTiff,
    lzwDecompressTiff,
} from "cap:lzw";
import {
    DeflateError,
    compressZlib,
    decompressZlib,
} from "cap:deflate";

const DEFAULT_MAX_WIDTH: u32 = 65535;
const DEFAULT_MAX_HEIGHT: u32 = 65535;
const DEFAULT_MAX_PAGES: usize = 1024;

const TIFF_MAGIC: u16 = 42;

const TAG_IMAGE_WIDTH: u16 = 256;
const TAG_IMAGE_LENGTH: u16 = 257;
const TAG_BITS_PER_SAMPLE: u16 = 258;
const TAG_COMPRESSION: u16 = 259;
const TAG_PHOTOMETRIC: u16 = 262;
const TAG_STRIP_OFFSETS: u16 = 273;
const TAG_SAMPLES_PER_PIXEL: u16 = 277;
const TAG_ROWS_PER_STRIP: u16 = 278;
const TAG_STRIP_BYTE_COUNTS: u16 = 279;
const TAG_PREDICTOR: u16 = 317;
const TAG_EXTRA_SAMPLES: u16 = 338;

const TYPE_BYTE: u16 = 1;
const TYPE_SHORT: u16 = 3;
const TYPE_LONG: u16 = 4;

/// Error set for TIFF encoding and decoding.
pub err TiffError {
    InvalidSignature,
    InvalidHeader,
    InvalidIfd,
    UnsupportedCompression,
    UnsupportedFormat,
    TruncatedData,
    ImageTooLarge,
    TooManyPages,
    OutOfMemory,
}

/// Byte order (endianness) of a TIFF container.
pub enum TiffEndian {
    /// Little-endian ("II").
    Little,
    /// Big-endian ("MM").
    Big,
}

/// Supported TIFF compression schemes.
pub enum TiffCompression: u16 {
    None = 1,
    Lzw = 5,
    Deflate = 8,
    PackBits = 32773,
}

/// Supported pixel formats for baseline 8-bit TIFF pages.
pub enum TiffPixelFormat {
    Gray8,
    Rgb24,
    Rgba32,
}

/// A single decoded or to-be-encoded TIFF page.
pub struct TiffPage {
    width: u32,
    height: u32,
    format: TiffPixelFormat,
    compression: TiffCompression,
    pixels: []u8,

    /// Frees the page-owned pixel buffer.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        allocator.free(_.pixels);
    }
}

/// A multi-page TIFF document.
pub struct TiffDocument {
    pages: []TiffPage,

    /// Frees all pages and the pages slice.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.pages.len) : (i += 1) {
            _.pages[i].deinit(allocator);
        }
        allocator.free(_.pages);
    }
}

fn samplesPerPixelForFormat(fmt: TiffPixelFormat): usize {
    return match (fmt) {
        TiffPixelFormat.Gray8 => 1,
        TiffPixelFormat.Rgb24 => 3,
        TiffPixelFormat.Rgba32 => 4,
    };
}

fn photometricForFormat(fmt: TiffPixelFormat): u16 {
    return match (fmt) {
        TiffPixelFormat.Gray8 => 1,
        TiffPixelFormat.Rgb24 => 2,
        TiffPixelFormat.Rgba32 => 2,
    };
}

fn readU16(buf: []const u8, pos: usize, is_big: bool): u16 {
    if (is_big) {
        return ((buf[pos] as u16) << 8) | (buf[pos + 1] as u16);
    }
    return (buf[pos] as u16) | ((buf[pos + 1] as u16) << 8);
}

fn readU32(buf: []const u8, pos: usize, is_big: bool): u32 {
    if (is_big) {
        return ((buf[pos] as u32) << 24) |
            ((buf[pos + 1] as u32) << 16) |
            ((buf[pos + 2] as u32) << 8) |
            (buf[pos + 3] as u32);
    }
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn appendU16(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u16, is_big: bool): void ! TiffError {
    if (is_big) {
        out.append(alloc, ((v >> 8) & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, (v & 0xff) as u8) catch return TiffError.OutOfMemory;
    } else {
        out.append(alloc, (v & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, ((v >> 8) & 0xff) as u8) catch return TiffError.OutOfMemory;
    }
}

fn appendU32(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32, is_big: bool): void ! TiffError {
    if (is_big) {
        out.append(alloc, ((v >> 24) & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, ((v >> 16) & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, ((v >> 8) & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, (v & 0xff) as u8) catch return TiffError.OutOfMemory;
    } else {
        out.append(alloc, (v & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, ((v >> 8) & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, ((v >> 16) & 0xff) as u8) catch return TiffError.OutOfMemory;
        out.append(alloc, ((v >> 24) & 0xff) as u8) catch return TiffError.OutOfMemory;
    }
}

fn writeU32At(buf: []u8, pos: usize, v: u32, is_big: bool) {
    if (is_big) {
        buf[pos] = ((v >> 24) & 0xff) as u8;
        buf[pos + 1] = ((v >> 16) & 0xff) as u8;
        buf[pos + 2] = ((v >> 8) & 0xff) as u8;
        buf[pos + 3] = (v & 0xff) as u8;
    } else {
        buf[pos] = (v & 0xff) as u8;
        buf[pos + 1] = ((v >> 8) & 0xff) as u8;
        buf[pos + 2] = ((v >> 16) & 0xff) as u8;
        buf[pos + 3] = ((v >> 24) & 0xff) as u8;
    }
}

fn appendIfdEntry(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    tag: u16,
    field_type: u16,
    count: u32,
    val_or_off: u32,
    is_big: bool,
): void ! TiffError {
    appendU16(alloc, out, tag, is_big)?;
    appendU16(alloc, out, field_type, is_big)?;
    appendU32(alloc, out, count, is_big)?;
    appendU32(alloc, out, val_or_off, is_big)?;
}

fn packShortInline(v: u16, is_big: bool): u32 {
    if (is_big) {
        return (v as u32) << 16;
    }
    return v as u32;
}

fn encodePackBits(alloc: std.mem.Allocator, src: []const u8): ([]u8) ! TiffError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < src.len) {
        if (i + 1 < src.len && src[i] == src[i + 1]) {
            let run_len: usize = 2;
            while (run_len < 128 && i + run_len < src.len && src[i + run_len] == src[i]) {
                run_len += 1;
            }
            const hdr: u8 = (257 - run_len) as u8;
            out.append(alloc, hdr) catch return TiffError.OutOfMemory;
            out.append(alloc, src[i]) catch return TiffError.OutOfMemory;
            i += run_len;
        } else {
            let lit_len: usize = 1;
            while (lit_len < 128 && i + lit_len < src.len) {
                if (i + lit_len + 2 < src.len &&
                    src[i + lit_len] == src[i + lit_len + 1] &&
                    src[i + lit_len] == src[i + lit_len + 2])
                {
                    break;
                }
                lit_len += 1;
            }
            const hdr: u8 = (lit_len - 1) as u8;
            out.append(alloc, hdr) catch return TiffError.OutOfMemory;
            out.appendSlice(alloc, src[i..(i + lit_len)]) catch return TiffError.OutOfMemory;
            i += lit_len;
        }
    }

    return out.toOwnedSlice(alloc) catch return TiffError.OutOfMemory;
}

fn decodePackBits(alloc: std.mem.Allocator, src: []const u8, max_bytes: usize): ([]u8) ! TiffError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let pos: usize = 0;
    while (pos < src.len) {
        const hdr = src[pos];
        pos += 1;
        if (hdr < 128) {
            const count: usize = (hdr as usize) + 1;
            if (pos + count > src.len) {
                return TiffError.TruncatedData;
            }
            if (out.items.len + count > max_bytes) {
                return TiffError.ImageTooLarge;
            }
            out.appendSlice(alloc, src[pos..(pos + count)]) catch return TiffError.OutOfMemory;
            pos += count;
        } else if (hdr > 128) {
            const count: usize = (257 as usize) - (hdr as usize);
            if (pos >= src.len) {
                return TiffError.TruncatedData;
            }
            const val = src[pos];
            pos += 1;
            if (out.items.len + count > max_bytes) {
                return TiffError.ImageTooLarge;
            }
            let k: usize = 0;
            while (k < count) : (k += 1) {
                out.append(alloc, val) catch return TiffError.OutOfMemory;
            }
        }
    }

    return out.toOwnedSlice(alloc) catch return TiffError.OutOfMemory;
}

fn compressStrip(alloc: std.mem.Allocator, raw: []const u8, compression: TiffCompression): ([]u8) ! TiffError {
    if (compression == TiffCompression.None) {
        const copy = alloc.alloc(u8, raw.len) catch return TiffError.OutOfMemory;
        @memcpy(copy, raw);
        return copy;
    }
    if (compression == TiffCompression.PackBits) {
        return encodePackBits(alloc, raw);
    }
    if (compression == TiffCompression.Lzw) {
        const cap_len = raw.len * 2 + 64;
        let tmp = alloc.alloc(u8, cap_len) catch return TiffError.OutOfMemory;
        defer alloc.free(tmp);
        if (written := lzwCompressTiff(raw, *tmp)) {
            const exact = alloc.alloc(u8, written) catch return TiffError.OutOfMemory;
            @memcpy(exact, tmp[0..written]);
            return exact;
        }
        return TiffError.InvalidHeader;
    }
    if (compression == TiffCompression.Deflate) {
        return compressZlib(alloc, raw) catch |e| {
            if (e == DeflateError.OutOfMemory) {
                return TiffError.OutOfMemory;
            }
            return TiffError.InvalidHeader;
        };
    }
    return TiffError.UnsupportedCompression;
}

fn decompressStrip(
    alloc: std.mem.Allocator,
    compressed: []const u8,
    compression: TiffCompression,
    max_bytes: usize,
): ([]u8) ! TiffError {
    if (compression == TiffCompression.None) {
        if (compressed.len > max_bytes) {
            const copy = alloc.alloc(u8, max_bytes) catch return TiffError.OutOfMemory;
            @memcpy(copy, compressed[0..max_bytes]);
            return copy;
        }
        const copy = alloc.alloc(u8, compressed.len) catch return TiffError.OutOfMemory;
        @memcpy(copy, compressed);
        return copy;
    }
    if (compression == TiffCompression.PackBits) {
        return decodePackBits(alloc, compressed, max_bytes);
    }
    if (compression == TiffCompression.Lzw) {
        let tmp = alloc.alloc(u8, max_bytes) catch return TiffError.OutOfMemory;
        defer alloc.free(tmp);
        if (written := lzwDecompressTiff(compressed, *tmp)) {
            const exact = alloc.alloc(u8, written) catch return TiffError.OutOfMemory;
            @memcpy(exact, tmp[0..written]);
            return exact;
        }
        return TiffError.TruncatedData;
    }
    if (compression == TiffCompression.Deflate) {
        return decompressZlib(alloc, compressed) catch |e| {
            if (e == DeflateError.OutOfMemory) {
                return TiffError.OutOfMemory;
            }
            if (e == DeflateError.UnexpectedEof) {
                return TiffError.TruncatedData;
            }
            return TiffError.InvalidHeader;
        };
    }
    return TiffError.UnsupportedCompression;
}

fn checkedPixelBytes(width: u32, height: u32, spp: usize): usize ! TiffError {
    const w = width as usize;
    const h = height as usize;
    const wh = @mulWithOverflow(w, h);
    if (wh[1] != 0) {
        return TiffError.ImageTooLarge;
    }
    const total = @mulWithOverflow(wh[0], spp);
    if (total[1] != 0) {
        return TiffError.ImageTooLarge;
    }
    return total[0];
}

/// Encodes a single-page TIFF image into a newly allocated buffer.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn encodeSingle(
    allocator: std.mem.Allocator,
    width: u32,
    height: u32,
    format: TiffPixelFormat,
    compression: TiffCompression,
    endian: TiffEndian,
    pixels: []const u8,
): ([]u8) ! TiffError {
    const page = TiffPage{
        width: width,
        height: height,
        format: format,
        compression: compression,
        pixels: @constCast(pixels),
    };
    let pages: [1]TiffPage = [page];
    return encodeMultiPage(allocator, pages[0..1], endian);
}

/// Encodes one or more TIFF pages into a multi-page TIFF container.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn encodeMultiPage(
    allocator: std.mem.Allocator,
    pages: []const TiffPage,
    endian: TiffEndian,
): ([]u8) ! TiffError {
    if (pages.len == 0) {
        return TiffError.InvalidHeader;
    }

    const is_big = endian == TiffEndian.Big;
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    // 8-byte TIFF header: "II"/"MM", 42, IFD0 offset (patched after).
    if (is_big) {
        out.append(allocator, 'M') catch return TiffError.OutOfMemory;
        out.append(allocator, 'M') catch return TiffError.OutOfMemory;
    } else {
        out.append(allocator, 'I') catch return TiffError.OutOfMemory;
        out.append(allocator, 'I') catch return TiffError.OutOfMemory;
    }
    appendU16(allocator, &out, TIFF_MAGIC, is_big)?;
    appendU32(allocator, &out, 8, is_big)?;

    let prev_next_ifd_pos: usize = 4;

    let p_idx: usize = 0;
    while (p_idx < pages.len) : (p_idx += 1) {
        const page = pages[p_idx];
        if (page.width == 0 || page.height == 0) {
            return TiffError.InvalidHeader;
        }
        const spp = samplesPerPixelForFormat(page.format);
        const expected_len = checkedPixelBytes(page.width, page.height, spp)?;
        if (page.pixels.len != expected_len) {
            return TiffError.TruncatedData;
        }

        const compressed = compressStrip(allocator, page.pixels, page.compression)?;
        defer allocator.free(compressed);

        // Align IFD to 2-byte boundary if needed.
        if ((out.items.len & 1) != 0) {
            out.append(allocator, 0) catch return TiffError.OutOfMemory;
        }

        const ifd_offset = out.items.len as u32;
        writeU32At(out.items, prev_next_ifd_pos, ifd_offset, is_big);

        const has_extra_samples = page.format == TiffPixelFormat.Rgba32;
        const num_entries: u16 = if (has_extra_samples) { 10 as u16 } else { 9 as u16 };
        appendU16(allocator, &out, num_entries, is_big)?;

        // Compute offsets of out-of-line data placed immediately after IFD + next_ifd_offset.
        const ifd_size = 2 + (num_entries as usize) * 12 + 4;
        let cursor = out.items.len - 2 + ifd_size;

        let bps_offset: u32 = 0;
        if (spp > 1) {
            bps_offset = cursor as u32;
            cursor += spp * 2;
        }

        const strip_offset: u32 = cursor as u32;
        const strip_byte_count: u32 = compressed.len as u32;

        // IFD entries MUST be sorted in ascending tag order.
        appendIfdEntry(allocator, &out, TAG_IMAGE_WIDTH, TYPE_LONG, 1, page.width, is_big)?;
        appendIfdEntry(allocator, &out, TAG_IMAGE_LENGTH, TYPE_LONG, 1, page.height, is_big)?;
        if (spp == 1) {
            appendIfdEntry(allocator, &out, TAG_BITS_PER_SAMPLE, TYPE_SHORT, 1, packShortInline(8, is_big), is_big)?;
        } else {
            appendIfdEntry(allocator, &out, TAG_BITS_PER_SAMPLE, TYPE_SHORT, spp as u32, bps_offset, is_big)?;
        }
        appendIfdEntry(
            allocator,
            &out,
            TAG_COMPRESSION,
            TYPE_SHORT,
            1,
            packShortInline(page.compression as u16, is_big),
            is_big,
        )?;
        appendIfdEntry(
            allocator,
            &out,
            TAG_PHOTOMETRIC,
            TYPE_SHORT,
            1,
            packShortInline(photometricForFormat(page.format), is_big),
            is_big,
        )?;
        appendIfdEntry(allocator, &out, TAG_STRIP_OFFSETS, TYPE_LONG, 1, strip_offset, is_big)?;
        appendIfdEntry(
            allocator,
            &out,
            TAG_SAMPLES_PER_PIXEL,
            TYPE_SHORT,
            1,
            packShortInline(spp as u16, is_big),
            is_big,
        )?;
        appendIfdEntry(allocator, &out, TAG_ROWS_PER_STRIP, TYPE_LONG, 1, page.height, is_big)?;
        appendIfdEntry(allocator, &out, TAG_STRIP_BYTE_COUNTS, TYPE_LONG, 1, strip_byte_count, is_big)?;
        if (has_extra_samples) {
            appendIfdEntry(allocator, &out, TAG_EXTRA_SAMPLES, TYPE_SHORT, 1, packShortInline(2, is_big), is_big)?;
        }

        // Next IFD offset placeholder (0 for last page).
        prev_next_ifd_pos = out.items.len;
        appendU32(allocator, &out, 0, is_big)?;

        // Out-of-line BitsPerSample array if spp > 1.
        if (spp > 1) {
            let s: usize = 0;
            while (s < spp) : (s += 1) {
                appendU16(allocator, &out, 8, is_big)?;
            }
        }

        // Strip pixel data.
        out.appendSlice(allocator, compressed) catch return TiffError.OutOfMemory;
    }

    return out.toOwnedSlice(allocator) catch return TiffError.OutOfMemory;
}

fn readIfdScalar(data: []const u8, entry_pos: usize, is_big: bool): u32 ! TiffError {
    const field_type = readU16(data, entry_pos + 2, is_big);
    const count = readU32(data, entry_pos + 4, is_big);
    if (count != 1) {
        return TiffError.InvalidIfd;
    }
    if (field_type == TYPE_BYTE) {
        return data[entry_pos + 8] as u32;
    }
    if (field_type == TYPE_SHORT) {
        return readU16(data, entry_pos + 8, is_big) as u32;
    }
    if (field_type == TYPE_LONG) {
        return readU32(data, entry_pos + 8, is_big);
    }
    return TiffError.InvalidIfd;
}

fn readIfdArray(alloc: std.mem.Allocator, data: []const u8, entry_pos: usize, is_big: bool): ([]u32) ! TiffError {
    const field_type = readU16(data, entry_pos + 2, is_big);
    const count = readU32(data, entry_pos + 4, is_big) as usize;
    if (count == 0) {
        return TiffError.InvalidIfd;
    }

    const elem_size: usize = if (field_type == TYPE_BYTE) {
        1
    } else if (field_type == TYPE_SHORT) {
        2
    } else if (field_type == TYPE_LONG) {
        4
    } else {
        return TiffError.InvalidIfd;
    };

    const total_bytes_ov = @mulWithOverflow(count, elem_size);
    if (total_bytes_ov[1] != 0) {
        return TiffError.InvalidIfd;
    }
    const total_bytes = total_bytes_ov[0];

    let src_pos: usize = entry_pos + 8;
    if (total_bytes > 4) {
        src_pos = readU32(data, entry_pos + 8, is_big) as usize;
        if (src_pos > data.len || total_bytes > data.len - src_pos) {
            return TiffError.TruncatedData;
        }
    }

    let result = alloc.alloc(u32, count) catch return TiffError.OutOfMemory;
    errdefer alloc.free(result);

    let i: usize = 0;
    while (i < count) : (i += 1) {
        const p = src_pos + i * elem_size;
        if (field_type == TYPE_BYTE) {
            result[i] = data[p] as u32;
        } else if (field_type == TYPE_SHORT) {
            result[i] = readU16(data, p, is_big) as u32;
        } else {
            result[i] = readU32(data, p, is_big);
        }
    }

    return result;
}

fn applyHorizontalPredictorDecode(pixels: []u8, width: u32, height: u32, spp: usize) {
    const w = width as usize;
    const h = height as usize;
    const row_stride = w * spp;
    let y: usize = 0;
    while (y < h) : (y += 1) {
        const row_start = y * row_stride;
        let x: usize = 1;
        while (x < w) : (x += 1) {
            let c: usize = 0;
            while (c < spp) : (c += 1) {
                const idx = row_start + x * spp + c;
                const prev = row_start + (x - 1) * spp + c;
                pixels[idx] = pixels[idx] +% pixels[prev];
            }
        }
    }
}

/// Decodes a TIFF container with default safety limits (65535x65535, 1024 pages).
/// Caller owns the returned `TiffDocument` and must call `doc.deinit(allocator)`.
pub fn decode(allocator: std.mem.Allocator, tiff_bytes: []const u8): TiffDocument ! TiffError {
    return decodeWithLimits(allocator, tiff_bytes, DEFAULT_MAX_WIDTH, DEFAULT_MAX_HEIGHT, DEFAULT_MAX_PAGES);
}

/// Decodes a TIFF container with explicit dimension and page-count limits.
/// Caller owns the returned `TiffDocument` and must call `doc.deinit(allocator)`.
pub fn decodeWithLimits(
    allocator: std.mem.Allocator,
    tiff_bytes: []const u8,
    max_w: u32,
    max_h: u32,
    max_pages: usize,
): TiffDocument ! TiffError {
    if (tiff_bytes.len < 8) {
        return TiffError.TruncatedData;
    }

    const is_big = tiff_bytes[0] == 'M' && tiff_bytes[1] == 'M';
    const is_little = tiff_bytes[0] == 'I' && tiff_bytes[1] == 'I';
    if (!is_big && !is_little) {
        return TiffError.InvalidSignature;
    }

    const magic = readU16(tiff_bytes, 2, is_big);
    if (magic != TIFF_MAGIC) {
        return TiffError.InvalidSignature;
    }

    let ifd_offset = readU32(tiff_bytes, 4, is_big) as usize;
    if (ifd_offset < 8) {
        return TiffError.InvalidIfd;
    }

    let pages: std.ArrayList(TiffPage) = .empty;
    errdefer {
        let k: usize = 0;
        while (k < pages.items.len) : (k += 1) {
            pages.items[k].deinit(allocator);
        }
        pages.deinit(allocator);
    }

    let visited_ifds: std.ArrayList(usize) = .empty;
    defer visited_ifds.deinit(allocator);

    while (ifd_offset != 0) {
        if (pages.items.len >= max_pages) {
            return TiffError.TooManyPages;
        }

        // Cycle detection for IFD linked list.
        let v_idx: usize = 0;
        while (v_idx < visited_ifds.items.len) : (v_idx += 1) {
            if (visited_ifds.items[v_idx] == ifd_offset) {
                return TiffError.InvalidIfd;
            }
        }
        visited_ifds.append(allocator, ifd_offset) catch return TiffError.OutOfMemory;

        if (ifd_offset >= tiff_bytes.len) {
            return TiffError.InvalidIfd;
        }
        if (ifd_offset + 2 > tiff_bytes.len) {
            return TiffError.TruncatedData;
        }

        const num_entries = readU16(tiff_bytes, ifd_offset, is_big) as usize;
        const entries_bytes = num_entries * 12;
        if (entries_bytes > tiff_bytes.len - (ifd_offset + 2)) {
            return TiffError.TruncatedData;
        }
        const next_ifd_pos = ifd_offset + 2 + entries_bytes;
        if (next_ifd_pos + 4 > tiff_bytes.len) {
            return TiffError.TruncatedData;
        }

        let width: u32 = 0;
        let height: u32 = 0;
        let has_width = false;
        let has_height = false;
        let compression_raw: u32 = 1;
        let photometric: u32 = 0;
        let has_photometric = false;
        let spp: usize = 1;
        let predictor: u32 = 1;

        let bps_list: ([]u32)? = null;
        defer if (b := bps_list) { allocator.free(b); }

        let strip_offsets: ([]u32)? = null;
        defer if (so := strip_offsets) { allocator.free(so); }

        let strip_byte_counts: ([]u32)? = null;
        defer if (sbc := strip_byte_counts) { allocator.free(sbc); }

        let e_idx: usize = 0;
        while (e_idx < num_entries) : (e_idx += 1) {
            const entry_pos = ifd_offset + 2 + e_idx * 12;
            const tag = readU16(tiff_bytes, entry_pos, is_big);

            if (tag == TAG_IMAGE_WIDTH) {
                width = readIfdScalar(tiff_bytes, entry_pos, is_big)?;
                has_width = true;
            } else if (tag == TAG_IMAGE_LENGTH) {
                height = readIfdScalar(tiff_bytes, entry_pos, is_big)?;
                has_height = true;
            } else if (tag == TAG_BITS_PER_SAMPLE) {
                if (bps_list != null) {
                    allocator.free(bps_list.?);
                    bps_list = null;
                }
                bps_list = readIfdArray(allocator, tiff_bytes, entry_pos, is_big)?;
            } else if (tag == TAG_COMPRESSION) {
                compression_raw = readIfdScalar(tiff_bytes, entry_pos, is_big)?;
            } else if (tag == TAG_PHOTOMETRIC) {
                photometric = readIfdScalar(tiff_bytes, entry_pos, is_big)?;
                has_photometric = true;
            } else if (tag == TAG_STRIP_OFFSETS) {
                if (strip_offsets != null) {
                    allocator.free(strip_offsets.?);
                    strip_offsets = null;
                }
                strip_offsets = readIfdArray(allocator, tiff_bytes, entry_pos, is_big)?;
            } else if (tag == TAG_SAMPLES_PER_PIXEL) {
                spp = readIfdScalar(tiff_bytes, entry_pos, is_big)? as usize;
            } else if (tag == TAG_STRIP_BYTE_COUNTS) {
                if (strip_byte_counts != null) {
                    allocator.free(strip_byte_counts.?);
                    strip_byte_counts = null;
                }
                strip_byte_counts = readIfdArray(allocator, tiff_bytes, entry_pos, is_big)?;
            } else if (tag == TAG_PREDICTOR) {
                predictor = readIfdScalar(tiff_bytes, entry_pos, is_big)?;
            }
        }

        if (!has_width || !has_height || width == 0 || height == 0) {
            return TiffError.InvalidHeader;
        }
        if (width > max_w || height > max_h) {
            return TiffError.ImageTooLarge;
        }
        if (!has_photometric) {
            return TiffError.InvalidHeader;
        }

        const compression: TiffCompression = if (compression_raw == 1) {
            TiffCompression.None
        } else if (compression_raw == 5) {
            TiffCompression.Lzw
        } else if (compression_raw == 8 || compression_raw == 32946) {
            TiffCompression.Deflate
        } else if (compression_raw == 32773) {
            TiffCompression.PackBits
        } else {
            return TiffError.UnsupportedCompression;
        };

        const format: TiffPixelFormat = if (photometric == 1 && spp == 1) {
            TiffPixelFormat.Gray8
        } else if (photometric == 2 && spp == 3) {
            TiffPixelFormat.Rgb24
        } else if (photometric == 2 && spp == 4) {
            TiffPixelFormat.Rgba32
        } else {
            return TiffError.UnsupportedFormat;
        };

        if (bps := bps_list) {
            if (bps.len < spp) {
                return TiffError.UnsupportedFormat;
            }
            let b_idx: usize = 0;
            while (b_idx < spp) : (b_idx += 1) {
                if (bps[b_idx] != 8) {
                    return TiffError.UnsupportedFormat;
                }
            }
        }

        if (predictor != 1 && predictor != 2) {
            return TiffError.UnsupportedFormat;
        }

        if (strip_offsets == null || strip_byte_counts == null) {
            return TiffError.InvalidIfd;
        }
        const offsets = strip_offsets.?;
        const counts = strip_byte_counts.?;
        if (offsets.len == 0 || offsets.len != counts.len) {
            return TiffError.InvalidIfd;
        }

        const expected_total = checkedPixelBytes(width, height, spp)?;
        let pixel_buf: std.ArrayList(u8) = .empty;
        errdefer pixel_buf.deinit(allocator);

        let s_idx: usize = 0;
        while (s_idx < offsets.len) : (s_idx += 1) {
            const off = offsets[s_idx] as usize;
            const cnt = counts[s_idx] as usize;
            if (off > tiff_bytes.len || cnt > tiff_bytes.len - off) {
                return TiffError.TruncatedData;
            }
            const remaining = if (expected_total > pixel_buf.items.len) {
                expected_total - pixel_buf.items.len
            } else {
                0
            };
            const strip_raw = tiff_bytes[off..(off + cnt)];
            const strip_dec = decompressStrip(allocator, strip_raw, compression, remaining)?;
            defer allocator.free(strip_dec);

            const take_len = if (strip_dec.len > remaining) { remaining } else { strip_dec.len };
            pixel_buf.appendSlice(allocator, strip_dec[0..take_len]) catch return TiffError.OutOfMemory;
        }

        if (pixel_buf.items.len < expected_total) {
            return TiffError.TruncatedData;
        }

        if (predictor == 2) {
            applyHorizontalPredictorDecode(pixel_buf.items[0..expected_total], width, height, spp);
        }

        const owned_pixels = pixel_buf.toOwnedSlice(allocator) catch return TiffError.OutOfMemory;
        pages.append(allocator, TiffPage{
            width: width,
            height: height,
            format: format,
            compression: compression,
            pixels: owned_pixels,
        }) catch {
            allocator.free(owned_pixels);
            return TiffError.OutOfMemory;
        };

        ifd_offset = readU32(tiff_bytes, next_ifd_pos, is_big) as usize;
    }

    if (pages.items.len == 0) {
        return TiffError.InvalidIfd;
    }

    return TiffDocument{
        pages: pages.toOwnedSlice(allocator) catch return TiffError.OutOfMemory,
    };
}

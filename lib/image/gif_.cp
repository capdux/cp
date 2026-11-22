// Reference Only: Functional overlap with lib/image/gif.cp | Kept for comparison/analysis
// Source: cp/lib/gif/gif.cp (Pure CP GIF87a / GIF89a Animated Image Codec via cap:lzw)

const std = @import("std");
import {
    LzwBitOrder,
    LzwConfig,
    LzwError,
    compress,
    decompressWithMaxSize,
} from "cap:lzw";

const DEFAULT_MAX_DIMENSION: u16 = 65535;
const DEFAULT_MAX_FRAMES: usize = 4096;

/// Error set for GIF87a / GIF89a encoding and decoding.
pub err GifError {
    InvalidSignature,
    InvalidHeader,
    TruncatedData,
    UnsupportedBlock,
    InvalidColorTable,
    ImageTooLarge,
    TooManyFrames,
    OutOfMemory,
}

/// 24-bit RGB palette entry.
pub struct GifColor {
    r: u8,
    g: u8,
    b: u8,
}

/// Frame disposal method from Graphic Control Extension (GCE).
pub enum DisposalMethod: u8 {
    Unspecified = 0,
    Keep = 1,
    RestoreBg = 2,
    RestorePrev = 3,
}

/// Decoded GIF frame containing both expanded RGBA8888 pixels and original palette indices.
pub struct GifFrame {
    left: u16,
    top: u16,
    width: u16,
    height: u16,
    delay_cs: u16,
    transparent_index: u8? = null,
    disposal: DisposalMethod = DisposalMethod.Unspecified,
    palette: ([]const GifColor)? = null,
    pixels_rgba: []u8,
    indices: []u8,

    /// Frees frame-owned pixel buffers and optional local palette.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        allocator.free(_.pixels_rgba);
        allocator.free(_.indices);
        if (pal := _.palette) {
            allocator.free(pal);
        }
    }
}

/// Input frame descriptor for `encodeIndexed`.
pub struct GifIndexedFrame {
    left: u16 = 0,
    top: u16 = 0,
    width: u16,
    height: u16,
    delay_cs: u16 = 0,
    transparent_index: u8? = null,
    disposal: DisposalMethod = DisposalMethod.Unspecified,
    local_palette: ([]const GifColor)? = null,
    indices: []const u8,
}

/// Decoded GIF container holding logical screen metadata, global palette, and frames.
pub struct GifImage {
    width: u16,
    height: u16,
    loop_count: u16,
    global_palette: []const GifColor,
    frames: []GifFrame,

    /// Frees all frames, frame buffers, and the global palette.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.frames.len) : (i += 1) {
            _.frames[i].deinit(allocator);
        }
        allocator.free(_.frames);
        if (_.global_palette.len > 0) {
            allocator.free(_.global_palette);
        }
    }
}

fn readU16LE(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn appendU16LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: u16): void ! GifError {
    out.append(alloc, (val & 0xff) as u8) catch return GifError.OutOfMemory;
    out.append(alloc, ((val >> 8) & 0xff) as u8) catch return GifError.OutOfMemory;
}

fn paletteSizeCode(count: usize): u8 ! GifError {
    if (count == 0 || count > 256) {
        return GifError.InvalidColorTable;
    }
    let code: u8 = 0;
    let cap: usize = 2;
    while (cap < count) : (code += 1) {
        cap *= 2;
    }
    return code;
}

fn minCodeSizeForPalette(count: usize): u8 ! GifError {
    if (count == 0 || count > 256) {
        return GifError.InvalidColorTable;
    }
    let m: u8 = 2;
    let cap: usize = 4;
    while (cap < count) : (m += 1) {
        cap *= 2;
    }
    return m;
}

fn writeColorTable(alloc: std.mem.Allocator, out: *std.ArrayList(u8), palette: []const GifColor, size_code: u8): void ! GifError {
    const table_entries: usize = (1 as usize) << ((size_code + 1) as u5);
    let i: usize = 0;
    while (i < table_entries) : (i += 1) {
        if (i < palette.len) {
            const c = palette[i];
            out.append(alloc, c.r) catch return GifError.OutOfMemory;
            out.append(alloc, c.g) catch return GifError.OutOfMemory;
            out.append(alloc, c.b) catch return GifError.OutOfMemory;
        } else {
            out.append(alloc, 0) catch return GifError.OutOfMemory;
            out.append(alloc, 0) catch return GifError.OutOfMemory;
            out.append(alloc, 0) catch return GifError.OutOfMemory;
        }
    }
}

fn skipSubBlocks(data: []const u8, pos: *usize): void ! GifError {
    while (true) {
        if (pos.* >= data.len) {
            return GifError.TruncatedData;
        }
        const sub_len = data[pos.*] as usize;
        pos.* += 1;
        if (sub_len == 0) {
            break;
        }
        if (pos.* + sub_len > data.len) {
            return GifError.TruncatedData;
        }
        pos.* += sub_len;
    }
}

fn parseDisposal(raw: u8): DisposalMethod {
    if (raw == 1) return DisposalMethod.Keep;
    if (raw == 2) return DisposalMethod.RestoreBg;
    if (raw == 3) return DisposalMethod.RestorePrev;
    return DisposalMethod.Unspecified;
}

fn mapLzwError(e: LzwError): GifError {
    if (e == LzwError.OutOfMemory) return GifError.OutOfMemory;
    if (e == LzwError.OutputTooLarge) return GifError.ImageTooLarge;
    return GifError.TruncatedData;
}

/// Encodes one or more palette-indexed frames into a valid GIF89a byte stream.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn encodeIndexed(
    allocator: std.mem.Allocator,
    width: u16,
    height: u16,
    loop_count: u16,
    global_palette: []const GifColor,
    indexed_frames: []const GifIndexedFrame,
): []u8 ! GifError {
    if (width == 0 || height == 0 || indexed_frames.len == 0) {
        return GifError.InvalidHeader;
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    // 1. GIF89a Signature
    out.appendSlice(allocator, "GIF89a") catch return GifError.OutOfMemory;

    // 2. Logical Screen Descriptor
    appendU16LE(allocator, &out, width)?;
    appendU16LE(allocator, &out, height)?;

    if (global_palette.len > 0) {
        const gct_code = paletteSizeCode(global_palette.len)?;
        const packed_lsd: u8 = (0xF0 as u8) | gct_code;
        out.append(allocator, packed_lsd) catch return GifError.OutOfMemory;
        out.append(allocator, 0) catch return GifError.OutOfMemory;
        out.append(allocator, 0) catch return GifError.OutOfMemory;
        writeColorTable(allocator, &out, global_palette, gct_code)?;
    } else {
        out.append(allocator, 0x70) catch return GifError.OutOfMemory;
        out.append(allocator, 0) catch return GifError.OutOfMemory;
        out.append(allocator, 0) catch return GifError.OutOfMemory;
    }

    // 3. Netscape 2.0 Application Extension for animated loop count
    if (indexed_frames.len > 1 || loop_count > 0) {
        out.append(allocator, 0x21) catch return GifError.OutOfMemory;
        out.append(allocator, 0xFF) catch return GifError.OutOfMemory;
        out.append(allocator, 0x0B) catch return GifError.OutOfMemory;
        out.appendSlice(allocator, "NETSCAPE2.0") catch return GifError.OutOfMemory;
        out.append(allocator, 0x03) catch return GifError.OutOfMemory;
        out.append(allocator, 0x01) catch return GifError.OutOfMemory;
        appendU16LE(allocator, &out, loop_count)?;
        out.append(allocator, 0x00) catch return GifError.OutOfMemory;
    }

    // 4. Encode each frame
    let fi: usize = 0;
    while (fi < indexed_frames.len) : (fi += 1) {
        const f = indexed_frames[fi];
        if (f.width == 0 || f.height == 0) {
            return GifError.InvalidHeader;
        }
        if ((f.left as u32) + (f.width as u32) > (width as u32) ||
            (f.top as u32) + (f.height as u32) > (height as u32)) {
            return GifError.InvalidHeader;
        }
        const expected_pixels: usize = (f.width as usize) * (f.height as usize);
        if (f.indices.len != expected_pixels) {
            return GifError.InvalidHeader;
        }

        let active_palette: []const GifColor = global_palette;
        if (lp := f.local_palette) {
            active_palette = lp;
        }
        if (active_palette.len == 0 || active_palette.len > 256) {
            return GifError.InvalidColorTable;
        }

        // Validate indices against active palette
        let pi: usize = 0;
        while (pi < f.indices.len) : (pi += 1) {
            if ((f.indices[pi] as usize) >= active_palette.len) {
                return GifError.InvalidColorTable;
            }
        }
        if (ti := f.transparent_index) {
            if ((ti as usize) >= active_palette.len) {
                return GifError.InvalidColorTable;
            }
        }

        // Graphic Control Extension (8 bytes)
        out.append(allocator, 0x21) catch return GifError.OutOfMemory;
        out.append(allocator, 0xF9) catch return GifError.OutOfMemory;
        out.append(allocator, 0x04) catch return GifError.OutOfMemory;
        const trans_flag: u8 = if (f.transparent_index != null) { 1 as u8 } else { 0 as u8 };
        const gce_packed: u8 = ((f.disposal as u8) << 2) | trans_flag;
        out.append(allocator, gce_packed) catch return GifError.OutOfMemory;
        appendU16LE(allocator, &out, f.delay_cs)?;
        let trans_val: u8 = 0;
        if (ti := f.transparent_index) {
            trans_val = ti;
        }
        out.append(allocator, trans_val) catch return GifError.OutOfMemory;
        out.append(allocator, 0x00) catch return GifError.OutOfMemory;

        // Image Descriptor (10 bytes)
        out.append(allocator, 0x2C) catch return GifError.OutOfMemory;
        appendU16LE(allocator, &out, f.left)?;
        appendU16LE(allocator, &out, f.top)?;
        appendU16LE(allocator, &out, f.width)?;
        appendU16LE(allocator, &out, f.height)?;

        if (lp := f.local_palette) {
            const lct_code = paletteSizeCode(lp.len)?;
            out.append(allocator, (0x80 as u8) | lct_code) catch return GifError.OutOfMemory;
            writeColorTable(allocator, &out, lp, lct_code)?;
        } else {
            out.append(allocator, 0x00) catch return GifError.OutOfMemory;
        }

        // LZW Image Data
        const min_code_size = minCodeSizeForPalette(active_palette.len)?;
        out.append(allocator, min_code_size) catch return GifError.OutOfMemory;

        const lzw_cfg = LzwConfig{
            min_code_size: min_code_size,
            bit_order: LzwBitOrder.LsbFirst,
            max_code_size: 12,
        };
        const compressed = compress(allocator, f.indices, lzw_cfg) catch e => {
            return mapLzwError(e);
        };
        defer allocator.free(compressed);

        // Emit <= 255-byte sub-blocks
        let offset: usize = 0;
        while (offset < compressed.len) {
            const rem = compressed.len - offset;
            const chunk_len: usize = if (rem > 255) { 255 } else { rem };
            out.append(allocator, chunk_len as u8) catch return GifError.OutOfMemory;
            out.appendSlice(allocator, compressed[offset .. (offset + chunk_len)]) catch return GifError.OutOfMemory;
            offset += chunk_len;
        }
        out.append(allocator, 0x00) catch return GifError.OutOfMemory;
    }

    // 5. GIF Trailer
    out.append(allocator, 0x3B) catch return GifError.OutOfMemory;

    return out.toOwnedSlice(allocator) catch return GifError.OutOfMemory;
}

/// Decodes a GIF87a / GIF89a byte stream using default dimension and frame limits.
/// Caller owns the returned `GifImage` and must call `img.deinit(allocator)`.
pub fn decode(allocator: std.mem.Allocator, gif_bytes: []const u8): GifImage ! GifError {
    return decodeWithLimits(allocator, gif_bytes, DEFAULT_MAX_DIMENSION, DEFAULT_MAX_DIMENSION, DEFAULT_MAX_FRAMES);
}

/// Decodes a GIF87a / GIF89a byte stream with explicit max width, max height, and max frame limits.
/// Caller owns the returned `GifImage` and must call `img.deinit(allocator)`.
pub fn decodeWithLimits(
    allocator: std.mem.Allocator,
    gif_bytes: []const u8,
    max_w: u16,
    max_h: u16,
    max_frames: usize,
): GifImage ! GifError {
    if (gif_bytes.len < 6) {
        return GifError.InvalidSignature;
    }
    const sig = gif_bytes[0..6];
    if (!std.mem.eql(u8, sig, "GIF87a") && !std.mem.eql(u8, sig, "GIF89a")) {
        return GifError.InvalidSignature;
    }
    if (gif_bytes.len < 13) {
        return GifError.TruncatedData;
    }

    const width = readU16LE(gif_bytes, 6);
    const height = readU16LE(gif_bytes, 8);
    if (width == 0 || height == 0) {
        return GifError.InvalidHeader;
    }
    if (width > max_w || height > max_h) {
        return GifError.ImageTooLarge;
    }

    const lsd_packed = gif_bytes[10];
    const has_gct = (lsd_packed & 0x80) != 0;
    const gct_size_code = lsd_packed & 0x07;

    let pos: usize = 13;

    let global_palette: []GifColor = allocator.alloc(GifColor, 0) catch return GifError.OutOfMemory;
    errdefer {
        if (global_palette.len > 0) {
            allocator.free(global_palette);
        }
    }

    if (has_gct) {
        const gct_count: usize = (1 as usize) << ((gct_size_code + 1) as u5);
        const gct_bytes: usize = gct_count * 3;
        if (pos + gct_bytes > gif_bytes.len) {
            return GifError.TruncatedData;
        }
        allocator.free(global_palette);
        global_palette = allocator.alloc(GifColor, gct_count) catch return GifError.OutOfMemory;
        let ci: usize = 0;
        while (ci < gct_count) : (ci += 1) {
            global_palette[ci] = GifColor{
                r: gif_bytes[pos],
                g: gif_bytes[pos + 1],
                b: gif_bytes[pos + 2],
            };
            pos += 3;
        }
    }

    let frames = std.ArrayList(GifFrame).empty;
    errdefer {
        let fi: usize = 0;
        while (fi < frames.items.len) : (fi += 1) {
            frames.items[fi].deinit(allocator);
        }
        frames.deinit(allocator);
    }

    let loop_count: u16 = 0;
    let pending_delay_cs: u16 = 0;
    let pending_transparent: u8? = null;
    let pending_disposal: DisposalMethod = DisposalMethod.Unspecified;
    let saw_trailer = false;

    while (pos < gif_bytes.len) {
        const block_id = gif_bytes[pos];
        pos += 1;

        if (block_id == 0x3B) {
            saw_trailer = true;
            break;
        }

        if (block_id == 0x21) {
            if (pos >= gif_bytes.len) {
                return GifError.TruncatedData;
            }
            const ext_label = gif_bytes[pos];
            pos += 1;

            if (ext_label == 0xF9) {
                // Graphic Control Extension
                if (pos >= gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                const blk_size = gif_bytes[pos] as usize;
                pos += 1;
                if (blk_size != 4) {
                    return GifError.InvalidHeader;
                }
                if (pos + 4 > gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                const gce_packed = gif_bytes[pos];
                pending_disposal = parseDisposal((gce_packed >> 2) & 0x07);
                const has_trans = (gce_packed & 0x01) != 0;
                pending_delay_cs = readU16LE(gif_bytes, pos + 1);
                const trans_idx = gif_bytes[pos + 3];
                pending_transparent = if (has_trans) { trans_idx } else { null };
                pos += 4;
                skipSubBlocks(gif_bytes, &pos)?;
            } else if (ext_label == 0xFF) {
                // Application Extension (check for NETSCAPE2.0 loop count)
                if (pos >= gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                const app_size = gif_bytes[pos] as usize;
                pos += 1;
                if (pos + app_size > gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                const is_netscape = (app_size == 11) && std.mem.eql(u8, gif_bytes[pos .. (pos + 11)], "NETSCAPE2.0");
                pos += app_size;
                if (is_netscape) {
                    let first_sub = true;
                    while (true) {
                        if (pos >= gif_bytes.len) {
                            return GifError.TruncatedData;
                        }
                        const sub_len = gif_bytes[pos] as usize;
                        pos += 1;
                        if (sub_len == 0) {
                            break;
                        }
                        if (pos + sub_len > gif_bytes.len) {
                            return GifError.TruncatedData;
                        }
                        if (first_sub && sub_len == 3 && gif_bytes[pos] == 0x01) {
                            loop_count = readU16LE(gif_bytes, pos + 1);
                        }
                        first_sub = false;
                        pos += sub_len;
                    }
                } else {
                    skipSubBlocks(gif_bytes, &pos)?;
                }
            } else {
                // Unknown / Comment (0xFE) / Plain Text (0x01) extension: safely skip sub-blocks
                skipSubBlocks(gif_bytes, &pos)?;
            }
            continue;
        }

        if (block_id == 0x2C) {
            if (frames.items.len >= max_frames) {
                return GifError.TooManyFrames;
            }
            if (pos + 9 > gif_bytes.len) {
                return GifError.TruncatedData;
            }

            const f_left = readU16LE(gif_bytes, pos);
            const f_top = readU16LE(gif_bytes, pos + 2);
            const f_width = readU16LE(gif_bytes, pos + 4);
            const f_height = readU16LE(gif_bytes, pos + 6);
            const f_packed = gif_bytes[pos + 8];
            pos += 9;

            if (f_width == 0 || f_height == 0) {
                return GifError.InvalidHeader;
            }
            if (f_width > max_w || f_height > max_h) {
                return GifError.ImageTooLarge;
            }

            const has_lct = (f_packed & 0x80) != 0;
            const is_interlaced = (f_packed & 0x40) != 0;
            const lct_size_code = f_packed & 0x07;

            let local_pal_opt: ([]GifColor)? = null;
            errdefer {
                if (lp := local_pal_opt) {
                    allocator.free(lp);
                }
            }

            if (has_lct) {
                const lct_count: usize = (1 as usize) << ((lct_size_code + 1) as u5);
                const lct_bytes: usize = lct_count * 3;
                if (pos + lct_bytes > gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                let lp = allocator.alloc(GifColor, lct_count) catch return GifError.OutOfMemory;
                let ci: usize = 0;
                while (ci < lct_count) : (ci += 1) {
                    lp[ci] = GifColor{
                        r: gif_bytes[pos],
                        g: gif_bytes[pos + 1],
                        b: gif_bytes[pos + 2],
                    };
                    pos += 3;
                }
                local_pal_opt = lp;
            }

            let active_palette: []const GifColor = global_palette;
            if (lp := local_pal_opt) {
                active_palette = lp;
            }
            if (active_palette.len == 0) {
                return GifError.InvalidColorTable;
            }

            if (pos >= gif_bytes.len) {
                return GifError.TruncatedData;
            }
            const min_code_size = gif_bytes[pos];
            pos += 1;
            if (min_code_size < 2 || min_code_size > 8) {
                return GifError.InvalidHeader;
            }

            // Collect LZW sub-blocks
            let lzw_stream = std.ArrayList(u8).empty;
            defer lzw_stream.deinit(allocator);

            while (true) {
                if (pos >= gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                const sub_len = gif_bytes[pos] as usize;
                pos += 1;
                if (sub_len == 0) {
                    break;
                }
                if (pos + sub_len > gif_bytes.len) {
                    return GifError.TruncatedData;
                }
                lzw_stream.appendSlice(allocator, gif_bytes[pos .. (pos + sub_len)]) catch return GifError.OutOfMemory;
                pos += sub_len;
            }

            const expected_pixels: usize = (f_width as usize) * (f_height as usize);
            const lzw_cfg = LzwConfig{
                min_code_size: min_code_size,
                bit_order: LzwBitOrder.LsbFirst,
                max_code_size: 12,
            };
            const raw_indices = decompressWithMaxSize(allocator, lzw_stream.items, lzw_cfg, expected_pixels) catch e => {
                return mapLzwError(e);
            };
            if (raw_indices.len < expected_pixels) {
                allocator.free(raw_indices);
                return GifError.TruncatedData;
            }

            let indices: []u8 = raw_indices;
            if (is_interlaced) {
                let deinterlaced = allocator.alloc(u8, expected_pixels) catch {
                    allocator.free(raw_indices);
                    return GifError.OutOfMemory;
                };
                const fw = f_width as usize;
                const fh = f_height as usize;
                let src_row: usize = 0;
                const starts: [4]usize = [0, 4, 2, 1];
                const steps: [4]usize = [8, 8, 4, 2];
                let pass: usize = 0;
                while (pass < 4) : (pass += 1) {
                    let y: usize = starts[pass];
                    while (y < fh) : (y += steps[pass]) {
                        const s_off = src_row * fw;
                        const d_off = y * fw;
                        @memcpy(deinterlaced[d_off .. (d_off + fw)], raw_indices[s_off .. (s_off + fw)]);
                        src_row += 1;
                    }
                }
                allocator.free(raw_indices);
                indices = deinterlaced;
            }
            errdefer allocator.free(indices);

            let pixels_rgba = allocator.alloc(u8, expected_pixels * 4) catch return GifError.OutOfMemory;
            errdefer allocator.free(pixels_rgba);

            let pi: usize = 0;
            while (pi < expected_pixels) : (pi += 1) {
                const idx = indices[pi];
                const rgba_off = pi * 4;
                let is_trans = false;
                if (ti := pending_transparent) {
                    if (idx == ti) {
                        is_trans = true;
                    }
                }
                if (is_trans) {
                    pixels_rgba[rgba_off] = 0;
                    pixels_rgba[rgba_off + 1] = 0;
                    pixels_rgba[rgba_off + 2] = 0;
                    pixels_rgba[rgba_off + 3] = 0;
                } else {
                    if ((idx as usize) >= active_palette.len) {
                        return GifError.InvalidColorTable;
                    }
                    const c = active_palette[idx as usize];
                    pixels_rgba[rgba_off] = c.r;
                    pixels_rgba[rgba_off + 1] = c.g;
                    pixels_rgba[rgba_off + 2] = c.b;
                    pixels_rgba[rgba_off + 3] = 255;
                }
            }

            let frame_pal: ([]const GifColor)? = null;
            if (lp := local_pal_opt) {
                frame_pal = lp;
            }
            local_pal_opt = null;

            frames.append(allocator, GifFrame{
                left: f_left,
                top: f_top,
                width: f_width,
                height: f_height,
                delay_cs: pending_delay_cs,
                transparent_index: pending_transparent,
                disposal: pending_disposal,
                palette: frame_pal,
                pixels_rgba: pixels_rgba,
                indices: indices,
            }) catch return GifError.OutOfMemory;

            pending_delay_cs = 0;
            pending_transparent = null;
            pending_disposal = DisposalMethod.Unspecified;
            continue;
        }

        return GifError.UnsupportedBlock;
    }

    if (!saw_trailer) {
        return GifError.TruncatedData;
    }

    const owned_frames = frames.toOwnedSlice(allocator) catch return GifError.OutOfMemory;
    return GifImage{
        width: width,
        height: height,
        loop_count: loop_count,
        global_palette: global_palette,
        frames: owned_frames,
    };
}

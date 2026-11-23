// Reference-hardened against image-rs/image-gif (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_gif.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/image/gif.cp — 工业级纯 CP 原生 GIF87a/GIF89a 图像与动画编解码器
// 对标 image-rs/image-gif (MIT/Apache-2.0) 加固：
//   - 完整 LZW 编解码器（2..12 bit 可变位宽、Clear/EOI、Deferred Clear 满表兼容、超界码字拦截）
//   - 多帧动画与完整扩展块解析（GCE 延迟/透明色/Disposal 0..3、NETSCAPE2.0 循环次数、
//     Comment/PlainText/Application 跳过、Local Color Table 覆盖）
//   - 4-Pass 隔行扫描（0,8 / 4,8 / 2,4 / 1,2）与子帧画布越界安全裁剪合成
//   - 16-lane @Vector(16, u8) SIMD 加速：调色板展开、矩形背景清零与无分支透明帧合成
//   - 严格防溢出、防超大尺寸内存耗尽、防截断流死循环与 0 内存泄漏
// ============================================================================

const std = @import("std");
import { Quantizer } from "./quantize.cp";

/// 单帧安全像素上限（64M 像素 = 256MB RGBA8，防畸形大尺寸内存耗尽）
const GIF_MAX_PIXELS: u64 = 67108864;

/// 单文件最大动画帧数上限（防畸形超长空循环）
const GIF_MAX_FRAMES: usize = 4096;

/// LZW 字典最大表项数（12-bit 上限）
const LZW_MAX_CODES: usize = 4096;

/// LZW 编码器开放定址哈希桶数量（2 的幂）
const LZW_HASH_BUCKETS: usize = 8192;

/// 16-lane SIMD 字节向量（4 个 RGBA 像素打包）
type Vec16u8 = @Vector(16, u8);

/// GIF 严格编解码错误码（对标 image-rs/image-gif DecodingError / EncodingError）
pub error GifError {
    InvalidSignature,
    TruncatedStream,
    InvalidHeader,
    DimensionsOverflow,
    ImageTooLarge,
    InvalidColorTable,
    MissingColorTable,
    UnknownBlock,
    InvalidLzwCodeSize,
    InvalidLzwCode,
    TooManyFrames,
    InvalidFrameDimensions,
    InvalidBufferSize,
    NoFrames,
    OutOfMemory,
}

/// GIF89a 帧处置方式（Graphic Control Extension Disposal Method 0..3）
pub enum GifDisposal: u8 {
    Unspecified = 0,
    Keep = 1,
    RestoreBackground = 2,
    RestorePrevious = 3,
}

/// GIF 逻辑屏幕描述符元数据（100% 向后兼容原有 4 字段）
pub struct GifHeader {
    pub width: usize,
    pub height: usize,
    pub has_gct: bool,
    pub gct_size: usize,
    pub bg_color_index: u8 = 0,
    pub is_gif89a: bool = true,
}

/// 解码后的单帧 RGBA8 图像（100% 向后兼容）
pub struct GifImage {
    pub width: usize,
    pub height: usize,
    pub rgba: []u8,

    /// 释放像素缓冲区
    pub fn deinit(*_) {
        if (_.rgba.len > 0) {
            std.heap.page_allocator.free(_.rgba);
            _.rgba = [];
        }
    }
}

/// 解码后的单帧完整数据（含局部几何、时序、处置方式、透明色、RGBA8 与索引图）
pub struct GifFrame {
    pub left: usize = 0,
    pub top: usize = 0,
    pub width: usize = 0,
    pub height: usize = 0,
    pub delay_cs: u16 = 0,
    pub disposal: GifDisposal = GifDisposal.Unspecified,
    pub transparent_index: i32 = -1,
    pub interlaced: bool = false,
    pub rgba: []u8 = [],
    pub indices: []u8 = [],

    /// 释放帧拥有的像素与索引缓冲
    pub fn deinit(*_) {
        if (_.rgba.len > 0) {
            std.heap.page_allocator.free(_.rgba);
            _.rgba = [];
        }
        if (_.indices.len > 0) {
            std.heap.page_allocator.free(_.indices);
            _.indices = [];
        }
    }
}

/// 完整解码后的 GIF 动画容器
pub struct GifAnimation {
    pub width: usize = 0,
    pub height: usize = 0,
    pub bg_color_index: u8 = 0,
    pub loop_count: u16 = 1,
    pub has_gct: bool = false,
    pub global_palette: []u8 = [],
    pub frames: []GifFrame = [],

    /// 释放全部帧数据与全局调色板
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.frames.len) : (i += 1) {
            _.frames[i].deinit();
        }
        if (_.frames.len > 0) {
            std.heap.page_allocator.free(_.frames);
            _.frames = [];
        }
        if (_.global_palette.len > 0) {
            std.heap.page_allocator.free(_.global_palette);
            _.global_palette = [];
        }
    }

    /// 按 Disposal Method 0..3 将所有帧依次合成为完整画布尺寸的 RGBA8 帧序列
    pub fn renderComposedFrames(*_): ([]GifImage)!GifError {
        if (_.width == 0 || _.height == 0) {
            return GifError.InvalidHeader;
        }
        let canvas_pix_u64 = (_.width as u64) * (_.height as u64);
        if (canvas_pix_u64 > GIF_MAX_PIXELS) {
            return GifError.ImageTooLarge;
        }
        let canvas_bytes = (canvas_pix_u64 as usize) * 4;
        let out_frames = std.heap.page_allocator.alloc(GifImage, _.frames.len) catch return GifError.OutOfMemory;
        let canvas = std.heap.page_allocator.alloc(u8, canvas_bytes) catch {
            std.heap.page_allocator.free(out_frames);
            return GifError.OutOfMemory;
        };
        defer std.heap.page_allocator.free(canvas);
        let prev_canvas = std.heap.page_allocator.alloc(u8, canvas_bytes) catch {
            std.heap.page_allocator.free(out_frames);
            return GifError.OutOfMemory;
        };
        defer std.heap.page_allocator.free(prev_canvas);

        // 初始画布清零为全透明 [0, 0, 0, 0]
        Gif.clearRectSimd(canvas, _.width, _.height, 0, 0, _.width, _.height, 0, 0, 0, 0);

        let fi: usize = 0;
        while (fi < _.frames.len) : (fi += 1) {
            const f = _.frames[fi];
            if (f.disposal == GifDisposal.RestorePrevious) {
                Gif.simdCopyBytes(prev_canvas, canvas);
            }

            Gif.composeCanvasFrame(canvas, _.width, _.height, f);

            const snap = std.heap.page_allocator.alloc(u8, canvas_bytes) catch {
                let k: usize = 0;
                while (k < fi) : (k += 1) {
                    out_frames[k].deinit();
                }
                std.heap.page_allocator.free(out_frames);
                return GifError.OutOfMemory;
            };
            Gif.simdCopyBytes(snap, canvas);
            out_frames[fi] = GifImage{
                width: _.width,
                height: _.height,
                rgba: snap,
            };

            if (f.disposal == GifDisposal.RestoreBackground) {
                Gif.clearRectSimd(canvas, _.width, _.height, f.left, f.top, f.width, f.height, 0, 0, 0, 0);
            } else if (f.disposal == GifDisposal.RestorePrevious) {
                Gif.simdCopyBytes(canvas, prev_canvas);
            }
        }

        return out_frames;
    }
}

/// 编码输入帧描述符（用于 encodeGifIndexed）
pub struct GifEncodeFrame {
    pub left: usize = 0,
    pub top: usize = 0,
    pub width: usize = 0,
    pub height: usize = 0,
    pub delay_cs: u16 = 0,
    pub disposal: GifDisposal = GifDisposal.Keep,
    pub transparent_index: i32 = -1,
    pub local_palette: []const u8 = "",
    pub indices: []const u8 = "",
}

/// GIF 子块 LSB-first 比特读取器（100% 纯 CP，无 zig {} 块）
struct GifBitReader {
    src: []const u8,
    i: usize,
    block: usize,
    acc: u64,
    nbits: u32,
    eof: bool,

    fn init(src: []const u8, start_offset: usize): GifBitReader {
        return GifBitReader{
            src: src,
            i: start_offset,
            block: 0,
            acc: 0,
            nbits: 0,
            eof: false,
        };
    }

    fn readBits(*_, n: u32): u16? {
        while (_.nbits < n && !_.eof) {
            if (_.block == 0) {
                if (_.i >= _.src.len) {
                    _.eof = true;
                    break;
                }
                let blk_len = _.src[_.i] as usize;
                _.i += 1;
                if (blk_len == 0) {
                    _.eof = true;
                    break;
                }
                _.block = blk_len;
            }
            if (_.i >= _.src.len) {
                _.eof = true;
                break;
            }
            let byte_val: u64 = _.src[_.i] as u64;
            let shift: u6 = _.nbits as u6;
            _.acc = _.acc | (byte_val << shift);
            _.i += 1;
            _.block -= 1;
            _.nbits += 8;
        }
        if (_.nbits < n) {
            return null;
        }
        let shift_n: u6 = n as u6;
        let mask: u64 = ((1 as u64) << shift_n) - 1;
        let v: u16 = (_.acc & mask) as u16;
        _.acc = _.acc >> shift_n;
        _.nbits -= n;
        return v;
    }

    /// 跳过当前图像剩余的所有 LZW 子块，返回紧随 0x00 终止符之后的字节偏移
    fn drain(*_): usize {
        if (_.eof) {
            return _.i;
        }
        let idx = _.i;
        if (_.block > 0) {
            let rem = _.src.len - idx;
            let step = if (_.block < rem) _.block else rem;
            idx += step;
            _.block = 0;
        }
        while (idx < _.src.len) {
            let n = _.src[idx] as usize;
            idx += 1;
            if (n == 0) {
                break;
            }
            let rem = _.src.len - idx;
            let step = if (n < rem) n else rem;
            idx += step;
            if (step < n) {
                break;
            }
        }
        _.i = idx;
        _.eof = true;
        return idx;
    }
}

/// GIF 子块 LSB-first 比特写入器（自动按 255 字节切分 GIF 子块）
struct GifBitWriter {
    acc: u64,
    nbits: u32,
    block_buf: [255]u8,
    block_len: usize,

    fn init(): GifBitWriter {
        return GifBitWriter{
            acc: 0,
            nbits: 0,
            block_buf: [0; 255],
            block_len: 0,
        };
    }

    fn flushBlock(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8)): void!GifError {
        if (_.block_len == 0) {
            return;
        }
        out.append(alloc, _.block_len as u8) catch return GifError.OutOfMemory;
        out.appendSlice(alloc, _.block_buf[0.._.block_len]) catch return GifError.OutOfMemory;
        _.block_len = 0;
    }

    fn writeBits(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8), code: u16, n: u32): void!GifError {
        let shift: u6 = _.nbits as u6;
        _.acc = _.acc | ((code as u64) << shift);
        _.nbits += n;
        while (_.nbits >= 8) {
            _.block_buf[_.block_len] = (_.acc & 0xFF) as u8;
            _.block_len += 1;
            _.acc = _.acc >> 8;
            _.nbits -= 8;
            if (_.block_len == 255) {
                try _.flushBlock(alloc, out);
            }
        }
    }

    fn finish(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8)): void!GifError {
        if (_.nbits > 0) {
            _.block_buf[_.block_len] = (_.acc & 0xFF) as u8;
            _.block_len += 1;
            _.acc = 0;
            _.nbits = 0;
        }
        try _.flushBlock(alloc, out);
        out.append(alloc, 0x00) catch return GifError.OutOfMemory;
    }
}

/// 内部单帧解码结果（含下一块偏移量）
struct DecodedFrameInternal {
    frame: GifFrame,
    next_offset: usize,
}

/// 内部小端 u16 读取
fn readU16Le(bytes: []const u8, off: usize): u16 {
    return (bytes[off] as u16) | ((bytes[off + 1] as u16) << 8);
}

/// 内部小端 u16 追加
fn appendU16Le(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: u16): void!GifError {
    out.append(alloc, (val & 0xFF) as u8) catch return GifError.OutOfMemory;
    out.append(alloc, ((val >> 8) & 0xFF) as u8) catch return GifError.OutOfMemory;
}

/// 解析 Disposal Method 原始位字段
fn parseDisposal(raw: u8): GifDisposal {
    return switch (raw) {
        0 => GifDisposal.Unspecified,
        1 => GifDisposal.Keep,
        2 => GifDisposal.RestoreBackground,
        3 => GifDisposal.RestorePrevious,
        else => GifDisposal.Unspecified,
    };
}

/// 计算调色板尺寸对应的 3-bit 指数编码 (2^(code+1) >= num_colors)
fn paletteSizeCode(num_colors: usize): u8!GifError {
    if (num_colors == 0 || num_colors > 256) {
        return GifError.InvalidColorTable;
    }
    let code: u8 = 0;
    let cap: usize = 2;
    while (cap < num_colors) : (code += 1) {
        cap = cap * 2;
    }
    return code;
}

/// 计算给定调色板颜色数对应的 LZW 最小码宽 (2..8)
fn minCodeSizeForColors(num_colors: usize): u8 {
    let bits: u8 = 2;
    let cap: usize = 4;
    while (cap < num_colors && bits < 8) {
        cap = cap * 2;
        bits += 1;
    }
    return bits;
}

/// 写入 2^(size_code+1) 项 RGB 调色板（不足补 0）
fn writeColorTable(alloc: std.mem.Allocator, out: *std.ArrayList(u8), palette_rgb: []const u8, size_code: u8): void!GifError {
    let table_colors: usize = (1 as usize) << ((size_code + 1) as u6);
    let table_bytes = table_colors * 3;
    let copy_len = if (palette_rgb.len < table_bytes) palette_rgb.len else table_bytes;
    if (copy_len > 0) {
        out.appendSlice(alloc, palette_rgb[0..copy_len]) catch return GifError.OutOfMemory;
    }
    let pad = table_bytes - copy_len;
    let p: usize = 0;
    while (p < pad) : (p += 1) {
        out.append(alloc, 0) catch return GifError.OutOfMemory;
    }
}

/// LZW 编码器：将调色板索引流压缩为 GIF 子块序列（含初始 min_code_size 与 0x00 终止符）
fn lzwEncodeSubBlocks(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    indices: []const u8,
    min_code_size_in: u8,
): void!GifError {
    let min_code: u32 = if (min_code_size_in < 2) 2 else if (min_code_size_in > 8) 8 else (min_code_size_in as u32);
    out.append(alloc, min_code as u8) catch return GifError.OutOfMemory;

    let clear: u16 = (1 as u16) << (min_code as u4);
    let eoi: u16 = clear + 1;
    let code_size: u32 = min_code + 1;
    let avail: u16 = eoi + 1;
    let max_symbol: u8 = (clear - 1) as u8;

    let hash_head = alloc.alloc(i16, LZW_HASH_BUCKETS) catch return GifError.OutOfMemory;
    defer alloc.free(hash_head);
    let hash_next = alloc.alloc(i16, LZW_MAX_CODES) catch return GifError.OutOfMemory;
    defer alloc.free(hash_next);
    let entry_prefix = alloc.alloc(u16, LZW_MAX_CODES) catch return GifError.OutOfMemory;
    defer alloc.free(entry_prefix);
    let entry_suffix = alloc.alloc(u8, LZW_MAX_CODES) catch return GifError.OutOfMemory;
    defer alloc.free(entry_suffix);

    @memset(hash_head, -1);

    let writer = GifBitWriter.init();
    try writer.writeBits(alloc, out, clear, code_size);

    if (indices.len == 0) {
        try writer.writeBits(alloc, out, eoi, code_size);
        try writer.finish(alloc, out);
        return;
    }

    let prefix: u16 = (indices[0] & max_symbol) as u16;
    let idx: usize = 1;
    while (idx < indices.len) : (idx += 1) {
        let k: u8 = indices[idx] & max_symbol;
        let bucket: usize = ((((prefix as usize) << 5) ^ (k as usize) ^ ((prefix as usize) >> 3)) & (LZW_HASH_BUCKETS - 1));

        let found: i32 = -1;
        let cur: i16 = hash_head[bucket];
        while (cur >= 0) {
            let cu = cur as usize;
            if (entry_prefix[cu] == prefix && entry_suffix[cu] == k) {
                found = cur as i32;
                break;
            }
            cur = hash_next[cu];
        }

        if (found >= 0) {
            prefix = found as u16;
        } else {
            try writer.writeBits(alloc, out, prefix, code_size);
            if ((avail as usize) < LZW_MAX_CODES) {
                let limit_code: u16 = (1 as u16) << (code_size as u4);
                if (avail == limit_code && code_size < 12) {
                    code_size += 1;
                }
                let au = avail as usize;
                entry_prefix[au] = prefix;
                entry_suffix[au] = k;
                hash_next[au] = hash_head[bucket];
                hash_head[bucket] = avail as i16;
                avail += 1;
            } else {
                // 字典满，发送 Clear Code 并重置
                try writer.writeBits(alloc, out, clear, code_size);
                @memset(hash_head, -1);
                code_size = min_code + 1;
                avail = eoi + 1;
            }
            prefix = k as u16;
        }
    }

    try writer.writeBits(alloc, out, prefix, code_size);
    try writer.writeBits(alloc, out, eoi, code_size);
    try writer.finish(alloc, out);
}

/// 纯 CP 工业级 GIF87a/GIF89a 编解码引擎
pub class Gif {
    static fn getTableSize(idx: usize): usize {
        return switch (idx) {
            0 => 2,
            1 => 4,
            2 => 8,
            3 => 16,
            4 => 32,
            5 => 64,
            6 => 128,
            7 => 256,
            else => 0,
        };
    }

    /// 16-lane SIMD 加速字节切片拷贝（0 误差）
    pub static fn simdCopyBytes(dst: []u8, src: []const u8) {
        let n = if (dst.len < src.len) dst.len else src.len;
        let i: usize = 0;
        while (i + 16 <= n) {
            let v: Vec16u8 = src[i .. i + 16][0..16].*;
            dst[i .. i + 16][0..16].* = v;
            i += 16;
        }
        while (i < n) {
            dst[i] = src[i];
            i += 1;
        }
    }

    /// 16-lane SIMD 加速矩形区域背景清零/填充（越界自动裁剪）
    pub static fn clearRectSimd(
        canvas_rgba: []u8,
        canvas_w: usize,
        canvas_h: usize,
        left: usize,
        top: usize,
        w: usize,
        h: usize,
        r: u8,
        g: u8,
        b: u8,
        a: u8,
    ) {
        if (canvas_w == 0 || canvas_h == 0 || left >= canvas_w || top >= canvas_h) {
            return;
        }
        let max_w = canvas_w - left;
        let max_h = canvas_h - top;
        let cw = if (w < max_w) w else max_w;
        let ch = if (h < max_h) h else max_h;
        if (cw == 0 || ch == 0) {
            return;
        }
        let row_bytes = cw * 4;
        let fill_vec: Vec16u8 = [r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a];

        let y: usize = 0;
        while (y < ch) : (y += 1) {
            let base = ((top + y) * canvas_w + left) * 4;
            if (base + row_bytes > canvas_rgba.len) {
                break;
            }
            let x: usize = 0;
            while (x + 16 <= row_bytes) {
                canvas_rgba[base + x .. base + x + 16][0..16].* = fill_vec;
                x += 16;
            }
            while (x + 4 <= row_bytes) {
                canvas_rgba[base + x + 0] = r;
                canvas_rgba[base + x + 1] = g;
                canvas_rgba[base + x + 2] = b;
                canvas_rgba[base + x + 3] = a;
                x += 4;
            }
        }
    }

    /// 16-lane SIMD 加速调色板索引批量展开至 RGBA8（与 image-rs/image-gif 像素语义完全一致）
    pub static fn expandPaletteToRgbaSimd(
        indices: []const u8,
        ct: []const u8,
        trns_idx: i32,
        out_rgba: []u8,
    ) {
        let npix = indices.len;
        if (npix == 0 || out_rgba.len < npix * 4) {
            return;
        }

        // 预烘焙 256 项 RGBA8 查找表（1024 字节，越界索引默认 [0, 0, 0, 0]）
        let lut: [1024]u8 = [0; 1024];
        let num_entries = ct.len / 3;
        let max_entries = if (num_entries < 256) num_entries else 256;
        let ci: usize = 0;
        while (ci < max_entries) : (ci += 1) {
            let co = ci * 3;
            let lo = ci * 4;
            lut[lo + 0] = ct[co + 0];
            lut[lo + 1] = ct[co + 1];
            lut[lo + 2] = ct[co + 2];
            lut[lo + 3] = if (trns_idx >= 0 && (ci as i32) == trns_idx) 0 else 255;
        }

        // 每次 4 像素 = 16 字节，通过 Vec16u8 一次性 128-bit 向量写入
        let p: usize = 0;
        while (p + 4 <= npix) {
            let o0 = (indices[p + 0] as usize) * 4;
            let o1 = (indices[p + 1] as usize) * 4;
            let o2 = (indices[p + 2] as usize) * 4;
            let o3 = (indices[p + 3] as usize) * 4;
            let v: Vec16u8 = [
                lut[o0 + 0], lut[o0 + 1], lut[o0 + 2], lut[o0 + 3],
                lut[o1 + 0], lut[o1 + 1], lut[o1 + 2], lut[o1 + 3],
                lut[o2 + 0], lut[o2 + 1], lut[o2 + 2], lut[o2 + 3],
                lut[o3 + 0], lut[o3 + 1], lut[o3 + 2], lut[o3 + 3],
            ];
            let dst = p * 4;
            out_rgba[dst .. dst + 16][0..16].* = v;
            p += 4;
        }
        while (p < npix) : (p += 1) {
            let lo = (indices[p] as usize) * 4;
            let dst = p * 4;
            out_rgba[dst + 0] = lut[lo + 0];
            out_rgba[dst + 1] = lut[lo + 1];
            out_rgba[dst + 2] = lut[lo + 2];
            out_rgba[dst + 3] = lut[lo + 3];
        }
    }

    /// 标量参考实现：单行透明掩码帧合成（用于测试 0 误差对拍）
    pub static fn compositeRowScalar(dst_row: []u8, src_row: []const u8) {
        let n = if (dst_row.len < src_row.len) dst_row.len else src_row.len;
        let i: usize = 0;
        while (i + 4 <= n) : (i += 4) {
            if (src_row[i + 3] != 0) {
                dst_row[i + 0] = src_row[i + 0];
                dst_row[i + 1] = src_row[i + 1];
                dst_row[i + 2] = src_row[i + 2];
                dst_row[i + 3] = src_row[i + 3];
            }
        }
    }

    /// 16-lane SIMD 无分支透明掩码单行合成（src Alpha != 0 覆盖，Alpha == 0 保留底图）
    pub static fn compositeRowSimd(dst_row: []u8, src_row: []const u8) {
        let n = if (dst_row.len < src_row.len) dst_row.len else src_row.len;
        const zero_vec: Vec16u8 = @splat(0);
        const alpha_mask_idx: [16]i32 = [3, 3, 3, 3, 7, 7, 7, 7, 11, 11, 11, 11, 15, 15, 15, 15];

        let i: usize = 0;
        while (i + 16 <= n) {
            let vd: Vec16u8 = dst_row[i .. i + 16][0..16].*;
            let vs: Vec16u8 = src_row[i .. i + 16][0..16].*;
            let alpha_lanes: Vec16u8 = @shuffle(u8, vs, zero_vec, alpha_mask_idx);
            let is_opaque = alpha_lanes != zero_vec;
            let vo: Vec16u8 = @select(u8, is_opaque, vs, vd);
            dst_row[i .. i + 16][0..16].* = vo;
            i += 16;
        }
        while (i + 4 <= n) : (i += 4) {
            if (src_row[i + 3] != 0) {
                dst_row[i + 0] = src_row[i + 0];
                dst_row[i + 1] = src_row[i + 1];
                dst_row[i + 2] = src_row[i + 2];
                dst_row[i + 3] = src_row[i + 3];
            }
        }
    }

    /// 将单个子帧按 left/top/w/h 越界安全裁剪并合成到主画布（使用 16-lane SIMD）
    pub static fn composeCanvasFrame(
        canvas_rgba: []u8,
        canvas_w: usize,
        canvas_h: usize,
        frame: GifFrame,
    ) {
        if (canvas_w == 0 || canvas_h == 0 || frame.width == 0 || frame.height == 0) {
            return;
        }
        if (frame.left >= canvas_w || frame.top >= canvas_h) {
            return;
        }
        let max_w = canvas_w - frame.left;
        let max_h = canvas_h - frame.top;
        let copy_w = if (frame.width < max_w) frame.width else max_w;
        let copy_h = if (frame.height < max_h) frame.height else max_h;
        let copy_bytes = copy_w * 4;

        let y: usize = 0;
        while (y < copy_h) : (y += 1) {
            let src_off = y * frame.width * 4;
            let dst_off = ((frame.top + y) * canvas_w + frame.left) * 4;
            if (src_off + copy_bytes > frame.rgba.len || dst_off + copy_bytes > canvas_rgba.len) {
                break;
            }
            Gif.compositeRowSimd(
                canvas_rgba[dst_off .. dst_off + copy_bytes],
                frame.rgba[src_off .. src_off + copy_bytes],
            );
        }
    }

    /// 严格探测 GIF87a/GIF89a 逻辑屏幕描述符
    pub static fn probeStrict(bytes: []const u8): GifHeader!GifError {
        if (bytes.len < 13) {
            return GifError.TruncatedStream;
        }
        let is_gif87 = bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == '8' && bytes[4] == '7' && bytes[5] == 'a';
        let is_gif89 = bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == '8' && bytes[4] == '9' && bytes[5] == 'a';
        if (!is_gif87 && !is_gif89) {
            return GifError.InvalidSignature;
        }

        let w = readU16Le(bytes, 6) as usize;
        let h = readU16Le(bytes, 8) as usize;
        let lsd_packed = bytes[10];
        let bg_color_index = bytes[11];
        let has_gct = (lsd_packed & 0x80) != 0;
        let gct_size: usize = if (has_gct) Gif.getTableSize((lsd_packed & 7) as usize) else 0;

        return GifHeader{
            width: w,
            height: h,
            has_gct: has_gct,
            gct_size: gct_size,
            bg_color_index: bg_color_index,
            is_gif89a: is_gif89,
        };
    }

    /// 快速探测 GIF 头与全局屏幕描述符（100% 向后兼容）
    pub static fn probe(bytes: []const u8): GifHeader? {
        const res = Gif.probeStrict(bytes) catch return null;
        return res;
    }

    /// 严格解码 GIF 完整动画（支持多帧、GCE、NETSCAPE2.0、LCT 覆盖、4-Pass 隔行扫描）
    pub static fn decodeAnimation(bytes: []const u8): GifAnimation!GifError {
        const hdr = try Gif.probeStrict(bytes);
        let gct_bytes: usize = hdr.gct_size * 3;
        let start_offset: usize = (13 as usize) + gct_bytes;
        if (start_offset > bytes.len) {
            return GifError.TruncatedStream;
        }

        const alloc = std.heap.page_allocator;
        let gct_slice: []const u8 = if (hdr.has_gct) bytes[13..start_offset] else bytes[0..0];

        let global_palette: []u8 = [];
        if (gct_slice.len > 0) {
            global_palette = alloc.alloc(u8, gct_slice.len) catch return GifError.OutOfMemory;
            Gif.simdCopyBytes(global_palette, gct_slice);
        }

        let frames_list: std.ArrayList(GifFrame) = std.ArrayList(GifFrame).empty;
        let cleanup_on_err = true;
        defer {
            if (cleanup_on_err) {
                let ci: usize = 0;
                while (ci < frames_list.items.len) : (ci += 1) {
                    frames_list.items[ci].deinit();
                }
                frames_list.deinit(alloc);
                if (global_palette.len > 0) {
                    alloc.free(global_palette);
                }
            }
        }

        let loop_count: u16 = 1;
        let trns_idx: i32 = -1;
        let delay_cs: u16 = 0;
        let disposal: GifDisposal = GifDisposal.Unspecified;
        let offset = start_offset;

        while (offset < bytes.len) {
            let marker = bytes[offset];
            if (marker == 0x3B) {
                // Trailer
                break;
            }

            if (marker == 0x21) {
                // Extension Introducer
                if (offset + 2 > bytes.len) {
                    if (frames_list.items.len > 0) {
                        break;
                    }
                    return GifError.TruncatedStream;
                }
                let label = bytes[offset + 1];
                offset += 2;

                if (label == 0xF9) {
                    // Graphic Control Extension (GIF89a §23)
                    if (offset >= bytes.len) {
                        return GifError.TruncatedStream;
                    }
                    let blk_size = bytes[offset] as usize;
                    if (blk_size == 4 && offset + 5 <= bytes.len) {
                        let gce_packed = bytes[offset + 1];
                        disposal = parseDisposal((gce_packed >> 2) & 0x07);
                        delay_cs = readU16Le(bytes, offset + 2);
                        if ((gce_packed & 1) != 0) {
                            trns_idx = bytes[offset + 4] as i32;
                        } else {
                            trns_idx = -1;
                        }
                    }
                    offset = Gif.skipSubBlocks(bytes, offset);
                } else if (label == 0xFF) {
                    // Application Extension (检测 NETSCAPE2.0 / ANIMEXTS1.0 循环计数)
                    if (offset < bytes.len) {
                        let app_len = bytes[offset] as usize;
                        if (app_len == 11 && offset + 12 <= bytes.len) {
                            let app_id = bytes[offset + 1 .. offset + 12];
                            let is_netscape = Gif.bytesEq(app_id, "NETSCAPE2.0") || Gif.bytesEq(app_id, "ANIMEXTS1.0");
                            let sub_off = offset + 12;
                            if (is_netscape && sub_off < bytes.len) {
                                let sub_len = bytes[sub_off] as usize;
                                if (sub_len >= 3 && sub_off + 1 + sub_len <= bytes.len && bytes[sub_off + 1] == 1) {
                                    loop_count = readU16Le(bytes, sub_off + 2);
                                }
                            }
                        }
                    }
                    offset = Gif.skipSubBlocks(bytes, offset);
                } else {
                    // Comment (0xFE), Plain Text (0x01) 或其他扩展：跳过全部子块
                    offset = Gif.skipSubBlocks(bytes, offset);
                }
            } else if (marker == 0x2C) {
                // Image Descriptor
                if (frames_list.items.len >= GIF_MAX_FRAMES) {
                    return GifError.TooManyFrames;
                }
                if (offset + 10 > bytes.len) {
                    return GifError.TruncatedStream;
                }
                let left = readU16Le(bytes, offset + 1) as usize;
                let top = readU16Le(bytes, offset + 3) as usize;
                let w = readU16Le(bytes, offset + 5) as usize;
                let h = readU16Le(bytes, offset + 7) as usize;
                let ipacked = bytes[offset + 9];
                let interlace = (ipacked & 0x40) != 0;
                let has_lct = (ipacked & 0x80) != 0;
                let lct_size: usize = if (has_lct) Gif.getTableSize((ipacked & 7) as usize) else 0;
                offset += 10;

                if (w == 0 || h == 0) {
                    return GifError.InvalidFrameDimensions;
                }
                let npix_u64 = (w as u64) * (h as u64);
                if (npix_u64 > GIF_MAX_PIXELS) {
                    return GifError.ImageTooLarge;
                }

                let ct: []const u8 = gct_slice;
                if (has_lct) {
                    let lct_bytes = lct_size * 3;
                    if (offset + lct_bytes > bytes.len) {
                        return GifError.TruncatedStream;
                    }
                    ct = bytes[offset .. offset + lct_bytes];
                    offset += lct_bytes;
                }

                if (ct.len == 0) {
                    return GifError.MissingColorTable;
                }
                if (offset >= bytes.len) {
                    return GifError.TruncatedStream;
                }

                let min_code = bytes[offset] as u32;
                offset += 1;

                const decoded = try Gif.decodeFrameInternal(
                    bytes,
                    offset,
                    left,
                    top,
                    w,
                    h,
                    interlace,
                    ct,
                    min_code,
                    trns_idx,
                    delay_cs,
                    disposal,
                );
                offset = decoded.next_offset;
                frames_list.append(alloc, decoded.frame) catch {
                    let mut_f = decoded.frame;
                    mut_f.deinit();
                    return GifError.OutOfMemory;
                };

                // GCE 仅作用于紧随其后的一帧，消费后重置
                trns_idx = -1;
                delay_cs = 0;
                disposal = GifDisposal.Unspecified;
            } else if (marker == 0x00) {
                // 容错跳过偶发的额外 0x00 填充字节（常见于部分野样本）
                offset += 1;
            } else {
                if (frames_list.items.len > 0) {
                    break;
                }
                return GifError.UnknownBlock;
            }
        }

        if (frames_list.items.len == 0) {
            return GifError.NoFrames;
        }

        const owned_frames = frames_list.toOwnedSlice(alloc) catch return GifError.OutOfMemory;
        cleanup_on_err = false;
        return GifAnimation{
            width: hdr.width,
            height: hdr.height,
            bg_color_index: hdr.bg_color_index,
            loop_count: loop_count,
            has_gct: hdr.has_gct,
            global_palette: global_palette,
            frames: owned_frames,
        };
    }

    /// 严格解码 GIF 首帧为 RGBA8（返回具体错误码）
    pub static fn decodeStrict(bytes: []const u8): GifImage!GifError {
        let anim = try Gif.decodeAnimation(bytes);
        defer anim.deinit();

        const f = anim.frames[0];
        let rgba_copy = std.heap.page_allocator.alloc(u8, f.rgba.len) catch return GifError.OutOfMemory;
        Gif.simdCopyBytes(rgba_copy, f.rgba);
        return GifImage{
            width: f.width,
            height: f.height,
            rgba: rgba_copy,
        };
    }

    /// 解码 GIF 首帧为完整 RGBA8 像素矩阵（100% 向后兼容）
    pub static fn decode(bytes: []const u8): GifImage? {
        const img = Gif.decodeStrict(bytes) catch return null;
        return img;
    }

    /// 编码单帧或多帧调色板索引图为标准 GIF89a 字节流（完整 LZW 编码器）
    pub static fn encodeIndexed(
        width: usize,
        height: usize,
        global_palette: []const u8,
        frames: []const GifEncodeFrame,
        loop_count: u16,
    ): ([]u8)!GifError {
        if (width == 0 || height == 0 || width > 65535 || height > 65535) {
            return GifError.InvalidHeader;
        }
        if (frames.len == 0) {
            return GifError.NoFrames;
        }
        if (frames.len > GIF_MAX_FRAMES) {
            return GifError.TooManyFrames;
        }

        let has_gct = global_palette.len >= 3;
        let gct_colors = global_palette.len / 3;
        if (has_gct && (global_palette.len % 3 != 0 || gct_colors > 256)) {
            return GifError.InvalidColorTable;
        }

        const alloc = std.heap.page_allocator;
        let out: std.ArrayList(u8) = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        // 1. Header: GIF89a
        out.appendSlice(alloc, "GIF89a") catch return GifError.OutOfMemory;

        // 2. Logical Screen Descriptor
        try appendU16Le(alloc, &out, width as u16);
        try appendU16Le(alloc, &out, height as u16);
        let gct_size_code: u8 = if (has_gct) try paletteSizeCode(gct_colors) else 0;
        let lsd_packed: u8 = if (has_gct) ((0x80 as u8) | (gct_size_code << 4) | gct_size_code) else (0x70 as u8);
        out.append(alloc, lsd_packed) catch return GifError.OutOfMemory;
        out.append(alloc, 0) catch return GifError.OutOfMemory; // bg color index
        out.append(alloc, 0) catch return GifError.OutOfMemory; // pixel aspect ratio

        // 3. Global Color Table
        if (has_gct) {
            try writeColorTable(alloc, &out, global_palette, gct_size_code);
        }

        // 4. NETSCAPE2.0 Application Extension (多帧或显式指定循环时写入)
        if (frames.len > 1 || loop_count == 0) {
            const netscape_hdr: [16]u8 = [
                0x21, 0xFF, 0x0B,
                'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
                0x03, 0x01,
            ];
            out.appendSlice(alloc, netscape_hdr[0..]) catch return GifError.OutOfMemory;
            try appendU16Le(alloc, &out, loop_count);
            out.append(alloc, 0x00) catch return GifError.OutOfMemory;
        }

        // 5. 逐帧写入 GCE + Image Descriptor + LCT + LZW Image Data
        let fi: usize = 0;
        while (fi < frames.len) : (fi += 1) {
            const f = frames[fi];
            if (f.width == 0 || f.height == 0 || f.width > 65535 || f.height > 65535) {
                return GifError.InvalidFrameDimensions;
            }
            if (f.left > 65535 || f.top > 65535) {
                return GifError.InvalidFrameDimensions;
            }
            let expected_pix = f.width * f.height;
            if (f.indices.len < expected_pix) {
                return GifError.InvalidBufferSize;
            }

            let has_lct = f.local_palette.len >= 3;
            let lct_colors = f.local_palette.len / 3;
            if (has_lct && (f.local_palette.len % 3 != 0 || lct_colors > 256)) {
                return GifError.InvalidColorTable;
            }
            if (!has_gct && !has_lct) {
                return GifError.MissingColorTable;
            }

            let active_colors = if (has_lct) lct_colors else gct_colors;

            // Graphic Control Extension
            let has_trns = f.transparent_index >= 0 && f.transparent_index <= 255;
            if (has_trns || f.delay_cs > 0 || f.disposal != GifDisposal.Unspecified || frames.len > 1) {
                let trns_bit: u8 = if (has_trns) 1 else 0;
                let gce_flags: u8 = ((f.disposal as u8) << 2) | trns_bit;
                out.append(alloc, 0x21) catch return GifError.OutOfMemory;
                out.append(alloc, 0xF9) catch return GifError.OutOfMemory;
                out.append(alloc, 0x04) catch return GifError.OutOfMemory;
                out.append(alloc, gce_flags) catch return GifError.OutOfMemory;
                try appendU16Le(alloc, &out, f.delay_cs);
                out.append(alloc, if (has_trns) (f.transparent_index as u8) else 0) catch return GifError.OutOfMemory;
                out.append(alloc, 0x00) catch return GifError.OutOfMemory;
            }

            // Image Descriptor
            out.append(alloc, 0x2C) catch return GifError.OutOfMemory;
            try appendU16Le(alloc, &out, f.left as u16);
            try appendU16Le(alloc, &out, f.top as u16);
            try appendU16Le(alloc, &out, f.width as u16);
            try appendU16Le(alloc, &out, f.height as u16);
            let lct_size_code: u8 = if (has_lct) try paletteSizeCode(lct_colors) else 0;
            let img_packed: u8 = if (has_lct) ((0x80 as u8) | lct_size_code) else 0;
            out.append(alloc, img_packed) catch return GifError.OutOfMemory;

            if (has_lct) {
                try writeColorTable(alloc, &out, f.local_palette, lct_size_code);
            }

            let min_code_size = minCodeSizeForColors(active_colors);
            try lzwEncodeSubBlocks(alloc, &out, f.indices[0..expected_pix], min_code_size);
        }

        // 6. Trailer
        out.append(alloc, 0x3B) catch return GifError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return GifError.OutOfMemory;
    }

    /// 编码 RGBA8 像素缓冲为标准 GIF89a（自动精确提取 <=256 色调色板或量化，支持 1-bit 透明色）
    pub static fn encodeRgba(rgba: []const u8, width: usize, height: usize): ([]u8)!GifError {
        if (width == 0 || height == 0 || width > 65535 || height > 65535) {
            return GifError.InvalidHeader;
        }
        let npix_u64 = (width as u64) * (height as u64);
        if (npix_u64 > GIF_MAX_PIXELS) {
            return GifError.ImageTooLarge;
        }
        let npix = npix_u64 as usize;
        if (rgba.len < npix * 4) {
            return GifError.InvalidBufferSize;
        }

        const alloc = std.heap.page_allocator;
        let q = Quantizer.quantize(rgba, width as u32, height as u32, 256);
        let num_colors = q.colors as usize;
        if (num_colors == 0) {
            return GifError.InvalidColorTable;
        }

        // 将 RGBA 调色板转换为 RGB 调色板，并检测首个透明色索引 (alpha == 0)
        let pal_rgb = alloc.alloc(u8, num_colors * 3) catch return GifError.OutOfMemory;
        defer alloc.free(pal_rgb);
        let trns_idx: i32 = -1;

        let ci: usize = 0;
        while (ci < num_colors) : (ci += 1) {
            let po = ci * 4;
            pal_rgb[ci * 3 + 0] = q.palette[po + 0];
            pal_rgb[ci * 3 + 1] = q.palette[po + 1];
            pal_rgb[ci * 3 + 2] = q.palette[po + 2];
            if (q.palette[po + 3] == 0 && trns_idx < 0) {
                trns_idx = ci as i32;
            }
        }

        // 若存在多个透明调色板项，统一映射到首个透明索引，保证 1-bit 透明语义严格还原
        if (trns_idx >= 0) {
            let p: usize = 0;
            while (p < npix) : (p += 1) {
                let idx = q.indices[p] as usize;
                if (idx < num_colors && q.palette[idx * 4 + 3] == 0) {
                    q.indices[p] = trns_idx as u8;
                }
            }
        }

        let frames: [1]GifEncodeFrame = [
            GifEncodeFrame{
                left: 0,
                top: 0,
                width: width,
                height: height,
                delay_cs: 0,
                disposal: GifDisposal.Keep,
                transparent_index: trns_idx,
                local_palette: "",
                indices: q.indices[0..npix],
            },
        ];
        return Gif.encodeIndexed(width, height, pal_rgb, frames[0..], 1);
    }

    static fn bytesEq(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) {
            return false;
        }
        let i: usize = 0;
        while (i < a.len) : (i += 1) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }

    /// 跳过从当前偏移开始的 GIF 子块序列，返回 0x00 之后的偏移
    static fn skipSubBlocks(bytes: []const u8, start: usize): usize {
        let off = start;
        while (off < bytes.len) {
            let n = bytes[off] as usize;
            off += 1;
            if (n == 0) {
                break;
            }
            let rem = bytes.len - off;
            let step = if (n < rem) n else rem;
            off += step;
            if (step < n) {
                break;
            }
        }
        return off;
    }

    static fn decodeFrameInternal(
        bytes: []const u8,
        lzw_offset: usize,
        left: usize,
        top: usize,
        w: usize,
        h: usize,
        interlace: bool,
        ct: []const u8,
        min_code_in: u32,
        trns_idx: i32,
        delay_cs: u16,
        disposal: GifDisposal,
    ): DecodedFrameInternal!GifError {
        let npix = w * h;
        if (npix == 0) {
            return GifError.InvalidFrameDimensions;
        }
        // 对标 image-rs/image-gif (LzwReader::check_code_size)：允许 min_code_size 为 1..11（兼容 1-bit 野样本如 set_hsts.gif）
        if (min_code_in < 1 || min_code_in > 11) {
            return GifError.InvalidLzwCodeSize;
        }
        let min_code = min_code_in;

        let clear: u16 = (1 as u16) << (min_code as u4);
        let eoi: u16 = clear + 1;
        let code_size: u32 = min_code + 1;
        let avail: u16 = eoi + 1;
        let prev_code: i32 = -1;

        const alloc = std.heap.page_allocator;
        let prefix = alloc.alloc(u16, LZW_MAX_CODES) catch return GifError.OutOfMemory;
        defer alloc.free(prefix);
        let suffix = alloc.alloc(u8, LZW_MAX_CODES) catch return GifError.OutOfMemory;
        defer alloc.free(suffix);
        let scratch = alloc.alloc(u8, LZW_MAX_CODES) catch return GifError.OutOfMemory;
        defer alloc.free(scratch);

        let raw_idx = alloc.alloc(u8, npix) catch return GifError.OutOfMemory;
        let keep_raw = !interlace;
        defer {
            if (!keep_raw) {
                alloc.free(raw_idx);
            }
        }
        @memset(raw_idx, 0);

        let written: usize = 0;
        let bits = GifBitReader.init(bytes, lzw_offset);

        while (written < npix) {
            let code_opt = bits.readBits(code_size);
            if (code_opt == null) {
                break;
            }
            let code = code_opt.?;

            if (code == clear) {
                code_size = min_code + 1;
                avail = eoi + 1;
                prev_code = -1;
                continue;
            }
            if (code == eoi) {
                break;
            }

            let first_byte: u8 = 0;
            if (code < avail) {
                let curr = code;
                let n: usize = 0;
                while (curr >= clear && n < LZW_MAX_CODES - 1) {
                    if (curr >= avail) {
                        break;
                    }
                    scratch[n] = suffix[curr as usize];
                    curr = prefix[curr as usize];
                    n += 1;
                }
                scratch[n] = (curr & 0xFF) as u8;
                n += 1;
                first_byte = scratch[n - 1];

                let cap = if (n < (npix - written)) n else (npix - written);
                let k: usize = 0;
                while (k < cap) : (k += 1) {
                    raw_idx[written + k] = scratch[n - 1 - k];
                }
                written += cap;
            } else if (code == avail && prev_code >= 0) {
                // KwKwK 特殊情况
                let curr = prev_code as u16;
                let n: usize = 0;
                while (curr >= clear && n < LZW_MAX_CODES - 1) {
                    if (curr >= avail) {
                        break;
                    }
                    scratch[n] = suffix[curr as usize];
                    curr = prefix[curr as usize];
                    n += 1;
                }
                scratch[n] = (curr & 0xFF) as u8;
                n += 1;
                first_byte = scratch[n - 1];

                let cap = if (n < (npix - written)) n else (npix - written);
                let k: usize = 0;
                while (k < cap) : (k += 1) {
                    raw_idx[written + k] = scratch[n - 1 - k];
                }
                written += cap;
                if (written < npix) {
                    raw_idx[written] = first_byte;
                    written += 1;
                }
            } else {
                // 非法超界码字：安全拦截并释放缓冲
                if (keep_raw) {
                    alloc.free(raw_idx);
                }
                return GifError.InvalidLzwCode;
            }

            // Deferred Clear Code 兼容：表满 (avail == 4096) 时停止新增表项但继续解码
            if (prev_code >= 0 && (avail as usize) < LZW_MAX_CODES) {
                prefix[avail as usize] = prev_code as u16;
                suffix[avail as usize] = first_byte;
                avail += 1;
                let limit_code: u16 = (1 as u16) << (code_size as u4);
                if (avail == limit_code && code_size < 12) {
                    code_size += 1;
                }
            }
            prev_code = code as i32;
        }

        let next_offset = bits.drain();

        // 若为 4-Pass 隔行扫描（0,8 / 4,8 / 2,4 / 1,2），去隔行还原为顺序行索引缓冲
        let final_indices: []u8 = raw_idx;
        if (interlace) {
            let deint = alloc.alloc(u8, npix) catch return GifError.OutOfMemory;
            @memset(deint, 0);
            let passes_start: [4]usize = [0, 4, 2, 1];
            let passes_step: [4]usize = [8, 8, 4, 2];
            let src_row: usize = 0;

            let pass: usize = 0;
            while (pass < 4) : (pass += 1) {
                let y = passes_start[pass];
                let step_y = passes_step[pass];
                while (y < h && src_row < h) {
                    let src_off = src_row * w;
                    let dst_off = y * w;
                    Gif.simdCopyBytes(deint[dst_off .. dst_off + w], raw_idx[src_off .. src_off + w]);
                    src_row += 1;
                    y += step_y;
                }
            }
            final_indices = deint;
        }

        // 16-lane SIMD 加速索引展开至 RGBA8
        let rgba_len = npix * 4;
        let rgba = alloc.alloc(u8, rgba_len) catch {
            alloc.free(final_indices);
            return GifError.OutOfMemory;
        };
        Gif.expandPaletteToRgbaSimd(final_indices, ct, trns_idx, rgba);

        return DecodedFrameInternal{
            frame: GifFrame{
                left: left,
                top: top,
                width: w,
                height: h,
                delay_cs: delay_cs,
                disposal: disposal,
                transparent_index: trns_idx,
                interlaced: interlace,
                rgba: rgba,
                indices: final_indices,
            },
            next_offset: next_offset,
        };
    }
}

/// 顶层便捷函数：编码 RGBA8 像素为 GIF89a 字节流
pub fn encodeGifRgba(rgba: []const u8, width: usize, height: usize): ([]u8)!GifError {
    return Gif.encodeRgba(rgba, width, height);
}

/// 顶层便捷函数：编码单帧或多帧调色板索引图为 GIF89a 字节流
pub fn encodeGifIndexed(
    width: usize,
    height: usize,
    global_palette: []const u8,
    frames: []const GifEncodeFrame,
    loop_count: u16,
): ([]u8)!GifError {
    return Gif.encodeIndexed(width, height, global_palette, frames, loop_count);
}

/// 顶层便捷函数：严格解码完整 GIF87a/GIF89a 动画
pub fn decodeGifAnimation(bytes: []const u8): GifAnimation!GifError {
    return Gif.decodeAnimation(bytes);
}

/// 顶层便捷函数：严格解码 GIF 首帧为 RGBA8
pub fn decodeGifStrict(bytes: []const u8): GifImage!GifError {
    return Gif.decodeStrict(bytes);
}

// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_jpeg.cp | Synced: 2026-11-22T06:39:08Z
// Reference-hardened against etemesi254/zune-image (MIT/Apache-2.0/Zlib)
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_jpeg.cp
// Extended with Production Baseline DCT JPEG (ITU-T T.81 / JFIF 1.01) Encoder
// and Zero-Heap SIMD-Accelerated Baseline DCT JPEG (SOF0) Decoder
// ============================================================================
const std = @import("std");
import { Codecs, ExifOrientation } from "./codecs";
import { Exif } from "./exif";

pub err JpegError {
    InvalidDimensions,
    InvalidBufferLength,
    BufferTooSmall,
    OutOfMemory,
    InvalidMarker,
    UnsupportedFormat,
    InvalidHuffman,
    InvalidQuantTable,
    InvalidSamplingFactor,
    InvalidScan,
    UnexpectedEof,
    DimensionTooLarge,
}

pub struct JpegHeader {
    pub width: usize,
    pub height: usize,
    pub components: usize,
    pub is_progressive: bool,
    pub orientation: ExifOrientation,
}

// ============================================================================
// ITU-T T.81 Annex K 标准表与 DCT 常量
// ============================================================================

/// 自然 8x8 顺序到 Zig-Zag 顺序的索引映射
const ZIGZAG: [64]usize = [
    0,  1,  8,  16, 9,  2,  3,  10,
    17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
];

/// ITU-T T.81 Annex K.1 标准亮度量化表（自然 8x8 顺序）
const STD_LUMA_QUANT: [64]u16 = [
    16, 11, 10, 16, 24,  40,  51,  61,
    12, 12, 14, 19, 26,  58,  60,  55,
    14, 13, 16, 24, 40,  57,  69,  56,
    14, 17, 22, 29, 51,  87,  80,  62,
    18, 22, 37, 56, 68,  109, 103, 77,
    24, 35, 55, 64, 81,  104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99,
];

/// ITU-T T.81 Annex K.2 标准色度量化表（自然 8x8 顺序）
const STD_CHROMA_QUANT: [64]u16 = [
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
];

/// Annex K.3.1 亮度 DC Huffman 码长计数（1..16 bits）与符号表
const STD_DC_LUMA_BITS: [16]u8 = [0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0];
const STD_DC_LUMA_VALS: [12]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];

/// Annex K.3.2 色度 DC Huffman 码长计数与符号表
const STD_DC_CHROMA_BITS: [16]u8 = [0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0];
const STD_DC_CHROMA_VALS: [12]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];

/// Annex K.3.3 亮度 AC Huffman 码长计数与符号表
const STD_AC_LUMA_BITS: [16]u8 = [0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125];
const STD_AC_LUMA_VALS: [162]u8 = [
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA,
];

/// Annex K.3.4 色度 AC Huffman 码长计数与符号表
const STD_AC_CHROMA_BITS: [16]u8 = [0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119];
const STD_AC_CHROMA_VALS: [162]u8 = [
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
    0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
    0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA,
];

/// 预计算 1D 8点正交 DCT 余弦矩阵: C[u*8 + x] = 0.5 * alpha(u) * cos((2x+1)*u*pi/16)
const DCT_COS: [64]f64 = [
    // u = 0: 0.5 / sqrt(2) = 0.35355339059327373
     0.35355339059327373,  0.35355339059327373,  0.35355339059327373,  0.35355339059327373,
     0.35355339059327373,  0.35355339059327373,  0.35355339059327373,  0.35355339059327373,
    // u = 1
     0.49039264020161522,  0.41573480615127262,  0.27778511650980114,  0.09754516100806417,
    -0.09754516100806410, -0.27778511650980109, -0.41573480615127262, -0.49039264020161522,
    // u = 2
     0.46193976625564337,  0.19134171618254492, -0.19134171618254486, -0.46193976625564337,
    -0.46193976625564342, -0.19134171618254500,  0.19134171618254478,  0.46193976625564337,
    // u = 3
     0.41573480615127262, -0.09754516100806410, -0.49039264020161522, -0.27778511650980120,
     0.27778511650980098,  0.49039264020161522,  0.09754516100806429, -0.41573480615127256,
    // u = 4
     0.35355339059327379, -0.35355339059327373, -0.35355339059327384,  0.35355339059327368,
     0.35355339059327384, -0.35355339059327362, -0.35355339059327390,  0.35355339059327357,
    // u = 5
     0.27778511650980114, -0.49039264020161522,  0.09754516100806399,  0.41573480615127278,
    -0.41573480615127256, -0.09754516100806441,  0.49039264020161516, -0.27778511650980137,
    // u = 6
     0.19134171618254492, -0.46193976625564342,  0.46193976625564326, -0.19134171618254528,
    -0.19134171618254467,  0.46193976625564320, -0.46193976625564353,  0.19134171618254434,
    // u = 7
     0.09754516100806417, -0.27778511650980120,  0.41573480615127278, -0.49039264020161516,
     0.49039264020161533, -0.41573480615127245,  0.27778511650980148, -0.09754516100806381,
];

/// 8-lane SIMD `@dot` 1D 正交 DCT 余弦基行向量 (`f32`)
const DCT_COS_F32: [64]f32 = [
     0.35355339,  0.35355339,  0.35355339,  0.35355339,  0.35355339,  0.35355339,  0.35355339,  0.35355339,
     0.49039264,  0.41573481,  0.27778512,  0.09754516, -0.09754516, -0.27778512, -0.41573481, -0.49039264,
     0.46193977,  0.19134172, -0.19134172, -0.46193977, -0.46193977, -0.19134172,  0.19134172,  0.46193977,
     0.41573481, -0.09754516, -0.49039264, -0.27778512,  0.27778512,  0.49039264,  0.09754516, -0.41573481,
     0.35355339, -0.35355339, -0.35355339,  0.35355339,  0.35355339, -0.35355339, -0.35355339,  0.35355339,
     0.27778512, -0.49039264,  0.09754516,  0.41573481, -0.41573481, -0.09754516,  0.49039264, -0.27778512,
     0.19134172, -0.46193977,  0.46193977, -0.19134172, -0.19134172,  0.46193977, -0.46193977,  0.19134172,
     0.09754516, -0.27778512,  0.41573481, -0.49039264,  0.49039264, -0.41573481,  0.27778512, -0.09754516,
];

struct HuffmanTable {
    codes: [256]u16,
    lens: [256]u8,
}

fn buildHuffmanTable(bits: []const u8, vals: []const u8): HuffmanTable {
    let tbl = HuffmanTable{
        codes: [0; 256],
        lens: [0; 256],
    };
    let code: u32 = 0;
    let val_idx: usize = 0;
    let bit_len: usize = 1;
    while (bit_len <= 16) : (bit_len += 1) {
        const count = bits[bit_len - 1] as usize;
        let j: usize = 0;
        while (j < count) : (j += 1) {
            if (val_idx < vals.len) {
                const sym = vals[val_idx] as usize;
                tbl.codes[sym] = code as u16;
                tbl.lens[sym] = bit_len as u8;
                val_idx += 1;
            }
            code += 1;
        }
        code <<= 1;
    }
    return tbl;
}

/// 按 IJG 质量缩放公式生成量化表（自然 8x8 顺序，钳位 1..255）
fn scaleQuantTable(base: [64]u16, quality: u8): [64]u8 {
    let q: u32 = quality as u32;
    if (q < 1) {
        q = 1;
    } else if (q > 100) {
        q = 100;
    }
    let scale: u32 = 0;
    if (q < 50) {
        const c5000: u32 = 5000;
        scale = c5000 / q;
    } else {
        const c200: u32 = 200;
        scale = c200 - q * 2;
    }
    let out: [64]u8 = [1; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        let v: u32 = ((base[i] as u32) * scale + 50) / 100;
        if (v < 1) {
            v = 1;
        } else if (v > 255) {
            v = 255;
        }
        out[i] = v as u8;
    }
    return out;
}

/// 统一字节输出汇（支持动态 ArrayList 或固定缓冲区零堆写入）
struct JpegByteSink {
    alloc: std.mem.Allocator?,
    list: *std.ArrayList(u8)?,
    fixed: []u8,
    pos: usize,

    fn writeByte(*_, b: u8): void ! JpegError {
        if (lst := _.list) {
            const al = _.alloc.?;
            lst.append(al, b) catch return JpegError.OutOfMemory;
        } else {
            if (_.pos >= _.fixed.len) {
                return JpegError.BufferTooSmall;
            }
            _.fixed[_.pos] = b;
            _.pos += 1;
        }
    }

    fn writeSlice(*_, data: []const u8): void ! JpegError {
        if (lst := _.list) {
            const al = _.alloc.?;
            lst.appendSlice(al, data) catch return JpegError.OutOfMemory;
        } else {
            if (_.pos + data.len > _.fixed.len) {
                return JpegError.BufferTooSmall;
            }
            let i: usize = 0;
            while (i < data.len) : (i += 1) {
                _.fixed[_.pos + i] = data[i];
            }
            _.pos += data.len;
        }
    }

    fn writeU16BE(*_, v: u16): void ! JpegError {
        _.writeByte(((v >> 8) & 0xFF) as u8)?;
        _.writeByte((v & 0xFF) as u8)?;
    }
}

/// JPEG MSB-first 位写入器，自动执行 0xFF -> 0xFF 0x00 字节填充
struct JpegBitWriter {
    sink: *JpegByteSink,
    bit_buf: u32,
    bit_count: u8,

    fn putBits(*_, bits: u32, count: u8): void ! JpegError {
        if (count == 0) {
            return;
        }
        const one: u32 = 1;
        const mask: u32 = (one << count) - one;
        _.bit_buf = (_.bit_buf << count) | (bits & mask);
        _.bit_count += count;

        while (_.bit_count >= 8) {
            _.bit_count -= 8;
            const byte: u8 = ((_.bit_buf >> _.bit_count) & 0xFF) as u8;
            _.sink.writeByte(byte)?;
            if (byte == 0xFF) {
                _.sink.writeByte(0x00)?;
            }
        }
    }

    fn flush(*_): void ! JpegError {
        if (_.bit_count > 0) {
            const eight: u8 = 8;
            const pad: u8 = eight - _.bit_count;
            const one: u32 = 1;
            const shifted: u32 = one << pad;
            const ones: u32 = shifted - one;
            _.putBits(ones, pad)?;
        }
    }
}

/// 计算系数类别（1..11）与 JPEG 附加位
fn coeffCategory(v: i32): u8 {
    let av: u32 = if (v < 0) ((-v) as u32) else (v as u32);
    let cat: u8 = 0;
    while (av > 0) {
        cat += 1;
        av >>= 1;
    }
    return cat;
}

fn coeffExtraBits(v: i32, cat: u8): u32 {
    if (v >= 0) {
        return v as u32;
    }
    const max_val: i32 = (@as(i32, 1) << cat) - 1;
    return (max_val + v) as u32;
}

/// 可分离 8x8 2D Forward DCT（8-lane SIMD `@dot` 硬件加速）+ 量化 + Zig-Zag 重排
fn forwardDctAndQuantize(
    block: [64]f64,
    quant: [64]u8,
    out_zz: *[64]i32,
) {
    let blk_f32: [64]f32 = [0.0; 64];
    let k_i: usize = 0;
    while (k_i < 64) : (k_i += 1) {
        blk_f32[k_i] = block[k_i] as f32;
    }

    // 1. 行方向 1D DCT（8-lane @dot，直接转置存入 tmp_t[u * 8 + y] 以便列变换连续向量化）
    let tmp_t: [64]f32 = [0.0; 64];
    let y: usize = 0;
    while (y < 8) : (y += 1) {
        const row_off = y * 8;
        const row_slice: []const f32 = blk_f32[row_off..row_off + 8];
        let u: usize = 0;
        while (u < 8) : (u += 1) {
            const c_off = u * 8;
            tmp_t[u * 8 + y] = @dot(row_slice, DCT_COS_F32[c_off..c_off + 8]);
        }
    }

    // 2. 列方向 1D DCT（8-lane @dot 连续列向量）+ 量化
    let qblock: [64]i32 = [0; 64];
    let v: usize = 0;
    while (v < 8) : (v += 1) {
        const c_off = v * 8;
        const cos_row: []const f32 = DCT_COS_F32[c_off..c_off + 8];
        let u: usize = 0;
        while (u < 8) : (u += 1) {
            const col_off = u * 8;
            const sum: f64 = @as(f64, @dot(tmp_t[col_off..col_off + 8], cos_row));
            const idx = v * 8 + u;
            const qf = quant[idx] as f64;
            const scaled = sum / qf;
            let qv: i32 = if (scaled >= 0.0) ((scaled + 0.5) as i32) else ((scaled - 0.5) as i32);
            if (qv > 2047) {
                qv = 2047;
            } else if (qv < -2047) {
                qv = -2047;
            }
            qblock[idx] = qv;
        }
    }

    // 3. Zig-Zag 扫描输出
    let k: usize = 0;
    while (k < 64) : (k += 1) {
        out_zz[k] = qblock[ZIGZAG[k]];
    }
}

/// 编码单个 8x8 量化块（DC 差分 + AC RLE/ZRL/EOB）
fn encodeBlock(
    bw: *JpegBitWriter,
    zz: [64]i32,
    prev_dc: *i32,
    dc_tbl: HuffmanTable,
    ac_tbl: HuffmanTable,
): void ! JpegError {
    // DC 差分编码
    const dc = zz[0];
    const diff = dc - prev_dc.*;
    prev_dc.* = dc;

    if (diff == 0) {
        bw.putBits(dc_tbl.codes[0] as u32, dc_tbl.lens[0])?;
    } else {
        const cat = coeffCategory(diff);
        const sym = cat as usize;
        bw.putBits(dc_tbl.codes[sym] as u32, dc_tbl.lens[sym])?;
        bw.putBits(coeffExtraBits(diff, cat), cat)?;
    }

    // 查找最后一个非零 AC 系数
    let last_nz: usize = 0;
    let i: usize = 63;
    while (i >= 1) {
        if (zz[i] != 0) {
            last_nz = i;
            break;
        }
        i -= 1;
    }

    if (last_nz == 0) {
        // 全零 AC -> EOB (0x00)
        bw.putBits(ac_tbl.codes[0] as u32, ac_tbl.lens[0])?;
        return;
    }

    let zero_run: u8 = 0;
    let k: usize = 1;
    while (k <= last_nz) : (k += 1) {
        const val = zz[k];
        if (val == 0) {
            zero_run += 1;
        } else {
            while (zero_run >= 16) {
                // ZRL (0xF0)
                bw.putBits(ac_tbl.codes[0xF0] as u32, ac_tbl.lens[0xF0])?;
                zero_run -= 16;
            }
            const cat = coeffCategory(val);
            const sym = (((zero_run as usize) << 4) | (cat as usize));
            bw.putBits(ac_tbl.codes[sym] as u32, ac_tbl.lens[sym])?;
            bw.putBits(coeffExtraBits(val, cat), cat)?;
            zero_run = 0;
        }
    }

    if (last_nz < 63) {
        // EOB (0x00)
        bw.putBits(ac_tbl.codes[0] as u32, ac_tbl.lens[0])?;
    }
}

fn writeDqt(sink: *JpegByteSink, table_id: u8, quant: [64]u8): void ! JpegError {
    sink.writeByte(0xFF)?;
    sink.writeByte(0xDB)?;
    sink.writeU16BE(67)?;
    sink.writeByte(table_id)?;
    let k: usize = 0;
    while (k < 64) : (k += 1) {
        sink.writeByte(quant[ZIGZAG[k]])?;
    }
}

fn writeDht(sink: *JpegByteSink, tc_th: u8, bits: []const u8, vals: []const u8): void ! JpegError {
    sink.writeByte(0xFF)?;
    sink.writeByte(0xC4)?;
    sink.writeU16BE((2 + 1 + 16 + vals.len) as u16)?;
    sink.writeByte(tc_th)?;
    sink.writeSlice(bits)?;
    sink.writeSlice(vals)?;
}

fn encodeJpegCore(
    sink: *JpegByteSink,
    rgba: []const u8,
    width: u32,
    height: u32,
    quality: u8,
): void ! JpegError {
    if (width == 0 || height == 0 || width > 65535 || height > 65535) {
        return JpegError.InvalidDimensions;
    }
    const expected_len: u64 = (width as u64) * (height as u64) * 4;
    if (expected_len != (rgba.len as u64)) {
        return JpegError.InvalidBufferLength;
    }

    const luma_q = scaleQuantTable(STD_LUMA_QUANT, quality);
    const chroma_q = scaleQuantTable(STD_CHROMA_QUANT, quality);

    const dc_luma_tbl = buildHuffmanTable(STD_DC_LUMA_BITS[0..16], STD_DC_LUMA_VALS[0..12]);
    const ac_luma_tbl = buildHuffmanTable(STD_AC_LUMA_BITS[0..16], STD_AC_LUMA_VALS[0..162]);
    const dc_chroma_tbl = buildHuffmanTable(STD_DC_CHROMA_BITS[0..16], STD_DC_CHROMA_VALS[0..12]);
    const ac_chroma_tbl = buildHuffmanTable(STD_AC_CHROMA_BITS[0..16], STD_AC_CHROMA_VALS[0..162]);

    // 1. SOI
    sink.writeByte(0xFF)?;
    sink.writeByte(0xD8)?;

    // 2. APP0 (JFIF 1.01)
    const app0: [18]u8 = [
        0xFF, 0xE0, 0x00, 0x10,
        'J',  'F',  'I',  'F',  0x00,
        0x01, 0x01, 0x00,
        0x00, 0x01, 0x00, 0x01,
        0x00, 0x00,
    ];
    sink.writeSlice(app0[0..18])?;

    // 3. DQT (Luma id=0, Chroma id=1)
    writeDqt(sink, 0, luma_q)?;
    writeDqt(sink, 1, chroma_q)?;

    // 4. SOF0 (Baseline DCT, 3 components YCbCr 4:4:4)
    sink.writeByte(0xFF)?;
    sink.writeByte(0xC0)?;
    sink.writeU16BE(17)?;
    sink.writeByte(8)?;
    sink.writeU16BE(height as u16)?;
    sink.writeU16BE(width as u16)?;
    sink.writeByte(3)?;
    // Component 1: Y  (H=1, V=1, Tq=0)
    sink.writeByte(1)?;
    sink.writeByte(0x11)?;
    sink.writeByte(0)?;
    // Component 2: Cb (H=1, V=1, Tq=1)
    sink.writeByte(2)?;
    sink.writeByte(0x11)?;
    sink.writeByte(1)?;
    // Component 3: Cr (H=1, V=1, Tq=1)
    sink.writeByte(3)?;
    sink.writeByte(0x11)?;
    sink.writeByte(1)?;

    // 5. DHT (4 标准表)
    writeDht(sink, 0x00, STD_DC_LUMA_BITS[0..16], STD_DC_LUMA_VALS[0..12])?;
    writeDht(sink, 0x10, STD_AC_LUMA_BITS[0..16], STD_AC_LUMA_VALS[0..162])?;
    writeDht(sink, 0x01, STD_DC_CHROMA_BITS[0..16], STD_DC_CHROMA_VALS[0..12])?;
    writeDht(sink, 0x11, STD_AC_CHROMA_BITS[0..16], STD_AC_CHROMA_VALS[0..162])?;

    // 6. SOS (Start of Scan)
    const sos: [14]u8 = [
        0xFF, 0xDA, 0x00, 0x0C,
        0x03,
        0x01, 0x00,
        0x02, 0x11,
        0x03, 0x11,
        0x00, 0x3F, 0x00,
    ];
    sink.writeSlice(sos[0..14])?;

    // 7. 8x8 MCU 扫描编码（边缘像素复制填充）
    let bw = JpegBitWriter{
        sink: sink,
        bit_buf: 0,
        bit_count: 0,
    };

    let prev_dc_y: i32 = 0;
    let prev_dc_cb: i32 = 0;
    let prev_dc_cr: i32 = 0;

    let y_block: [64]f64 = [0.0; 64];
    let cb_block: [64]f64 = [0.0; 64];
    let cr_block: [64]f64 = [0.0; 64];
    let zz: [64]i32 = [0; 64];

    const w = width as usize;
    const h = height as usize;

    let by: usize = 0;
    while (by < h) : (by += 8) {
        let bx: usize = 0;
        while (bx < w) : (bx += 8) {
            let dy: usize = 0;
            while (dy < 8) : (dy += 1) {
                const py = if (by + dy < h) (by + dy) else (h - 1);
                let dx: usize = 0;
                while (dx < 8) : (dx += 1) {
                    const px = if (bx + dx < w) (bx + dx) else (w - 1);
                    const off = (py * w + px) * 4;
                    const r = rgba[off] as f64;
                    const g = rgba[off + 1] as f64;
                    const b = rgba[off + 2] as f64;
                    const idx = dy * 8 + dx;

                    y_block[idx] = 0.299 * r + 0.587 * g + 0.114 * b - 128.0;
                    cb_block[idx] = -0.168736 * r - 0.331264 * g + 0.5 * b;
                    cr_block[idx] = 0.5 * r - 0.418688 * g - 0.081312 * b;
                }
            }

            forwardDctAndQuantize(y_block, luma_q, &zz);
            encodeBlock(&bw, zz, &prev_dc_y, dc_luma_tbl, ac_luma_tbl)?;

            forwardDctAndQuantize(cb_block, chroma_q, &zz);
            encodeBlock(&bw, zz, &prev_dc_cb, dc_chroma_tbl, ac_chroma_tbl)?;

            forwardDctAndQuantize(cr_block, chroma_q, &zz);
            encodeBlock(&bw, zz, &prev_dc_cr, dc_chroma_tbl, ac_chroma_tbl)?;
        }
    }

    bw.flush()?;

    // 8. EOI
    sink.writeByte(0xFF)?;
    sink.writeByte(0xD9)?;
}

/// 将 RGBA8 像素编码为合标 Baseline DCT JPEG (JFIF 1.01, YCbCr 4:4:4) 字节流。
/// `quality` 范围 `1..100`（自动钳位）。调用方需使用 `allocator.free(...)` 释放返回切片。
pub fn jpegEncode(
    allocator: std.mem.Allocator,
    rgba: []const u8,
    width: u32,
    height: u32,
    quality: u8,
): ([]u8) ! JpegError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    let dummy: [1]u8 = [0];
    let sink = JpegByteSink{
        alloc: allocator,
        list: &out,
        fixed: dummy[0..0],
        pos: 0,
    };
    encodeJpegCore(&sink, rgba, width, height, quality)?;
    return out.toOwnedSlice(allocator) catch return JpegError.OutOfMemory;
}

/// 零堆分配 Baseline DCT JPEG 编码器：直接编码至 `out` 缓冲区并返回实际写入切片。
/// 若 `out` 容量不足返回 `JpegError.BufferTooSmall`。
pub fn jpegEncodeInto(
    rgba: []const u8,
    width: u32,
    height: u32,
    quality: u8,
    out: []u8,
): ([]const u8) ! JpegError {
    let sink = JpegByteSink{
        alloc: null,
        list: null,
        fixed: out,
        pos: 0,
    };
    encodeJpegCore(&sink, rgba, width, height, quality)?;
    return out[0..sink.pos];
}

// ============================================================================
// ITU-T T.81 Baseline DCT JPEG (SOF0) Zero-Heap SIMD Decoder (Ref: zune-jpeg)
// ============================================================================

const JPEG_MAX_DIM: u32 = 16384;

/// 8-lane SIMD `@dot` 1D 正交 Inverse DCT 余弦基行向量 (`IDCT_COS_F32 = DCT_COS_F32^T`)
const IDCT_COS_F32: [64]f32 = [
     0.35355339,  0.49039264,  0.46193977,  0.41573481,  0.35355339,  0.27778512,  0.19134172,  0.09754516,
     0.35355339,  0.41573481,  0.19134172, -0.09754516, -0.35355339, -0.49039264, -0.46193977, -0.27778512,
     0.35355339,  0.27778512, -0.19134172, -0.49039264, -0.35355339,  0.09754516,  0.46193977,  0.41573481,
     0.35355339,  0.09754516, -0.46193977, -0.27778512,  0.35355339,  0.41573481, -0.19134172, -0.49039264,
     0.35355339, -0.09754516, -0.46193977,  0.27778512,  0.35355339, -0.41573481, -0.19134172,  0.49039264,
     0.35355339, -0.27778512, -0.19134172,  0.49039264, -0.35355339, -0.09754516,  0.46193977, -0.41573481,
     0.35355339, -0.41573481,  0.19134172,  0.09754516, -0.35355339,  0.49039264, -0.46193977,  0.27778512,
     0.35355339, -0.49039264,  0.46193977, -0.41573481,  0.35355339, -0.27778512,  0.19134172, -0.09754516,
];

/// Per-component frame descriptor parsed from `SOF0` / `SOF2`.
pub struct JpegComponentInfo {
    id: u8 = 0,
    h_samp: u8 = 1,
    v_samp: u8 = 1,
    quant_id: u8 = 0,
}

/// Complete JPEG frame metadata parsed by `parseJpegHeader`.
pub struct JpegHeaderInfo {
    width: u32 = 0,
    height: u32 = 0,
    components: u8 = 0,
    precision: u8 = 8,
    is_progressive: bool = false,
    restart_interval: u16 = 0,
    max_h_samp: u8 = 1,
    max_v_samp: u8 = 1,
    comp: [4]JpegComponentInfo = [
        JpegComponentInfo{},
        JpegComponentInfo{},
        JpegComponentInfo{},
        JpegComponentInfo{},
    ],
    orientation: ExifOrientation = ExifOrientation.Normal,
}

/// Decoded RGBA8 JPEG image owned by caller's allocator.
pub struct JpegDecodedImage {
    width: u32 = 0,
    height: u32 = 0,
    components: u8 = 0,
    rgba: []u8 = [],

    fn deinit(*_, allocator: std.mem.Allocator) {
        if (rgba.len > 0) {
            allocator.free(rgba);
            rgba = [];
        }
    }
}

/// Decoded ITU-T T.81 Huffman table with 9-bit fast lookahead LUT + 1..16-bit canonical slow path.
struct JpegHuffDecTable {
    valid: bool = false,
    // Fast 9-bit LUT: high bits (>> 9) = code length (1..9, or 10 if slow path), low 8 bits = symbol
    fast_lut: [512]u16 = [0; 512],
    max_code: [18]i32 = [0; 18],
    val_offset: [18]i32 = [0; 18],
    values: [256]u8 = [0; 256],

    static fn build(bits: []const u8, vals: []const u8): JpegHuffDecTable ! JpegError {
        if (bits.len < 16 || vals.len > 256) return JpegError.InvalidHuffman;
        let t = JpegHuffDecTable{ valid: true };
        const slow_sentinel: u16 = @as(u16, 10) << 9;
        let li: usize = 0;
        while (li < 512) : (li += 1) {
            t.fast_lut[li] = slow_sentinel;
        }
        let vi: usize = 0;
        while (vi < vals.len) : (vi += 1) {
            t.values[vi] = vals[vi];
        }

        let code: i32 = 0;
        let sym_idx: i32 = 0;
        let k: usize = 1;
        while (k <= 16) : (k += 1) {
            const cnt: i32 = @as(i32, bits[k - 1]);
            if (cnt == 0) {
                t.max_code[k] = -1;
                t.val_offset[k] = 0;
            } else {
                t.val_offset[k] = sym_idx - code;
                let j: i32 = 0;
                while (j < cnt) : (j += 1) {
                    if (sym_idx >= @as(i32, @intCast(vals.len))) return JpegError.InvalidHuffman;
                    const sym: u8 = vals[@as(usize, @intCast(sym_idx))];
                    if (k <= 9) {
                        const pad_bits: u4 = @as(u4, @intCast(9 - k));
                        const base: usize = @as(usize, @intCast(code)) << pad_bits;
                        const span: usize = @as(usize, 1) << pad_bits;
                        const entry: u16 = (@as(u16, @intCast(k)) << 9) | @as(u16, sym);
                        let s: usize = 0;
                        while (s < span) : (s += 1) {
                            t.fast_lut[base + s] = entry;
                        }
                    }
                    code += 1;
                    sym_idx += 1;
                }
                if (code > (@as(i32, 1) << @as(u5, @intCast(k)))) {
                    return JpegError.InvalidHuffman;
                }
                t.max_code[k] = (code << @as(u5, @intCast(16 - k)));
            }
            code <<= 1;
        }
        t.max_code[17] = 0x10000;
        return t;
    }
}

/// MSB-first byte-stuffed JPEG entropy bitstream reader (ref: zune-jpeg/src/bitstream.rs).
struct JpegBitReader {
    src: []const u8 = "",
    pos: usize = 0,
    buf: u64 = 0,
    nbits: u8 = 0,
    seen_marker: u8 = 0,

    static fn init(src: []const u8, start_pos: usize): JpegBitReader {
        return JpegBitReader{ src: src, pos: start_pos, buf: 0, nbits: 0, seen_marker: 0 };
    }

    fn refill(*_) {
        while (nbits <= 56) {
            if (seen_marker != 0 || pos >= src.len) {
                break;
            }
            const b: u8 = src[pos];
            pos += 1;
            if (b == 0xFF) {
                // Skip any fill 0xFF bytes
                let next_b: u8 = 0xFF;
                while (pos < src.len) {
                    next_b = src[pos];
                    pos += 1;
                    if (next_b != 0xFF) break;
                }
                if (next_b == 0x00) {
                    // Byte-stuffed literal 0xFF
                    buf = (buf << 8) | 0xFF;
                    nbits += 8;
                } else {
                    // Encountered a marker (RSTm / EOI / etc.) — stop entropy reading
                    seen_marker = next_b;
                    break;
                }
            } else {
                buf = (buf << 8) | @as(u64, b);
                nbits += 8;
            }
        }
    }

    fn peekBits(*_, n: u8): u32 {
        if (nbits < n) refill();
        if (nbits >= n) {
            const sh: u6 = @as(u6, @intCast(nbits - n));
            const mask: u64 = (@as(u64, 1) << @as(u6, @intCast(n))) - 1;
            return @as(u32, @intCast((buf >> sh) & mask));
        }
        // Zero-pad remaining bits at end of entropy segment (per ITU-T T.81 & zune-jpeg)
        const pad: u6 = @as(u6, @intCast(n - nbits));
        const mask: u64 = (@as(u64, 1) << @as(u6, @intCast(n))) - 1;
        return @as(u32, @intCast((buf << pad) & mask));
    }

    fn dropBits(*_, n: u8): void ! JpegError {
        if (nbits < n) refill();
        if (nbits < n) return JpegError.UnexpectedEof;
        nbits -= n;
        if (nbits == 0) {
            buf = 0;
        } else {
            const mask: u64 = (@as(u64, 1) << @as(u6, @intCast(nbits))) - 1;
            buf &= mask;
        }
    }

    fn readBits(*_, n: u8): u32 ! JpegError {
        if (n == 0) return 0;
        if (nbits < n) refill();
        if (nbits < n) return JpegError.UnexpectedEof;
        const sh: u6 = @as(u6, @intCast(nbits - n));
        const mask: u64 = (@as(u64, 1) << @as(u6, @intCast(n))) - 1;
        const val: u32 = @as(u32, @intCast((buf >> sh) & mask));
        nbits -= n;
        if (nbits == 0) {
            buf = 0;
        } else {
            buf &= (@as(u64, 1) << @as(u6, @intCast(nbits))) - 1;
        }
        return val;
    }

    fn decodeHuffman(*_, tbl: *const JpegHuffDecTable): u8 ! JpegError {
        if (!tbl.valid) return JpegError.InvalidHuffman;
        if (nbits < 16) refill();

        const p9: usize = @as(usize, peekBits(9));
        const entry: u16 = tbl.fast_lut[p9];
        const clen: u8 = @as(u8, @intCast(entry >> 9));
        if (clen <= 9) {
            dropBits(clen)?;
            return @as(u8, @intCast(entry & 0xFF));
        }

        // Slow path: 10..16-bit canonical decode
        const p16: i32 = @as(i32, @intCast(peekBits(16)));
        let k: usize = 10;
        while (k <= 16) : (k += 1) {
            if (p16 < tbl.max_code[k]) {
                const code_val: i32 = p16 >> @as(u5, @intCast(16 - k));
                const idx: i32 = code_val + tbl.val_offset[k];
                if (idx < 0 || idx >= 256) return JpegError.InvalidHuffman;
                dropBits(@as(u8, @intCast(k)))?;
                return tbl.values[@as(usize, @intCast(idx))];
            }
        }
        return JpegError.InvalidHuffman;
    }

    /// Consumes a restart marker `RST0..RST7` at an MCU restart boundary and resets bit buffer.
    fn consumeRestart(*_) {
        buf = 0;
        nbits = 0;
        if (seen_marker >= 0xD0 && seen_marker <= 0xD7) {
            seen_marker = 0;
            return;
        }
        if (seen_marker != 0) return;
        // Scan forward for 0xFF 0xD0..0xD7
        while (pos + 1 < src.len) {
            if (src[pos] == 0xFF) {
                let p: usize = pos + 1;
                while (p < src.len && src[p] == 0xFF) : (p += 1) {}
                if (p < src.len && src[p] >= 0xD0 && src[p] <= 0xD7) {
                    pos = p + 1;
                    return;
                }
                break;
            }
            pos += 1;
        }
    }
}

fn jpegReceiveExtend(val: u32, nbits: u8): i32 {
    if (nbits == 0) return 0;
    const half: u32 = @as(u32, 1) << @as(u5, @intCast(nbits - 1));
    if (val < half) {
        const full: i32 = @as(i32, 1) << @as(u5, @intCast(nbits));
        return @as(i32, @intCast(val)) - (full - 1);
    }
    return @as(i32, @intCast(val));
}

fn clampF32ToU8(v: f32): u8 {
    if (v <= 0.0) return 0;
    if (v >= 255.0) return 255;
    return @as(u8, @intFromFloat(v + 0.5));
}

/// 8-lane SIMD `@dot` hardware-accelerated separable 2D 8x8 Inverse DCT (`X = C^T * Y * C + 128`).
fn inverseDctSimd8x8(deq: [64]f32, out_pixels: *[64]u8) {
    // 1. Row-wise 1D IDCT via 8-lane @dot, transposed directly into `tmp_t[x * 8 + v]`
    let tmp_t: [64]f32 = [0.0; 64];
    let v: usize = 0;
    while (v < 8) : (v += 1) {
        const row_off = v * 8;
        const row_slice: []const f32 = deq[row_off .. row_off + 8];
        let x: usize = 0;
        while (x < 8) : (x += 1) {
            const c_off = x * 8;
            tmp_t[x * 8 + v] = @dot(row_slice, IDCT_COS_F32[c_off .. c_off + 8]);
        }
    }

    // 2. Column-wise 1D IDCT via 8-lane @dot + level shift (+128.0) + clamp [0, 255]
    let y: usize = 0;
    while (y < 8) : (y += 1) {
        const c_off = y * 8;
        const cos_row: []const f32 = IDCT_COS_F32[c_off .. c_off + 8];
        let x: usize = 0;
        while (x < 8) : (x += 1) {
            const col_off = x * 8;
            const val: f32 = @dot(tmp_t[col_off .. col_off + 8], cos_row) + 128.0;
            out_pixels[y * 8 + x] = clampF32ToU8(val);
        }
    }
}

fn decodeMcuBlock(
    br: *JpegBitReader,
    dc_tbl: *const JpegHuffDecTable,
    ac_tbl: *const JpegHuffDecTable,
    quant: *const [64]f32,
    dc_pred: *i32,
    out_pixels: *[64]u8,
): void ! JpegError {
    let deq: [64]f32 = [0.0; 64];

    // 1. DC coefficient
    const dc_cat: u8 = br.decodeHuffman(dc_tbl)?;
    if (dc_cat > 11) return JpegError.InvalidHuffman;
    const dc_bits: u32 = br.readBits(dc_cat)?;
    const dc_diff: i32 = jpegReceiveExtend(dc_bits, dc_cat);
    dc_pred.* = dc_pred.* +% dc_diff;
    deq[0] = @as(f32, @floatFromInt(dc_pred.*)) * quant[0];

    // 2. AC coefficients (1..63 in Zig-Zag order)
    let k: usize = 1;
    while (k < 64) {
        const rs: u8 = br.decodeHuffman(ac_tbl)?;
        if (rs == 0x00) {
            // EOB: remaining AC coefficients are 0
            break;
        }
        if (rs == 0xF0) {
            // ZRL: skip 16 zeros
            k += 16;
            continue;
        }
        const run: usize = @as(usize, rs >> 4);
        const cat: u8 = rs & 0x0F;
        if (cat == 0 || cat > 10) return JpegError.InvalidHuffman;
        k += run;
        if (k >= 64) return JpegError.InvalidHuffman;
        const ac_bits: u32 = br.readBits(cat)?;
        const ac_val: i32 = jpegReceiveExtend(ac_bits, cat);
        const nat_idx: usize = ZIGZAG[k];
        deq[nat_idx] = @as(f32, @floatFromInt(ac_val)) * quant[nat_idx];
        k += 1;
    }

    inverseDctSimd8x8(deq, out_pixels);
}

/// Parses JPEG frame header (`SOF0`/`SOF2`, `DRI`, `EXIF` orientation) with strict bounds validation.
pub fn parseJpegHeader(bytes: []const u8): JpegHeaderInfo ! JpegError {
    if (bytes.len < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
        return JpegError.InvalidMarker;
    }
    let info = JpegHeaderInfo{
        orientation: Exif.readJpeg(bytes),
    };
    let pos: usize = 2;
    let found_sof: bool = false;

    while (pos + 2 <= bytes.len) {
        if (bytes[pos] != 0xFF) {
            pos += 1;
            continue;
        }
        while (pos < bytes.len && bytes[pos] == 0xFF) : (pos += 1) {}
        if (pos >= bytes.len) break;
        const marker: u8 = bytes[pos];
        pos += 1;

        if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) {
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            break;
        }
        if (pos + 2 > bytes.len) return JpegError.UnexpectedEof;
        const seglen: usize = (@as(usize, bytes[pos]) << 8) | @as(usize, bytes[pos + 1]);
        if (seglen < 2 || pos + seglen > bytes.len) return JpegError.InvalidMarker;
        const body = bytes[pos + 2 .. pos + seglen];
        pos += seglen;

        if (marker == 0xDD) {
            // DRI
            if (body.len < 2) return JpegError.InvalidMarker;
            info.restart_interval = (@as(u16, body[0]) << 8) | @as(u16, body[1]);
        } else if (marker == 0xC0 || marker == 0xC2) {
            if (body.len < 6) return JpegError.InvalidMarker;
            info.precision = body[0];
            info.height = (@as(u32, body[1]) << 8) | @as(u32, body[2]);
            info.width = (@as(u32, body[3]) << 8) | @as(u32, body[4]);
            info.components = body[5];
            info.is_progressive = (marker == 0xC2);
            if (info.width == 0 || info.height == 0) return JpegError.InvalidDimensions;
            if (info.width > JPEG_MAX_DIM || info.height > JPEG_MAX_DIM) return JpegError.DimensionTooLarge;
            if (info.components == 0 || info.components > 4) return JpegError.UnsupportedFormat;
            if (body.len < @as(usize, 6) + @as(usize, info.components) * 3) return JpegError.InvalidMarker;

            let max_h: u8 = 1;
            let max_v: u8 = 1;
            let ci: usize = 0;
            while (ci < @as(usize, info.components)) : (ci += 1) {
                const cid: u8 = body[6 + ci * 3];
                const hv: u8 = body[6 + ci * 3 + 1];
                const qid: u8 = body[6 + ci * 3 + 2];
                const h: u8 = (hv >> 4) & 0x0F;
                const v: u8 = hv & 0x0F;
                if (h == 0 || h > 4 || v == 0 || v > 4 || qid > 3) {
                    return JpegError.InvalidSamplingFactor;
                }
                if (h > max_h) max_h = h;
                if (v > max_v) max_v = v;
                info.comp[ci] = JpegComponentInfo{ id: cid, h_samp: h, v_samp: v, quant_id: qid };
            }
            info.max_h_samp = max_h;
            info.max_v_samp = max_v;
            found_sof = true;
            break;
        }
    }

    if (!found_sof) return JpegError.InvalidMarker;
    return info;
}

/// Zero-heap Baseline DCT JPEG (`SOF0`) decoder into caller-provided `out_rgba` (`width * height * 4` bytes).
/// Supports 1-component grayscale and 3-component YCbCr (4:4:4, 4:2:2, 4:4:0, 4:2:0),
/// DQT (8/16-bit), DHT (standard/custom), DRI + RST0..RST7 restart markers,
/// 8-lane SIMD `@dot` 8x8 IDCT, and `@fma` BT.601 YCbCr->RGBA conversion.
pub fn decodeJpegToRgbaInto(bytes: []const u8, out_rgba: []u8): JpegHeaderInfo ! JpegError {
    if (bytes.len < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
        return JpegError.InvalidMarker;
    }

    let info = JpegHeaderInfo{
        orientation: Exif.readJpeg(bytes),
    };
    let quant_tables: [4][64]f32 = [[0.0; 64]; 4];
    let quant_valid: [4]bool = [false; 4];
    let dc_tables: [4]JpegHuffDecTable = [
        JpegHuffDecTable{},
        JpegHuffDecTable{},
        JpegHuffDecTable{},
        JpegHuffDecTable{},
    ];
    let ac_tables: [4]JpegHuffDecTable = [
        JpegHuffDecTable{},
        JpegHuffDecTable{},
        JpegHuffDecTable{},
        JpegHuffDecTable{},
    ];

    let found_sof: bool = false;
    let found_sos: bool = false;
    let sos_comp_dc: [4]usize = [0; 4];
    let sos_comp_ac: [4]usize = [0; 4];
    let sos_comp_idx: [4]usize = [0; 4];
    let sos_ns: usize = 0;
    let entropy_start: usize = 0;

    let pos: usize = 2;
    while (pos + 2 <= bytes.len) {
        if (bytes[pos] != 0xFF) {
            pos += 1;
            continue;
        }
        while (pos < bytes.len && bytes[pos] == 0xFF) : (pos += 1) {}
        if (pos >= bytes.len) break;
        const marker: u8 = bytes[pos];
        pos += 1;

        if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) {
            continue;
        }
        if (marker == 0xD9) {
            break;
        }
        if (pos + 2 > bytes.len) return JpegError.UnexpectedEof;
        const seglen: usize = (@as(usize, bytes[pos]) << 8) | @as(usize, bytes[pos + 1]);
        if (seglen < 2 || pos + seglen > bytes.len) return JpegError.InvalidMarker;
        const body = bytes[pos + 2 .. pos + seglen];
        pos += seglen;

        if (marker == 0xDB) {
            // DQT: one or more quantization tables
            let off: usize = 0;
            while (off < body.len) {
                const pq_tq: u8 = body[off];
                off += 1;
                const pq: u8 = (pq_tq >> 4) & 0x0F;
                const tq: usize = @as(usize, pq_tq & 0x0F);
                if (pq > 1 || tq > 3) return JpegError.InvalidQuantTable;
                const elem_size: usize = if (pq == 0) 1 else 2;
                if (off + 64 * elem_size > body.len) return JpegError.InvalidQuantTable;
                let k: usize = 0;
                while (k < 64) : (k += 1) {
                    const qval: u16 = if (pq == 0)
                        @as(u16, body[off + k])
                    else
                        (@as(u16, body[off + k * 2]) << 8) | @as(u16, body[off + k * 2 + 1]);
                    quant_tables[tq][ZIGZAG[k]] = @as(f32, @floatFromInt(qval));
                }
                quant_valid[tq] = true;
                off += 64 * elem_size;
            }
        } else if (marker == 0xC4) {
            // DHT: one or more Huffman tables
            let off: usize = 0;
            while (off < body.len) {
                if (off + 17 > body.len) return JpegError.InvalidHuffman;
                const tc_th: u8 = body[off];
                const tc: u8 = (tc_th >> 4) & 0x0F;
                const th: usize = @as(usize, tc_th & 0x0F);
                if (tc > 1 || th > 3) return JpegError.InvalidHuffman;
                const bits = body[off + 1 .. off + 17];
                let sum: usize = 0;
                let bi: usize = 0;
                while (bi < 16) : (bi += 1) {
                    sum += @as(usize, bits[bi]);
                }
                if (sum > 256 || off + 17 + sum > body.len) return JpegError.InvalidHuffman;
                const vals = body[off + 17 .. off + 17 + sum];
                const tbl = JpegHuffDecTable.build(bits, vals)?;
                if (tc == 0) {
                    dc_tables[th] = tbl;
                } else {
                    ac_tables[th] = tbl;
                }
                off += 17 + sum;
            }
        } else if (marker == 0xDD) {
            // DRI
            if (body.len < 2) return JpegError.InvalidMarker;
            info.restart_interval = (@as(u16, body[0]) << 8) | @as(u16, body[1]);
        } else if (marker == 0xC0) {
            // SOF0 Baseline DCT
            if (body.len < 6) return JpegError.InvalidMarker;
            info.precision = body[0];
            if (info.precision != 8) return JpegError.UnsupportedFormat;
            info.height = (@as(u32, body[1]) << 8) | @as(u32, body[2]);
            info.width = (@as(u32, body[3]) << 8) | @as(u32, body[4]);
            info.components = body[5];
            info.is_progressive = false;
            if (info.width == 0 || info.height == 0) return JpegError.InvalidDimensions;
            if (info.width > JPEG_MAX_DIM || info.height > JPEG_MAX_DIM) return JpegError.DimensionTooLarge;
            if (info.components != 1 && info.components != 3) return JpegError.UnsupportedFormat;
            if (body.len < @as(usize, 6) + @as(usize, info.components) * 3) return JpegError.InvalidMarker;

            let max_h: u8 = 1;
            let max_v: u8 = 1;
            let ci: usize = 0;
            while (ci < @as(usize, info.components)) : (ci += 1) {
                const cid: u8 = body[6 + ci * 3];
                const hv: u8 = body[6 + ci * 3 + 1];
                const qid: u8 = body[6 + ci * 3 + 2];
                const h: u8 = (hv >> 4) & 0x0F;
                const v: u8 = hv & 0x0F;
                if (h == 0 || h > 2 || v == 0 || v > 2 || qid > 3) {
                    return JpegError.InvalidSamplingFactor;
                }
                if (h > max_h) max_h = h;
                if (v > max_v) max_v = v;
                info.comp[ci] = JpegComponentInfo{ id: cid, h_samp: h, v_samp: v, quant_id: qid };
            }
            // Validate chroma sampling for 3-component images (chroma must divide luma)
            if (info.components == 3) {
                if ((max_h % info.comp[1].h_samp) != 0 || (max_v % info.comp[1].v_samp) != 0 ||
                    (max_h % info.comp[2].h_samp) != 0 || (max_v % info.comp[2].v_samp) != 0)
                {
                    return JpegError.InvalidSamplingFactor;
                }
            }
            info.max_h_samp = max_h;
            info.max_v_samp = max_v;
            found_sof = true;
        } else if (marker == 0xC1 || marker == 0xC2 || marker == 0xC3 ||
                   (marker >= 0xC5 && marker <= 0xC7) ||
                   (marker >= 0xC9 && marker <= 0xCB) ||
                   (marker >= 0xCD && marker <= 0xCF))
        {
            return JpegError.UnsupportedFormat;
        } else if (marker == 0xDA) {
            // SOS: Start of Scan
            if (!found_sof || body.len < 4) return JpegError.InvalidScan;
            sos_ns = @as(usize, body[0]);
            if (sos_ns != @as(usize, info.components)) return JpegError.UnsupportedFormat;
            if (body.len < @as(usize, 1) + sos_ns * 2 + 3) return JpegError.InvalidScan;

            let si: usize = 0;
            while (si < sos_ns) : (si += 1) {
                const cs: u8 = body[1 + si * 2];
                const td_ta: u8 = body[1 + si * 2 + 1];
                const td: usize = @as(usize, (td_ta >> 4) & 0x0F);
                const ta: usize = @as(usize, td_ta & 0x0F);
                if (td > 3 || ta > 3) return JpegError.InvalidScan;

                let matched: usize = 4;
                let ci: usize = 0;
                while (ci < @as(usize, info.components)) : (ci += 1) {
                    if (info.comp[ci].id == cs) {
                        matched = ci;
                        break;
                    }
                }
                if (matched == 4) return JpegError.InvalidScan;
                sos_comp_idx[si] = matched;
                sos_comp_dc[si] = td;
                sos_comp_ac[si] = ta;
            }
            // Baseline SOS spectral selection: Ss == 0, Se == 63, Ah == 0, Al == 0
            const ss: u8 = body[1 + sos_ns * 2];
            const se: u8 = body[1 + sos_ns * 2 + 1];
            if (ss != 0 || se != 63) return JpegError.UnsupportedFormat;

            entropy_start = pos;
            found_sos = true;
            break;
        }
    }

    if (!found_sof || !found_sos) return JpegError.InvalidMarker;

    const total_rgba: usize = @as(usize, info.width) * @as(usize, info.height) * 4;
    if (out_rgba.len < total_rgba) return JpegError.BufferTooSmall;

    // Validate all referenced quantization & Huffman tables exist
    let si_chk: usize = 0;
    while (si_chk < sos_ns) : (si_chk += 1) {
        const ci = sos_comp_idx[si_chk];
        if (!quant_valid[@as(usize, info.comp[ci].quant_id)]) return JpegError.InvalidQuantTable;
        if (!dc_tables[sos_comp_dc[si_chk]].valid || !ac_tables[sos_comp_ac[si_chk]].valid) {
            return JpegError.InvalidHuffman;
        }
    }

    const mcu_w: usize = @as(usize, info.max_h_samp) * 8;
    const mcu_h: usize = @as(usize, info.max_v_samp) * 8;
    const mcus_x: usize = (@as(usize, info.width) + mcu_w - 1) / mcu_w;
    const mcus_y: usize = (@as(usize, info.height) + mcu_h - 1) / mcu_h;

    let br = JpegBitReader.init(bytes, entropy_start);
    let dc_preds: [4]i32 = [0; 4];
    let mcu_counter: u32 = 0;

    // Stack-allocated MCU component planes (max 16x16 = 256 bytes per component — zero heap!)
    let comp_buf: [3][256]u8 = [[128; 256]; 3];
    let blk: [64]u8 = [0; 64];

    let my: usize = 0;
    while (my < mcus_y) : (my += 1) {
        let mx: usize = 0;
        while (mx < mcus_x) : (mx += 1) {
            if (info.restart_interval > 0 && mcu_counter > 0 && (mcu_counter % @as(u32, info.restart_interval)) == 0) {
                br.consumeRestart();
                dc_preds[0] = 0;
                dc_preds[1] = 0;
                dc_preds[2] = 0;
                dc_preds[3] = 0;
            }

            // Decode all data units in this MCU
            let s_idx: usize = 0;
            while (s_idx < sos_ns) : (s_idx += 1) {
                const c_idx: usize = sos_comp_idx[s_idx];
                const h_s: usize = @as(usize, info.comp[c_idx].h_samp);
                const v_s: usize = @as(usize, info.comp[c_idx].v_samp);
                const qid: usize = @as(usize, info.comp[c_idx].quant_id);
                const dcid: usize = sos_comp_dc[s_idx];
                const acid: usize = sos_comp_ac[s_idx];
                const plane_w: usize = h_s * 8;

                let bv: usize = 0;
                while (bv < v_s) : (bv += 1) {
                    let bh: usize = 0;
                    while (bh < h_s) : (bh += 1) {
                        decodeMcuBlock(
                            &br,
                            &dc_tables[dcid],
                            &ac_tables[acid],
                            &quant_tables[qid],
                            &dc_preds[c_idx],
                            &blk,
                        )?;
                        let r: usize = 0;
                        while (r < 8) : (r += 1) {
                            const dst_row: usize = (bv * 8 + r) * plane_w + bh * 8;
                            let col: usize = 0;
                            while (col < 8) : (col += 1) {
                                comp_buf[c_idx][dst_row + col] = blk[r * 8 + col];
                            }
                        }
                    }
                }
            }

            // Upsample & convert MCU to RGBA8 directly into `out_rgba`
            const base_x: usize = mx * mcu_w;
            const base_y: usize = my * mcu_h;
            const img_w: usize = @as(usize, info.width);
            const img_h: usize = @as(usize, info.height);

            if (info.components == 1) {
                let py: usize = 0;
                while (py < mcu_h && base_y + py < img_h) : (py += 1) {
                    const row_out: usize = (base_y + py) * img_w;
                    let px: usize = 0;
                    while (px < mcu_w && base_x + px < img_w) : (px += 1) {
                        const yv: u8 = comp_buf[0][py * mcu_w + px];
                        const o: usize = (row_out + base_x + px) * 4;
                        out_rgba[o] = yv;
                        out_rgba[o + 1] = yv;
                        out_rgba[o + 2] = yv;
                        out_rgba[o + 3] = 255;
                    }
                }
            } else {
                const cb_h_shift: u6 = if (info.max_h_samp > info.comp[1].h_samp) 1 else 0;
                const cb_v_shift: u6 = if (info.max_v_samp > info.comp[1].v_samp) 1 else 0;
                const cr_h_shift: u6 = if (info.max_h_samp > info.comp[2].h_samp) 1 else 0;
                const cr_v_shift: u6 = if (info.max_v_samp > info.comp[2].v_samp) 1 else 0;
                const y_w: usize = @as(usize, info.comp[0].h_samp) * 8;
                const cb_w: usize = @as(usize, info.comp[1].h_samp) * 8;
                const cr_w: usize = @as(usize, info.comp[2].h_samp) * 8;

                let py: usize = 0;
                while (py < mcu_h && base_y + py < img_h) : (py += 1) {
                    const row_out: usize = (base_y + py) * img_w;
                    const cby: usize = py >> cb_v_shift;
                    const cry: usize = py >> cr_v_shift;
                    let px: usize = 0;
                    while (px < mcu_w && base_x + px < img_w) : (px += 1) {
                        const yf: f32 = @as(f32, @floatFromInt(comp_buf[0][py * y_w + px]));
                        const cbf: f32 = @as(f32, @floatFromInt(comp_buf[1][cby * cb_w + (px >> cb_h_shift)])) - 128.0;
                        const crf: f32 = @as(f32, @floatFromInt(comp_buf[2][cry * cr_w + (px >> cr_h_shift)])) - 128.0;

                        // BT.601 YCbCr -> RGB using hardware @fma
                        const rf: f32 = @fma(1.40200, crf, yf);
                        const gf: f32 = @fma(-0.71414, crf, @fma(-0.34414, cbf, yf));
                        const bf: f32 = @fma(1.77200, cbf, yf);

                        const o: usize = (row_out + base_x + px) * 4;
                        out_rgba[o] = clampF32ToU8(rf);
                        out_rgba[o + 1] = clampF32ToU8(gf);
                        out_rgba[o + 2] = clampF32ToU8(bf);
                        out_rgba[o + 3] = 255;
                    }
                }
            }

            mcu_counter += 1;
        }
    }

    return info;
}

/// Decodes a Baseline DCT JPEG (`SOF0`) into a newly allocated RGBA8 image.
pub fn decodeJpegToRgba(
    allocator: std.mem.Allocator,
    bytes: []const u8,
): JpegDecodedImage ! JpegError {
    const hdr = parseJpegHeader(bytes)?;
    if (hdr.is_progressive) return JpegError.UnsupportedFormat;
    const total: usize = @as(usize, hdr.width) * @as(usize, hdr.height) * 4;
    const rgba = allocator.alloc(u8, total) catch return JpegError.OutOfMemory;
    const decoded_hdr = decodeJpegToRgbaInto(bytes, rgba) catch |e| {
        allocator.free(rgba);
        return e;
    };
    return JpegDecodedImage{
        width: decoded_hdr.width,
        height: decoded_hdr.height,
        components: decoded_hdr.components,
        rgba: rgba,
    };
}

pub class Jpeg {
    /// 快速探测 JPEG 图像尺寸、采样与 EXIF 方向
    pub static fn probe(bytes: []const u8): JpegHeader? {
        if (bytes.len < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
            return null;
        }

        let orientation = Exif.readJpeg(bytes);
        let offset: usize = 2;

        while (offset + 4 <= bytes.len) {
            if (bytes[offset] != 0xFF) {
                offset = offset + 1;
                continue;
            }
            let marker = bytes[offset + 1];
            if (marker == 0xFF) {
                offset = offset + 1;
                continue;
            }
            if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) {
                offset = offset + 2;
                continue;
            }
            if (marker == 0xDA || marker == 0xD9) {
                // SOS / EOI
                break;
            }

            let seglen = ((bytes[offset + 2] as usize) << 8) | (bytes[offset + 3] as usize);
            if (seglen < 2 || offset + 2 + seglen > bytes.len) {
                break;
            }

            // All SOF markers (SOF0..SOF3, SOF5..SOF7, SOF9..SOF11, SOF13..SOF15)
            const is_sof = (marker >= 0xC0 && marker <= 0xC3) ||
                (marker >= 0xC5 && marker <= 0xC7) ||
                (marker >= 0xC9 && marker <= 0xCB) ||
                (marker >= 0xCD && marker <= 0xCF);
            if (is_sof) {
                if (seglen < 8) {
                    return null;
                }
                let h = ((bytes[offset + 5] as usize) << 8) | (bytes[offset + 6] as usize);
                let w = ((bytes[offset + 7] as usize) << 8) | (bytes[offset + 8] as usize);
                let comp = (bytes[offset + 9] as usize);
                let is_prog = (marker == 0xC2);

                return JpegHeader{
                    width: w,
                    height: h,
                    components: comp,
                    is_progressive: is_prog,
                    orientation: orientation,
                };
            }

            offset = offset + 2 + seglen;
        }
        return null;
    }

    /// 编码 RGBA8 像素为 Baseline DCT JPEG 字节流
    pub static fn encode(
        allocator: std.mem.Allocator,
        rgba: []const u8,
        width: u32,
        height: u32,
        quality: u8,
    ): ([]u8) ! JpegError {
        return jpegEncode(allocator, rgba, width, height, quality);
    }

    /// 解析 JPEG 帧头与采样/重启间隔信息
    pub static fn parseHeader(bytes: []const u8): JpegHeaderInfo ! JpegError {
        return parseJpegHeader(bytes);
    }

    /// 零堆解码 Baseline DCT JPEG 到调用方外部 RGBA8 切片
    pub static fn decodeToRgbaInto(bytes: []const u8, out_rgba: []u8): JpegHeaderInfo ! JpegError {
        return decodeJpegToRgbaInto(bytes, out_rgba);
    }

    /// 解码 Baseline DCT JPEG 到新分配 RGBA8 图像
    pub static fn decodeToRgba(allocator: std.mem.Allocator, bytes: []const u8): JpegDecodedImage ! JpegError {
        return decodeJpegToRgba(allocator, bytes);
    }
}

/// JPEG 图像编码器与探测器别名类（提供 `JpegImage.encode` / `JpegImage.probe` / `JpegImage.decodeToRgba`）
pub class JpegImage {
    pub static fn probe(bytes: []const u8): JpegHeader? {
        return Jpeg.probe(bytes);
    }

    pub static fn encode(
        allocator: std.mem.Allocator,
        rgba: []const u8,
        width: u32,
        height: u32,
        quality: u8,
    ): ([]u8) ! JpegError {
        return jpegEncode(allocator, rgba, width, height, quality);
    }

    pub static fn parseHeader(bytes: []const u8): JpegHeaderInfo ! JpegError {
        return parseJpegHeader(bytes);
    }

    pub static fn decodeToRgbaInto(bytes: []const u8, out_rgba: []u8): JpegHeaderInfo ! JpegError {
        return decodeJpegToRgbaInto(bytes, out_rgba);
    }

    pub static fn decodeToRgba(allocator: std.mem.Allocator, bytes: []const u8): JpegDecodedImage ! JpegError {
        return decodeJpegToRgba(allocator, bytes);
    }
}

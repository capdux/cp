// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_jpeg.cp | Synced: 2026-11-22T06:39:08Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_jpeg.cp
// Extended with Production Baseline DCT JPEG (ITU-T T.81 / JFIF 1.01) Encoder
// ============================================================================
const std = @import("std");
import { Codecs, ExifOrientation } from "./codecs";
import { Exif } from "./exif";

pub err JpegError {
    InvalidDimensions,
    InvalidBufferLength,
    BufferTooSmall,
    OutOfMemory,
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

/// 可分离 8x8 2D Forward DCT + 量化 + Zig-Zag 重排
fn forwardDctAndQuantize(
    block: [64]f64,
    quant: [64]u8,
    out_zz: *[64]i32,
) {
    let tmp: [64]f64 = [0.0; 64];

    // 1. 行方向 1D DCT
    let y: usize = 0;
    while (y < 8) : (y += 1) {
        const row_off = y * 8;
        let u: usize = 0;
        while (u < 8) : (u += 1) {
            const c_off = u * 8;
            let sum: f64 = 0.0;
            let x: usize = 0;
            while (x < 8) : (x += 1) {
                sum += block[row_off + x] * DCT_COS[c_off + x];
            }
            tmp[row_off + u] = sum;
        }
    }

    // 2. 列方向 1D DCT + 量化
    let qblock: [64]i32 = [0; 64];
    let v: usize = 0;
    while (v < 8) : (v += 1) {
        const c_off = v * 8;
        let u: usize = 0;
        while (u < 8) : (u += 1) {
            let sum: f64 = 0.0;
            let y2: usize = 0;
            while (y2 < 8) : (y2 += 1) {
                sum += tmp[y2 * 8 + u] * DCT_COS[c_off + y2];
            }
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
}

/// JPEG 图像编码器与探测器别名类（提供 `JpegImage.encode` / `JpegImage.probe`）
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
}

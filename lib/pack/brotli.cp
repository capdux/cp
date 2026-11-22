// Source: Absorbed from /data/dev/bun/src/brotli/brotli.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pack/brotli.cp — Brotli (RFC 7932) 压缩边界计算、帧头解析与完整纯 CP 解压引擎
// 吸收自 Bun src/brotli/brotli.cp，扩展完整 RFC 7932 解码器（含静态字典、121 种词变换、
// 4 种精确上下文查表、Canonical Huffman 前缀码与 LZ77 滑动窗口）
// ============================================================================

const std = @import("std");

import {
    BrotliOperation,
    BrotliDecoderStateStruct,
    BrotliEncoderStateStruct,
    BrotliDecoderCreateInstance,
    BrotliDecoderDestroyInstance,
    BrotliEncoderMaxCompressedSize,
    CompressBoundEstimator,
} from "./_codecs.cp";

import {
    BROTLI_DICT_SIZE_BITS,
    BROTLI_DICT_OFFSETS,
    BROTLI_CONTEXT_LUT,
    BROTLI_PREFIX_SUFFIX,
    BROTLI_PREFIX_SUFFIX_MAP,
    BROTLI_TRANSFORMS_DATA,
    BROTLI_STATIC_DICT,
} from "./_brotliDict.cp";

pub const BROTLI_MIN_QUALITY: i32 = 0;
pub const BROTLI_MAX_QUALITY: i32 = 11;
pub const BROTLI_DEFAULT_QUALITY: i32 = 11;

pub const BROTLI_MIN_WINDOW_BITS: i32 = 10;
pub const BROTLI_MAX_WINDOW_BITS: i32 = 24;
pub const BROTLI_LARGE_MAX_WINDOW_BITS: i32 = 30;
pub const BROTLI_DEFAULT_WINDOW: i32 = 22;

pub const BROTLI_MIN_INPUT_BLOCK_BITS: i32 = 16;
pub const BROTLI_MAX_INPUT_BLOCK_BITS: i32 = 24;

pub const BROTLI_DEFAULT_MAX_WINDOW_BYTES: usize = 16777216;
pub const BROTLI_DEFAULT_MAX_OUTPUT_BYTES: usize = 67108864;

/// RFC 7932 Brotli 解压错误集
pub err BrotliError {
    UnexpectedEof,
    InvalidWindowBits,
    InvalidMetaBlock,
    InvalidHuffmanCode,
    InvalidDistance,
    OutputTooLarge,
    WindowTooLarge,
    OutputBufferTooSmall,
    OutOfMemory,
}

/// RFC 7932 Brotli 编码器优化模式
pub enum BrotliMode: i32 {
    /// 默认通用模式，不对输入分布做先验假设
    Generic = 0,
    /// 针对 UTF-8 文本优化的上下文建模模式
    Text = 1,
    /// 针对 WOFF 2.0 字体数据优化的压缩模式
    Font = 2,
}

/// Brotli 流式解码器返回状态码（对标 Bun BrotliReaderArrayList）
pub enum BrotliDecoderResult: i32 {
    Error = 0,
    Success = 1,
    NeedsMoreInput = 2,
    NeedsMoreOutput = 3,
}

/// Brotli 流式状态机阶段
pub enum BrotliStreamState: u8 {
    Uninitialized = 0,
    Inflating = 1,
    End = 2,
    Error = 3,
}

/// Brotli 解码器配置参数（对标 Bun DecoderOptions）
pub struct BrotliDecoderOptions {
    large_window: bool = true,
    disable_ring_buffer_reallocation: bool = false,
}

/// Brotli 编码器配置参数
pub struct BrotliEncoderOptions {
    mode: BrotliMode = BrotliMode.Generic,
    quality: i32 = BROTLI_DEFAULT_QUALITY,
    lgwin: i32 = BROTLI_DEFAULT_WINDOW,
    lgblock: i32 = 0,
    large_window: bool = false,
}

/// RFC 7932 Section 9.1 Brotli 流头解析视图
pub struct BrotliHeader {
    window_bits: u8 = 22,
    is_last: bool = false,
    is_empty: bool = false,

    /// 根据 RFC 7932 Section 9.1 规范解码首字节中的滑动窗口位数 (WBITS: 10..24)
    pub static fn decodeWindowBits(first_byte: u8): u8 {
        if ((first_byte & 0x01) == 0) {
            return 16;
        }
        const low3 = (first_byte >> 1) & 0x07;
        if (low3 != 0) {
            return (17 as u8) + low3;
        }
        const mid3 = (first_byte >> 4) & 0x07;
        if (mid3 == 1) {
            return 0; // RFC 7932 保留非法组合
        }
        if (mid3 == 0) {
            return 17;
        }
        return (8 as u8) + mid3;
    }

    /// 解析 Brotli 字节流起始帧头
    pub static fn parse(src: []const u8): BrotliHeader? {
        if (src.len == 0) {
            return null;
        }
        const b0 = src[0];
        const wbits = BrotliHeader.decodeWindowBits(b0);
        if (wbits == 0) {
            return null;
        }

        // 计算 WBITS 占用的比特数 (1, 4, 或 7 bits)
        let wbits_len: u8 = 7;
        if ((b0 & 0x01) == 0) {
            wbits_len = 1;
        } else if (((b0 >> 1) & 0x07) != 0) {
            wbits_len = 4;
        }

        let is_last = false;
        let is_empty = false;
        if (wbits_len == 1) {
            is_last = ((b0 >> 1) & 0x01) != 0;
            if (is_last) {
                is_empty = ((b0 >> 2) & 0x01) != 0;
            }
        } else if (wbits_len == 4) {
            is_last = ((b0 >> 4) & 0x01) != 0;
            if (is_last) {
                is_empty = ((b0 >> 5) & 0x01) != 0;
            }
        } else {
            is_last = ((b0 >> 7) & 0x01) != 0;
            if (is_last && src.len > 1) {
                is_empty = (src[1] & 0x01) != 0;
            }
        }

        return BrotliHeader{
            window_bits: wbits,
            is_last: is_last,
            is_empty: is_empty,
        };
    }
}

/// 根据 RFC 7932 Section 9.1 由 WBITS (10..24) 计算滑动窗口字节数 `(1 << WBITS) - 16`
pub fn brotliWindowSizeFromBits(window_bits: u8): usize ! BrotliError {
    if (window_bits < 10 || window_bits > 24) {
        return BrotliError.InvalidWindowBits;
    }
    return ((1 as usize) << (window_bits as u6)) - 16;
}

// ============================================================================
// RFC 7932 纯 CP 解压引擎内部实现
// ============================================================================

const BR_MAX_HUFFMAN_BITS: usize = 15;
const BR_NUM_LITERAL_SYMBOLS: usize = 256;
const BR_NUM_INSERT_COPY_SYMBOLS: usize = 704;
const BR_NUM_BLOCK_LEN_SYMBOLS: usize = 26;
const BR_NUM_CODE_LEN_CODES: usize = 18;
const BR_NUM_TRANSFORMS: usize = 121;

const BR_CODE_LEN_ORDER: [18]u8 = [
    1, 2, 3, 4, 0, 5, 17, 6, 16, 7, 8, 9, 10, 11, 12, 13, 14, 15,
];

const BR_BLOCK_LEN_OFFSET: [26]u32 = [
    1, 5, 9, 13, 17, 25, 33, 41, 49, 65, 81, 97, 113,
    145, 177, 209, 241, 305, 369, 497, 753, 1265, 2289, 4337, 8433, 16625,
];

const BR_BLOCK_LEN_NBITS: [26]u6 = [
    2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5,
    5, 5, 5, 6, 6, 7, 8, 9, 10, 11, 12, 13, 24,
];

const BR_INSERT_OFFSET: [24]u32 = [
    0, 1, 2, 3, 4, 5, 6, 8, 10, 14, 18, 26,
    34, 50, 66, 98, 130, 194, 322, 578, 1090, 2114, 6210, 22594,
];

const BR_INSERT_NBITS: [24]u6 = [
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3,
    4, 4, 5, 5, 6, 7, 8, 9, 10, 12, 14, 24,
];

const BR_COPY_OFFSET: [24]u32 = [
    2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 18,
    22, 30, 38, 54, 70, 102, 134, 198, 326, 582, 1094, 2118,
];

const BR_COPY_NBITS: [24]u6 = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2,
    3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 24,
];

const BR_INSERT_RANGE_LUT: [11]u8 = [0, 0, 0, 8, 8, 16, 0, 8, 16, 16, 16];
const BR_COPY_RANGE_LUT: [11]u8 = [0, 8, 0, 0, 8, 0, 16, 16, 8, 16, 16];

/// LSB-first 位流读取器
struct BrBitReader {
    data: []const u8,
    byte_pos: usize,
    bit_pos: u4,

    static fn init(data: []const u8): BrBitReader {
        return BrBitReader{
            data: data,
            byte_pos: 0,
            bit_pos: 0,
        };
    }

    fn readBit(*_): u32 ! BrotliError {
        if (_.byte_pos >= _.data.len) {
            return BrotliError.UnexpectedEof;
        }
        const b = _.data[_.byte_pos];
        const bit = ((b >> (_.bit_pos as u3)) & 1) as u32;
        if (_.bit_pos == 7) {
            _.bit_pos = 0;
            _.byte_pos += 1;
        } else {
            _.bit_pos += 1;
        }
        return bit;
    }

    fn readBits(*_, n: u6): u32 ! BrotliError {
        if (n == 0) {
            return 0;
        }
        let val: u32 = 0;
        let i: u6 = 0;
        while (i < n) : (i += 1) {
            const bit = _.readBit()?;
            val |= (bit << (i as u5));
        }
        return val;
    }

    fn alignToByte(*_): void ! BrotliError {
        if (_.bit_pos != 0) {
            const rem: u6 = (8 - (_.bit_pos as u6));
            const pad = _.readBits(rem)?;
            if (pad != 0) {
                return BrotliError.InvalidMetaBlock;
            }
        }
    }

    fn readBytes(*_, len: usize): []const u8 ! BrotliError {
        if (len > _.data.len - _.byte_pos) {
            return BrotliError.UnexpectedEof;
        }
        const slice = _.data[_.byte_pos .. (_.byte_pos + len)];
        _.byte_pos += len;
        return slice;
    }

    fn skipBytes(*_, len: usize): void ! BrotliError {
        if (len > _.data.len - _.byte_pos) {
            return BrotliError.UnexpectedEof;
        }
        _.byte_pos += len;
    }
}

/// 统一输出缓冲抽象：同时支持动态分配 (`ArrayList`) 与外部零堆切片 (`[]u8`)
struct BrOutputSink {
    ext_buf: []u8,
    use_ext: bool,
    ext_pos: usize,
    list: std.ArrayList(u8),
    alloc: std.mem.Allocator,

    static fn initDynamic(alloc: std.mem.Allocator): BrOutputSink {
        return BrOutputSink{
            ext_buf: std.ArrayList(u8).empty.items,
            use_ext: false,
            ext_pos: 0,
            list: std.ArrayList(u8).empty,
            alloc: alloc,
        };
    }

    static fn initExternal(buf: []u8, alloc: std.mem.Allocator): BrOutputSink {
        return BrOutputSink{
            ext_buf: buf,
            use_ext: true,
            ext_pos: 0,
            list: std.ArrayList(u8).empty,
            alloc: alloc,
        };
    }

    fn deinit(*_) {
        if (!_.use_ext) {
            _.list.deinit(_.alloc);
        }
    }

    fn len(): usize {
        if (_.use_ext) {
            return _.ext_pos;
        }
        return _.list.items.len;
    }

    fn byteAt(idx: usize): u8 {
        if (_.use_ext) {
            return _.ext_buf[idx];
        }
        return _.list.items[idx];
    }

    fn appendByte(*_, b: u8): void ! BrotliError {
        if (_.use_ext) {
            if (_.ext_pos >= _.ext_buf.len) {
                return BrotliError.OutputBufferTooSmall;
            }
            _.ext_buf[_.ext_pos] = b;
            _.ext_pos += 1;
            return;
        }
        _.list.append(_.alloc, b) catch return BrotliError.OutOfMemory;
    }

    fn appendSlice(*_, data: []const u8): void ! BrotliError {
        if (data.len == 0) {
            return;
        }
        if (_.use_ext) {
            if (data.len > _.ext_buf.len - _.ext_pos) {
                return BrotliError.OutputBufferTooSmall;
            }
            @memcpy(_.ext_buf[_.ext_pos .. (_.ext_pos + data.len)], data);
            _.ext_pos += data.len;
            return;
        }
        _.list.appendSlice(_.alloc, data) catch return BrotliError.OutOfMemory;
    }
}

/// Canonical Huffman 前缀码树（RFC 7932 §3.3）
struct BrHuffmanTree {
    counts: [16]u16,
    symbols: []u16,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.symbols.len > 0) {
            alloc.free(_.symbols);
        }
    }

    static fn build(alloc: std.mem.Allocator, lengths: []const u8): BrHuffmanTree ! BrotliError {
        let counts: [16]u16 = [0; 16];
        let nonzero_count: usize = 0;
        let single_sym: u16 = 0;

        let i: usize = 0;
        while (i < lengths.len) : (i += 1) {
            const len = lengths[i];
            if (len > BR_MAX_HUFFMAN_BITS) {
                return BrotliError.InvalidHuffmanCode;
            }
            if (len > 0) {
                counts[len] += 1;
                nonzero_count += 1;
                single_sym = i as u16;
            }
        }

        if (nonzero_count == 0) {
            return BrotliError.InvalidHuffmanCode;
        }

        let symbols = alloc.alloc(u16, nonzero_count) catch return BrotliError.OutOfMemory;
        errdefer alloc.free(symbols);

        if (nonzero_count == 1) {
            symbols[0] = single_sym;
            return BrHuffmanTree{
                counts: counts,
                symbols: symbols,
            };
        }

        let left: i32 = 1;
        let b: usize = 1;
        while (b <= BR_MAX_HUFFMAN_BITS) : (b += 1) {
            left <<= 1;
            left -= counts[b] as i32;
            if (left < 0) {
                return BrotliError.InvalidHuffmanCode;
            }
        }
        if (left != 0) {
            return BrotliError.InvalidHuffmanCode;
        }

        let offsets: [16]u16 = [0; 16];
        let total: u16 = 0;
        b = 1;
        while (b <= BR_MAX_HUFFMAN_BITS) : (b += 1) {
            offsets[b] = total;
            total += counts[b];
        }

        i = 0;
        while (i < lengths.len) : (i += 1) {
            const len = lengths[i];
            if (len > 0) {
                symbols[offsets[len]] = i as u16;
                offsets[len] += 1;
            }
        }

        return BrHuffmanTree{
            counts: counts,
            symbols: symbols,
        };
    }

    fn decode(br: *BrBitReader): u16 ! BrotliError {
        if (_.symbols.len == 1) {
            return _.symbols[0];
        }
        let code: i32 = 0;
        let first: i32 = 0;
        let index: i32 = 0;
        let len: usize = 1;
        while (len <= BR_MAX_HUFFMAN_BITS) : (len += 1) {
            const bit = br.readBit()? as i32;
            code |= bit;
            const count = _.counts[len] as i32;
            if (code - count < first) {
                const idx = (index + (code - first)) as usize;
                return _.symbols[idx];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return BrotliError.InvalidHuffmanCode;
    }
}

fn brBitWidth(alphabet_size: usize): u6 {
    let bits: u6 = 0;
    let v: usize = 1;
    while (v < alphabet_size) {
        v <<= 1;
        bits += 1;
    }
    return bits;
}

/// RFC 7932 §3.5 码长前缀码的固定编码解码（LSB-first: 00->0, 01->4, 10->3, 011->2, 0111->1, 1111->5）
fn brReadCodeLengthCode(br: *BrBitReader): u8 ! BrotliError {
    const b0 = br.readBit()?;
    const b1 = br.readBit()?;
    const low2 = b0 | (b1 << 1);
    if (low2 == 0) {
        return 0;
    } else if (low2 == 1) {
        return 4;
    } else if (low2 == 2) {
        return 3;
    } else {
        const b2 = br.readBit()?;
        if (b2 == 0) {
            return 2;
        }
        const b3 = br.readBit()?;
        if (b3 == 0) {
            return 1;
        }
        return 5;
    }
}

fn brReadPrefixCode(alloc: std.mem.Allocator, br: *BrBitReader, alphabet_size: usize): BrHuffmanTree ! BrotliError {
    let lengths = alloc.alloc(u8, alphabet_size) catch return BrotliError.OutOfMemory;
    defer alloc.free(lengths);
    @memset(lengths, 0);

    const hskip = br.readBits(2)?;
    if (hskip == 1) {
        // Simple prefix code (RFC 7932 §3.4)
        const nsym = (br.readBits(2)? as usize) + 1;
        const max_bits = brBitWidth(alphabet_size);
        let syms: [4]usize = [0; 4];
        let i: usize = 0;
        while (i < nsym) : (i += 1) {
            const sym = br.readBits(max_bits)? as usize;
            if (sym >= alphabet_size) {
                return BrotliError.InvalidHuffmanCode;
            }
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (syms[j] == sym) {
                    return BrotliError.InvalidHuffmanCode;
                }
            }
            syms[i] = sym;
        }
        if (nsym == 1) {
            lengths[syms[0]] = 1;
            return BrHuffmanTree.build(alloc, lengths);
        } else if (nsym == 2) {
            lengths[syms[0]] = 1;
            lengths[syms[1]] = 1;
        } else if (nsym == 3) {
            lengths[syms[0]] = 1;
            lengths[syms[1]] = 2;
            lengths[syms[2]] = 2;
        } else {
            const tree_select = br.readBit()?;
            if (tree_select == 0) {
                lengths[syms[0]] = 2;
                lengths[syms[1]] = 2;
                lengths[syms[2]] = 2;
                lengths[syms[3]] = 2;
            } else {
                lengths[syms[0]] = 1;
                lengths[syms[1]] = 2;
                lengths[syms[2]] = 3;
                lengths[syms[3]] = 3;
            }
        }
        return BrHuffmanTree.build(alloc, lengths);
    }

    // Complex prefix code (RFC 7932 §3.5)
    let cl_lengths: [18]u8 = [0; 18];
    let space: i32 = 32;
    let num_codes: usize = 0;
    let idx: usize = hskip as usize;
    while (idx < BR_NUM_CODE_LEN_CODES && space > 0) : (idx += 1) {
        const code_idx = BR_CODE_LEN_ORDER[idx] as usize;
        const cl_len = brReadCodeLengthCode(br)?;
        cl_lengths[code_idx] = cl_len;
        if (cl_len != 0) {
            space -= ((32 as i32) >> (cl_len as u5));
            num_codes += 1;
        }
    }
    if (num_codes == 0 || (num_codes > 1 && space != 0)) {
        return BrotliError.InvalidHuffmanCode;
    }

    let cl_tree = BrHuffmanTree.build(alloc, &cl_lengths)?;
    defer cl_tree.deinit(alloc);

    let sym_idx: usize = 0;
    let prev_len: u8 = 8;
    let repeat_code: u16 = 0;
    let repeat: usize = 0;
    let cl_space: i32 = 32768;

    while (sym_idx < alphabet_size && cl_space > 0) {
        const code = cl_tree.decode(br)?;
        if (code < 16) {
            repeat = 0;
            lengths[sym_idx] = code as u8;
            sym_idx += 1;
            if (code != 0) {
                prev_len = code as u8;
                cl_space -= ((32768 as i32) >> (code as u5));
            }
        } else {
            const extra_bits: u6 = if (code == 16) 2 else 3;
            const rep_val: u8 = if (code == 16) prev_len else 0;
            if (repeat_code != code) {
                repeat = 0;
                repeat_code = code;
            }
            const old_repeat = repeat;
            if (repeat > 0) {
                repeat -= 2;
                repeat <<= (extra_bits as u5);
            }
            repeat += (br.readBits(extra_bits)? as usize) + 3;
            const delta = repeat - old_repeat;
            if (sym_idx + delta > alphabet_size) {
                return BrotliError.InvalidHuffmanCode;
            }
            let k: usize = 0;
            while (k < delta) : (k += 1) {
                lengths[sym_idx] = rep_val;
                sym_idx += 1;
            }
            if (rep_val != 0) {
                cl_space -= (delta as i32) * ((32768 as i32) >> (rep_val as u5));
            }
        }
    }

    if (cl_space != 0) {
        return BrotliError.InvalidHuffmanCode;
    }

    return BrHuffmanTree.build(alloc, lengths);
}

fn brDecodeVarUint8(br: *BrBitReader): usize ! BrotliError {
    const flag = br.readBit()?;
    if (flag == 0) {
        return 0;
    }
    const nbits = br.readBits(3)? as u6;
    if (nbits == 0) {
        return 1;
    }
    return ((1 as usize) << (nbits as u5)) + (br.readBits(nbits)? as usize);
}

fn brDecodeBlockLength(tree: *const BrHuffmanTree, br: *BrBitReader): usize ! BrotliError {
    const sym = tree.decode(br)? as usize;
    if (sym >= BR_NUM_BLOCK_LEN_SYMBOLS) {
        return BrotliError.InvalidHuffmanCode;
    }
    const extra = br.readBits(BR_BLOCK_LEN_NBITS[sym])?;
    return (BR_BLOCK_LEN_OFFSET[sym] + extra) as usize;
}

fn brDecodeBlockType(
    num_types: usize,
    type_tree: *const BrHuffmanTree,
    br: *BrBitReader,
    cur_type: *usize,
    prev_type: *usize,
): void ! BrotliError {
    const sym = type_tree.decode(br)? as usize;
    let next_type: usize = 0;
    if (sym == 0) {
        next_type = prev_type.*;
    } else if (sym == 1) {
        next_type = cur_type.* + 1;
        if (next_type >= num_types) {
            next_type = 0;
        }
    } else {
        next_type = sym - 2;
        if (next_type >= num_types) {
            return BrotliError.InvalidMetaBlock;
        }
    }
    prev_type.* = cur_type.*;
    cur_type.* = next_type;
}

struct BrContextMapResult {
    num_trees: usize,
    map: []u8,
}

fn brDecodeContextMap(
    alloc: std.mem.Allocator,
    br: *BrBitReader,
    map_size: usize,
): BrContextMapResult ! BrotliError {
    const num_trees = brDecodeVarUint8(br)? + 1;
    let map = alloc.alloc(u8, map_size) catch return BrotliError.OutOfMemory;
    errdefer alloc.free(map);

    if (num_trees == 1) {
        @memset(map, 0);
        return BrContextMapResult{ num_trees: 1, map: map };
    }

    const use_rle = br.readBit()?;
    let max_run_len_prefix: usize = 0;
    if (use_rle != 0) {
        max_run_len_prefix = (br.readBits(4)? as usize) + 1;
    }

    let tree = brReadPrefixCode(alloc, br, num_trees + max_run_len_prefix)?;
    defer tree.deinit(alloc);

    let i: usize = 0;
    while (i < map_size) {
        const code = tree.decode(br)? as usize;
        if (code == 0) {
            map[i] = 0;
            i += 1;
        } else if (code <= max_run_len_prefix) {
            const reps = ((1 as usize) << (code as u5)) + (br.readBits(code as u6)? as usize);
            let k: usize = 0;
            while (k < reps) : (k += 1) {
                if (i >= map_size) {
                    return BrotliError.InvalidMetaBlock;
                }
                map[i] = 0;
                i += 1;
            }
        } else {
            map[i] = (code - max_run_len_prefix) as u8;
            i += 1;
        }
    }

    const imtf = br.readBit()?;
    if (imtf != 0) {
        let mtf: [256]u8 = [0; 256];
        let m: usize = 0;
        while (m < 256) : (m += 1) {
            mtf[m] = m as u8;
        }
        i = 0;
        while (i < map_size) : (i += 1) {
            const idx = map[i] as usize;
            const val = mtf[idx];
            map[i] = val;
            let j = idx;
            while (j > 0) : (j -= 1) {
                mtf[j] = mtf[j - 1];
            }
            mtf[0] = val;
        }
    }

    return BrContextMapResult{ num_trees: num_trees, map: map };
}

/// RFC 7932 §7.1 精确 4 模式字面量上下文查表 (LSB6, MSB6, UTF8, Signed)
fn brLiteralContext(mode: u8, prev1: u8, prev2: u8): usize {
    const base = (mode as usize) * 512;
    const c1 = BROTLI_CONTEXT_LUT[base + (prev1 as usize)] as usize;
    const c2 = BROTLI_CONTEXT_LUT[base + 256 + (prev2 as usize)] as usize;
    return (c1 | c2) & 0x3f;
}

/// RFC 7932 §8 UTF-8 简化大写变换
fn brToUpperCaseUtf8(buf: []u8, offset: usize): usize {
    const b0 = buf[offset];
    if (b0 < 0xC0) {
        if (b0 >= 97 && b0 <= 122) {
            buf[offset] = b0 ^ 32;
        }
        return 1;
    }
    if (b0 < 0xE0) {
        if (offset + 1 < buf.len) {
            buf[offset + 1] ^= 32;
        }
        return 2;
    }
    if (offset + 2 < buf.len) {
        buf[offset + 2] ^= 5;
    }
    return 3;
}

/// RFC 7932 §8 对静态字典词应用 121 种标准词变换（前缀 + 裁切/大写 + 后缀）
fn brApplyDictTransform(dst: []u8, word: []const u8, transform_idx: usize): usize {
    const base = transform_idx * 3;
    const prefix_id = BROTLI_TRANSFORMS_DATA[base] as usize;
    const t_type = BROTLI_TRANSFORMS_DATA[base + 1];
    const suffix_id = BROTLI_TRANSFORMS_DATA[base + 2] as usize;

    let idx: usize = 0;

    // 1. Prefix
    const p_off = BROTLI_PREFIX_SUFFIX_MAP[prefix_id] as usize;
    const p_len = BROTLI_PREFIX_SUFFIX[p_off] as usize;
    if (p_len > 0) {
        @memcpy(dst[idx .. (idx + p_len)], BROTLI_PREFIX_SUFFIX[(p_off + 1) .. (p_off + 1 + p_len)]);
        idx += p_len;
    }

    // 2. Transformed word body
    let w_start: usize = 0;
    let w_len: usize = word.len;
    if (t_type <= 9) {
        const omit = t_type as usize;
        if (omit >= w_len) {
            w_len = 0;
        } else {
            w_len -= omit;
        }
    } else if (t_type >= 12 && t_type <= 20) {
        const skip = (t_type - 11) as usize;
        if (skip >= w_len) {
            w_start = word.len;
            w_len = 0;
        } else {
            w_start = skip;
            w_len -= skip;
        }
    }

    const word_dst_start = idx;
    if (w_len > 0) {
        @memcpy(dst[idx .. (idx + w_len)], word[w_start .. (w_start + w_len)]);
        idx += w_len;
    }

    if (t_type == 10 && w_len > 0) {
        * = brToUpperCaseUtf8(dst[0..idx], word_dst_start);
    } else if (t_type == 11 && w_len > 0) {
        let u_pos = word_dst_start;
        while (u_pos < idx) {
            const step = brToUpperCaseUtf8(dst[0..idx], u_pos);
            u_pos += step;
        }
    }

    // 3. Suffix
    const s_off = BROTLI_PREFIX_SUFFIX_MAP[suffix_id] as usize;
    const s_len = BROTLI_PREFIX_SUFFIX[s_off] as usize;
    if (s_len > 0) {
        @memcpy(dst[idx .. (idx + s_len)], BROTLI_PREFIX_SUFFIX[(s_off + 1) .. (s_off + 1 + s_len)]);
        idx += s_len;
    }

    return idx;
}

fn brFreeTreeSlice(alloc: std.mem.Allocator, trees: []BrHuffmanTree) {
    let i: usize = 0;
    while (i < trees.len) : (i += 1) {
        trees[i].deinit(alloc);
    }
    alloc.free(trees);
}

fn brDecodeWindowBits(br: *BrBitReader): u8 ! BrotliError {
    const first = br.readBit()?;
    if (first == 0) {
        return 16;
    }
    const n = br.readBits(3)?;
    if (n != 0) {
        return (17 + n) as u8;
    }
    const m = br.readBits(3)?;
    if (m == 0) {
        return 17;
    }
    if (m == 1) {
        return BrotliError.InvalidWindowBits;
    }
    return (8 + m) as u8;
}

fn brDecodeCompressedMetaBlock(
    alloc: std.mem.Allocator,
    br: *BrBitReader,
    sink: *BrOutputSink,
    mlen: usize,
    max_window_size: usize,
    dist_rb: *[4]usize,
    dist_rb_idx: *usize,
): void ! BrotliError {
    const start_out_len = sink.len();
    const target_out_len = start_out_len + mlen;

    // 1. Block types and block length prefix codes (Literal, Insert-and-Copy, Distance)
    const nbltypes_l = brDecodeVarUint8(br)? + 1;
    let l_type_tree: BrHuffmanTree = undefined;
    let l_len_tree: BrHuffmanTree = undefined;
    let has_l_type = false;
    let has_l_len = false;
    let has_l_switch = false;
    let l_block_len: usize = 0;
    if (nbltypes_l > 1) {
        l_type_tree = brReadPrefixCode(alloc, br, nbltypes_l + 2)?;
        has_l_type = true;
        l_len_tree = brReadPrefixCode(alloc, br, BR_NUM_BLOCK_LEN_SYMBOLS)?;
        has_l_len = true;
        has_l_switch = true;
        l_block_len = brDecodeBlockLength(&l_len_tree, br)?;
    }
    defer if (has_l_type) { l_type_tree.deinit(alloc); }
    defer if (has_l_len) { l_len_tree.deinit(alloc); }

    const nbltypes_i = brDecodeVarUint8(br)? + 1;
    let i_type_tree: BrHuffmanTree = undefined;
    let i_len_tree: BrHuffmanTree = undefined;
    let has_i_type = false;
    let has_i_len = false;
    let has_i_switch = false;
    let i_block_len: usize = 0;
    if (nbltypes_i > 1) {
        i_type_tree = brReadPrefixCode(alloc, br, nbltypes_i + 2)?;
        has_i_type = true;
        i_len_tree = brReadPrefixCode(alloc, br, BR_NUM_BLOCK_LEN_SYMBOLS)?;
        has_i_len = true;
        has_i_switch = true;
        i_block_len = brDecodeBlockLength(&i_len_tree, br)?;
    }
    defer if (has_i_type) { i_type_tree.deinit(alloc); }
    defer if (has_i_len) { i_len_tree.deinit(alloc); }

    const nbltypes_d = brDecodeVarUint8(br)? + 1;
    let d_type_tree: BrHuffmanTree = undefined;
    let d_len_tree: BrHuffmanTree = undefined;
    let has_d_type = false;
    let has_d_len = false;
    let has_d_switch = false;
    let d_block_len: usize = 0;
    if (nbltypes_d > 1) {
        d_type_tree = brReadPrefixCode(alloc, br, nbltypes_d + 2)?;
        has_d_type = true;
        d_len_tree = brReadPrefixCode(alloc, br, BR_NUM_BLOCK_LEN_SYMBOLS)?;
        has_d_len = true;
        has_d_switch = true;
        d_block_len = brDecodeBlockLength(&d_len_tree, br)?;
    }
    defer if (has_d_type) { d_type_tree.deinit(alloc); }
    defer if (has_d_len) { d_len_tree.deinit(alloc); }

    // 2. Distance parameters: NPOSTFIX (2 bits), NDIRECT (4 bits << NPOSTFIX)
    const npostfix = br.readBits(2)?;
    const ndirect = br.readBits(4)? << (npostfix as u5);
    const num_dist_syms = 16 + (ndirect as usize) + ((48 as usize) << (npostfix as u5));

    // 3. Context modes for literal block types
    let context_modes = alloc.alloc(u8, nbltypes_l) catch return BrotliError.OutOfMemory;
    defer alloc.free(context_modes);
    let cm_i: usize = 0;
    while (cm_i < nbltypes_l) : (cm_i += 1) {
        context_modes[cm_i] = br.readBits(2)? as u8;
    }

    // 4. Context maps for Literals and Distances
    const lit_cmap = brDecodeContextMap(alloc, br, nbltypes_l * 64)?;
    defer alloc.free(lit_cmap.map);

    const dist_cmap = brDecodeContextMap(alloc, br, nbltypes_d * 4)?;
    defer alloc.free(dist_cmap.map);

    // 5. Prefix Code Trees
    let lit_trees = alloc.alloc(BrHuffmanTree, lit_cmap.num_trees) catch return BrotliError.OutOfMemory;
    let lit_built: usize = 0;
    errdefer brFreeTreeSlice(alloc, lit_trees[0..lit_built]);
    while (lit_built < lit_cmap.num_trees) : (lit_built += 1) {
        lit_trees[lit_built] = brReadPrefixCode(alloc, br, BR_NUM_LITERAL_SYMBOLS)?;
    }
    defer brFreeTreeSlice(alloc, lit_trees);

    let iac_trees = alloc.alloc(BrHuffmanTree, nbltypes_i) catch return BrotliError.OutOfMemory;
    let iac_built: usize = 0;
    errdefer brFreeTreeSlice(alloc, iac_trees[0..iac_built]);
    while (iac_built < nbltypes_i) : (iac_built += 1) {
        iac_trees[iac_built] = brReadPrefixCode(alloc, br, BR_NUM_INSERT_COPY_SYMBOLS)?;
    }
    defer brFreeTreeSlice(alloc, iac_trees);

    let dist_trees = alloc.alloc(BrHuffmanTree, dist_cmap.num_trees) catch return BrotliError.OutOfMemory;
    let dist_built: usize = 0;
    errdefer brFreeTreeSlice(alloc, dist_trees[0..dist_built]);
    while (dist_built < dist_cmap.num_trees) : (dist_built += 1) {
        dist_trees[dist_built] = brReadPrefixCode(alloc, br, num_dist_syms)?;
    }
    defer brFreeTreeSlice(alloc, dist_trees);

    // 6. Main Meta-Block Decode Loop
    let l_type: usize = 0;
    let l_prev_type: usize = 1;
    let i_type: usize = 0;
    let i_prev_type: usize = 1;
    let d_type: usize = 0;
    let d_prev_type: usize = 1;

    while (sink.len() < target_out_len) {
        if (has_i_switch) {
            if (i_block_len == 0) {
                brDecodeBlockType(nbltypes_i, &i_type_tree, br, &i_type, &i_prev_type)?;
                i_block_len = brDecodeBlockLength(&i_len_tree, br)?;
            }
            i_block_len -= 1;
        }

        const cmd = iac_trees[i_type].decode(br)? as usize;
        const range_idx = cmd >> 6;
        const use_zero_dist = range_idx < 2;
        const ins_code = (BR_INSERT_RANGE_LUT[range_idx] as usize) + ((cmd >> 3) & 7);
        const copy_code = (BR_COPY_RANGE_LUT[range_idx] as usize) + (cmd & 7);

        const ins_len = (BR_INSERT_OFFSET[ins_code] + br.readBits(BR_INSERT_NBITS[ins_code])?) as usize;
        const copy_len = (BR_COPY_OFFSET[copy_code] + br.readBits(BR_COPY_NBITS[copy_code])?) as usize;

        // Emit literals
        let k: usize = 0;
        while (k < ins_len) : (k += 1) {
            if (sink.len() >= target_out_len) {
                return BrotliError.InvalidMetaBlock;
            }
            if (has_l_switch) {
                if (l_block_len == 0) {
                    brDecodeBlockType(nbltypes_l, &l_type_tree, br, &l_type, &l_prev_type)?;
                    l_block_len = brDecodeBlockLength(&l_len_tree, br)?;
                }
                l_block_len -= 1;
            }

            const n = sink.len();
            const prev1: u8 = if (n >= 1) sink.byteAt(n - 1) else 0;
            const prev2: u8 = if (n >= 2) sink.byteAt(n - 2) else 0;
            const ctx = brLiteralContext(context_modes[l_type], prev1, prev2);
            const tree_idx = lit_cmap.map[l_type * 64 + ctx] as usize;
            const lit = lit_trees[tree_idx].decode(br)? as u8;
            sink.appendByte(lit)?;
        }

        if (sink.len() >= target_out_len) {
            break;
        }

        // Decode distance (RFC 7932 §4)
        let dist: usize = 0;
        let should_push_rb = false;
        if (use_zero_dist) {
            dist = dist_rb[(dist_rb_idx.* + 3) & 3];
        } else {
            if (has_d_switch) {
                if (d_block_len == 0) {
                    brDecodeBlockType(nbltypes_d, &d_type_tree, br, &d_type, &d_prev_type)?;
                    d_block_len = brDecodeBlockLength(&d_len_tree, br)?;
                }
                d_block_len -= 1;
            }

            const d_ctx: usize = if (copy_len > 4) 3 else (copy_len - 2);
            const d_tree_idx = dist_cmap.map[d_type * 4 + d_ctx] as usize;
            const d_sym = dist_trees[d_tree_idx].decode(br)? as usize;

            if (d_sym == 0) {
                dist = dist_rb[(dist_rb_idx.* + 3) & 3];
            } else if (d_sym < 4) {
                dist = dist_rb[(dist_rb_idx.* + 3 - d_sym) & 3];
                should_push_rb = true;
            } else if (d_sym < 10) {
                const last_d = dist_rb[(dist_rb_idx.* + 3) & 3];
                const delta = ((d_sym - 4) >> 1) + 1;
                if ((d_sym & 1) == 0) {
                    if (last_d <= delta) {
                        return BrotliError.InvalidDistance;
                    }
                    dist = last_d - delta;
                } else {
                    dist = last_d + delta;
                }
                should_push_rb = true;
            } else if (d_sym < 16) {
                const second_last = dist_rb[(dist_rb_idx.* + 2) & 3];
                const delta = ((d_sym - 10) >> 1) + 1;
                if ((d_sym & 1) == 0) {
                    if (second_last <= delta) {
                        return BrotliError.InvalidDistance;
                    }
                    dist = second_last - delta;
                } else {
                    dist = second_last + delta;
                }
                should_push_rb = true;
            } else if (d_sym < 16 + (ndirect as usize)) {
                dist = d_sym - 15;
                should_push_rb = true;
            } else {
                const dcode = d_sym - 16 - (ndirect as usize);
                const ndistbits: u6 = (1 + (dcode >> ((npostfix as u5) + 1))) as u6;
                if (ndistbits > 24) {
                    return BrotliError.InvalidDistance;
                }
                const hcode = dcode >> (npostfix as u5);
                const postfix_mask = ((1 as usize) << (npostfix as u5)) - 1;
                const lcode = dcode & postfix_mask;
                const offset_val: usize = (((2 + (hcode & 1)) as usize) << (ndistbits as u5)) - 4;
                const extra_bits = br.readBits(ndistbits)? as usize;
                dist = (((offset_val + extra_bits) << (npostfix as u5)) + lcode + (ndirect as usize) + 1);
                should_push_rb = true;
            }
        }

        if (dist == 0) {
            return BrotliError.InvalidDistance;
        }

        const cur_len = sink.len();
        const max_backward = if (cur_len < max_window_size) cur_len else max_window_size;

        if (dist <= max_backward) {
            // Standard LZ77 backward copy (supports overlapping copy)
            if (should_push_rb) {
                dist_rb[dist_rb_idx.* & 3] = dist;
                dist_rb_idx.* += 1;
            }
            if (cur_len + copy_len > target_out_len) {
                return BrotliError.InvalidMetaBlock;
            }
            let c: usize = 0;
            while (c < copy_len) : (c += 1) {
                const b = sink.byteAt(sink.len() - dist);
                sink.appendByte(b)?;
            }
        } else {
            // RFC 7932 §8 Static Dictionary Word Reference (must NOT push dist to dist_rb)
            if (copy_len < 4 || copy_len > 24) {
                return BrotliError.InvalidDistance;
            }
            const word_id = dist - max_backward - 1;
            const n_bits = BROTLI_DICT_SIZE_BITS[copy_len];
            if (n_bits == 0) {
                return BrotliError.InvalidDistance;
            }
            const index_mask = ((1 as usize) << (n_bits as u5)) - 1;
            const dict_index = word_id & index_mask;
            const transform_id = word_id >> (n_bits as u5);
            if (transform_id >= BR_NUM_TRANSFORMS) {
                return BrotliError.InvalidDistance;
            }
            const base_off = (BROTLI_DICT_OFFSETS[copy_len] as usize) + dict_index * copy_len;
            const word = BROTLI_STATIC_DICT[base_off .. (base_off + copy_len)];
            let word_buf: [64]u8 = [0; 64];
            const out_word_len = brApplyDictTransform(word_buf[0..], word, transform_id);
            if (sink.len() + out_word_len > target_out_len) {
                return BrotliError.InvalidMetaBlock;
            }
            sink.appendSlice(word_buf[0..out_word_len])?;
        }
    }
}

fn brDecodeStream(
    work_alloc: std.mem.Allocator,
    compressed: []const u8,
    sink: *BrOutputSink,
    max_window_bytes: usize,
    max_output_bytes: usize,
): void ! BrotliError {
    if (compressed.len == 0) {
        return BrotliError.UnexpectedEof;
    }

    let br = BrBitReader.init(compressed);
    const wbits = brDecodeWindowBits(&br)?;
    const window_size = brotliWindowSizeFromBits(wbits)?;
    if (window_size > max_window_bytes) {
        return BrotliError.WindowTooLarge;
    }

    let dist_rb: [4]usize = [16, 15, 11, 4];
    let dist_rb_idx: usize = 0;

    while (true) {
        const is_last = br.readBit()?;
        if (is_last != 0) {
            const is_last_empty = br.readBit()?;
            if (is_last_empty != 0) {
                break;
            }
        }

        const mnibbles = br.readBits(2)?;
        if (mnibbles == 3) {
            // Metadata meta-block (RFC 7932 §9.2)
            const reserved = br.readBit()?;
            if (reserved != 0) {
                return BrotliError.InvalidMetaBlock;
            }
            const mskipbits = br.readBits(2)?;
            let mskipbytes: usize = 0;
            if (mskipbits > 0) {
                const skip_val = br.readBits((mskipbits * 8) as u6)?;
                if (mskipbits > 1 && (skip_val >> (((mskipbits - 1) * 8) as u5)) == 0) {
                    return BrotliError.InvalidMetaBlock;
                }
                mskipbytes = (skip_val as usize) + 1;
            }
            br.alignToByte()?;
            br.skipBytes(mskipbytes)?;
            if (is_last != 0) {
                break;
            }
            continue;
        }

        const nibble_count = mnibbles + 4;
        const mlen_m1 = br.readBits((nibble_count * 4) as u6)?;
        if (mnibbles > 0 && (mlen_m1 >> (((nibble_count - 1) * 4) as u5)) == 0) {
            return BrotliError.InvalidMetaBlock;
        }
        const mlen = (mlen_m1 as usize) + 1;

        if (sink.use_ext && (mlen > sink.ext_buf.len || sink.len() > sink.ext_buf.len - mlen)) {
            return BrotliError.OutputBufferTooSmall;
        }
        if (mlen > max_output_bytes || sink.len() > max_output_bytes - mlen) {
            return BrotliError.OutputTooLarge;
        }

        let is_uncompressed: u32 = 0;
        if (is_last == 0) {
            is_uncompressed = br.readBit()?;
        }

        if (is_uncompressed != 0) {
            br.alignToByte()?;
            const raw = br.readBytes(mlen)?;
            sink.appendSlice(raw)?;
        } else {
            brDecodeCompressedMetaBlock(
                work_alloc,
                &br,
                sink,
                mlen,
                window_size,
                &dist_rb,
                &dist_rb_idx,
            )?;
        }

        if (is_last != 0) {
            break;
        }
    }
}

/// 完整 RFC 7932 Brotli 解压器（带自定义窗口与输出体积上限防护）
pub fn brotliDecompressWithLimits(
    allocator: std.mem.Allocator,
    compressed: []const u8,
    max_window_bytes: usize,
    max_output_bytes: usize,
): []u8 ! BrotliError {
    let sink = BrOutputSink.initDynamic(allocator);
    errdefer sink.deinit();

    brDecodeStream(allocator, compressed, &sink, max_window_bytes, max_output_bytes)?;
    return sink.list.toOwnedSlice(allocator) catch return BrotliError.OutOfMemory;
}

/// 完整 RFC 7932 Brotli 解压器（使用默认安全上限：16MiB 窗口 / 64MiB 输出）
pub fn brotliDecompress(
    allocator: std.mem.Allocator,
    compressed: []const u8,
): []u8 ! BrotliError {
    return brotliDecompressWithLimits(
        allocator,
        compressed,
        BROTLI_DEFAULT_MAX_WINDOW_BYTES,
        BROTLI_DEFAULT_MAX_OUTPUT_BYTES,
    );
}

/// 完整 RFC 7932 Brotli 零输出堆分配解压器：直接写入调用方提供的外部切片 `out: *[]u8` 并返回实际写入字节数
pub fn brotliDecompressInto(
    compressed: []const u8,
    out: *[]u8,
): usize ! BrotliError {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const work_alloc = arena.allocator();

    let sink = BrOutputSink.initExternal(out, work_alloc);
    brDecodeStream(
        work_alloc,
        compressed,
        &sink,
        BROTLI_DEFAULT_MAX_WINDOW_BYTES,
        out.len,
    )?;
    return sink.ext_pos;
}

/// Brotli 面向对象门面与边界预估器
pub struct Brotli {
    /// 纯算术预估 Brotli 压缩最大输出字节数（对标 BrotliEncoderMaxCompressedSize，零 C 依赖）
    pub static fn bound(source_len: usize): usize {
        return CompressBoundEstimator.brotliBound(source_len);
    }

    /// 计算单次 Brotli 压缩所需的最大目标缓冲区长度
    pub static fn compressBound(source_len: usize): usize {
        return CompressBoundEstimator.brotliBound(source_len);
    }

    /// 校验压缩质量参数是否处于合法区间 [0, 11]
    pub static fn isValidQuality(quality: i32): bool {
        return quality >= BROTLI_MIN_QUALITY && quality <= BROTLI_MAX_QUALITY;
    }

    /// 校验滑动窗口位数 lgwin 是否处于合法区间 [10, 24]（或 Large Window 模式下 [10, 30]）
    pub static fn isValidWindowBits(lgwin: i32, large_window: bool = false): bool {
        const max_w = if (large_window) BROTLI_LARGE_MAX_WINDOW_BITS else BROTLI_MAX_WINDOW_BITS;
        return lgwin >= BROTLI_MIN_WINDOW_BITS && lgwin <= max_w;
    }

    /// 解压 RFC 7932 Brotli 压缩数据（分配器版本）
    pub static fn decompress(allocator: std.mem.Allocator, compressed: []const u8): []u8 ! BrotliError {
        return brotliDecompress(allocator, compressed);
    }

    /// 解压 RFC 7932 Brotli 压缩数据（带自定义窗口与输出体积上限）
    pub static fn decompressWithLimits(
        allocator: std.mem.Allocator,
        compressed: []const u8,
        max_window_bytes: usize,
        max_output_bytes: usize,
    ): []u8 ! BrotliError {
        return brotliDecompressWithLimits(allocator, compressed, max_window_bytes, max_output_bytes);
    }

    /// 解压 RFC 7932 Brotli 压缩数据至外部目标切片（零输出堆分配）
    pub static fn decompressInto(compressed: []const u8, out: *[]u8): usize ! BrotliError {
        return brotliDecompressInto(compressed, out);
    }
}

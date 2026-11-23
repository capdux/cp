// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_webp.cp | Synced: 2026-11-22T06:39:08Z
// Reference-hardened against image-rs/image-webp (MIT/Apache-2.0)
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_webp.cp
// Extended with Production RFC 9649 WebP Lossless (VP8L) Encoder, Full VP8L
// Decoder (5-Group Canonical Huffman, LZ77 + 2D Distance Map, Color Cache,
// Predictor/Cross-Color/SubtractGreen/Palette Transforms) & VP8X Container Parser
// ============================================================================
const std = @import("std");
import { Codecs } from "./codecs";

pub err WebpError {
    InvalidDimensions,
    InvalidBufferLength,
    DimensionTooLarge,
    OutOfMemory,
    InvalidContainer,
    UnsupportedFormat,
    InvalidBitstream,
    InvalidHuffman,
    InvalidTransform,
    InvalidColorCacheBits,
    BufferTooSmall,
    UnexpectedEof,
}

pub struct WebpHeader {
    pub width: usize,
    pub height: usize,
    pub has_alpha: bool,
    pub is_lossless: bool,
    pub is_animation: bool,
}

// ============================================================================
// RFC 9649 WebP Lossless (VP8L) 编码器实现
// ============================================================================

const VP8L_MAX_DIM: u32 = 16384;
const VP8L_MAX_MATCH_LEN: usize = 4096;

/// RFC 9649 §4.2.2 二级 Code Length Code 顺序（注意：16 位于索引 8，紧跟 5 之后）
const CODE_LEN_ORDER: [19]usize = [
    17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
];

/// VP8L LSB-first 位写入器
struct Vp8lBitWriter {
    alloc: std.mem.Allocator,
    buf: *std.ArrayList(u8),
    bit_buf: u64,
    bit_count: u8,

    fn vp8lWriteBits(*_, val: u32, nbits: u8): void ! WebpError {
        if (nbits == 0) {
            return;
        }
        const one: u64 = 1;
        const mask: u64 = (one << nbits) - one;
        _.bit_buf |= ((val as u64) & mask) << _.bit_count;
        _.bit_count += nbits;
        while (_.bit_count >= 8) {
            const b: u8 = (_.bit_buf & 0xFF) as u8;
            _.buf.append(_.alloc, b) catch return WebpError.OutOfMemory;
            _.bit_buf >>= 8;
            _.bit_count -= 8;
        }
    }

    fn flush(*_): void ! WebpError {
        if (_.bit_count > 0) {
            const b: u8 = (_.bit_buf & 0xFF) as u8;
            _.buf.append(_.alloc, b) catch return WebpError.OutOfMemory;
            _.bit_buf = 0;
            _.bit_count = 0;
        }
    }
}

/// 反转 `nbits` 位（Canonical Huffman MSB-first -> VP8L LSB-first）
fn webpReverseBits(code: u32, nbits: u8): u32 {
    let res: u32 = 0;
    let c = code;
    let i: u8 = 0;
    while (i < nbits) : (i += 1) {
        res = (res << 1) | (c & 1);
        c >>= 1;
    }
    return res;
}

/// 根据码长表构建 bit-reversed Canonical Huffman 码字
fn webpBuildCanonicalCodes(
    lens: []const u8,
    codes_out: []u32,
) {
    let bl_count: [16]u32 = [0; 16];
    let i: usize = 0;
    while (i < lens.len) : (i += 1) {
        const l = lens[i] as usize;
        if (l > 0 && l < 16) {
            bl_count[l] += 1;
        }
    }

    let next_code: [16]u32 = [0; 16];
    let code: u32 = 0;
    let bits: usize = 1;
    while (bits < 16) : (bits += 1) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    let s: usize = 0;
    while (s < lens.len) : (s += 1) {
        const l = lens[s];
        if (l > 0 && (l as usize) < 16) {
            const c = next_code[l as usize];
            next_code[l as usize] += 1;
            codes_out[s] = webpReverseBits(c, l);
        } else {
            codes_out[s] = 0;
        }
    }
}

/// 构建长度受限（`<= max_bits`）且满足 Kraft 满树约束的 Canonical Huffman 码长表
/// 仅使用固定栈数组（支持 `alphabet_size <= 280`），零堆分配
fn webpBuildLimitedCodeLengths(
    freqs: []const u32,
    max_bits: u8,
    lens_out: []u8,
) {
    let i: usize = 0;
    while (i < lens_out.len) : (i += 1) {
        lens_out[i] = 0;
    }

    const n = freqs.len;
    // 收集活跃符号
    let active_syms: [280]u16 = [0; 280];
    let active_cnt: usize = 0;
    i = 0;
    while (i < n) : (i += 1) {
        if (freqs[i] > 0) {
            active_syms[active_cnt] = i as u16;
            active_cnt += 1;
        }
    }

    if (active_cnt == 0) {
        return;
    }
    if (active_cnt == 1) {
        lens_out[active_syms[0] as usize] = 1;
        return;
    }
    if (active_cnt == 2) {
        lens_out[active_syms[0] as usize] = 1;
        lens_out[active_syms[1] as usize] = 1;
        return;
    }

    // 构造 Huffman 树（最多 2 * 280 - 1 = 559 个节点）
    let weight: [560]u64 = [0; 560];
    let parent: [560]i16 = [-1; 560];
    let is_active_node: [560]bool = [false; 560];

    i = 0;
    while (i < active_cnt) : (i += 1) {
        weight[i] = freqs[active_syms[i] as usize] as u64;
        is_active_node[i] = true;
    }

    let node_cnt: usize = active_cnt;
    let remaining: usize = active_cnt;
    while (remaining > 1) {
        // 寻找两个最小权重活跃节点
        let min1: usize = 560;
        let min2: usize = 560;
        let k: usize = 0;
        while (k < node_cnt) : (k += 1) {
            if (!is_active_node[k]) {
                continue;
            }
            if (min1 == 560 || weight[k] < weight[min1]) {
                min2 = min1;
                min1 = k;
            } else if (min2 == 560 || weight[k] < weight[min2]) {
                min2 = k;
            }
        }
        is_active_node[min1] = false;
        is_active_node[min2] = false;
        weight[node_cnt] = weight[min1] + weight[min2];
        parent[min1] = node_cnt as i16;
        parent[min2] = node_cnt as i16;
        is_active_node[node_cnt] = true;
        node_cnt += 1;
        remaining -= 1;
    }

    // 计算每个叶节点深度
    let depths: [280]u8 = [0; 280];
    let max_d: u8 = 0;
    i = 0;
    while (i < active_cnt) : (i += 1) {
        let d: u8 = 0;
        let cur: i16 = i as i16;
        while (parent[cur as usize] >= 0) {
            d += 1;
            cur = parent[cur as usize];
        }
        if (d > max_d) {
            max_d = d;
        }
        depths[i] = d;
    }

    // 若深度超出 max_bits，执行标准 Kraft 满树修复
    if (max_d > max_bits) {
        const one64: u64 = 1;
        let target: u64 = one64 << max_bits;
        let kraft: u64 = 0;
        i = 0;
        while (i < active_cnt) : (i += 1) {
            if (depths[i] > max_bits) {
                depths[i] = max_bits;
            }
            kraft += one64 << (max_bits - depths[i]);
        }

        // 若过载（kraft > target），将深度 < max_bits 的低频节点深度 +1
        while (kraft > target) {
            let best_idx: usize = 280;
            let best_freq: u32 = std.math.maxInt(u32);
            let k: usize = 0;
            while (k < active_cnt) : (k += 1) {
                if (depths[k] < max_bits) {
                    const f = freqs[active_syms[k] as usize];
                    if (best_idx == 280 || f < best_freq) {
                        best_idx = k;
                        best_freq = f;
                    }
                }
            }
            if (best_idx == 280) {
                break;
            }
            const old_w: u64 = one64 << (max_bits - depths[best_idx]);
            depths[best_idx] += 1;
            const new_w: u64 = one64 << (max_bits - depths[best_idx]);
            kraft -= (old_w - new_w);
        }

        // 若欠载（kraft < target），将深度 > 1 的高频节点深度 -1（不超 target）
        while (kraft < target) {
            let best_idx: usize = 280;
            let best_freq: u32 = 0;
            let k: usize = 0;
            while (k < active_cnt) : (k += 1) {
                if (depths[k] > 1) {
                    const add_w: u64 = one64 << (max_bits - depths[k]);
                    if (kraft + add_w <= target) {
                        const f = freqs[active_syms[k] as usize];
                        if (best_idx == 280 || f > best_freq) {
                            best_idx = k;
                            best_freq = f;
                        }
                    }
                }
            }
            if (best_idx == 280) {
                break;
            }
            const add_w: u64 = one64 << (max_bits - depths[best_idx]);
            depths[best_idx] -= 1;
            kraft += add_w;
        }
    }

    i = 0;
    while (i < active_cnt) : (i += 1) {
        lens_out[active_syms[i] as usize] = depths[i];
    }
}

struct PrefixCodeTable {
    codes: [280]u32,
    lens: [280]u8,
}

/// 将单组 Prefix Code 写入 VP8L 位流并输出其编码表
fn writePrefixCode(
    bw: *Vp8lBitWriter,
    freqs: []const u32,
    out_tbl: *PrefixCodeTable,
): void ! WebpError {
    let i: usize = 0;
    while (i < 280) : (i += 1) {
        out_tbl.codes[i] = 0;
        out_tbl.lens[i] = 0;
    }

    const n = freqs.len;
    let active_cnt: usize = 0;
    let first_sym: usize = 0;
    let second_sym: usize = 0;
    let s_idx: usize = 0;
    while (s_idx < n) : (s_idx += 1) {
        if (freqs[s_idx] > 0) {
            if (active_cnt == 0) {
                first_sym = s_idx;
            } else if (active_cnt == 1) {
                second_sym = s_idx;
            }
            active_cnt += 1;
        }
    }

    // 情况 1: 无活跃符号 -> Simple 1-symbol (sym = 0)
    if (active_cnt == 0) {
        bw.vp8lWriteBits(1, 1)?; // is_simple = 1
        bw.vp8lWriteBits(0, 1)?; // num_symbols - 1 = 0
        bw.vp8lWriteBits(0, 1)?; // is_first_8bits = 0
        bw.vp8lWriteBits(0, 1)?; // symbol = 0
        return;
    }

    // 情况 2: 1 个活跃符号且 < 256 -> Simple 1-symbol (0 bits/symbol)
    if (active_cnt == 1 && first_sym < 256) {
        bw.vp8lWriteBits(1, 1)?; // is_simple = 1
        bw.vp8lWriteBits(0, 1)?; // num_symbols - 1 = 0
        if (first_sym <= 1) {
            bw.vp8lWriteBits(0, 1)?;
            bw.vp8lWriteBits(first_sym as u32, 1)?;
        } else {
            bw.vp8lWriteBits(1, 1)?;
            bw.vp8lWriteBits(first_sym as u32, 8)?;
        }
        out_tbl.codes[first_sym] = 0;
        out_tbl.lens[first_sym] = 0;
        return;
    }

    // 情况 3: 2 个活跃符号且均 < 256 -> Simple 2-symbol (1 bit/symbol)
    if (active_cnt == 2 && first_sym < 256 && second_sym < 256) {
        bw.vp8lWriteBits(1, 1)?; // is_simple = 1
        bw.vp8lWriteBits(1, 1)?; // num_symbols - 1 = 1
        if (first_sym <= 1) {
            bw.vp8lWriteBits(0, 1)?;
            bw.vp8lWriteBits(first_sym as u32, 1)?;
        } else {
            bw.vp8lWriteBits(1, 1)?;
            bw.vp8lWriteBits(first_sym as u32, 8)?;
        }
        bw.vp8lWriteBits(second_sym as u32, 8)?;
        out_tbl.codes[first_sym] = 0;
        out_tbl.lens[first_sym] = 1;
        out_tbl.codes[second_sym] = 1;
        out_tbl.lens[second_sym] = 1;
        return;
    }

    // 情况 4: Normal Code Length Code (RFC 9649 §4.2.2)
    bw.vp8lWriteBits(0, 1)?; // is_simple = 0

    let lens: [280]u8 = [0; 280];
    webpBuildLimitedCodeLengths(freqs, 15, lens[0..n]);
    webpBuildCanonicalCodes(lens[0..n], out_tbl.codes[0..n]);
    i = 0;
    while (i < n) : (i += 1) {
        out_tbl.lens[i] = lens[i];
    }

    // 对主码长数组做 RLE 压缩（使用符号 0..15, 16, 17, 18）
    let rle_syms: [280]u8 = [0; 280];
    let rle_extra: [280]u8 = [0; 280];
    let rle_ebits: [280]u8 = [0; 280];
    let rle_len: usize = 0;
    let cl_freqs: [19]u32 = [0; 19];

    let pos: usize = 0;
    while (pos < n) {
        const cur = lens[pos];
        if (cur == 0) {
            let run: usize = 1;
            while (pos + run < n && lens[pos + run] == 0 && run < 138) {
                run += 1;
            }
            if (run >= 11) {
                rle_syms[rle_len] = 18;
                rle_extra[rle_len] = (run - 11) as u8;
                rle_ebits[rle_len] = 7;
                cl_freqs[18] += 1;
                rle_len += 1;
                pos += run;
            } else if (run >= 3) {
                rle_syms[rle_len] = 17;
                rle_extra[rle_len] = (run - 3) as u8;
                rle_ebits[rle_len] = 3;
                cl_freqs[17] += 1;
                rle_len += 1;
                pos += run;
            } else {
                rle_syms[rle_len] = 0;
                rle_extra[rle_len] = 0;
                rle_ebits[rle_len] = 0;
                cl_freqs[0] += 1;
                rle_len += 1;
                pos += 1;
            }
        } else {
            rle_syms[rle_len] = cur;
            rle_extra[rle_len] = 0;
            rle_ebits[rle_len] = 0;
            cl_freqs[cur as usize] += 1;
            rle_len += 1;
            pos += 1;
        }
    }

    // 确保二级树至少有 2 个活跃符号，杜绝单符号二级树解码器兼容差异
    let cl_active: usize = 0;
    let cl_only: usize = 0;
    let ci: usize = 0;
    while (ci < 19) : (ci += 1) {
        if (cl_freqs[ci] > 0) {
            cl_active += 1;
            cl_only = ci;
        }
    }
    if (cl_active == 1) {
        const dummy_sym: usize = if (cl_only == 0) 1 else 0;
        cl_freqs[dummy_sym] = 1;
    }

    let cl_lens: [19]u8 = [0; 19];
    let cl_codes: [19]u32 = [0; 19];
    webpBuildLimitedCodeLengths(cl_freqs[0..19], 7, cl_lens[0..19]);
    webpBuildCanonicalCodes(cl_lens[0..19], cl_codes[0..19]);

    // 计算 num_code_lengths (>= 4)
    let num_cl: usize = 19;
    while (num_cl > 4 && cl_lens[CODE_LEN_ORDER[num_cl - 1]] == 0) {
        num_cl -= 1;
    }

    bw.vp8lWriteBits((num_cl - 4) as u32, 4)?;
    i = 0;
    while (i < num_cl) : (i += 1) {
        bw.vp8lWriteBits(cl_lens[CODE_LEN_ORDER[i]] as u32, 3)?;
    }

    // use_max_symbol = 0（直接读取全部 alphabet_size 个码长）
    bw.vp8lWriteBits(0, 1)?;

    // 写入 RLE 编码后的主码长流
    i = 0;
    while (i < rle_len) : (i += 1) {
        const s = rle_syms[i] as usize;
        bw.vp8lWriteBits(cl_codes[s], cl_lens[s])?;
        if (rle_ebits[i] > 0) {
            bw.vp8lWriteBits(rle_extra[i] as u32, rle_ebits[i])?;
        }
    }
}

/// 将匹配长度 `len`（`1..4096`）编码为 RFC 9649 Length Prefix Code (0..23) + extra bits
struct PrefixLenCode {
    code: u8,
    extra_bits: u8,
    extra_val: u32,
}

fn encodeLengthPrefix(len: usize): PrefixLenCode {
    if (len <= 4) {
        return PrefixLenCode{
            code: (len - 1) as u8,
            extra_bits: 0,
            extra_val: 0,
        };
    }
    const v: u32 = (len - 1) as u32;
    let msb: u8 = 0;
    let tmp = v;
    while (tmp > 1) {
        msb += 1;
        tmp >>= 1;
    }
    const extra_bits: u8 = msb - 1;
    const second_bit: u32 = (v >> extra_bits) & 1;
    const code: u8 = (2 * (msb as u32) + second_bit) as u8;
    const one32: u32 = 1;
    const extra_mask: u32 = (one32 << extra_bits) - one32;
    return PrefixLenCode{
        code: code,
        extra_bits: extra_bits,
        extra_val: v & extra_mask,
    };
}

/// `quality < 100` 时的轻量可控感知量化（降低熵并保持严格合标 VP8L 流）
fn quantizeChannel(v: u8, shift: u8): u8 {
    if (shift == 0) {
        return v;
    }
    const one16: u16 = 1;
    const step: u16 = one16 << shift;
    const half: u16 = step >> 1;
    const rounded: u16 = ((v as u16) + half) & (~(step - 1));
    if (rounded >= 255) {
        return 255;
    }
    return rounded as u8;
}

fn qualityToShift(quality: u8): u8 {
    if (quality >= 100) {
        return 0;
    }
    if (quality >= 90) {
        return 1;
    }
    if (quality >= 75) {
        return 2;
    }
    if (quality >= 50) {
        return 3;
    }
    if (quality >= 25) {
        return 4;
    }
    return 5;
}

/// 编码流 Token（Literal 或 LZ77 反向复制）
struct Vp8lToken {
    is_copy: bool,
    // Literal: Subtract-Green 后的 (g, r_sg, b_sg, a)
    g: u8,
    r: u8,
    b: u8,
    a: u8,
    // Copy: length (1..4096) 与 distance prefix code (0=上邻, 1=左邻)
    copy_len: u16,
    dist_code: u8,
}

fn pixelAtQuantized(rgba: []const u8, idx: usize, shift: u8): u32 {
    const off = idx * 4;
    const r = quantizeChannel(rgba[off], shift) as u32;
    const g = quantizeChannel(rgba[off + 1], shift) as u32;
    const b = quantizeChannel(rgba[off + 2], shift) as u32;
    const a = quantizeChannel(rgba[off + 3], shift) as u32;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

fn webpAppendU32LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void ! WebpError {
    const bytes: [4]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 24) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, bytes[0..4]) catch return WebpError.OutOfMemory;
}

/// 将 RGBA8 像素编码为合标 RFC 9649 WebP (VP8L) 字节流。
/// - `quality == 100`：严格逐像素比特无损（Subtract Green + LZ77 + Canonical Huffman）；
/// - `quality < 100`：按质量等级对低有效位做可控感知量化后输出合标 VP8L 字节流，确保所有标准浏览器/libwebp/Pillow 100% 可解码。
/// 调用方需使用 `allocator.free(...)` 释放返回切片。
pub fn webpEncode(
    allocator: std.mem.Allocator,
    rgba: []const u8,
    width: u32,
    height: u32,
    quality: u8,
): ([]u8) ! WebpError {
    if (width == 0 || height == 0) {
        return WebpError.InvalidDimensions;
    }
    if (width > VP8L_MAX_DIM || height > VP8L_MAX_DIM) {
        return WebpError.DimensionTooLarge;
    }
    const expected_len: u64 = (width as u64) * (height as u64) * 4;
    if (expected_len != (rgba.len as u64)) {
        return WebpError.InvalidBufferLength;
    }

    const w = width as usize;
    const h = height as usize;
    const num_pixels = w * h;
    const shift = qualityToShift(quality);

    // 预生成量化后的 ARGB 像素数组（单次遍历，便于高速 LZ77 匹配与 Alpha 检测）
    let pixels = allocator.alloc(u32, num_pixels) catch return WebpError.OutOfMemory;
    defer allocator.free(pixels);

    let alpha_used = false;
    let i: usize = 0;
    while (i < num_pixels) : (i += 1) {
        const px = pixelAtQuantized(rgba, i, shift);
        pixels[i] = px;
        if (((px >> 24) & 0xFF) != 255) {
            alpha_used = true;
        }
    }

    // Pass 1: 水平（左邻 dist_code=1）与垂直（上邻 dist_code=0）LZ77 匹配 + 直方图统计
    let tokens: std.ArrayList(Vp8lToken) = .empty;
    defer tokens.deinit(allocator);

    let green_freq: [280]u32 = [0; 280];
    let red_freq: [256]u32 = [0; 256];
    let blue_freq: [256]u32 = [0; 256];
    let alpha_freq: [256]u32 = [0; 256];
    let dist_freq: [40]u32 = [0; 40];

    let pos: usize = 0;
    while (pos < num_pixels) {
        let best_len: usize = 0;
        let best_dist: u8 = 1;

        // 检查左邻（distance_map[1] = (1, 0) -> offset = 1）
        if (pos >= 1 && pixels[pos] == pixels[pos - 1]) {
            let l: usize = 1;
            const max_l = if (num_pixels - pos < VP8L_MAX_MATCH_LEN) (num_pixels - pos) else VP8L_MAX_MATCH_LEN;
            while (l < max_l && pixels[pos + l] == pixels[pos + l - 1]) {
                l += 1;
            }
            if (l > best_len) {
                best_len = l;
                best_dist = 1;
            }
        }

        // 检查上邻（distance_map[0] = (0, 1) -> offset = w）
        if (pos >= w && pixels[pos] == pixels[pos - w]) {
            let l: usize = 1;
            const max_l = if (num_pixels - pos < VP8L_MAX_MATCH_LEN) (num_pixels - pos) else VP8L_MAX_MATCH_LEN;
            while (l < max_l && pixels[pos + l] == pixels[pos + l - w]) {
                l += 1;
            }
            if (l > best_len) {
                best_len = l;
                best_dist = 0;
            }
        }

        if (best_len >= 3) {
            const lc = encodeLengthPrefix(best_len);
            green_freq[256 + (lc.code as usize)] += 1;
            dist_freq[best_dist as usize] += 1;
            tokens.append(allocator, Vp8lToken{
                is_copy: true,
                g: 0,
                r: 0,
                b: 0,
                a: 0,
                copy_len: best_len as u16,
                dist_code: best_dist,
            }) catch return WebpError.OutOfMemory;
            pos += best_len;
        } else {
            const px = pixels[pos];
            const a: u8 = ((px >> 24) & 0xFF) as u8;
            const r: u8 = ((px >> 16) & 0xFF) as u8;
            const g: u8 = ((px >> 8) & 0xFF) as u8;
            const b: u8 = (px & 0xFF) as u8;
            // Subtract Green Transform
            const r_sg: u8 = r -% g;
            const b_sg: u8 = b -% g;

            green_freq[g as usize] += 1;
            red_freq[r_sg as usize] += 1;
            blue_freq[b_sg as usize] += 1;
            alpha_freq[a as usize] += 1;

            tokens.append(allocator, Vp8lToken{
                is_copy: false,
                g: g,
                r: r_sg,
                b: b_sg,
                a: a,
                copy_len: 0,
                dist_code: 0,
            }) catch return WebpError.OutOfMemory;
            pos += 1;
        }
    }

    // Pass 2: 编码 VP8L payload
    let payload: std.ArrayList(u8) = .empty;
    defer payload.deinit(allocator);

    // 5 字节 VP8L 头部
    payload.append(allocator, 0x2F) catch return WebpError.OutOfMemory;
    const w_m1: u32 = width - 1;
    const h_m1: u32 = height - 1;
    const alpha_bit: u32 = if (alpha_used) 1 else 0;
    const hdr_bits: u32 = (w_m1 & 0x3FFF) | ((h_m1 & 0x3FFF) << 14) | (alpha_bit << 28);
    webpAppendU32LE(allocator, &payload, hdr_bits)?;

    let bw = Vp8lBitWriter{
        alloc: allocator,
        buf: &payload,
        bit_buf: 0,
        bit_count: 0,
    };

    // Transform: Subtract Green (has_transform=1, type=2, next_has_transform=0)
    bw.vp8lWriteBits(1, 1)?;
    bw.vp8lWriteBits(2, 2)?;
    bw.vp8lWriteBits(0, 1)?;

    // Color cache = 0, Meta prefix codes = 0
    bw.vp8lWriteBits(0, 1)?;
    bw.vp8lWriteBits(0, 1)?;

    // 5 组 Prefix Code（Green+Len, Red, Blue, Alpha, Distance）
    let green_tbl = PrefixCodeTable{ codes: [0; 280], lens: [0; 280] };
    let red_tbl = PrefixCodeTable{ codes: [0; 280], lens: [0; 280] };
    let blue_tbl = PrefixCodeTable{ codes: [0; 280], lens: [0; 280] };
    let alpha_tbl = PrefixCodeTable{ codes: [0; 280], lens: [0; 280] };
    let dist_tbl = PrefixCodeTable{ codes: [0; 280], lens: [0; 280] };

    writePrefixCode(&bw, green_freq[0..280], &green_tbl)?;
    writePrefixCode(&bw, red_freq[0..256], &red_tbl)?;
    writePrefixCode(&bw, blue_freq[0..256], &blue_tbl)?;
    writePrefixCode(&bw, alpha_freq[0..256], &alpha_tbl)?;
    writePrefixCode(&bw, dist_freq[0..40], &dist_tbl)?;

    // 写入像素与 LZ77 符号位流
    let ti: usize = 0;
    while (ti < tokens.items.len) : (ti += 1) {
        const tok = tokens.items[ti];
        if (tok.is_copy) {
            const lc = encodeLengthPrefix(tok.copy_len as usize);
            const g_sym = 256 + (lc.code as usize);
            bw.vp8lWriteBits(green_tbl.codes[g_sym], green_tbl.lens[g_sym])?;
            if (lc.extra_bits > 0) {
                bw.vp8lWriteBits(lc.extra_val, lc.extra_bits)?;
            }
            const d_sym = tok.dist_code as usize;
            bw.vp8lWriteBits(dist_tbl.codes[d_sym], dist_tbl.lens[d_sym])?;
        } else {
            bw.vp8lWriteBits(green_tbl.codes[tok.g as usize], green_tbl.lens[tok.g as usize])?;
            bw.vp8lWriteBits(red_tbl.codes[tok.r as usize], red_tbl.lens[tok.r as usize])?;
            bw.vp8lWriteBits(blue_tbl.codes[tok.b as usize], blue_tbl.lens[tok.b as usize])?;
            bw.vp8lWriteBits(alpha_tbl.codes[tok.a as usize], alpha_tbl.lens[tok.a as usize])?;
        }
    }

    bw.flush()?;

    // 封装 RIFF / WEBP / VP8L 容器（chunk 字节偶数对齐）
    const vp8l_len: u32 = payload.items.len as u32;
    const pad: u32 = vp8l_len & 1;
    const hdr_overhead: u32 = 12;
    const riff_size: u32 = hdr_overhead + vp8l_len + pad;

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    out.appendSlice(allocator, "RIFF") catch return WebpError.OutOfMemory;
    webpAppendU32LE(allocator, &out, riff_size)?;
    out.appendSlice(allocator, "WEBP") catch return WebpError.OutOfMemory;
    out.appendSlice(allocator, "VP8L") catch return WebpError.OutOfMemory;
    webpAppendU32LE(allocator, &out, vp8l_len)?;
    out.appendSlice(allocator, payload.items) catch return WebpError.OutOfMemory;
    if (pad == 1) {
        out.append(allocator, 0x00) catch return WebpError.OutOfMemory;
    }

    return out.toOwnedSlice(allocator) catch return WebpError.OutOfMemory;
}

/// 将 RGBA8 像素严格逐比特无损编码为 RFC 9649 WebP (VP8L) 字节流（等价于 `webpEncode(..., 100)`）。
pub fn webpEncodeLossless(
    allocator: std.mem.Allocator,
    rgba: []const u8,
    width: u32,
    height: u32,
): ([]u8) ! WebpError {
    return webpEncode(allocator, rgba, width, height, 100);
}

// ============================================================================
// RFC 9649 WebP VP8X Container Metadata Parser & Full VP8L Lossless Decoder
// ============================================================================

/// Parsed RIFF/WEBP container metadata including VP8X flags, canvas dimensions,
/// and zero-copy slices for `VP8L`, `VP8 `, `ICCP`, `EXIF`, `XMP `, and `ANIM` chunks.
pub struct WebpExtendedInfo {
    is_extended: bool = false,
    has_icc: bool = false,
    has_alpha: bool = false,
    has_exif: bool = false,
    has_xmp: bool = false,
    is_animated: bool = false,
    canvas_width: u32 = 0,
    canvas_height: u32 = 0,
    vp8l_payload: []const u8 = "",
    vp8_payload: []const u8 = "",
    icc_profile: []const u8 = "",
    exif_data: []const u8 = "",
    xmp_data: []const u8 = "",
    anim_data: []const u8 = "",
}

fn readLeU32(bytes: []const u8, off: usize): u32 {
    return @as(u32, bytes[off])
        | (@as(u32, bytes[off + 1]) << 8)
        | (@as(u32, bytes[off + 2]) << 16)
        | (@as(u32, bytes[off + 3]) << 24);
}

fn readLeU24(bytes: []const u8, off: usize): u32 {
    return @as(u32, bytes[off])
        | (@as(u32, bytes[off + 1]) << 8)
        | (@as(u32, bytes[off + 2]) << 16);
}

fn fourccEq(bytes: []const u8, off: usize, a: u8, b: u8, c: u8, d: u8): bool {
    return bytes[off] == a && bytes[off + 1] == b && bytes[off + 2] == c && bytes[off + 3] == d;
}

/// Parses a RIFF/WEBP container (both simple `VP8L`/`VP8 ` and extended `VP8X`),
/// extracting flags, canvas dimensions, and zero-copy chunk payload slices.
pub fn parseWebpChunks(bytes: []const u8): WebpExtendedInfo ! WebpError {
    if (bytes.len < 20) return WebpError.InvalidContainer;
    if (!fourccEq(bytes, 0, 'R', 'I', 'F', 'F') || !fourccEq(bytes, 8, 'W', 'E', 'B', 'P')) {
        return WebpError.InvalidContainer;
    }
    const riff_size: usize = @as(usize, readLeU32(bytes, 4));
    const riff_end: usize = if (riff_size + 8 <= bytes.len) riff_size + 8 else bytes.len;

    let info = WebpExtendedInfo{};
    let pos: usize = 12;
    let chunk_idx: usize = 0;

    while (pos + 8 <= riff_end) {
        const chunk_size: usize = @as(usize, readLeU32(bytes, pos + 4));
        const data_start: usize = pos + 8;
        if (chunk_size > riff_end - data_start) {
            return WebpError.UnexpectedEof;
        }
        const payload = bytes[data_start .. data_start + chunk_size];

        if (fourccEq(bytes, pos, 'V', 'P', '8', 'X')) {
            if (chunk_idx != 0 || chunk_size < 10) {
                return WebpError.InvalidContainer;
            }
            const flags: u8 = payload[0];
            info.is_extended = true;
            info.has_icc = (flags & 0x20) != 0;
            info.has_alpha = (flags & 0x10) != 0;
            info.has_exif = (flags & 0x08) != 0;
            info.has_xmp = (flags & 0x04) != 0;
            info.is_animated = (flags & 0x02) != 0;
            info.canvas_width = readLeU24(payload, 4) + 1;
            info.canvas_height = readLeU24(payload, 7) + 1;
        } else if (fourccEq(bytes, pos, 'V', 'P', '8', 'L')) {
            if (info.vp8l_payload.len == 0) {
                info.vp8l_payload = payload;
                if (!info.is_extended && payload.len >= 5 && payload[0] == 0x2F) {
                    const b0: u32 = payload[1];
                    const b1: u32 = payload[2];
                    const b2: u32 = payload[3];
                    const b3: u32 = payload[4];
                    info.canvas_width = (b0 | ((b1 & 0x3F) << 8)) + 1;
                    info.canvas_height = ((b1 >> 6) | (b2 << 2) | ((b3 & 0x0F) << 10)) + 1;
                    info.has_alpha = (b3 & 0x10) != 0;
                }
            }
        } else if (fourccEq(bytes, pos, 'V', 'P', '8', ' ')) {
            if (info.vp8_payload.len == 0) {
                info.vp8_payload = payload;
                if (!info.is_extended && payload.len >= 10 && payload[3] == 0x9D && payload[4] == 0x01 && payload[5] == 0x2A) {
                    info.canvas_width = (@as(u32, payload[6]) | (@as(u32, payload[7]) << 8)) & 0x3FFF;
                    info.canvas_height = (@as(u32, payload[8]) | (@as(u32, payload[9]) << 8)) & 0x3FFF;
                }
            }
        } else if (fourccEq(bytes, pos, 'I', 'C', 'C', 'P')) {
            if (info.icc_profile.len == 0) info.icc_profile = payload;
        } else if (fourccEq(bytes, pos, 'E', 'X', 'I', 'F')) {
            if (info.exif_data.len == 0) info.exif_data = payload;
        } else if (fourccEq(bytes, pos, 'X', 'M', 'P', ' ')) {
            if (info.xmp_data.len == 0) info.xmp_data = payload;
        } else if (fourccEq(bytes, pos, 'A', 'N', 'I', 'M')) {
            if (info.anim_data.len == 0) info.anim_data = payload;
        }

        const padded_size: usize = chunk_size + (chunk_size & 1);
        if (padded_size > riff_end - data_start) {
            pos = riff_end;
        } else {
            pos = data_start + padded_size;
        }
        chunk_idx += 1;
    }

    if (chunk_idx == 0) return WebpError.InvalidContainer;
    return info;
}

/// RFC 9649 §4.2.2 120-entry 2D spatial distance mapping table (xoffset, yoffset).
const VP8L_DIST_MAP_X: [120]i8 = [
    0,  1,  1, -1,  0,  2,  1, -1,
    2, -2,  2, -2,  0,  3,  1, -1,
    3, -3,  2, -2,  3, -3,  0,  4,
    1, -1,  4, -4,  3, -3,  2, -2,
    4, -4,  0,  3, -3,  4, -4,  5,
    1, -1,  5, -5,  2, -2,  5, -5,
    4, -4,  3, -3,  5, -5,  0,  6,
    1, -1,  6, -6,  2, -2,  6, -6,
    4, -4,  5, -5,  3, -3,  6, -6,
    0,  7,  1, -1,  5, -5,  7, -7,
    4, -4,  6, -6,  2, -2,  7, -7,
    3, -3,  7, -7,  5, -5,  6, -6,
    8,  4, -4,  7, -7,  8,  8,  6,
   -6,  8,  5, -5,  7, -7,  8,  6,
   -6,  7, -7,  8,  7, -7,  8,  8,
];

const VP8L_DIST_MAP_Y: [120]i8 = [
    1, 0, 1, 1, 2, 0, 2, 2,
    1, 1, 2, 2, 3, 0, 3, 3,
    1, 1, 3, 3, 2, 2, 4, 0,
    4, 4, 1, 1, 3, 3, 4, 4,
    2, 2, 5, 4, 4, 3, 3, 0,
    5, 5, 1, 1, 5, 5, 2, 2,
    4, 4, 5, 5, 3, 3, 6, 0,
    6, 6, 1, 1, 6, 6, 2, 2,
    5, 5, 4, 4, 6, 6, 3, 3,
    7, 0, 7, 7, 5, 5, 1, 1,
    6, 6, 4, 4, 7, 7, 2, 2,
    7, 7, 3, 3, 6, 6, 5, 5,
    0, 7, 7, 4, 4, 1, 2, 6,
    6, 3, 7, 7, 5, 5, 4, 7,
    7, 6, 6, 5, 7, 7, 6, 7,
];

const VP8L_ALPHABET_SIZES: [5]u16 = [280, 256, 256, 256, 40];

fn vp8lSubsampleSize(size: u32, bits: u8): u32 {
    const sh: u5 = @as(u5, @intCast(bits));
    return (size + (@as(u32, 1) << sh) - 1) >> sh;
}

/// LSB-first VP8L bitstream reader with strict EOF enforcement.
struct Vp8lBitReader {
    src: []const u8 = "",
    pos: usize = 0,
    buf: u64 = 0,
    nbits: u8 = 0,

    static fn init(src: []const u8): Vp8lBitReader {
        return Vp8lBitReader{ src: src, pos: 0, buf: 0, nbits: 0 };
    }

    fn fill(*_) {
        while (nbits <= 56 && pos < src.len) {
            buf |= @as(u64, src[pos]) << @as(u6, @intCast(nbits));
            pos += 1;
            nbits += 8;
        }
    }

    fn readBits(*_, n: u8): u32 ! WebpError {
        if (n == 0) return 0;
        if (n > 32) return WebpError.InvalidBitstream;
        if (nbits < n) fill();
        if (nbits < n) return WebpError.UnexpectedEof;
        const mask: u64 = if (n == 32) @as(u64, 0xFFFF_FFFF) else ((@as(u64, 1) << @as(u6, @intCast(n))) - 1);
        const val: u32 = @as(u32, @intCast(buf & mask));
        buf >>= @as(u6, @intCast(n));
        nbits -= n;
        return val;
    }
}

/// Canonical Huffman decoder tree for VP8L (supports 1..15 bit codes + fast 8-bit LUT).
struct Vp8lHuffTree {
    is_single: bool = true,
    single_sym: u16 = 0,
    max_bits: u8 = 0,
    // Fast 8-bit lookup table: low 12 bits = symbol, high 4 bits = code length (0 => slow path > 8 bits)
    fast_lut: [256]u16 = [0; 256],
    // Canonical slow-path tables for code lengths 1..15
    count: [16]u16 = [0; 16],
    first_code: [16]u32 = [0; 16],
    first_sym: [16]u16 = [0; 16],
    syms: []u16 = [],

    static fn single(sym: u16): Vp8lHuffTree {
        return Vp8lHuffTree{ is_single: true, single_sym: sym };
    }

    static fn twoNode(alloc: std.mem.Allocator, zero_sym: u16, one_sym: u16): Vp8lHuffTree ! WebpError {
        let t = Vp8lHuffTree{ is_single: false, max_bits: 1 };
        let syms = alloc.alloc(u16, 2) catch return WebpError.OutOfMemory;
        syms[0] = zero_sym;
        syms[1] = one_sym;
        t.syms = syms;
        t.count[1] = 2;
        t.first_code[1] = 0;
        t.first_sym[1] = 0;
        let i: usize = 0;
        while (i < 256) : (i += 1) {
            const sym: u16 = if ((i & 1) == 0) zero_sym else one_sym;
            t.fast_lut[i] = (@as(u16, 1) << 12) | (sym & 0x0FFF);
        }
        return t;
    }

    static fn buildFromLengths(alloc: std.mem.Allocator, lengths: []const u8): Vp8lHuffTree ! WebpError {
        let hist: [16]u16 = [0; 16];
        let num_syms: usize = 0;
        let only_sym: u16 = 0;
        let i: usize = 0;
        while (i < lengths.len) : (i += 1) {
            const l = lengths[i];
            if (l > 15) return WebpError.InvalidHuffman;
            if (l > 0) {
                hist[l as usize] += 1;
                num_syms += 1;
                only_sym = @as(u16, @intCast(i));
            }
        }
        if (num_syms == 0) return WebpError.InvalidHuffman;
        if (num_syms == 1) {
            return Vp8lHuffTree.single(only_sym);
        }

        let max_len: usize = 15;
        while (max_len > 1 && hist[max_len] == 0) : (max_len -= 1) {}

        // Verify exact Kraft equality for complete canonical tree
        let codespace: u32 = 0;
        let b: usize = 1;
        while (b <= max_len) : (b += 1) {
            codespace = (codespace << 1) + @as(u32, hist[b]);
        }
        if (codespace != (@as(u32, 1) << @as(u5, @intCast(max_len)))) {
            return WebpError.InvalidHuffman;
        }

        let t = Vp8lHuffTree{
            is_single: false,
            max_bits: @as(u8, @intCast(max_len)),
        };
        let syms = alloc.alloc(u16, num_syms) catch return WebpError.OutOfMemory;
        t.syms = syms;

        let next_sym_idx: [16]u16 = [0; 16];
        let code: u32 = 0;
        let sym_off: u16 = 0;
        b = 1;
        while (b <= max_len) : (b += 1) {
            t.count[b] = hist[b];
            t.first_code[b] = code;
            t.first_sym[b] = sym_off;
            next_sym_idx[b] = sym_off;
            code = (code + @as(u32, hist[b])) << 1;
            sym_off += hist[b];
        }

        i = 0;
        while (i < lengths.len) : (i += 1) {
            const l = lengths[i];
            if (l > 0) {
                const idx = next_sym_idx[l as usize];
                syms[idx as usize] = @as(u16, @intCast(i));
                next_sym_idx[l as usize] = idx + 1;
            }
        }

        // Populate 8-bit fast LUT for all code lengths <= 8 (and symbols < 4096)
        b = 1;
        while (b <= max_len && b <= 8) : (b += 1) {
            const cnt = t.count[b];
            let k: u16 = 0;
            while (k < cnt) : (k += 1) {
                const canon_code: u32 = t.first_code[b] + @as(u32, k);
                const rev: usize = @as(usize, webpReverseBits(canon_code, @as(u8, @intCast(b))));
                const sym: u16 = syms[@as(usize, t.first_sym[b] + k)];
                if (sym < 4096) {
                    const entry: u16 = (@as(u16, @intCast(b)) << 12) | sym;
                    const step: usize = @as(usize, 1) << @as(u6, @intCast(b));
                    let slot: usize = rev;
                    while (slot < 256) : (slot += step) {
                        t.fast_lut[slot] = entry;
                    }
                }
            }
        }
        return t;
    }

    fn decodeSymbol(br: *Vp8lBitReader): u16 ! WebpError {
        if (is_single) return single_sym;
        br.fill();
        if (br.nbits == 0) return WebpError.UnexpectedEof;

        const peek8: usize = @as(usize, br.buf & 0xFF);
        const fast_entry: u16 = fast_lut[peek8];
        if (fast_entry != 0) {
            const clen: u8 = @as(u8, @intCast(fast_entry >> 12));
            if (br.nbits < clen) return WebpError.UnexpectedEof;
            br.buf >>= @as(u6, @intCast(clen));
            br.nbits -= clen;
            return fast_entry & 0x0FFF;
        }

        // Slow path: canonical bit-by-bit decode (lengths 1..15)
        let code: u32 = 0;
        let len: usize = 1;
        while (len <= @as(usize, max_bits)) : (len += 1) {
            if (br.nbits == 0) {
                br.fill();
                if (br.nbits == 0) return WebpError.UnexpectedEof;
            }
            const bit: u32 = @as(u32, @intCast(br.buf & 1));
            br.buf >>= 1;
            br.nbits -= 1;
            code = (code << 1) | bit;
            const fc: u32 = first_code[len];
            const cnt: u32 = @as(u32, count[len]);
            if (cnt > 0 && code >= fc && code < fc + cnt) {
                const idx: usize = @as(usize, first_sym[len]) + @as(usize, code - fc);
                return syms[idx];
            }
        }
        return WebpError.InvalidHuffman;
    }
}

struct Vp8lHuffGroup {
    trees: [5]Vp8lHuffTree = [
        Vp8lHuffTree{},
        Vp8lHuffTree{},
        Vp8lHuffTree{},
        Vp8lHuffTree{},
        Vp8lHuffTree{},
    ],
}

struct Vp8lColorCache {
    bits: u8 = 0,
    table: [][4]u8 = [],

    fn insert(*_, r: u8, g: u8, b: u8, a: u8) {
        if (bits == 0) return;
        const argb: u32 = (@as(u32, a) << 24) | (@as(u32, r) << 16) | (@as(u32, g) << 8) | @as(u32, b);
        const shift: u5 = @as(u5, @intCast(32 - @as(u32, bits)));
        const idx: usize = @as(usize, (@as(u32, 0x1E35_A7BD) *% argb) >> shift);
        table[idx] = [r, g, b, a];
    }
}

fn vp8lReadHuffCode(alloc: std.mem.Allocator, br: *Vp8lBitReader, alphabet_size: u16): Vp8lHuffTree ! WebpError {
    const is_simple = br.readBits(1)?;
    if (is_simple == 1) {
        const num_syms = br.readBits(1)? + 1;
        const is_first_8bits = br.readBits(1)?;
        const first_bits: u8 = if (is_first_8bits == 1) 8 else 1;
        const sym0: u16 = @as(u16, @intCast(br.readBits(first_bits)?));
        if (sym0 >= alphabet_size) return WebpError.InvalidHuffman;
        if (num_syms == 1) {
            return Vp8lHuffTree.single(sym0);
        }
        const sym1: u16 = @as(u16, @intCast(br.readBits(8)?));
        if (sym1 >= alphabet_size) return WebpError.InvalidHuffman;
        return Vp8lHuffTree.twoNode(alloc, sym0, sym1);
    }

    // Normal Code Length Code
    const num_cl_codes: usize = @as(usize, br.readBits(4)?) + 4;
    if (num_cl_codes > 19) return WebpError.InvalidHuffman;

    let cl_lens: [19]u8 = [0; 19];
    let i: usize = 0;
    while (i < num_cl_codes) : (i += 1) {
        cl_lens[CODE_LEN_ORDER[i]] = @as(u8, @intCast(br.readBits(3)?));
    }
    const cl_tree = Vp8lHuffTree.buildFromLengths(alloc, cl_lens[0..])?;

    let max_symbol: usize = @as(usize, alphabet_size);
    const use_max_sym = br.readBits(1)?;
    if (use_max_sym == 1) {
        const length_nbits: u8 = @as(u8, 2) + @as(u8, 2) * @as(u8, @intCast(br.readBits(3)?));
        const max_minus_two: usize = @as(usize, br.readBits(length_nbits)?);
        if (alphabet_size < 2 || max_minus_two > @as(usize, alphabet_size - 2)) {
            return WebpError.InvalidHuffman;
        }
        max_symbol = @as(usize, 2) + max_minus_two;
    }

    let lengths = alloc.alloc(u8, @as(usize, alphabet_size)) catch return WebpError.OutOfMemory;
    i = 0;
    while (i < lengths.len) : (i += 1) {
        lengths[i] = 0;
    }

    let prev_len: u8 = 8;
    let sym: usize = 0;
    let remaining_codes: usize = max_symbol;
    while (sym < @as(usize, alphabet_size) && remaining_codes > 0) {
        remaining_codes -= 1;
        const code_len: u16 = cl_tree.decodeSymbol(br)?;
        if (code_len < 16) {
            lengths[sym] = @as(u8, @intCast(code_len));
            sym += 1;
            if (code_len != 0) prev_len = @as(u8, @intCast(code_len));
        } else if (code_len == 16) {
            const rep: usize = @as(usize, br.readBits(2)?) + 3;
            if (sym + rep > @as(usize, alphabet_size)) return WebpError.InvalidHuffman;
            let r: usize = 0;
            while (r < rep) : (r += 1) {
                lengths[sym] = prev_len;
                sym += 1;
            }
        } else if (code_len == 17) {
            const rep: usize = @as(usize, br.readBits(3)?) + 3;
            if (sym + rep > @as(usize, alphabet_size)) return WebpError.InvalidHuffman;
            let r: usize = 0;
            while (r < rep) : (r += 1) {
                lengths[sym] = 0;
                sym += 1;
            }
        } else if (code_len == 18) {
            const rep: usize = @as(usize, br.readBits(7)?) + 11;
            if (sym + rep > @as(usize, alphabet_size)) return WebpError.InvalidHuffman;
            let r: usize = 0;
            while (r < rep) : (r += 1) {
                lengths[sym] = 0;
                sym += 1;
            }
        } else {
            return WebpError.InvalidHuffman;
        }
    }

    return Vp8lHuffTree.buildFromLengths(alloc, lengths);
}

fn vp8lReadCopyDistance(br: *Vp8lBitReader, prefix_code: u16): usize ! WebpError {
    if (prefix_code < 4) {
        return @as(usize, prefix_code) + 1;
    }
    const extra_bits: u8 = @as(u8, @intCast((prefix_code - 2) >> 1));
    if (extra_bits > 24) return WebpError.InvalidBitstream;
    const offset: usize = (@as(usize, 2) + (@as(usize, prefix_code) & 1)) << @as(u6, @intCast(extra_bits));
    const extra: usize = @as(usize, br.readBits(extra_bits)?);
    return offset + extra + 1;
}

fn vp8lPlaneCodeToDistance(xsize: u32, plane_code: usize): usize {
    if (plane_code == 0) return 1;
    if (plane_code > 120) return plane_code - 120;
    const dx: i64 = @as(i64, VP8L_DIST_MAP_X[plane_code - 1]);
    const dy: i64 = @as(i64, VP8L_DIST_MAP_Y[plane_code - 1]);
    const dist: i64 = dx + dy * @as(i64, xsize);
    if (dist < 1) return 1;
    return @as(usize, @intCast(dist));
}

fn vp8lDecodeImageStream(
    alloc: std.mem.Allocator,
    br: *Vp8lBitReader,
    xsize: u32,
    ysize: u32,
    is_argb_img: bool,
    out_rgba: []u8,
): void ! WebpError {
    const num_pixels: usize = @as(usize, xsize) * @as(usize, ysize);
    if (out_rgba.len < num_pixels * 4) return WebpError.BufferTooSmall;

    // 1. Optional Color Cache
    let cc = Vp8lColorCache{};
    const use_cc = br.readBits(1)?;
    if (use_cc == 1) {
        const cc_bits: u8 = @as(u8, @intCast(br.readBits(4)?));
        if (cc_bits < 1 || cc_bits > 11) return WebpError.InvalidColorCacheBits;
        const cc_size: usize = @as(usize, 1) << @as(u6, @intCast(cc_bits));
        let tbl = alloc.alloc([4]u8, cc_size) catch return WebpError.OutOfMemory;
        let ci: usize = 0;
        while (ci < cc_size) : (ci += 1) {
            tbl[ci] = [0, 0, 0, 0];
        }
        cc.bits = cc_bits;
        cc.table = tbl;
    }

    // 2. Optional Meta-Huffman Entropy Image
    let huff_bits: u8 = 0;
    let huff_xsize: u32 = 1;
    let num_groups: usize = 1;
    let entropy_ids: []u16 = alloc.alloc(u16, 0) catch return WebpError.OutOfMemory;

    if (is_argb_img && br.readBits(1)? == 1) {
        huff_bits = @as(u8, @intCast(br.readBits(3)?)) + 2;
        huff_xsize = vp8lSubsampleSize(xsize, huff_bits);
        const huff_ysize: u32 = vp8lSubsampleSize(ysize, huff_bits);
        const ent_pixels: usize = @as(usize, huff_xsize) * @as(usize, huff_ysize);
        let ent_rgba = alloc.alloc(u8, ent_pixels * 4) catch return WebpError.OutOfMemory;
        vp8lDecodeImageStream(alloc, br, huff_xsize, huff_ysize, false, ent_rgba)?;

        entropy_ids = alloc.alloc(u16, ent_pixels) catch return WebpError.OutOfMemory;
        let ei: usize = 0;
        while (ei < ent_pixels) : (ei += 1) {
            const gid: u16 = (@as(u16, ent_rgba[ei * 4]) << 8) | @as(u16, ent_rgba[ei * 4 + 1]);
            entropy_ids[ei] = gid;
            if (@as(usize, gid) + 1 > num_groups) {
                num_groups = @as(usize, gid) + 1;
            }
        }
    }

    // 3. Read `num_groups` * 5 Canonical Huffman Trees
    let groups = alloc.alloc(Vp8lHuffGroup, num_groups) catch return WebpError.OutOfMemory;
    const cc_extra: u16 = if (cc.bits > 0) @as(u16, 1) << @as(u4, @intCast(cc.bits)) else 0;
    let gi: usize = 0;
    while (gi < num_groups) : (gi += 1) {
        groups[gi] = Vp8lHuffGroup{};
        let ti: usize = 0;
        while (ti < 5) : (ti += 1) {
            const asize: u16 = if (ti == 0) VP8L_ALPHABET_SIZES[0] + cc_extra else VP8L_ALPHABET_SIZES[ti];
            groups[gi].trees[ti] = vp8lReadHuffCode(alloc, br, asize)?;
        }
    }

    // 4. Decode Pixel Stream (Literals, LZ77 Back-References, Color Cache Lookups)
    let cur_gid: usize = 0;
    const zero_idx: usize = 0;
    if (huff_bits > 0 && entropy_ids.len > 0) {
        cur_gid = @as(usize, entropy_ids[zero_idx]);
    }
    if (cur_gid >= num_groups) return WebpError.InvalidBitstream;
    const huff_mask: usize = if (huff_bits == 0) 0 else (@as(usize, 1) << @as(u6, @intCast(huff_bits))) - 1;
    let next_block_start: usize = 0;

    let idx: usize = 0;
    while (idx < num_pixels) {
        if (huff_bits > 0 && idx >= next_block_start) {
            const x: usize = idx % @as(usize, xsize);
            const y: usize = idx / @as(usize, xsize);
            const blk_end_x: usize = if ((x | huff_mask) < @as(usize, xsize - 1)) (x | huff_mask) else @as(usize, xsize - 1);
            next_block_start = blk_end_x + y * @as(usize, xsize) + 1;
            const ent_idx: usize = (y >> @as(u6, @intCast(huff_bits))) * @as(usize, huff_xsize) + (x >> @as(u6, @intCast(huff_bits)));
            if (ent_idx >= entropy_ids.len) return WebpError.InvalidBitstream;
            cur_gid = @as(usize, entropy_ids[ent_idx]);
            if (cur_gid >= num_groups) return WebpError.InvalidBitstream;
        }

        const green_code: u16 = groups[cur_gid].trees[0].decodeSymbol(br)?;
        if (green_code < 256) {
            const g: u8 = @as(u8, @intCast(green_code));
            const r: u8 = @as(u8, @intCast(groups[cur_gid].trees[1].decodeSymbol(br)?));
            const b: u8 = @as(u8, @intCast(groups[cur_gid].trees[2].decodeSymbol(br)?));
            const a: u8 = @as(u8, @intCast(groups[cur_gid].trees[3].decodeSymbol(br)?));
            const off: usize = idx * 4;
            out_rgba[off] = r;
            out_rgba[off + 1] = g;
            out_rgba[off + 2] = b;
            out_rgba[off + 3] = a;
            if (cc.bits > 0) cc.insert(r, g, b, a);
            idx += 1;
        } else if (green_code < 280) {
            const len_sym: u16 = green_code - 256;
            const copy_len: usize = vp8lReadCopyDistance(br, len_sym)?;
            const dist_sym: u16 = groups[cur_gid].trees[4].decodeSymbol(br)?;
            const dist_code: usize = vp8lReadCopyDistance(br, dist_sym)?;
            const dist: usize = vp8lPlaneCodeToDistance(xsize, dist_code);
            if (idx < dist || num_pixels - idx < copy_len) {
                return WebpError.InvalidBitstream;
            }
            let k: usize = 0;
            while (k < copy_len) : (k += 1) {
                const dst_off: usize = (idx + k) * 4;
                const src_off: usize = (idx + k - dist) * 4;
                const r = out_rgba[src_off];
                const g = out_rgba[src_off + 1];
                const b = out_rgba[src_off + 2];
                const a = out_rgba[src_off + 3];
                out_rgba[dst_off] = r;
                out_rgba[dst_off + 1] = g;
                out_rgba[dst_off + 2] = b;
                out_rgba[dst_off + 3] = a;
                if (cc.bits > 0) cc.insert(r, g, b, a);
            }
            idx += copy_len;
        } else {
            if (cc.bits == 0) return WebpError.InvalidBitstream;
            const cc_idx: usize = @as(usize, green_code - 280);
            if (cc_idx >= cc.table.len) return WebpError.InvalidBitstream;
            const px = cc.table[cc_idx];
            const off: usize = idx * 4;
            out_rgba[off] = px[0];
            out_rgba[off + 1] = px[1];
            out_rgba[off + 2] = px[2];
            out_rgba[off + 3] = px[3];
            idx += 1;
        }
    }
}

// --- VP8L Inverse Transforms (RFC 9649 §3) ---

fn vp8lAvg2(a: u8, b: u8): u8 {
    return @as(u8, @intCast((@as(u16, a) + @as(u16, b)) >> 1));
}

fn vp8lClampU8(v: i32): u8 {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return @as(u8, @intCast(v));
}

fn vp8lColorTransformDelta(t: u8, c: u8): u8 {
    const ti: i32 = @as(i32, @as(i8, @bitCast(t)));
    const ci: i32 = @as(i32, @as(i8, @bitCast(c)));
    const prod: u32 = @as(u32, @bitCast(ti * ci));
    return @as(u8, @intCast((prod >> 5) & 0xFF));
}

fn vp8lApplySubtractGreen(rgba: []u8) {
    let i: usize = 0;
    while (i + 3 < rgba.len) : (i += 4) {
        const g = rgba[i + 1];
        rgba[i] = rgba[i] +% g;
        rgba[i + 2] = rgba[i + 2] +% g;
    }
}

fn vp8lApplyColorTransform(rgba: []u8, width: u32, height: u32, size_bits: u8, tf_data: []const u8) {
    const bx_size: usize = @as(usize, vp8lSubsampleSize(width, size_bits));
    const sh: u6 = @as(u6, @intCast(size_bits));
    const w: usize = @as(usize, width);
    const h: usize = @as(usize, height);

    let y: usize = 0;
    while (y < h) : (y += 1) {
        const by: usize = y >> sh;
        let x: usize = 0;
        while (x < w) : (x += 1) {
            const bx: usize = x >> sh;
            const tf_off: usize = (by * bx_size + bx) * 4;
            const r2b: u8 = tf_data[tf_off];
            const g2b: u8 = tf_data[tf_off + 1];
            const g2r: u8 = tf_data[tf_off + 2];

            const p: usize = (y * w + x) * 4;
            const g: u8 = rgba[p + 1];
            const r_new: u8 = rgba[p] +% vp8lColorTransformDelta(g2r, g);
            let b_new: u8 = rgba[p + 2] +% vp8lColorTransformDelta(g2b, g);
            b_new = b_new +% vp8lColorTransformDelta(r2b, r_new);
            rgba[p] = r_new;
            rgba[p + 2] = b_new;
        }
    }
}

fn vp8lApplyPredictorTransform(rgba: []u8, width: u32, height: u32, size_bits: u8, pred_data: []const u8) {
    const w: usize = @as(usize, width);
    const h: usize = @as(usize, height);
    if (w == 0 || h == 0) return;

    // Top-left pixel (0, 0): black opaque [0, 0, 0, 255]
    rgba[3] = rgba[3] +% 255;

    // Top row (y = 0, x >= 1): mode 1 (Left)
    let x: usize = 1;
    while (x < w) : (x += 1) {
        const p: usize = x * 4;
        rgba[p] = rgba[p] +% rgba[p - 4];
        rgba[p + 1] = rgba[p + 1] +% rgba[p - 3];
        rgba[p + 2] = rgba[p + 2] +% rgba[p - 2];
        rgba[p + 3] = rgba[p + 3] +% rgba[p - 1];
    }

    // Left column (y >= 1, x = 0): mode 2 (Top)
    let y: usize = 1;
    while (y < h) : (y += 1) {
        const p: usize = y * w * 4;
        const t: usize = (y - 1) * w * 4;
        rgba[p] = rgba[p] +% rgba[t];
        rgba[p + 1] = rgba[p + 1] +% rgba[t + 1];
        rgba[p + 2] = rgba[p + 2] +% rgba[t + 2];
        rgba[p + 3] = rgba[p + 3] +% rgba[t + 3];
    }

    const bx_size: usize = @as(usize, vp8lSubsampleSize(width, size_bits));
    const sh: u6 = @as(u6, @intCast(size_bits));

    let py: usize = 1;
    while (py < h) : (py += 1) {
        const by: usize = py >> sh;
        let px: usize = 1;
        while (px < w) : (px += 1) {
            const bx: usize = px >> sh;
            const mode: u8 = pred_data[(by * bx_size + bx) * 4 + 1];

            const p: usize = (py * w + px) * 4;
            const lp: usize = p - 4;
            const tp: usize = p - w * 4;
            const tlp: usize = tp - 4;
            // RFC 9649 §3: for right-most column (x == w - 1), TopRight wraps to first pixel of current row (which is tp + 4 = (y*w)*4)!
            const trp: usize = tp + 4;

            if (mode == 0) {
                rgba[p + 3] = rgba[p + 3] +% 255;
            } else if (mode == 1) {
                let c: usize = 0;
                while (c < 4) : (c += 1) { rgba[p + c] = rgba[p + c] +% rgba[lp + c]; }
            } else if (mode == 2) {
                let c: usize = 0;
                while (c < 4) : (c += 1) { rgba[p + c] = rgba[p + c] +% rgba[tp + c]; }
            } else if (mode == 3) {
                let c: usize = 0;
                while (c < 4) : (c += 1) { rgba[p + c] = rgba[p + c] +% rgba[trp + c]; }
            } else if (mode == 4) {
                let c: usize = 0;
                while (c < 4) : (c += 1) { rgba[p + c] = rgba[p + c] +% rgba[tlp + c]; }
            } else if (mode == 5) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% vp8lAvg2(vp8lAvg2(rgba[lp + c], rgba[trp + c]), rgba[tp + c]);
                }
            } else if (mode == 6) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% vp8lAvg2(rgba[lp + c], rgba[tlp + c]);
                }
            } else if (mode == 7) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% vp8lAvg2(rgba[lp + c], rgba[tp + c]);
                }
            } else if (mode == 8) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% vp8lAvg2(rgba[tlp + c], rgba[tp + c]);
                }
            } else if (mode == 9) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% vp8lAvg2(rgba[tp + c], rgba[trp + c]);
                }
            } else if (mode == 10) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% vp8lAvg2(
                        vp8lAvg2(rgba[lp + c], rgba[tlp + c]),
                        vp8lAvg2(rgba[tp + c], rgba[trp + c]),
                    );
                }
            } else if (mode == 11) {
                let pred_l: i32 = 0;
                let pred_t: i32 = 0;
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    const lv: i32 = @as(i32, rgba[lp + c]);
                    const tv: i32 = @as(i32, rgba[tp + c]);
                    const tlv: i32 = @as(i32, rgba[tlp + c]);
                    const pr: i32 = lv + tv - tlv;
                    let dl: i32 = pr - lv;
                    if (dl < 0) dl = -dl;
                    let dt: i32 = pr - tv;
                    if (dt < 0) dt = -dt;
                    pred_l += dl;
                    pred_t += dt;
                }
                const use_src: usize = if (pred_l < pred_t) lp else tp;
                c = 0;
                while (c < 4) : (c += 1) {
                    rgba[p + c] = rgba[p + c] +% rgba[use_src + c];
                }
            } else if (mode == 12) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    const v: i32 = @as(i32, rgba[lp + c]) + @as(i32, rgba[tp + c]) - @as(i32, rgba[tlp + c]);
                    rgba[p + c] = rgba[p + c] +% vp8lClampU8(v);
                }
            } else if (mode == 13) {
                let c: usize = 0;
                while (c < 4) : (c += 1) {
                    const avg: i32 = (@as(i32, rgba[lp + c]) + @as(i32, rgba[tp + c])) >> 1;
                    const tlv: i32 = @as(i32, rgba[tlp + c]);
                    const diff: i32 = avg - tlv;
                    const half_diff: i32 = @divTrunc(diff, 2);
                    rgba[p + c] = rgba[p + c] +% vp8lClampU8(avg + half_diff);
                }
            }
        }
    }
}

fn vp8lApplyColorIndexing(
    rgba: []u8,
    width: u32,
    height: u32,
    table_size: u32,
    table_data: []const u8,
) {
    const w: usize = @as(usize, width);
    const h: usize = @as(usize, height);
    if (w == 0 || h == 0) return;

    if (table_size > 16) {
        let i: usize = 0;
        const total_px: usize = w * h;
        while (i < total_px) : (i += 1) {
            const off: usize = i * 4;
            const k: usize = @as(usize, rgba[off + 1]);
            if (k < @as(usize, table_size)) {
                const toff: usize = k * 4;
                rgba[off] = table_data[toff];
                rgba[off + 1] = table_data[toff + 1];
                rgba[off + 2] = table_data[toff + 2];
                rgba[off + 3] = table_data[toff + 3];
            } else {
                rgba[off] = 0;
                rgba[off + 1] = 0;
                rgba[off + 2] = 0;
                rgba[off + 3] = 0;
            }
        }
        return;
    }

    // Small palette (<= 16 entries): unpack 1-bit / 2-bit / 4-bit indices from bottom-right to top-left in-place!
    const w_bits: u8 = if (table_size <= 2) 3 else if (table_size <= 4) 2 else 1;
    const ppb: usize = @as(usize, 1) << @as(u6, @intCast(w_bits)); // 8, 4, or 2 pixels per packed pixel
    const bpe: u8 = @as(u8, @intCast(8 / ppb));                    // 1, 2, or 4 bits per entry
    const mask: u8 = (@as(u8, 1) << @as(u3, @intCast(bpe))) - 1;
    const packed_w: usize = @as(usize, vp8lSubsampleSize(width, w_bits));

    let y_rev: usize = 0;
    while (y_rev < h) : (y_rev += 1) {
        const y: usize = h - 1 - y_rev;
        let x_rev: usize = 0;
        while (x_rev < w) : (x_rev += 1) {
            const x: usize = w - 1 - x_rev;
            const packed_idx: usize = y * packed_w + (x / ppb);
            const packed_byte: u8 = rgba[packed_idx * 4 + 1];
            const sub: usize = x % ppb;
            const shift: u3 = @as(u3, @intCast(sub * @as(usize, bpe)));
            const k: usize = @as(usize, (packed_byte >> shift) & mask);

            const dst_off: usize = (y * w + x) * 4;
            if (k < @as(usize, table_size)) {
                const toff: usize = k * 4;
                rgba[dst_off] = table_data[toff];
                rgba[dst_off + 1] = table_data[toff + 1];
                rgba[dst_off + 2] = table_data[toff + 2];
                rgba[dst_off + 3] = table_data[toff + 3];
            } else {
                rgba[dst_off] = 0;
                rgba[dst_off + 1] = 0;
                rgba[dst_off + 2] = 0;
                rgba[dst_off + 3] = 0;
            }
        }
    }
}

struct Vp8lTransformSlot {
    present: bool = false,
    size_bits: u8 = 0,
    table_size: u32 = 0,
    data: []u8 = [],
}

/// Decodes a raw VP8L bitstream (starting with `0x2F`) or a full RIFF/WEBP container
/// (simple `VP8L` or extended `VP8X` containing `VP8L`) into caller-provided `out_rgba`.
pub fn webpDecodeLosslessInto(bytes: []const u8, out_rgba: []u8): WebpHeader ! WebpError {
    let vp8l_bytes: []const u8 = bytes;
    if (bytes.len >= 12 && fourccEq(bytes, 0, 'R', 'I', 'F', 'F') && fourccEq(bytes, 8, 'W', 'E', 'B', 'P')) {
        const chunks = parseWebpChunks(bytes)?;
        if (chunks.vp8l_payload.len == 0) {
            return WebpError.UnsupportedFormat;
        }
        vp8l_bytes = chunks.vp8l_payload;
    }

    if (vp8l_bytes.len < 5) return WebpError.UnexpectedEof;
    let br = Vp8lBitReader.init(vp8l_bytes);
    const sig = br.readBits(8)?;
    if (sig != 0x2F) return WebpError.InvalidBitstream;

    const width: u32 = br.readBits(14)? + 1;
    const height: u32 = br.readBits(14)? + 1;
    const has_alpha: bool = br.readBits(1)? != 0;
    const version: u32 = br.readBits(3)?;
    if (version != 0) return WebpError.UnsupportedFormat;
    if (width == 0 || height == 0 || width > VP8L_MAX_DIM || height > VP8L_MAX_DIM) {
        return WebpError.InvalidDimensions;
    }

    const total_rgba: usize = @as(usize, width) * @as(usize, height) * 4;
    if (out_rgba.len < total_rgba) return WebpError.BufferTooSmall;

    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Read up to 4 transforms (each type at most once)
    let transforms: [4]Vp8lTransformSlot = [
        Vp8lTransformSlot{},
        Vp8lTransformSlot{},
        Vp8lTransformSlot{},
        Vp8lTransformSlot{},
    ];
    let order: [4]u8 = [0; 4];
    let order_len: usize = 0;
    let cur_width: u32 = width;

    while (br.readBits(1)? == 1) {
        const ttype: u8 = @as(u8, @intCast(br.readBits(2)?));
        if (transforms[ttype as usize].present || order_len >= 4) {
            return WebpError.InvalidTransform;
        }
        transforms[ttype as usize].present = true;
        order[order_len] = ttype;
        order_len += 1;

        if (ttype == 0 || ttype == 1) {
            // Predictor (0) or Cross-Color (1)
            const sbits: u8 = @as(u8, @intCast(br.readBits(3)?)) + 2;
            const bw: u32 = vp8lSubsampleSize(cur_width, sbits);
            const bh: u32 = vp8lSubsampleSize(height, sbits);
            const sub_buf = alloc.alloc(u8, @as(usize, bw) * @as(usize, bh) * 4) catch return WebpError.OutOfMemory;
            vp8lDecodeImageStream(alloc, &br, bw, bh, false, sub_buf)?;
            transforms[ttype as usize].size_bits = sbits;
            transforms[ttype as usize].data = sub_buf;
        } else if (ttype == 2) {
            // Subtract Green (no sideband data)
        } else {
            // Color Indexing / Palette (3)
            const table_size: u32 = br.readBits(8)? + 1;
            let cmap = alloc.alloc(u8, @as(usize, table_size) * 4) catch return WebpError.OutOfMemory;
            vp8lDecodeImageStream(alloc, &br, table_size, 1, false, cmap)?;
            let ci: usize = 4;
            while (ci < cmap.len) : (ci += 1) {
                cmap[ci] = cmap[ci] +% cmap[ci - 4];
            }
            const pbits: u8 = if (table_size <= 2) 3 else if (table_size <= 4) 2 else if (table_size <= 16) 1 else 0;
            cur_width = vp8lSubsampleSize(cur_width, pbits);
            transforms[3].table_size = table_size;
            transforms[3].data = cmap;
        }
    }

    // 2. Decode main ARGB image stream
    const transformed_size: usize = @as(usize, cur_width) * @as(usize, height) * 4;
    vp8lDecodeImageStream(alloc, &br, cur_width, height, true, out_rgba[0..transformed_size])?;

    // 3. Apply inverse transforms in reverse order
    let active_width: u32 = cur_width;
    let active_size: usize = transformed_size;
    let oi: usize = order_len;
    while (oi > 0) {
        oi -= 1;
        const ttype: u8 = order[oi];
        if (ttype == 0) {
            vp8lApplyPredictorTransform(
                out_rgba[0..active_size],
                active_width,
                height,
                transforms[0].size_bits,
                transforms[0].data,
            );
        } else if (ttype == 1) {
            vp8lApplyColorTransform(
                out_rgba[0..active_size],
                active_width,
                height,
                transforms[1].size_bits,
                transforms[1].data,
            );
        } else if (ttype == 2) {
            vp8lApplySubtractGreen(out_rgba[0..active_size]);
        } else {
            active_width = width;
            active_size = total_rgba;
            vp8lApplyColorIndexing(
                out_rgba[0..active_size],
                width,
                height,
                transforms[3].table_size,
                transforms[3].data,
            );
        }
    }

    return WebpHeader{
        width: @as(usize, width),
        height: @as(usize, height),
        has_alpha: has_alpha,
        is_lossless: true,
        is_animation: false,
    };
}

/// Decoded RGBA8 WebP image owned by caller's allocator.
pub struct WebpDecodedImage {
    width: u32 = 0,
    height: u32 = 0,
    has_alpha: bool = false,
    rgba: []u8 = [],

    fn deinit(*_, allocator: std.mem.Allocator) {
        if (rgba.len > 0) {
            allocator.free(rgba);
            rgba = [];
        }
    }
}

/// Decodes a lossless WebP (VP8L or VP8X+VP8L) image into a newly allocated RGBA8 buffer.
pub fn webpDecodeLossless(
    allocator: std.mem.Allocator,
    bytes: []const u8,
): WebpDecodedImage ! WebpError {
    const info = parseWebpChunks(bytes)?;
    if (info.vp8l_payload.len < 5 || info.vp8l_payload[0] != 0x2F) {
        return WebpError.UnsupportedFormat;
    }
    const b0: u32 = info.vp8l_payload[1];
    const b1: u32 = info.vp8l_payload[2];
    const b2: u32 = info.vp8l_payload[3];
    const b3: u32 = info.vp8l_payload[4];
    const w: u32 = (b0 | ((b1 & 0x3F) << 8)) + 1;
    const h: u32 = ((b1 >> 6) | (b2 << 2) | ((b3 & 0x0F) << 10)) + 1;
    if (w == 0 || h == 0 || w > VP8L_MAX_DIM || h > VP8L_MAX_DIM) {
        return WebpError.InvalidDimensions;
    }

    const total: usize = @as(usize, w) * @as(usize, h) * 4;
    const out = allocator.alloc(u8, total) catch return WebpError.OutOfMemory;
    const hdr = webpDecodeLosslessInto(info.vp8l_payload, out) catch |e| {
        allocator.free(out);
        return e;
    };
    return WebpDecodedImage{
        width: @as(u32, @intCast(hdr.width)),
        height: @as(u32, @intCast(hdr.height)),
        has_alpha: hdr.has_alpha,
        rgba: out,
    };
}

pub class Webp {
    /// 快速探测 WebP RIFF 容器、VP8/VP8L/VP8X 标志与尺寸
    pub static fn probe(bytes: []const u8): WebpHeader? {
        if (bytes.len < 16) {
            return null;
        }
        // RIFF .... WEBP
        let is_riff = bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F';
        let is_webp = bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P';
        if (!is_riff || !is_webp) {
            return null;
        }

        let chunk = bytes[12..16];
        if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == ' ') {
            // VP8 Simple Lossy (check VP8 keyframe sync code 0x9D 0x01 0x2A at offset 23..25)
            if (bytes.len < 30 || bytes[23] != 0x9D || bytes[24] != 0x01 || bytes[25] != 0x2A) {
                return null;
            }
            let w = ((bytes[26] as usize) | ((bytes[27] as usize) << 8)) & 0x3FFF;
            let h = ((bytes[28] as usize) | ((bytes[29] as usize) << 8)) & 0x3FFF;
            return WebpHeader{
                width: w,
                height: h,
                has_alpha: false,
                is_lossless: false,
                is_animation: false,
            };
        } else if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'L') {
            // VP8L Lossless
            if (bytes.len < 25 || bytes[20] != 0x2F) {
                return null;
            }
            let b0 = (bytes[21] as usize);
            let b1 = (bytes[22] as usize);
            let b2 = (bytes[23] as usize);
            let b3 = (bytes[24] as usize);
            let w = (b0 | ((b1 & 0x3F) << 8)) + 1;
            let h = (((b1 >> 6) | (b2 << 2) | ((b3 & 0x0F) << 10))) + 1;
            return WebpHeader{
                width: w,
                height: h,
                has_alpha: (b3 & 0x10) != 0,
                is_lossless: true,
                is_animation: false,
            };
        } else if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'X') {
            // VP8X Extended
            if (bytes.len < 30) {
                return null;
            }
            let flags = bytes[20];
            let has_anim = (flags & 0x02) != 0;
            let has_alpha = (flags & 0x10) != 0;
            let w = ((bytes[24] as usize) | ((bytes[25] as usize) << 8) | ((bytes[26] as usize) << 16)) + 1;
            let h = ((bytes[27] as usize) | ((bytes[28] as usize) << 8) | ((bytes[29] as usize) << 16)) + 1;
            return WebpHeader{
                width: w,
                height: h,
                has_alpha: has_alpha,
                is_lossless: false,
                is_animation: has_anim,
            };
        }

        return null;
    }

    /// 编码 RGBA8 像素为 RFC 9649 WebP (VP8L) 字节流
    pub static fn encode(
        allocator: std.mem.Allocator,
        rgba: []const u8,
        width: u32,
        height: u32,
        quality: u8,
    ): ([]u8) ! WebpError {
        return webpEncode(allocator, rgba, width, height, quality);
    }

    /// 严格无损编码 RGBA8 像素为 RFC 9649 WebP (VP8L) 字节流
    pub static fn encodeLossless(
        allocator: std.mem.Allocator,
        rgba: []const u8,
        width: u32,
        height: u32,
    ): ([]u8) ! WebpError {
        return webpEncodeLossless(allocator, rgba, width, height);
    }

    /// 解析 RIFF/WEBP (含 VP8X/ICCP/EXIF/XMP/ANIM/VP8L) 容器元数据与分块切片
    pub static fn parseChunks(bytes: []const u8): WebpExtendedInfo ! WebpError {
        return parseWebpChunks(bytes);
    }

    /// 无损解码 VP8L / VP8X+VP8L 到调用方外部 RGBA8 切片
    pub static fn decodeLosslessInto(bytes: []const u8, out_rgba: []u8): WebpHeader ! WebpError {
        return webpDecodeLosslessInto(bytes, out_rgba);
    }

    /// 无损解码 VP8L / VP8X+VP8L 到新分配 RGBA8 图像
    pub static fn decodeLossless(allocator: std.mem.Allocator, bytes: []const u8): WebpDecodedImage ! WebpError {
        return webpDecodeLossless(allocator, bytes);
    }
}

/// WebP 图像编码器与探测器别名类（提供 `WebpImage.encode` / `WebpImage.encodeLossless` / `WebpImage.probe`）
pub class WebpImage {
    pub static fn probe(bytes: []const u8): WebpHeader? {
        return Webp.probe(bytes);
    }

    pub static fn encode(
        allocator: std.mem.Allocator,
        rgba: []const u8,
        width: u32,
        height: u32,
        quality: u8,
    ): ([]u8) ! WebpError {
        return webpEncode(allocator, rgba, width, height, quality);
    }

    pub static fn encodeLossless(
        allocator: std.mem.Allocator,
        rgba: []const u8,
        width: u32,
        height: u32,
    ): ([]u8) ! WebpError {
        return webpEncodeLossless(allocator, rgba, width, height);
    }

    pub static fn parseChunks(bytes: []const u8): WebpExtendedInfo ! WebpError {
        return parseWebpChunks(bytes);
    }

    pub static fn decodeLosslessInto(bytes: []const u8, out_rgba: []u8): WebpHeader ! WebpError {
        return webpDecodeLosslessInto(bytes, out_rgba);
    }

    pub static fn decodeLossless(allocator: std.mem.Allocator, bytes: []const u8): WebpDecodedImage ! WebpError {
        return webpDecodeLossless(allocator, bytes);
    }
}

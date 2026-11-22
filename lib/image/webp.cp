// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_webp.cp | Synced: 2026-11-22T06:39:08Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_webp.cp
// Extended with Production RFC 9649 WebP Lossless (VP8L) Encoder
// ============================================================================
const std = @import("std");
import { Codecs } from "./codecs";

pub err WebpError {
    InvalidDimensions,
    InvalidBufferLength,
    DimensionTooLarge,
    OutOfMemory,
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
}

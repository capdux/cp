// @cap/huffman — Zero-allocation RFC 1951 Canonical Huffman codec for CP
// Clean-room MIT implementation inspired by Rust `huffman-compress` and RFC 1951 Section 3.2.2.
//
// Features:
// - Full 256-byte symbol alphabet (`0..=255`) with maximum code length `MAX_CODE_BITS = 16`.
// - `fromFrequencies(freqs)`: builds an optimal Huffman tree on the stack, enforces <= 16-bit
//   Kraft-valid lengths, and canonicalizes per RFC 1951.
// - `fromCodeLengths(lengths)`: reconstructs the exact same canonical codes purely from bit lengths.
// - MSB-first bitstream `encode` / `decode` with 4-byte big-endian original length header for exact
//   end-of-stream truncation and strict malformed/truncated bitstream detection (returns `-1`).
// - Accelerated with 16-lane `@Vector(16, u8)` / 8-lane `@Vector(8, u16)` zeroing & sparse-block skip,
//   64-bit bit-buffer accumulation, and O(1) 8-bit fast-path decode LUT.

type HufVec16u8 = @Vector(16, u8);
type HufVec8u16 = @Vector(8, u16);

pub const MAX_SYMBOLS: usize = 256;
pub const MAX_CODE_BITS: u8 = 16;
const MAX_TREE_NODES: usize = 512;

fn zeroU8x256(buf: []u8) {
    const zv: HufVec16u8 = @splat(0 as u8);
    const z: [16]u8 = zv;
    let i: usize = 0;
    while (i < 256) {
        @memcpy(buf[i .. i + 16], z[0..16]);
        i += 16;
    }
}

fn zeroU16x256(buf: []u16) {
    const zv: HufVec8u16 = @splat(0 as u16);
    const z: [8]u16 = zv;
    let i: usize = 0;
    while (i < 256) {
        @memcpy(buf[i .. i + 8], z[0..8]);
        i += 8;
    }
}

/// RFC 1951 Canonical Huffman encoder & decoder for 256 byte symbols (zero heap allocation).
pub struct HuffmanCodec {
    pub code_len: [MAX_SYMBOLS]u8,
    pub codes: [MAX_SYMBOLS]u16,
    pub symbol_count: usize,

    pub static fn empty() -> HuffmanCodec {
        let lens: [MAX_SYMBOLS]u8 = undefined;
        let cds: [MAX_SYMBOLS]u16 = undefined;
        zeroU8x256(lens[0..MAX_SYMBOLS]);
        zeroU16x256(cds[0..MAX_SYMBOLS]);
        return HuffmanCodec{
            code_len: lens,
            codes: cds,
            symbol_count: 0,
        };
    }

    /// Reconstructs a Canonical Huffman table strictly from symbol bit lengths (RFC 1951 §3.2.2).
    pub static fn fromCodeLengths(lengths: []const u8) -> HuffmanCodec {
        let codec = HuffmanCodec.empty();
        let bl_count: [17]u16 = undefined;
        let next_code: [17]u32 = undefined;

        let b: usize = 0;
        while (b <= 16) {
            bl_count[b] = 0;
            next_code[b] = 0;
            b += 1;
        }

        let limit = lengths.len;
        if (limit > MAX_SYMBOLS) {
            limit = MAX_SYMBOLS;
        }

        let s: usize = 0;
        while (s + 16 <= limit) {
            const v: HufVec16u8 = lengths[s .. s + 16][0..16].*;
            if (@reduce(.Or, v) != (0 as u8)) {
                let k: usize = 0;
                while (k < 16) {
                    const l = lengths[s + k];
                    if (l > 0 && l <= MAX_CODE_BITS) {
                        codec.code_len[s + k] = l;
                        bl_count[l as usize] += 1;
                        codec.symbol_count += 1;
                    }
                    k += 1;
                }
            }
            s += 16;
        }
        while (s < limit) {
            const l = lengths[s];
            if (l > 0 && l <= MAX_CODE_BITS) {
                codec.code_len[s] = l;
                bl_count[l as usize] += 1;
                codec.symbol_count += 1;
            }
            s += 1;
        }

        let code: u32 = 0;
        b = 1;
        while (b <= 16) {
            code = (code + (bl_count[b - 1] as u32)) << 1;
            next_code[b] = code;
            b += 1;
        }

        s = 0;
        while (s < MAX_SYMBOLS) {
            const v: HufVec16u8 = codec.code_len[s .. s + 16][0..16].*;
            if (@reduce(.Or, v) != (0 as u8)) {
                let k: usize = 0;
                while (k < 16) {
                    const l = codec.code_len[s + k];
                    if (l > 0) {
                        codec.codes[s + k] = (next_code[l as usize] & 0xFFFF) as u16;
                        next_code[l as usize] += 1;
                    }
                    k += 1;
                }
            }
            s += 16;
        }

        return codec;
    }

    /// Builds a length-limited (<= 16 bits) Canonical Huffman table from symbol frequencies.
    pub static fn fromFrequencies(freqs: []const u32) -> HuffmanCodec {
        let lengths: [MAX_SYMBOLS]u8 = undefined;
        zeroU8x256(lengths[0..MAX_SYMBOLS]);

        let limit = freqs.len;
        if (limit > MAX_SYMBOLS) {
            limit = MAX_SYMBOLS;
        }

        let sym_of: [MAX_SYMBOLS]u8 = undefined;
        let weight: [MAX_TREE_NODES]u64 = undefined;
        let parent: [MAX_TREE_NODES]i16 = undefined;
        let active: [MAX_TREE_NODES]bool = undefined;

        let n: usize = 0;
        let i: usize = 0;
        while (i < limit) {
            if (freqs[i] > 0) {
                sym_of[n] = i as u8;
                weight[n] = freqs[i] as u64;
                parent[n] = -1;
                active[n] = true;
                n += 1;
            }
            i += 1;
        }

        if (n == 0) {
            return HuffmanCodec.empty();
        }
        if (n == 1) {
            lengths[sym_of[0] as usize] = 1;
            return HuffmanCodec.fromCodeLengths(lengths[0..MAX_SYMBOLS]);
        }

        // Standard Huffman tree construction: merge two minimum-weight active nodes (n - 1) times
        let total_nodes = n;
        let step: usize = 0;
        while (step + 1 < n) {
            let min1: usize = MAX_TREE_NODES;
            let min2: usize = MAX_TREE_NODES;
            let k: usize = 0;
            while (k < total_nodes) {
                if (active[k]) {
                    if (min1 == MAX_TREE_NODES || weight[k] < weight[min1]) {
                        min2 = min1;
                        min1 = k;
                    } else if (min2 == MAX_TREE_NODES || weight[k] < weight[min2]) {
                        min2 = k;
                    }
                }
                k += 1;
            }

            const nid = total_nodes;
            weight[nid] = weight[min1] + weight[min2];
            parent[nid] = -1;
            active[nid] = true;
            parent[min1] = nid as i16;
            parent[min2] = nid as i16;
            active[min1] = false;
            active[min2] = false;
            total_nodes += 1;
            step += 1;
        }

        // Compute leaf depths and clamp to MAX_CODE_BITS (16)
        let overflowed = false;
        let k: usize = 0;
        while (k < n) {
            let depth: u8 = 0;
            let cur = k;
            while (parent[cur] >= 0) {
                depth += 1;
                cur = parent[cur] as usize;
            }
            if (depth > MAX_CODE_BITS) {
                depth = MAX_CODE_BITS;
                overflowed = true;
            }
            lengths[sym_of[k] as usize] = depth;
            k += 1;
        }

        // Enforce Kraft inequality (sum 2^(16 - l_i) <= 65536) if any depth was clamped
        if (overflowed) {
            let kraft: u32 = 0;
            k = 0;
            while (k < n) {
                const l = lengths[sym_of[k] as usize];
                kraft += (1 as u32) << ((16 - l) as u5);
                k += 1;
            }
            while (kraft > 65536) {
                let best_idx: usize = n;
                let best_len: u8 = 0;
                k = 0;
                while (k < n) {
                    const l = lengths[sym_of[k] as usize];
                    if (l < MAX_CODE_BITS && l > best_len) {
                        best_len = l;
                        best_idx = k;
                    }
                    k += 1;
                }
                if (best_idx == n) {
                    break;
                }
                const sym = sym_of[best_idx] as usize;
                const old_l = lengths[sym];
                lengths[sym] = old_l + 1;
                kraft -= (1 as u32) << ((15 - old_l) as u5);
            }
        }

        return HuffmanCodec.fromCodeLengths(lengths[0..MAX_SYMBOLS]);
    }

    pub fn codeLength(sym: u8) -> u8 {
        return _.code_len[sym as usize];
    }

    pub fn codeBits(sym: u8) -> u16 {
        return _.codes[sym as usize];
    }

    /// Encodes `src` into `dst` (4-byte BE original length + MSB-first Huffman bitstream).
    /// Returns total bytes written, or `-1` on buffer overflow or unregistered symbol.
    pub fn encode(src: []const u8, dst: []u8) -> i64 {
        if (dst.len < 4) {
            return -1;
        }
        const orig_len: u32 = src.len as u32;
        dst[0] = ((orig_len >> 24) & 0xFF) as u8;
        dst[1] = ((orig_len >> 16) & 0xFF) as u8;
        dst[2] = ((orig_len >> 8) & 0xFF) as u8;
        dst[3] = (orig_len & 0xFF) as u8;

        if (src.len == 0) {
            return 4;
        }

        let bit_buf: u64 = 0;
        let bit_count: u8 = 0;
        let out_pos: usize = 4;

        let i: usize = 0;
        while (i < src.len) {
            const sym = src[i];
            const clen = _.code_len[sym as usize];
            if (clen == 0) {
                return -1;
            }
            const cbits: u64 = _.codes[sym as usize] as u64;
            bit_buf = (bit_buf << (clen as u6)) | cbits;
            bit_count += clen;

            if (bit_count >= 32) {
                if (out_pos + 4 > dst.len) {
                    while (bit_count >= 8) {
                        bit_count -= 8;
                        if (out_pos >= dst.len) {
                            return -1;
                        }
                        dst[out_pos] = ((bit_buf >> (bit_count as u6)) & 0xFF) as u8;
                        out_pos += 1;
                    }
                } else {
                    const shift = (bit_count - 32) as u6;
                    const w: u32 = ((bit_buf >> shift) & 0xFFFFFFFF) as u32;
                    dst[out_pos] = ((w >> 24) & 0xFF) as u8;
                    dst[out_pos + 1] = ((w >> 16) & 0xFF) as u8;
                    dst[out_pos + 2] = ((w >> 8) & 0xFF) as u8;
                    dst[out_pos + 3] = (w & 0xFF) as u8;
                    out_pos += 4;
                    bit_count -= 32;
                }
            }
            i += 1;
        }

        while (bit_count >= 8) {
            bit_count -= 8;
            if (out_pos >= dst.len) {
                return -1;
            }
            dst[out_pos] = ((bit_buf >> (bit_count as u6)) & 0xFF) as u8;
            out_pos += 1;
        }

        if (bit_count > 0) {
            if (out_pos >= dst.len) {
                return -1;
            }
            const pad = (8 - bit_count) as u6;
            dst[out_pos] = ((bit_buf << pad) & 0xFF) as u8;
            out_pos += 1;
        }

        return out_pos as i64;
    }

    /// Decodes a Canonical Huffman bitstream produced by `encode` into `dst`.
    /// Returns decoded byte count, or `-1` on truncated/malformed input or insufficient `dst`.
    pub fn decode(src: []const u8, dst: []u8) -> i64 {
        if (src.len < 4) {
            return -1;
        }
        const orig_len: usize = (((src[0] as u32) << 24) |
            ((src[1] as u32) << 16) |
            ((src[2] as u32) << 8) |
            (src[3] as u32)) as usize;

        if (orig_len > dst.len) {
            return -1;
        }
        if (orig_len == 0) {
            return 0;
        }
        if (_.symbol_count == 0) {
            return -1;
        }

        // Build compact canonical decode lookup arrays + 8-bit fast LUT on the stack
        let bl_count: [17]u16 = undefined;
        let first_code: [17]u32 = undefined;
        let first_idx: [17]u16 = undefined;
        let sorted_syms: [MAX_SYMBOLS]u8 = undefined;
        let fast_lut: [256]u16 = undefined;
        zeroU16x256(fast_lut[0..256]);

        let b: usize = 0;
        while (b <= 16) {
            bl_count[b] = 0;
            first_code[b] = 0;
            first_idx[b] = 0;
            b += 1;
        }

        let s: usize = 0;
        while (s < MAX_SYMBOLS) {
            const v: HufVec16u8 = _.code_len[s .. s + 16][0..16].*;
            if (@reduce(.Or, v) != (0 as u8)) {
                let k: usize = 0;
                while (k < 16) {
                    const l = _.code_len[s + k];
                    if (l > 0 && l <= MAX_CODE_BITS) {
                        bl_count[l as usize] += 1;
                    }
                    k += 1;
                }
            }
            s += 16;
        }

        let code_acc: u32 = 0;
        let idx_acc: u16 = 0;
        b = 1;
        while (b <= 16) {
            code_acc = (code_acc + (bl_count[b - 1] as u32)) << 1;
            first_code[b] = code_acc;
            first_idx[b] = idx_acc;
            idx_acc += bl_count[b];
            b += 1;
        }

        let cursor_idx: [17]u16 = undefined;
        @memcpy(cursor_idx[0..17], first_idx[0..17]);

        s = 0;
        while (s < MAX_SYMBOLS) {
            const v: HufVec16u8 = _.code_len[s .. s + 16][0..16].*;
            if (@reduce(.Or, v) != (0 as u8)) {
                let k: usize = 0;
                while (k < 16) {
                    const sym_idx = s + k;
                    const l = _.code_len[sym_idx];
                    if (l > 0 && l <= MAX_CODE_BITS) {
                        const pos_in_len = cursor_idx[l as usize];
                        sorted_syms[pos_in_len as usize] = sym_idx as u8;
                        cursor_idx[l as usize] = pos_in_len + 1;
                        if (l <= 8) {
                            const code_val = first_code[l as usize] + ((pos_in_len - first_idx[l as usize]) as u32);
                            const pad_bits = (8 - l) as u5;
                            const base_entry = (code_val << pad_bits) as usize;
                            const span = (1 as usize) << pad_bits;
                            const entry: u16 = ((l as u16) << 8) | (sym_idx as u16);
                            let fill: usize = 0;
                            while (fill < span) {
                                fast_lut[base_entry + fill] = entry;
                                fill += 1;
                            }
                        }
                    }
                    k += 1;
                }
            }
            s += 16;
        }

        // Decode MSB-first bitstream using 64-bit accumulator + O(1) 8-bit fast LUT
        let byte_pos: usize = 4;
        let bit_buf: u64 = 0;
        let bits_left: u8 = 0;
        let decoded: usize = 0;

        while (decoded < orig_len) {
            while (bits_left <= 56 && byte_pos < src.len) {
                bit_buf = (bit_buf << 8) | (src[byte_pos] as u64);
                byte_pos += 1;
                bits_left += 8;
            }

            // O(1) fast path for codes <= 8 bits when >= 8 bits are buffered
            if (bits_left >= 8) {
                const top8 = ((bit_buf >> ((bits_left - 8) as u6)) & 0xFF) as usize;
                const entry = fast_lut[top8];
                if (entry != 0) {
                    const flen = (entry >> 8) as u8;
                    dst[decoded] = (entry & 0xFF) as u8;
                    decoded += 1;
                    bits_left -= flen;
                    continue;
                }
            }

            // Slow path for codes > 8 bits or end-of-stream (< 8 bits left)
            let cur_code: u32 = 0;
            let cur_len: usize = 0;
            let matched = false;

            while (cur_len < 16) {
                if (bits_left == 0) {
                    if (byte_pos >= src.len) {
                        return -1;
                    }
                    bit_buf = (bit_buf << 8) | (src[byte_pos] as u64);
                    byte_pos += 1;
                    bits_left = 8;
                }
                bits_left -= 1;
                const bit: u32 = ((bit_buf >> (bits_left as u6)) & 1) as u32;
                cur_code = (cur_code << 1) | bit;
                cur_len += 1;

                const cnt: u32 = bl_count[cur_len] as u32;
                if (cnt > 0 && cur_code >= first_code[cur_len] && cur_code < first_code[cur_len] + cnt) {
                    const offset = (cur_code - first_code[cur_len]) as usize;
                    dst[decoded] = sorted_syms[(first_idx[cur_len] as usize) + offset];
                    decoded += 1;
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                return -1;
            }
        }

        return decoded as i64;
    }
}

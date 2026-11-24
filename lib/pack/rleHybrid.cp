// Origin: Pure-CP Stdlib (Zero-Alloc Apache Parquet / Arrow Hybrid RLE + Bit-Packed Codec using @clz) | Created: 2026-11-22
// Pure CP zero-heap-allocation Parquet / Arrow Hybrid RLE + Bit-Packed integer codec:
//   - Follows Parquet RLE_DICTIONARY / Definition-Level hybrid format:
//       * RLE run header        : ULEB128 `(run_len << 1) | 0` + `ceil(bit_width / 8)` LE value bytes
//       * Bit-Packed run header : ULEB128 `(num_groups << 1) | 1` + `num_groups * bit_width` bytes
//         (each group packs 8 values LSB-first into exactly `bit_width` bytes)
//   - Full `bit_width ∈ 0..32` lossless encoding & decoding with automatic RLE switching (`run >= 8`)
//   - Zero-copy streaming batch decoder (`HybridRleDecoder`) with strict malformed/truncation guards

// ============================================================================
// Constants & Bit-Width Helpers
// ============================================================================

pub const HYBRID_RLE_MAX_BIT_WIDTH: u8 = 32;
pub const HYBRID_RLE_MIN_RUN_LEN: usize = 8;
pub const HYBRID_RLE_GROUP_SIZE: usize = 8;
pub const HYBRID_RLE_MAX_BITPACK_GROUPS: usize = 63;

/// Computes minimum bit width (`0..=32`) required to represent `max_val` using native `@clz`.
pub fn hybridBitWidthU32(max_val: u32): u8 {
    if (max_val == 0) {
        return 0;
    }
    const lz: u32 = @clz(max_val) as u32;
    return (32 - lz) as u8;
}

/// Scans `values` and returns the minimum `bit_width ∈ 0..=32` via native `@clz`.
pub fn hybridMinBitWidthSlice(values: []const u32): u8 {
    let max_v: u32 = 0;
    let i: usize = 0;
    while (i < values.len) : (i += 1) {
        if (values[i] > max_v) {
            max_v = values[i];
        }
    }
    return hybridBitWidthU32(max_v);
}

/// Returns `ceil(bit_width / 8)` byte width for an RLE repeated value (`0` when `bit_width == 0`).
pub fn hybridValueByteWidth(bit_width: u8): usize {
    if (bit_width == 0) {
        return 0;
    }
    return ((bit_width as usize) + 7) / 8;
}

/// Conservative upper bound on encoded byte size for `count` values at `bit_width`.
pub fn hybridMaxEncodedSize(count: usize, bit_width: u8): usize {
    if (count == 0) {
        return 0;
    }
    if (bit_width == 0) {
        return 5; // Single ULEB128 RLE header
    }
    const groups: usize = (count + 7) / 8;
    const headers: usize = ((groups + HYBRID_RLE_MAX_BITPACK_GROUPS - 1) / HYBRID_RLE_MAX_BITPACK_GROUPS) * 5;
    return headers + groups * (bit_width as usize) + 16;
}

fn valueFitsBitWidth(val: u32, bit_width: u8): bool {
    if (bit_width == 0) {
        return val == 0;
    }
    if (bit_width >= 32) {
        return true;
    }
    const limit: u64 = (1 as u64) << (bit_width as u6);
    return (val as u64) < limit;
}

fn writeUleb128U32(dst: *[]u8, pos: usize, val_in: u32): usize? {
    let v: u32 = val_in;
    let p: usize = pos;
    while (true) {
        if (p >= dst.len) {
            return null;
        }
        let byte: u8 = (v & 0x7F) as u8;
        v >>= 7;
        if (v != 0) {
            byte |= 0x80;
        }
        dst[p] = byte;
        p += 1;
        if (v == 0) {
            return p;
        }
    }
    return null;
}

struct VarintU32Decoded {
    ok: bool = false;
    value: u32 = 0;
    next_pos: usize = 0;
}

fn readUleb128U32(src: []const u8, pos: usize): VarintU32Decoded {
    let p: usize = pos;
    let result: u32 = 0;
    let shift: u32 = 0;
    let count: usize = 0;

    while (count < 5) : (count += 1) {
        if (p >= src.len) {
            return VarintU32Decoded {};
        }
        const byte: u8 = src[p];
        p += 1;

        // 5th byte (shift == 28) may only carry 4 low bits without overflowing u32.
        if (count == 4 && (byte & 0xF0) != 0) {
            return VarintU32Decoded {};
        }

        const payload: u32 = (byte & 0x7F) as u32;
        result |= payload << (shift as u5);
        if ((byte & 0x80) == 0) {
            return VarintU32Decoded {
                ok: true,
                value: result,
                next_pos: p,
            };
        }
        shift += 7;
    }
    return VarintU32Decoded {};
}

fn packGroup8Lsb(vals: [8]u32, bit_width: u8, dst: *[]u8, pos: usize): usize? {
    const needed: usize = bit_width as usize;
    if (pos + needed > dst.len) {
        return null;
    }
    if (needed == 0) {
        return pos;
    }

    let acc: u64 = 0;
    let bits: u32 = 0;
    let out_p: usize = pos;
    const bw: u32 = bit_width as u32;

    let i: usize = 0;
    while (i < 8) : (i += 1) {
        acc |= (vals[i] as u64) << (bits as u6);
        bits += bw;
        while (bits >= 8) {
            dst[out_p] = (acc & 0xFF) as u8;
            out_p += 1;
            acc >>= 8;
            bits -= 8;
        }
    }
    return out_p;
}

fn unpackGroup8Lsb(src: []const u8, pos: usize, bit_width: u8, out_group: *[8]u32): usize? {
    const needed: usize = bit_width as usize;
    if (pos + needed > src.len) {
        return null;
    }
    if (needed == 0) {
        let z: usize = 0;
        while (z < 8) : (z += 1) {
            out_group[z] = 0;
        }
        return pos;
    }

    const mask: u64 = if (bit_width == 32) {
        0xFFFFFFFF
    } else {
        ((1 as u64) << (bit_width as u6)) - 1
    };

    let acc: u64 = 0;
    let bits: u32 = 0;
    let in_p: usize = pos;
    const bw: u32 = bit_width as u32;

    let i: usize = 0;
    while (i < 8) : (i += 1) {
        while (bits < bw) {
            acc |= (src[in_p] as u64) << (bits as u6);
            in_p += 1;
            bits += 8;
        }
        out_group[i] = (acc & mask) as u32;
        acc >>= (bw as u6);
        bits -= bw;
    }
    return in_p;
}

// ============================================================================
// 1. Status & Result Models
// ============================================================================

/// Status code for Hybrid RLE + Bit-Packed encoding and decoding.
pub enum HybridRleStatus {
    ok,
    invalid_bit_width,
    value_out_of_range,
    buffer_too_small,
    malformed_varint,
    truncated_stream,
    zero_run_header,
}

/// Summary returned by `encodeHybridRleU32`.
pub struct HybridRleEncodeResult {
    status: HybridRleStatus = HybridRleStatus.ok;
    bytes_written: usize = 0;
    rle_runs: usize = 0;
    bitpack_groups: usize = 0;
}

/// Summary returned by `decodeHybridRleU32` and `HybridRleDecoder.nextBatch`.
pub struct HybridRleDecodeResult {
    status: HybridRleStatus = HybridRleStatus.ok;
    values_decoded: usize = 0;
    bytes_consumed: usize = 0;
}

// ============================================================================
// 2. Hybrid RLE + Bit-Packed Encoder
// ============================================================================

fn countRunLen(values: []const u32, start: usize): usize {
    const v0: u32 = values[start];
    let n: usize = 1;
    while (start + n < values.len && values[start + n] == v0) : (n += 1) {}
    return n;
}

/// Encodes `values` using Apache Parquet Hybrid RLE + Bit-Packed encoding at `bit_width ∈ 0..=32`.
/// Automatically switches to RLE runs for repeated sequences of length >= 8 and Bit-Packed
/// 8-value groups for high-entropy spans. Zero heap allocation.
pub fn encodeHybridRleU32(
    values: []const u32,
    bit_width: u8,
    out_buf: *[]u8,
): HybridRleEncodeResult {
    if (bit_width > HYBRID_RLE_MAX_BIT_WIDTH) {
        return HybridRleEncodeResult { status: HybridRleStatus.invalid_bit_width };
    }
    if (values.len == 0) {
        return HybridRleEncodeResult { status: HybridRleStatus.ok };
    }

    // Validate every value fits within `bit_width` before emitting bytes.
    let v_idx: usize = 0;
    while (v_idx < values.len) : (v_idx += 1) {
        if (!valueFitsBitWidth(values[v_idx], bit_width)) {
            return HybridRleEncodeResult { status: HybridRleStatus.value_out_of_range };
        }
    }

    // Fast path for bit_width == 0: entire zero column collapses into a single RLE header.
    if (bit_width == 0) {
        if (values.len > 0x7FFFFFFF) {
            return HybridRleEncodeResult { status: HybridRleStatus.buffer_too_small };
        }
        const header: u32 = ((values.len as u32) << 1);
        const next_p = writeUleb128U32(out_buf, 0, header) ?? {
            return HybridRleEncodeResult { status: HybridRleStatus.buffer_too_small };
        };
        return HybridRleEncodeResult {
            status: HybridRleStatus.ok,
            bytes_written: next_p,
            rle_runs: 1,
            bitpack_groups: 0,
        };
    }

    const val_bytes: usize = hybridValueByteWidth(bit_width);
    let out_pos: usize = 0;
    let rle_runs: usize = 0;
    let bp_groups_total: usize = 0;
    let pos: usize = 0;

    while (pos < values.len) {
        const run_len: usize = countRunLen(values, pos);
        if (run_len >= HYBRID_RLE_MIN_RUN_LEN) {
            // Emit RLE run: header = (run_len << 1) | 0, followed by `val_bytes` LE bytes.
            const max_chunk: usize = 0x7FFFFFFF;
            const emit_len: usize = if (run_len < max_chunk) run_len else max_chunk;
            const header: u32 = (emit_len as u32) << 1;
            const after_hdr = writeUleb128U32(out_buf, out_pos, header) ?? {
                return HybridRleEncodeResult { status: HybridRleStatus.buffer_too_small };
            };
            if (after_hdr + val_bytes > out_buf.len) {
                return HybridRleEncodeResult { status: HybridRleStatus.buffer_too_small };
            }
            const rle_val: u32 = values[pos];
            let b: usize = 0;
            while (b < val_bytes) : (b += 1) {
                const shift: u5 = (b * 8) as u5;
                out_buf[after_hdr + b] = ((rle_val >> shift) & 0xFF) as u8;
            }
            out_pos = after_hdr + val_bytes;
            pos += emit_len;
            rle_runs += 1;
        } else {
            // Collect up to HYBRID_RLE_MAX_BITPACK_GROUPS (63) 8-value bit-packed groups,
            // stopping early if an RLE-worthy run (>= 8 identical values) begins at a group boundary.
            let num_groups: usize = 0;
            let scan: usize = pos;
            while (scan < values.len && num_groups < HYBRID_RLE_MAX_BITPACK_GROUPS) {
                if (num_groups > 0 && countRunLen(values, scan) >= HYBRID_RLE_MIN_RUN_LEN) {
                    break;
                }
                num_groups += 1;
                const adv: usize = if (values.len - scan < 8) (values.len - scan) else 8;
                scan += adv;
            }

            const header: u32 = ((num_groups as u32) << 1) | 1;
            const after_hdr = writeUleb128U32(out_buf, out_pos, header) ?? {
                return HybridRleEncodeResult { status: HybridRleStatus.buffer_too_small };
            };
            out_pos = after_hdr;

            let g: usize = 0;
            while (g < num_groups) : (g += 1) {
                let grp: [8]u32 = [0; 8];
                let k: usize = 0;
                while (k < 8 && pos < values.len) : (k += 1) {
                    grp[k] = values[pos];
                    pos += 1;
                }
                const after_grp = packGroup8Lsb(grp, bit_width, out_buf, out_pos) ?? {
                    return HybridRleEncodeResult { status: HybridRleStatus.buffer_too_small };
                };
                out_pos = after_grp;
            }
            bp_groups_total += num_groups;
        }
    }

    return HybridRleEncodeResult {
        status: HybridRleStatus.ok,
        bytes_written: out_pos,
        rle_runs: rle_runs,
        bitpack_groups: bp_groups_total,
    };
}

// ============================================================================
// 3. Zero-Copy Streaming Hybrid RLE Decoder
// ============================================================================

/// Zero-copy streaming batch decoder for Parquet Hybrid RLE + Bit-Packed streams.
pub struct HybridRleDecoder {
    data: []const u8 = "";
    byte_pos: usize = 0;
    bit_width: u8 = 0;
    expected_count: usize = 0;
    decoded_count: usize = 0;
    in_rle: bool = false;
    rle_val: u32 = 0;
    rle_rem: usize = 0;
    bp_groups_rem: usize = 0;
    bp_buf: [8]u32 = [0; 8];
    bp_idx: usize = 8;
    last_status: HybridRleStatus = HybridRleStatus.ok;

    /// Initializes a streaming decoder over `data` for `expected_count` values at `bit_width ∈ 0..=32`.
    /// Returns `null` if `bit_width > 32`.
    static fn init(data: []const u8, bit_width: u8, expected_count: usize): HybridRleDecoder? {
        if (bit_width > HYBRID_RLE_MAX_BIT_WIDTH) {
            return null;
        }
        return HybridRleDecoder {
            data: data,
            bit_width: bit_width,
            expected_count: expected_count,
        };
    }

    fn remaining(): usize {
        return _.expected_count - _.decoded_count;
    }

    fn isDone(): bool {
        return _.decoded_count >= _.expected_count;
    }

    fn bytesConsumed(): usize {
        return _.byte_pos;
    }

    fn status(): HybridRleStatus {
        return _.last_status;
    }

    fn loadNextRun(*_): HybridRleStatus {
        if (_.byte_pos >= _.data.len) {
            _.last_status = HybridRleStatus.truncated_stream;
            return HybridRleStatus.truncated_stream;
        }

        const hdr = readUleb128U32(_.data, _.byte_pos);
        if (!hdr.ok) {
            _.last_status = HybridRleStatus.malformed_varint;
            return HybridRleStatus.malformed_varint;
        }
        _.byte_pos = hdr.next_pos;

        const is_bitpacked: bool = (hdr.value & 1) != 0;
        const count_field: usize = (hdr.value >> 1) as usize;
        if (count_field == 0) {
            _.last_status = HybridRleStatus.zero_run_header;
            return HybridRleStatus.zero_run_header;
        }

        if (!is_bitpacked) {
            const val_bytes: usize = hybridValueByteWidth(_.bit_width);
            if (_.byte_pos + val_bytes > _.data.len) {
                _.last_status = HybridRleStatus.truncated_stream;
                return HybridRleStatus.truncated_stream;
            }
            let val: u32 = 0;
            let b: usize = 0;
            while (b < val_bytes) : (b += 1) {
                const shift: u5 = (b * 8) as u5;
                val |= (_.data[_.byte_pos + b] as u32) << shift;
            }
            if (!valueFitsBitWidth(val, _.bit_width)) {
                _.last_status = HybridRleStatus.value_out_of_range;
                return HybridRleStatus.value_out_of_range;
            }
            _.byte_pos += val_bytes;
            _.in_rle = true;
            _.rle_val = val;
            _.rle_rem = count_field;
            _.bp_groups_rem = 0;
            _.bp_idx = 8;
            return HybridRleStatus.ok;
        }

        // Bit-packed run of `count_field` 8-value groups.
        _.in_rle = false;
        _.rle_rem = 0;
        _.bp_groups_rem = count_field;
        _.bp_idx = 8;
        return HybridRleStatus.ok;
    }

    /// Decodes up to `min(out_vals.len, _.remaining())` values into `out_vals`.
    fn nextBatch(*_, out_vals: *[]u32): HybridRleDecodeResult {
        if (_.last_status != HybridRleStatus.ok) {
            return HybridRleDecodeResult {
                status: _.last_status,
                values_decoded: 0,
                bytes_consumed: _.byte_pos,
            };
        }

        const rem: usize = _.remaining();
        const target: usize = if (out_vals.len < rem) out_vals.len else rem;
        let written: usize = 0;

        while (written < target) {
            if (_.in_rle && _.rle_rem > 0) {
                const take: usize = if (target - written < _.rle_rem) (target - written) else _.rle_rem;
                let k: usize = 0;
                while (k < take) : (k += 1) {
                    out_vals[written + k] = _.rle_val;
                }
                written += take;
                _.rle_rem -= take;
                _.decoded_count += take;
                continue;
            }

            if (_.bp_idx < 8) {
                const avail: usize = (8 as usize) - _.bp_idx;
                const take: usize = if (target - written < avail) (target - written) else avail;
                let k: usize = 0;
                while (k < take) : (k += 1) {
                    out_vals[written + k] = _.bp_buf[_.bp_idx + k];
                }
                written += take;
                _.bp_idx += take;
                _.decoded_count += take;
                continue;
            }

            if (_.bp_groups_rem > 0) {
                if (next_p := unpackGroup8Lsb(_.data, _.byte_pos, _.bit_width, *_.bp_buf)) {
                    _.byte_pos = next_p;
                    _.bp_groups_rem -= 1;
                    _.bp_idx = 0;
                    continue;
                }
                _.last_status = HybridRleStatus.truncated_stream;
                return HybridRleDecodeResult {
                    status: HybridRleStatus.truncated_stream,
                    values_decoded: written,
                    bytes_consumed: _.byte_pos,
                };
            }

            // Need next run header from stream.
            const st = _.loadNextRun();
            if (st != HybridRleStatus.ok) {
                return HybridRleDecodeResult {
                    status: st,
                    values_decoded: written,
                    bytes_consumed: _.byte_pos,
                };
            }
        }

        return HybridRleDecodeResult {
            status: HybridRleStatus.ok,
            values_decoded: written,
            bytes_consumed: _.byte_pos,
        };
    }
}

/// One-shot decodes `expected_count` values from a Parquet Hybrid RLE + Bit-Packed byte slice
/// into `out_vals`. Zero heap allocation.
pub fn decodeHybridRleU32(
    data: []const u8,
    bit_width: u8,
    expected_count: usize,
    out_vals: *[]u32,
): HybridRleDecodeResult {
    if (bit_width > HYBRID_RLE_MAX_BIT_WIDTH) {
        return HybridRleDecodeResult { status: HybridRleStatus.invalid_bit_width };
    }
    if (expected_count == 0) {
        return HybridRleDecodeResult { status: HybridRleStatus.ok };
    }
    if (out_vals.len < expected_count) {
        return HybridRleDecodeResult { status: HybridRleStatus.buffer_too_small };
    }

    let decoder = HybridRleDecoder.init(data, bit_width, expected_count) ?? return HybridRleDecodeResult {
        status: HybridRleStatus.invalid_bit_width,
    };
    return decoder.nextBatch(out_vals);
}

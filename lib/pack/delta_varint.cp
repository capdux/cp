// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc ZigZag Varint, Delta & Delta-of-Delta Codec) | Created: 2026-11-22
// Pure CP zero-heap-allocation Integer Sequence Compression Codecs for Inverted Indexes (Lucene/BM25 DocIDs),
// Time-Series Databases (Gorilla/Prometheus Timestamps), Columnar Storage (Parquet/Arrow) & WALs:
//   - Overflow-free full-range ZigZag encoding/decoding (`i64 <-> u64`, including `i64::MIN` / `i64::MAX`)
//   - Strict ULEB128 64-bit unsigned & signed varints with 10-byte / 0x01 overflow & truncation guards
//   - Monotonic `u64` Delta codec (`encodeDeltaU64` / `decodeDeltaU64`) rejecting non-monotonic inputs
//   - Periodic time-series `i64` Delta-of-Delta codec (`encodeDeltaOfDeltaI64` / `decodeDeltaOfDeltaI64`)
//   - Compression telemetry helper (`computeDeltaCompressionStats`)

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum encoded byte length of a 64-bit ULEB128 varint (`ceil(64 / 7) = 10`).
pub const VARINT_MAX_U64_BYTES: usize = 10;

/// Result of decoding an unsigned 64-bit ULEB128 varint.
pub struct VarintDecodeResult {
    ok: bool = false;
    value: u64 = 0;
    bytes_read: usize = 0;
}

/// Result of decoding a signed ZigZag-encoded 64-bit varint.
pub struct SignedVarintDecodeResult {
    ok: bool = false;
    value: i64 = 0;
    bytes_read: usize = 0;
}

/// Compression statistics summary for a delta-encoded 64-bit integer block.
pub struct DeltaBlockStats {
    count: usize = 0;
    encoded_bytes: usize = 0;
    raw_bytes: usize = 0;
    compression_ratio_x100: u32 = 0;
}

// ============================================================================
// 2. ZigZag & Varint Primitives (100% Zero-Alloc)
// ============================================================================

/// Maps any signed 64-bit integer (including `i64::MIN` and `i64::MAX`) bijectively onto `u64`
/// such that small magnitudes encode to small unsigned values (`0->0, -1->1, 1->2, -2->3, ...`).
pub fn zigzagEncodeI64(v: i64): u64 {
    const bits: u64 = @bitCast(v);
    const sign_mask: u64 = if (v < 0) 0xFFFFFFFFFFFFFFFF else 0;
    return (bits << 1) ^ sign_mask;
}

/// Exact inverse of `zigzagEncodeI64`, mapping any `u64` back to `i64` without overflow.
pub fn zigzagDecodeU64(z: u64): i64 {
    const sign_mask: u64 = if ((z & 1) != 0) 0xFFFFFFFFFFFFFFFF else 0;
    const bits: u64 = (z >> 1) ^ sign_mask;
    return @bitCast(bits);
}

/// Encodes `v: u64` as ULEB128 into `out_buf`, returning bytes written (`1..=10`),
/// or `null` if `out_buf` does not have sufficient capacity.
pub fn encodeUvarint64(v: u64, out_buf: *[]u8): usize? {
    let rem: u64 = v;
    let written: usize = 0;

    while (true) {
        if (written >= out_buf.len) return null;
        if (rem >= 0x80) {
            out_buf[written] = @intCast((rem & 0x7F) | 0x80);
            rem = rem >> 7;
            written += 1;
        } else {
            out_buf[written] = @intCast(rem);
            written += 1;
            return written;
        }
    }
}

/// Decodes an unsigned 64-bit ULEB128 varint from `data`.
/// Strictly rejects empty/truncated streams, streams exceeding 10 bytes, and 10th-byte values `> 0x01`.
pub fn decodeUvarint64(data: []const u8): VarintDecodeResult {
    if (data.len == 0) {
        return VarintDecodeResult{ ok: false, value: 0, bytes_read: 0 };
    }

    let result: u64 = 0;
    const limit: usize = if (data.len < VARINT_MAX_U64_BYTES) data.len else VARINT_MAX_U64_BYTES;

    let i: usize = 0;
    while (i < limit) : (i += 1) {
        const b: u8 = data[i];
        if (i == 9) {
            // 10th byte may only carry bit 63 (0x00 or 0x01) with continuation bit cleared
            if (b > 0x01) {
                return VarintDecodeResult{ ok: false, value: 0, bytes_read: 0 };
            }
            result |= ((b as u64) << 63);
            return VarintDecodeResult{
                ok: true,
                value: result,
                bytes_read: 10,
            };
        }

        const shift: u6 = @intCast(i * 7);
        result |= (((b & 0x7F) as u64) << shift);
        if ((b & 0x80) == 0) {
            return VarintDecodeResult{
                ok: true,
                value: result,
                bytes_read: i + 1,
            };
        }
    }

    return VarintDecodeResult{ ok: false, value: 0, bytes_read: 0 };
}

/// Encodes `v: i64` as a ZigZag ULEB128 signed varint into `out_buf`.
pub fn encodeSvarint64(v: i64, out_buf: *[]u8): usize? {
    return encodeUvarint64(zigzagEncodeI64(v), out_buf);
}

/// Decodes a ZigZag ULEB128 signed 64-bit varint from `data`.
pub fn decodeSvarint64(data: []const u8): SignedVarintDecodeResult {
    const u = decodeUvarint64(data);
    if (!u.ok) {
        return SignedVarintDecodeResult{ ok: false, value: 0, bytes_read: 0 };
    }
    return SignedVarintDecodeResult{
        ok: true,
        value: zigzagDecodeU64(u.value),
        bytes_read: u.bytes_read,
    };
}

// ============================================================================
// 3. Monotonic Delta (DocIDs) & Time-Series Delta-of-Delta Codecs
// ============================================================================

/// Encodes a monotonically non-decreasing `u64` sequence (`sorted_vals[i] >= sorted_vals[i - 1]`)
/// as `[count][v0][v1 - v0]...[v_{n-1} - v_{n-2}]` using ULEB128 varints.
/// Returns `null` if `sorted_vals` is non-monotonic or `out_buf` is too small.
pub fn encodeDeltaU64(sorted_vals: []const u64, out_buf: *[]u8): usize? {
    // Pre-flight monotonicity check
    let chk: usize = 1;
    while (chk < sorted_vals.len) : (chk += 1) {
        if (sorted_vals[chk] < sorted_vals[chk - 1]) return null;
    }

    let pos: usize = 0;
    let tail0: []u8 = out_buf[pos..];
    const n_cnt = encodeUvarint64(sorted_vals.len as u64, tail0) ?? return null;
    pos += n_cnt;

    if (sorted_vals.len == 0) {
        return pos;
    }

    let tail1: []u8 = out_buf[pos..];
    const n_first = encodeUvarint64(sorted_vals[0], tail1) ?? return null;
    pos += n_first;

    let i: usize = 1;
    while (i < sorted_vals.len) : (i += 1) {
        const delta: u64 = sorted_vals[i] - sorted_vals[i - 1];
        let tail_i: []u8 = out_buf[pos..];
        const n_i = encodeUvarint64(delta, tail_i) ?? return null;
        pos += n_i;
    }

    return pos;
}

/// Decodes a `u64` delta-encoded block produced by `encodeDeltaU64` into `out_vals`.
/// Returns the decoded element count, or `null` on malformed/truncated data, `u64` overflow,
/// or insufficient `out_vals` capacity.
pub fn decodeDeltaU64(data: []const u8, out_vals: *[]u64): usize? {
    let pos: usize = 0;
    const c_res = decodeUvarint64(data[pos..]);
    if (!c_res.ok) return null;
    pos += c_res.bytes_read;

    if (c_res.value > (out_vals.len as u64)) return null;
    const count: usize = @intCast(c_res.value);
    if (count == 0) return 0;

    const first_res = decodeUvarint64(data[pos..]);
    if (!first_res.ok) return null;
    pos += first_res.bytes_read;

    let prev: u64 = first_res.value;
    out_vals[0] = prev;

    let i: usize = 1;
    while (i < count) : (i += 1) {
        const d_res = decodeUvarint64(data[pos..]);
        if (!d_res.ok) return null;
        pos += d_res.bytes_read;

        if (prev > 0xFFFFFFFFFFFFFFFF - d_res.value) return null;
        prev += d_res.value;
        out_vals[i] = prev;
    }

    return count;
}

/// Encodes an `i64` time-series sequence using Delta-of-Delta + ZigZag Varint compression:
/// `[count][t0][t1 -% t0][(t2 -% t1) -% (t1 -% t0)]...`.
/// Achieves near-1-byte/sample compression on periodic timestamps with micro-jitter.
/// Returns bytes written or `null` if `out_buf` is too small.
pub fn encodeDeltaOfDeltaI64(timestamps: []const i64, out_buf: *[]u8): usize? {
    let pos: usize = 0;
    let tail0: []u8 = out_buf[pos..];
    const n_cnt = encodeUvarint64(timestamps.len as u64, tail0) ?? return null;
    pos += n_cnt;

    if (timestamps.len == 0) {
        return pos;
    }

    let tail1: []u8 = out_buf[pos..];
    const n_t0 = encodeSvarint64(timestamps[0], tail1) ?? return null;
    pos += n_t0;

    if (timestamps.len == 1) {
        return pos;
    }

    let prev_delta: i64 = timestamps[1] -% timestamps[0];
    let tail2: []u8 = out_buf[pos..];
    const n_d1 = encodeSvarint64(prev_delta, tail2) ?? return null;
    pos += n_d1;

    let i: usize = 2;
    while (i < timestamps.len) : (i += 1) {
        const cur_delta: i64 = timestamps[i] -% timestamps[i - 1];
        const dod: i64 = cur_delta -% prev_delta;
        let tail_i: []u8 = out_buf[pos..];
        const n_i = encodeSvarint64(dod, tail_i) ?? return null;
        pos += n_i;
        prev_delta = cur_delta;
    }

    return pos;
}

/// Decodes a Delta-of-Delta block produced by `encodeDeltaOfDeltaI64` into `out_vals`.
/// Returns the decoded element count, or `null` on malformed/truncated data or insufficient `out_vals`.
pub fn decodeDeltaOfDeltaI64(data: []const u8, out_vals: *[]i64): usize? {
    let pos: usize = 0;
    const c_res = decodeUvarint64(data[pos..]);
    if (!c_res.ok) return null;
    pos += c_res.bytes_read;

    if (c_res.value > (out_vals.len as u64)) return null;
    const count: usize = @intCast(c_res.value);
    if (count == 0) return 0;

    const t0_res = decodeSvarint64(data[pos..]);
    if (!t0_res.ok) return null;
    pos += t0_res.bytes_read;
    out_vals[0] = t0_res.value;

    if (count == 1) return 1;

    const d1_res = decodeSvarint64(data[pos..]);
    if (!d1_res.ok) return null;
    pos += d1_res.bytes_read;

    let prev_delta: i64 = d1_res.value;
    let prev_val: i64 = t0_res.value +% prev_delta;
    out_vals[1] = prev_val;

    let i: usize = 2;
    while (i < count) : (i += 1) {
        const dod_res = decodeSvarint64(data[pos..]);
        if (!dod_res.ok) return null;
        pos += dod_res.bytes_read;

        prev_delta = prev_delta +% dod_res.value;
        prev_val = prev_val +% prev_delta;
        out_vals[i] = prev_val;
    }

    return count;
}

/// Computes raw vs encoded byte footprint and `compression_ratio_x100` (`raw * 100 / encoded`)
/// for a block of `count` 64-bit integers.
pub fn computeDeltaCompressionStats(count: usize, encoded_bytes: usize): DeltaBlockStats {
    const raw_bytes: usize = count * 8;
    let ratio_x100: u32 = 0;
    if (encoded_bytes > 0) {
        const r_u64: u64 = ((raw_bytes as u64) * 100) / (encoded_bytes as u64);
        ratio_x100 = if (r_u64 > 0xFFFFFFFF) 0xFFFFFFFF else @intCast(r_u64);
    }
    return DeltaBlockStats{
        count: count,
        encoded_bytes: encoded_bytes,
        raw_bytes: raw_bytes,
        compression_ratio_x100: ratio_x100,
    };
}

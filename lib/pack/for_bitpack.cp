// Origin: Pure-CP Stdlib (Zero-Alloc Frame-of-Reference & Delta-FOR BitPacking Codec) | Created: 2026-11-22
// Pure CP zero-heap-allocation bitpacking, Frame-of-Reference (FOR), and Sorted Delta-FOR block codec
// for columnar storage, inverted DocID posting lists, and vector ID blocks:
//   - Native `@clz` minimum bit-width computation (`bitWidthU32`) supporting full 0..32-bit range
//   - Tight LSB-first bitpacking (`packBitsU32` / `unpackBitsU32`) + O(1) random-access `unpackSingleU32`
//   - Frame-of-Reference (`encodeForBlockU32` / `decodeForBlockU32`) with 0-bit constant block fast path
//   - Sorted Delta-FOR (`encodeDeltaForBlockU32` / `decodeDeltaForBlockU32`) with monotonic enforcement
//     and 14-byte skip-pointer `ForBlockHeader` enabling O(1) range pruning (`blockMayContain`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed encoded header length in bytes for FOR & Delta-FOR blocks.
/// Layout (big-endian): `mode(1) | bit_width(1) | count(4) | base_val(4) | max_val(4)`.
pub const FOR_BLOCK_HEADER_LEN: usize = 14;

/// Maximum supported bit width for `u32` bitpacking.
pub const FOR_MAX_BIT_WIDTH: u8 = 32;

/// Encoding mode stored in `ForBlockHeader`.
pub enum ForBlockMode {
    for_plain,
    delta_for_sorted,
}

/// Operation status returned by bitpack & FOR block codec functions.
pub enum ForPackStatus {
    ok,
    invalid_bit_width,
    value_exceeds_bit_width,
    buffer_too_small,
    truncated_input,
    invalid_header,
    not_monotonic,
    index_out_of_bounds,
}

/// Skip-pointer metadata header for an encoded FOR or Delta-FOR block.
pub struct ForBlockHeader {
    mode: ForBlockMode = ForBlockMode.for_plain;
    bit_width: u8 = 0;
    count: u32 = 0;
    base_val: u32 = 0;
    max_val: u32 = 0;

    /// Returns the packed payload size in bytes following the 14-byte header.
    fn payloadBytes(): usize {
        if (_.count == 0 || _.bit_width == 0) {
            return 0;
        }
        if (_.mode == ForBlockMode.delta_for_sorted) {
            if (_.count <= 1) {
                return 0;
            }
            return packedByteLen((_.count as usize) - 1, _.bit_width);
        }
        return packedByteLen(_.count as usize, _.bit_width);
    }

    /// Returns the total encoded block size (header + packed payload) in bytes.
    fn totalEncodedBytes(): usize {
        return FOR_BLOCK_HEADER_LEN + _.payloadBytes();
    }
}

/// Result returned by `packBitsU32`, `encodeForBlockU32`, and `encodeDeltaForBlockU32`.
pub struct ForPackResult {
    status: ForPackStatus = ForPackStatus.ok;
    bytes_written: usize = 0;
    bit_width: u8 = 0;
    header: ForBlockHeader = ForBlockHeader{};
}

/// Result returned by `unpackBitsU32`, `decodeForBlockU32`, `decodeDeltaForBlockU32`, and `decodeForBlockHeader`.
pub struct ForDecodeResult {
    status: ForPackStatus = ForPackStatus.ok;
    elements_decoded: usize = 0;
    bytes_consumed: usize = 0;
    header: ForBlockHeader = ForBlockHeader{};
}

/// Result returned by O(1) random-access `unpackSingleU32`.
pub struct ForSingleResult {
    status: ForPackStatus = ForPackStatus.ok;
    value: u32 = 0;
}

// ============================================================================
// 2. Native `@clz` Bit-Width & Low-Level Helpers
// ============================================================================

/// Computes the minimum bit width (`0..=32`) required to represent `max_val` using native `@clz`.
pub fn bitWidthU32(max_val: u32): u8 {
    if (max_val == 0) {
        return 0;
    }
    const lz: u32 = @clz(max_val) as u32;
    return (32 - lz) as u8;
}

/// Computes the exact number of bytes required to bitpack `count` values at `bit_width` bits each.
pub fn packedByteLen(count: usize, bit_width: u8): usize {
    if (count == 0 || bit_width == 0) {
        return 0;
    }
    const total_bits: u64 = (count as u64) * (bit_width as u64);
    return ((total_bits + 7) / 8) as usize;
}

/// O(1) skip-pointer range pruning check: returns `true` iff `target` lies within `[base_val, max_val]`.
pub fn blockMayContain(header: ForBlockHeader, target: u32): bool {
    return header.count > 0 && target >= header.base_val && target <= header.max_val;
}

fn writeBeU32(dst: *[]u8, off: usize, v: u32) {
    dst[off] = ((v >> 24) & 0xFF) as u8;
    dst[off + 1] = ((v >> 16) & 0xFF) as u8;
    dst[off + 2] = ((v >> 8) & 0xFF) as u8;
    dst[off + 3] = (v & 0xFF) as u8;
}

fn readBeU32(src: []const u8, off: usize): u32 {
    return ((src[off] as u32) << 24) |
        ((src[off + 1] as u32) << 16) |
        ((src[off + 2] as u32) << 8) |
        (src[off + 3] as u32);
}

fn writeHeader(hdr: ForBlockHeader, out_buf: *[]u8) {
    out_buf[0] = if (hdr.mode == ForBlockMode.delta_for_sorted) 1 else 0;
    out_buf[1] = hdr.bit_width;
    writeBeU32(out_buf, 2, hdr.count);
    writeBeU32(out_buf, 6, hdr.base_val);
    writeBeU32(out_buf, 10, hdr.max_val);
}

/// Decodes and validates the 14-byte `ForBlockHeader` prefix without unpacking the payload.
pub fn decodeForBlockHeader(encoded: []const u8): ForDecodeResult {
    if (encoded.len < FOR_BLOCK_HEADER_LEN) {
        return ForDecodeResult{ status: ForPackStatus.truncated_input };
    }
    const raw_mode: u8 = encoded[0];
    if (raw_mode > 1) {
        return ForDecodeResult{ status: ForPackStatus.invalid_header };
    }
    const bw: u8 = encoded[1];
    if (bw > FOR_MAX_BIT_WIDTH) {
        return ForDecodeResult{ status: ForPackStatus.invalid_bit_width };
    }
    const count: u32 = readBeU32(encoded, 2);
    const base_val: u32 = readBeU32(encoded, 6);
    const max_val: u32 = readBeU32(encoded, 10);
    if (count > 0 && max_val < base_val) {
        return ForDecodeResult{ status: ForPackStatus.invalid_header };
    }

    const hdr = ForBlockHeader{
        mode: if (raw_mode == 1) ForBlockMode.delta_for_sorted else ForBlockMode.for_plain,
        bit_width: bw,
        count: count,
        base_val: base_val,
        max_val: max_val,
    };

    return ForDecodeResult{
        status: ForPackStatus.ok,
        elements_decoded: 0,
        bytes_consumed: FOR_BLOCK_HEADER_LEN,
        header: hdr,
    };
}

// ============================================================================
// 3. Raw BitPacking Codec (`packBitsU32` / `unpackBitsU32` / `unpackSingleU32`)
// ============================================================================

/// Packs `values` tightly at `bit_width` (`0..=32`) bits per value in LSB-first byte order.
pub fn packBitsU32(
    values: []const u32,
    bit_width: u8,
    out_buf: *[]u8,
): ForPackResult {
    if (bit_width > FOR_MAX_BIT_WIDTH) {
        return ForPackResult{ status: ForPackStatus.invalid_bit_width };
    }
    const need: usize = packedByteLen(values.len, bit_width);
    if (out_buf.len < need) {
        return ForPackResult{ status: ForPackStatus.buffer_too_small };
    }

    if (bit_width == 0) {
        let i: usize = 0;
        while (i < values.len) : (i += 1) {
            if (values[i] != 0) {
                return ForPackResult{ status: ForPackStatus.value_exceeds_bit_width };
            }
        }
        return ForPackResult{
            status: ForPackStatus.ok,
            bytes_written: 0,
            bit_width: 0,
        };
    }

    const bw_u32: u32 = bit_width as u32;
    let acc: u64 = 0;
    let acc_bits: u32 = 0;
    let out_pos: usize = 0;

    let i: usize = 0;
    while (i < values.len) : (i += 1) {
        const v: u32 = values[i];
        if (bit_width < 32) {
            if ((v >> (bit_width as u5)) != 0) {
                return ForPackResult{ status: ForPackStatus.value_exceeds_bit_width };
            }
        }
        acc |= (v as u64) << (acc_bits as u6);
        acc_bits += bw_u32;
        while (acc_bits >= 8) {
            out_buf[out_pos] = (acc & 0xFF) as u8;
            out_pos += 1;
            acc >>= 8;
            acc_bits -= 8;
        }
    }

    if (acc_bits > 0) {
        out_buf[out_pos] = (acc & 0xFF) as u8;
        out_pos += 1;
    }

    return ForPackResult{
        status: ForPackStatus.ok,
        bytes_written: out_pos,
        bit_width: bit_width,
    };
}

/// Unpacks `count` tightly packed `bit_width`-bit (`0..=32`) unsigned integers into `out_values`.
pub fn unpackBitsU32(
    packed_bytes: []const u8,
    count: usize,
    bit_width: u8,
    out_values: *[]u32,
): ForDecodeResult {
    if (bit_width > FOR_MAX_BIT_WIDTH) {
        return ForDecodeResult{ status: ForPackStatus.invalid_bit_width };
    }
    if (out_values.len < count) {
        return ForDecodeResult{ status: ForPackStatus.buffer_too_small };
    }
    const need: usize = packedByteLen(count, bit_width);
    if (packed_bytes.len < need) {
        return ForDecodeResult{ status: ForPackStatus.truncated_input };
    }

    if (bit_width == 0) {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out_values[i] = 0;
        }
        return ForDecodeResult{
            status: ForPackStatus.ok,
            elements_decoded: count,
            bytes_consumed: 0,
        };
    }

    const bw_u32: u32 = bit_width as u32;
    const mask: u64 = if (bit_width == 32) 0xFFFFFFFF else (((1 as u64) << (bit_width as u6)) - 1);
    let acc: u64 = 0;
    let acc_bits: u32 = 0;
    let in_pos: usize = 0;

    let i: usize = 0;
    while (i < count) : (i += 1) {
        while (acc_bits < bw_u32) {
            acc |= (packed_bytes[in_pos] as u64) << (acc_bits as u6);
            in_pos += 1;
            acc_bits += 8;
        }
        out_values[i] = (acc & mask) as u32;
        acc >>= (bit_width as u6);
        acc_bits -= bw_u32;
    }

    return ForDecodeResult{
        status: ForPackStatus.ok,
        elements_decoded: count,
        bytes_consumed: need,
    };
}

/// O(1) random-access single-element decoder at element index `idx` from a `bit_width`-packed buffer.
pub fn unpackSingleU32(
    packed_bytes: []const u8,
    bit_width: u8,
    idx: usize,
): ForSingleResult {
    if (bit_width > FOR_MAX_BIT_WIDTH) {
        return ForSingleResult{ status: ForPackStatus.invalid_bit_width };
    }
    if (bit_width == 0) {
        return ForSingleResult{ status: ForPackStatus.ok, value: 0 };
    }

    const bit_pos: u64 = (idx as u64) * (bit_width as u64);
    const byte_idx: usize = (bit_pos / 8) as usize;
    const bit_off: u6 = (bit_pos % 8) as u6;
    const bytes_needed: usize = (((bit_off as usize) + (bit_width as usize) + 7) / 8);

    if (byte_idx > packed_bytes.len || bytes_needed > packed_bytes.len - byte_idx) {
        return ForSingleResult{ status: ForPackStatus.truncated_input };
    }

    let raw: u64 = 0;
    let k: usize = 0;
    while (k < bytes_needed) : (k += 1) {
        raw |= (packed_bytes[byte_idx + k] as u64) << ((k * 8) as u6);
    }

    const shifted: u64 = raw >> bit_off;
    const mask: u64 = if (bit_width == 32) 0xFFFFFFFF else (((1 as u64) << (bit_width as u6)) - 1);
    return ForSingleResult{
        status: ForPackStatus.ok,
        value: (shifted & mask) as u32,
    };
}

// ============================================================================
// 4. Frame-of-Reference (FOR) Block Codec
// ============================================================================

/// Encodes `values` as a Frame-of-Reference block: extracts `min_val` and `max_val`, computes
/// minimal `bit_width = bitWidthU32(max_val - min_val)` via `@clz`, and bitpacks `v - min_val`.
/// Constant blocks (`min_val == max_val`) use `bit_width == 0` and emit only the 14-byte header.
pub fn encodeForBlockU32(
    values: []const u32,
    out_buf: *[]u8,
): ForPackResult {
    if (values.len == 0) {
        if (out_buf.len < FOR_BLOCK_HEADER_LEN) {
            return ForPackResult{ status: ForPackStatus.buffer_too_small };
        }
        const empty_hdr = ForBlockHeader{
            mode: ForBlockMode.for_plain,
            bit_width: 0,
            count: 0,
            base_val: 0,
            max_val: 0,
        };
        writeHeader(empty_hdr, out_buf);
        return ForPackResult{
            status: ForPackStatus.ok,
            bytes_written: FOR_BLOCK_HEADER_LEN,
            bit_width: 0,
            header: empty_hdr,
        };
    }

    let min_v: u32 = values[0];
    let max_v: u32 = values[0];
    let i: usize = 1;
    while (i < values.len) : (i += 1) {
        if (values[i] < min_v) min_v = values[i];
        if (values[i] > max_v) max_v = values[i];
    }

    const span: u32 = max_v - min_v;
    const bw: u8 = bitWidthU32(span);
    const hdr = ForBlockHeader{
        mode: ForBlockMode.for_plain,
        bit_width: bw,
        count: values.len as u32,
        base_val: min_v,
        max_val: max_v,
    };

    const total_need: usize = hdr.totalEncodedBytes();
    if (out_buf.len < total_need) {
        return ForPackResult{ status: ForPackStatus.buffer_too_small };
    }

    writeHeader(hdr, out_buf);
    if (bw == 0) {
        return ForPackResult{
            status: ForPackStatus.ok,
            bytes_written: FOR_BLOCK_HEADER_LEN,
            bit_width: 0,
            header: hdr,
        };
    }

    const bw_u32: u32 = bw as u32;
    let acc: u64 = 0;
    let acc_bits: u32 = 0;
    let out_pos: usize = FOR_BLOCK_HEADER_LEN;

    let j: usize = 0;
    while (j < values.len) : (j += 1) {
        const offset_v: u32 = values[j] - min_v;
        acc |= (offset_v as u64) << (acc_bits as u6);
        acc_bits += bw_u32;
        while (acc_bits >= 8) {
            out_buf[out_pos] = (acc & 0xFF) as u8;
            out_pos += 1;
            acc >>= 8;
            acc_bits -= 8;
        }
    }
    if (acc_bits > 0) {
        out_buf[out_pos] = (acc & 0xFF) as u8;
        out_pos += 1;
    }

    return ForPackResult{
        status: ForPackStatus.ok,
        bytes_written: out_pos,
        bit_width: bw,
        header: hdr,
    };
}

/// Decodes a Frame-of-Reference block encoded by `encodeForBlockU32` into `out_values`.
pub fn decodeForBlockU32(
    encoded: []const u8,
    out_values: *[]u32,
): ForDecodeResult {
    const hres = decodeForBlockHeader(encoded);
    if (hres.status != ForPackStatus.ok) {
        return hres;
    }
    const hdr = hres.header;
    if (hdr.mode != ForBlockMode.for_plain) {
        return ForDecodeResult{ status: ForPackStatus.invalid_header };
    }

    const count: usize = hdr.count as usize;
    const total_bytes: usize = hdr.totalEncodedBytes();
    if (encoded.len < total_bytes) {
        return ForDecodeResult{ status: ForPackStatus.truncated_input };
    }
    if (out_values.len < count) {
        return ForDecodeResult{ status: ForPackStatus.buffer_too_small };
    }

    if (count == 0) {
        return ForDecodeResult{
            status: ForPackStatus.ok,
            elements_decoded: 0,
            bytes_consumed: FOR_BLOCK_HEADER_LEN,
            header: hdr,
        };
    }

    if (hdr.bit_width == 0) {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out_values[i] = hdr.base_val;
        }
        return ForDecodeResult{
            status: ForPackStatus.ok,
            elements_decoded: count,
            bytes_consumed: FOR_BLOCK_HEADER_LEN,
            header: hdr,
        };
    }

    const payload = encoded[FOR_BLOCK_HEADER_LEN..total_bytes];
    const ures = unpackBitsU32(payload, count, hdr.bit_width, out_values);
    if (ures.status != ForPackStatus.ok) {
        return ures;
    }

    let k: usize = 0;
    while (k < count) : (k += 1) {
        out_values[k] = out_values[k] +% hdr.base_val;
    }

    return ForDecodeResult{
        status: ForPackStatus.ok,
        elements_decoded: count,
        bytes_consumed: total_bytes,
        header: hdr,
    };
}

// ============================================================================
// 5. Sorted Delta-FOR Block Codec (Inverted DocID Lists)
// ============================================================================

/// Encodes a monotonically non-decreasing `sorted_values` sequence as a Delta-FOR block.
/// Stores `first_val` (`base_val`) and `last_val` (`max_val`) in the skip-pointer header and
/// bitpacks the `count - 1` adjacent deltas `sorted_values[i] - sorted_values[i - 1]`.
/// Rejects non-monotonic inputs with `ForPackStatus.not_monotonic`.
pub fn encodeDeltaForBlockU32(
    sorted_values: []const u32,
    out_buf: *[]u8,
): ForPackResult {
    if (sorted_values.len == 0) {
        if (out_buf.len < FOR_BLOCK_HEADER_LEN) {
            return ForPackResult{ status: ForPackStatus.buffer_too_small };
        }
        const empty_hdr = ForBlockHeader{
            mode: ForBlockMode.delta_for_sorted,
            bit_width: 0,
            count: 0,
            base_val: 0,
            max_val: 0,
        };
        writeHeader(empty_hdr, out_buf);
        return ForPackResult{
            status: ForPackStatus.ok,
            bytes_written: FOR_BLOCK_HEADER_LEN,
            bit_width: 0,
            header: empty_hdr,
        };
    }

    let max_delta: u32 = 0;
    let i: usize = 1;
    while (i < sorted_values.len) : (i += 1) {
        if (sorted_values[i] < sorted_values[i - 1]) {
            return ForPackResult{ status: ForPackStatus.not_monotonic };
        }
        const d: u32 = sorted_values[i] - sorted_values[i - 1];
        if (d > max_delta) {
            max_delta = d;
        }
    }

    const bw: u8 = if (sorted_values.len <= 1) 0 else bitWidthU32(max_delta);
    const hdr = ForBlockHeader{
        mode: ForBlockMode.delta_for_sorted,
        bit_width: bw,
        count: sorted_values.len as u32,
        base_val: sorted_values[0],
        max_val: sorted_values[sorted_values.len - 1],
    };

    const total_need: usize = hdr.totalEncodedBytes();
    if (out_buf.len < total_need) {
        return ForPackResult{ status: ForPackStatus.buffer_too_small };
    }

    writeHeader(hdr, out_buf);
    if (sorted_values.len <= 1 || bw == 0) {
        return ForPackResult{
            status: ForPackStatus.ok,
            bytes_written: FOR_BLOCK_HEADER_LEN,
            bit_width: 0,
            header: hdr,
        };
    }

    const bw_u32: u32 = bw as u32;
    let acc: u64 = 0;
    let acc_bits: u32 = 0;
    let out_pos: usize = FOR_BLOCK_HEADER_LEN;

    let j: usize = 1;
    while (j < sorted_values.len) : (j += 1) {
        const d: u32 = sorted_values[j] - sorted_values[j - 1];
        acc |= (d as u64) << (acc_bits as u6);
        acc_bits += bw_u32;
        while (acc_bits >= 8) {
            out_buf[out_pos] = (acc & 0xFF) as u8;
            out_pos += 1;
            acc >>= 8;
            acc_bits -= 8;
        }
    }
    if (acc_bits > 0) {
        out_buf[out_pos] = (acc & 0xFF) as u8;
        out_pos += 1;
    }

    return ForPackResult{
        status: ForPackStatus.ok,
        bytes_written: out_pos,
        bit_width: bw,
        header: hdr,
    };
}

/// Decodes a Sorted Delta-FOR block encoded by `encodeDeltaForBlockU32` into `out_values`.
pub fn decodeDeltaForBlockU32(
    encoded: []const u8,
    out_values: *[]u32,
): ForDecodeResult {
    const hres = decodeForBlockHeader(encoded);
    if (hres.status != ForPackStatus.ok) {
        return hres;
    }
    const hdr = hres.header;
    if (hdr.mode != ForBlockMode.delta_for_sorted) {
        return ForDecodeResult{ status: ForPackStatus.invalid_header };
    }

    const count: usize = hdr.count as usize;
    const total_bytes: usize = hdr.totalEncodedBytes();
    if (encoded.len < total_bytes) {
        return ForDecodeResult{ status: ForPackStatus.truncated_input };
    }
    if (out_values.len < count) {
        return ForDecodeResult{ status: ForPackStatus.buffer_too_small };
    }

    if (count == 0) {
        return ForDecodeResult{
            status: ForPackStatus.ok,
            elements_decoded: 0,
            bytes_consumed: FOR_BLOCK_HEADER_LEN,
            header: hdr,
        };
    }

    out_values[0] = hdr.base_val;
    if (count == 1 || hdr.bit_width == 0) {
        let i: usize = 1;
        while (i < count) : (i += 1) {
            out_values[i] = hdr.base_val;
        }
        return ForDecodeResult{
            status: ForPackStatus.ok,
            elements_decoded: count,
            bytes_consumed: FOR_BLOCK_HEADER_LEN,
            header: hdr,
        };
    }

    const bw: u8 = hdr.bit_width;
    const bw_u32: u32 = bw as u32;
    const mask: u64 = if (bw == 32) 0xFFFFFFFF else (((1 as u64) << (bw as u6)) - 1);
    let acc: u64 = 0;
    let acc_bits: u32 = 0;
    let in_pos: usize = FOR_BLOCK_HEADER_LEN;
    let cur: u32 = hdr.base_val;

    let i: usize = 1;
    while (i < count) : (i += 1) {
        while (acc_bits < bw_u32) {
            acc |= (encoded[in_pos] as u64) << (acc_bits as u6);
            in_pos += 1;
            acc_bits += 8;
        }
        const delta: u32 = (acc & mask) as u32;
        acc >>= (bw as u6);
        acc_bits -= bw_u32;
        cur = cur +% delta;
        out_values[i] = cur;
    }

    return ForDecodeResult{
        status: ForPackStatus.ok,
        elements_decoded: count,
        bytes_consumed: total_bytes,
        header: hdr,
    };
}

// @cap/cobs — Zero-allocation Consistent Overhead Byte Stuffing (COBS) codec for CP
// Clean-room MIT implementation inspired by Rust `cobs` and Cheshire & Baker (1999).

type CobsVec16 = @Vector(16, u8);

pub const COBS_DECODER_CAP: usize = 512;

fn cobsFindZero(buf: []const u8, start: usize, limit: usize): usize {
    let i: usize = start;
    const v_zero: CobsVec16 = @splat(0 as u8);
    while (i + 16 <= limit) {
        const v: CobsVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_zero)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == 0) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < limit) {
        if (buf[i] == 0) {
            return i;
        }
        i += 1;
    }
    return limit;
}

// ---------------------------------------------------------------------------
// 1. Core COBS Encoding & Decoding
// ---------------------------------------------------------------------------

/// Computes the maximum COBS encoded length (without trailing sentinel) for `source_len` bytes.
pub fn max_encoded_len(source_len: usize) -> usize {
    if (source_len == 0) {
        return 1;
    }
    return source_len + (source_len / 254) + 1;
}

/// Encodes `src` using standard COBS (eliminating all `0x00` bytes, no trailing sentinel) into `dst`.
/// Returns number of bytes written, or `0` if `dst` is too small.
pub fn encode(src: []const u8, dst: []u8) -> usize {
    if (dst.len == 0) {
        return 0;
    }
    if (src.len == 0) {
        dst[0] = 0x01;
        return 1;
    }

    let write_idx: usize = 0;
    let i: usize = 0;

    while (i < src.len) {
        const rem = src.len - i;
        const max_run = if (rem < 254) rem else 254;
        const zero_pos = cobsFindZero(src, i, i + max_run);
        const run_len = zero_pos - i;
        if (write_idx + 1 + run_len > dst.len) {
            return 0;
        }
        dst[write_idx] = (run_len + 1) as u8;
        write_idx += 1;
        if (run_len > 0) {
            @memcpy(dst[write_idx .. write_idx + run_len], src[i .. i + run_len]);
            write_idx += run_len;
        }
        i = zero_pos;
        if (i < src.len && src[i] == 0 && run_len < 254) {
            // Consumed a zero byte; if it was the last byte of src, emit trailing 0x01 code
            i += 1;
            if (i == src.len) {
                if (write_idx >= dst.len) {
                    return 0;
                }
                dst[write_idx] = 0x01;
                write_idx += 1;
            }
        }
    }

    return write_idx;
}

/// Decodes a standard COBS buffer `src` (without trailing `0x00`) into `dst`.
/// Returns decoded byte count, or `-1` on invalid `0x00` byte, truncated block, or small `dst`.
pub fn decode(src: []const u8, dst: []u8) -> i64 {
    return decode_with_sentinel(src, dst, 0 as u8);
}

// ---------------------------------------------------------------------------
// 2. Custom Sentinel (COBS/XOR)
// ---------------------------------------------------------------------------

/// Encodes `src` into `dst` eliminating `sentinel` via COBS + XOR `sentinel`.
/// Returns bytes written, or `0` if `dst` is too small.
pub fn encode_with_sentinel(src: []const u8, dst: []u8, sentinel: u8) -> usize {
    const n = encode(src, dst);
    if (n == 0 || sentinel == 0) {
        return n;
    }
    let i: usize = 0;
    const v_sent: CobsVec16 = @splat(sentinel);
    while (i + 16 <= n) {
        const v: CobsVec16 = dst[i .. i + 16][0..16].*;
        const x: [16]u8 = v ^ v_sent;
        @memcpy(dst[i .. i + 16], x[0..16]);
        i += 16;
    }
    while (i < n) {
        dst[i] = dst[i] ^ sentinel;
        i += 1;
    }
    return n;
}

/// Decodes a COBS/XOR buffer `src` with custom `sentinel` into `dst` (zero temporary buffers).
/// Returns decoded byte count, or `-1` on malformed input or buffer overflow.
pub fn decode_with_sentinel(src: []const u8, dst: []u8, sentinel: u8) -> i64 {
    if (src.len == 0) {
        return 0;
    }
    let read_idx: usize = 0;
    let write_idx: usize = 0;
    const v_zero: CobsVec16 = @splat(0 as u8);
    const v_sent: CobsVec16 = @splat(sentinel);

    while (read_idx < src.len) {
        const code = src[read_idx] ^ sentinel;
        read_idx += 1;
        if (code == 0) {
            return -1;
        }
        const block_len = (code - 1) as usize;
        if (read_idx + block_len > src.len) {
            return -1;
        }
        if (write_idx + block_len > dst.len) {
            return -1;
        }
        if (sentinel == 0) {
            if (cobsFindZero(src, read_idx, read_idx + block_len) < read_idx + block_len) {
                return -1;
            }
            if (block_len > 0) {
                @memcpy(dst[write_idx .. write_idx + block_len], src[read_idx .. read_idx + block_len]);
                read_idx += block_len;
                write_idx += block_len;
            }
        } else {
            let k: usize = 0;
            while (k + 16 <= block_len) {
                const v: CobsVec16 = src[read_idx .. read_idx + 16][0..16].*;
                const dec_vec: CobsVec16 = v ^ v_sent;
                if (@reduce(.Or, dec_vec == v_zero)) {
                    return -1;
                }
                const arr: [16]u8 = dec_vec;
                @memcpy(dst[write_idx .. write_idx + 16], arr[0..16]);
                read_idx += 16;
                write_idx += 16;
                k += 16;
            }
            while (k < block_len) {
                const b = src[read_idx] ^ sentinel;
                if (b == 0) {
                    return -1;
                }
                dst[write_idx] = b;
                read_idx += 1;
                write_idx += 1;
                k += 1;
            }
        }
        if (code < 0xFF && read_idx < src.len) {
            if (write_idx >= dst.len) {
                return -1;
            }
            dst[write_idx] = 0;
            write_idx += 1;
        }
    }

    return write_idx as i64;
}

// ---------------------------------------------------------------------------
// 3. Framed COBS (with trailing 0x00 delimiter)
// ---------------------------------------------------------------------------

/// Encodes `src` with COBS and appends a trailing `0x00` frame delimiter into `dst`.
/// Returns total bytes written (including delimiter), or `0` if `dst` is too small.
pub fn encode_framed(src: []const u8, dst: []u8) -> usize {
    if (dst.len < 2) {
        return 0;
    }
    const n = encode(src, dst[0..(dst.len - 1)]);
    if (n == 0) {
        return 0;
    }
    dst[n] = 0x00;
    return n + 1;
}

/// Decodes a framed COBS buffer stopping at the first `0x00` delimiter.
/// Returns decoded byte count, or `-1` on malformed frame or small `dst`.
pub fn decode_framed(src: []const u8, dst: []u8) -> i64 {
    const end = cobsFindZero(src, 0, src.len);
    return decode(src[0..end], dst);
}

// ---------------------------------------------------------------------------
// 4. Streaming Incremental CobsDecoder (Fixed 512-byte Frame Buffer)
// ---------------------------------------------------------------------------

pub struct CobsDecoder {
    pub buf: [512]u8,
    pub write_len: usize,
    pub block_rem: u8,
    pub cur_code: u8,
    pub has_data: bool,
    pub failed: bool,
    pub frame_ready: bool,

    pub static fn init() -> CobsDecoder {
        let buf: [512]u8 = undefined;
        return CobsDecoder{
            buf: buf,
            write_len: 0,
            block_rem: 0,
            cur_code: 0,
            has_data: false,
            failed: false,
            frame_ready: false,
        };
    }

    pub fn reset(*_) {
        _.write_len = 0;
        _.block_rem = 0;
        _.cur_code = 0;
        _.has_data = false;
        _.failed = false;
        _.frame_ready = false;
    }

    /// Returns the most recently completed decoded frame slice.
    pub fn frame() -> []const u8 {
        return _.buf[0.._.write_len];
    }

    /// Feeds one byte from a `0x00`-delimited COBS stream.
    /// Returns:
    /// - `0`: need more bytes (or empty delimiter skipped)
    /// - `> 0`: complete decoded frame length (call `frame()` immediately to read it)
    /// - `-1`: framing error (truncated block or buffer overflow)
    pub fn push(*_, byte: u8) -> i64 {
        if (byte == 0x00) {
            if (_.failed || _.block_rem > 0) {
                _.reset();
                return -1;
            }
            if (!_.has_data) {
                // Empty frame / consecutive delimiter
                _.reset();
                return 0;
            }
            const completed_len = _.write_len as i64;
            _.block_rem = 0;
            _.cur_code = 0;
            _.has_data = false;
            _.failed = false;
            _.frame_ready = true;
            return completed_len;
        }

        if (_.frame_ready) {
            _.reset();
        }
        if (_.failed) {
            return -1;
        }

        if (_.block_rem == 0) {
            if (_.has_data && _.cur_code < 0xFF) {
                if (_.write_len >= COBS_DECODER_CAP) {
                    _.failed = true;
                    return -1;
                }
                _.buf[_.write_len] = 0x00;
                _.write_len += 1;
            }
            _.cur_code = byte;
            _.block_rem = byte - 1;
            _.has_data = true;
            return 0;
        }

        if (_.write_len >= COBS_DECODER_CAP) {
            _.failed = true;
            return -1;
        }
        _.buf[_.write_len] = byte;
        _.write_len += 1;
        _.block_rem -= 1;
        return 0;
    }
}

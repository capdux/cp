// @cap/utf-8 — Zero-copy incremental streaming UTF-8 decoder & RFC 3629 validator
// Ported from crates.io:utf-8 (https://github.com/SimonSapin/rust-utf8)
// Accelerated with 16-lane @Vector(16, u8) SIMD ASCII fast-path, comptime 256-entry LUT, and @memcpy.

type U8dVec16 = @Vector(16, u8);

fn u8dBuildLeadLenLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 256) {
        if (i < 0x80) {
            lut[i] = 1;
        } else if (i >= 0xC2 && i <= 0xDF) {
            lut[i] = 2;
        } else if (i >= 0xE0 && i <= 0xEF) {
            lut[i] = 3;
        } else if (i >= 0xF0 && i <= 0xF4) {
            lut[i] = 4;
        }
        i += 1;
    }
    return lut;
}

const U8D_LEAD_LEN_LUT: [256]u8 = u8dBuildLeadLenLut();

pub enum Utf8DecodeStatus {
    Ok,
    InvalidSequence,
    Incomplete,
}

pub struct Utf8ChunkResult {
    status: Utf8DecodeStatus,
    valid_prefix: string,
    consumed: usize,
    error_len: usize,
}

fn u8dExpectedLen(b0: u8): usize {
    return U8D_LEAD_LEN_LUT[b0 as usize] as usize;
}

fn u8dIsValidSecond(b0: u8, b1: u8): bool {
    if (b0 == 0xE0) return b1 >= 0xA0 && b1 <= 0xBF;
    if (b0 == 0xED) return b1 >= 0x80 && b1 <= 0x9F;
    if (b0 == 0xF0) return b1 >= 0x90 && b1 <= 0xBF;
    if (b0 == 0xF4) return b1 >= 0x80 && b1 <= 0x8F;
    return (b1 & 0xC0) == 0x80;
}

fn u8dIsValidCont(b0: u8, idx: usize, b: u8): bool {
    if (idx == 1) return u8dIsValidSecond(b0, b);
    return (b & 0xC0) == 0x80;
}

fn u8dWriteSlice(out: []u8, pos: usize, src: []const u8): usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const copy_len = if (src.len < avail) src.len else avail;
    @memcpy(out[pos .. pos + copy_len], src[0..copy_len]);
    return pos + copy_len;
}

fn u8dWriteReplacement(out: []u8, pos: usize): usize {
    if (pos + 3 <= out.len) {
        out[pos] = 0xEF;
        out[pos + 1] = 0xBF;
        out[pos + 2] = 0xBD;
        return pos + 3;
    }
    return pos;
}

/// Zero-copy RFC 3629 UTF-8 chunk validator & splitter.
/// Distinguishes valid UTF-8, invalid sequences (with Unicode Maximal Subpart error_len),
/// and incomplete trailing multi-byte sequences.
pub fn decodeUtf8Chunk(input: []const u8): Utf8ChunkResult {
    let i: usize = 0;
    const v_80: U8dVec16 = @splat(0x80 as u8);
    while (i < input.len) {
        // 16-lane SIMD fast-path for pure 7-bit ASCII runs
        while (i + 16 <= input.len) {
            const v: U8dVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, v >= v_80)) {
                break;
            }
            i += 16;
        }
        if (i >= input.len) {
            break;
        }
        const b0: u8 = input[i];
        if (b0 < 0x80) {
            i += 1;
            continue;
        }
        if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= input.len) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.Incomplete,
                    valid_prefix: input[0..i],
                    consumed: i,
                    error_len: 1,
                };
            }
            const b1: u8 = input[i + 1];
            if ((b1 & 0xC0) != 0x80) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.InvalidSequence,
                    valid_prefix: input[0..i],
                    consumed: i + 1,
                    error_len: 1,
                };
            }
            i += 2;
            continue;
        }
        if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 1 >= input.len) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.Incomplete,
                    valid_prefix: input[0..i],
                    consumed: i,
                    error_len: 1,
                };
            }
            const b1: u8 = input[i + 1];
            if (!u8dIsValidSecond(b0, b1)) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.InvalidSequence,
                    valid_prefix: input[0..i],
                    consumed: i + 1,
                    error_len: 1,
                };
            }
            if (i + 2 >= input.len) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.Incomplete,
                    valid_prefix: input[0..i],
                    consumed: i,
                    error_len: 2,
                };
            }
            const b2: u8 = input[i + 2];
            if ((b2 & 0xC0) != 0x80) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.InvalidSequence,
                    valid_prefix: input[0..i],
                    consumed: i + 2,
                    error_len: 2,
                };
            }
            i += 3;
            continue;
        }
        if (b0 >= 0xF0 && b0 <= 0xF4) {
            if (i + 1 >= input.len) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.Incomplete,
                    valid_prefix: input[0..i],
                    consumed: i,
                    error_len: 1,
                };
            }
            const b1: u8 = input[i + 1];
            if (!u8dIsValidSecond(b0, b1)) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.InvalidSequence,
                    valid_prefix: input[0..i],
                    consumed: i + 1,
                    error_len: 1,
                };
            }
            if (i + 2 >= input.len) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.Incomplete,
                    valid_prefix: input[0..i],
                    consumed: i,
                    error_len: 2,
                };
            }
            const b2: u8 = input[i + 2];
            if ((b2 & 0xC0) != 0x80) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.InvalidSequence,
                    valid_prefix: input[0..i],
                    consumed: i + 2,
                    error_len: 2,
                };
            }
            if (i + 3 >= input.len) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.Incomplete,
                    valid_prefix: input[0..i],
                    consumed: i,
                    error_len: 3,
                };
            }
            const b3: u8 = input[i + 3];
            if ((b3 & 0xC0) != 0x80) {
                return Utf8ChunkResult{
                    status: Utf8DecodeStatus.InvalidSequence,
                    valid_prefix: input[0..i],
                    consumed: i + 3,
                    error_len: 3,
                };
            }
            i += 4;
            continue;
        }
        // Invalid lead byte (0x80..0xC1 or 0xF5..0xFF)
        return Utf8ChunkResult{
            status: Utf8DecodeStatus.InvalidSequence,
            valid_prefix: input[0..i],
            consumed: i + 1,
            error_len: 1,
        };
    }

    return Utf8ChunkResult{
        status: Utf8DecodeStatus.Ok,
        valid_prefix: input,
        consumed: input.len,
        error_len: 0,
    };
}

/// Fast full-buffer RFC 3629 UTF-8 validity check.
pub fn isValidUtf8(input: []const u8): bool {
    const res = decodeUtf8Chunk(input);
    return res.status == Utf8DecodeStatus.Ok;
}

/// Incremental cross-chunk lossy UTF-8 decoder.
/// Replaces ill-formed subsequences and truncated tails with U+FFFD (EF BF BD)
/// following the Unicode Maximal Subpart rule.
pub struct LossyUtf8Decoder {
    pending: [4]u8,
    pending_len: usize,

    static fn new(): LossyUtf8Decoder {
        const z: u8 = 0;
        return LossyUtf8Decoder{
            pending: [z, z, z, z],
            pending_len: 0,
        };
    }

    fn hasPending(): bool {
        return _.pending_len > 0;
    }

    fn pendingLen(): usize {
        return _.pending_len;
    }

    fn reset(*_) {
        _.pending_len = 0;
    }

    fn push(*_, chunk: []const u8, out: []u8): usize {
        let written: usize = 0;
        let pos: usize = 0;

        // 1. Resolve any incomplete multi-byte prefix carried from the previous chunk
        if (_.pending_len > 0 && pos < chunk.len) {
            const b0: u8 = _.pending[0];
            const exp: usize = u8dExpectedLen(b0);
            while (_.pending_len < exp && pos < chunk.len) {
                const next_b: u8 = chunk[pos];
                if (u8dIsValidCont(b0, _.pending_len, next_b)) {
                    _.pending[_.pending_len] = next_b;
                    _.pending_len += 1;
                    pos += 1;
                } else {
                    // Pending prefix is a maximal ill-formed subpart; emit U+FFFD
                    // and leave chunk[pos] to be decoded normally.
                    written = u8dWriteReplacement(out, written);
                    _.pending_len = 0;
                    break;
                }
            }
            if (_.pending_len == exp && exp > 0) {
                let k: usize = 0;
                while (k < exp && written < out.len) {
                    out[written] = _.pending[k];
                    written += 1;
                    k += 1;
                }
                _.pending_len = 0;
            }
        }

        // 2. Decode remaining bytes in the current chunk
        while (pos < chunk.len) {
            const res = decodeUtf8Chunk(chunk[pos..]);
            if (res.valid_prefix.len > 0) {
                written = u8dWriteSlice(out, written, res.valid_prefix);
            }
            if (res.status == Utf8DecodeStatus.Ok) {
                pos = chunk.len;
                break;
            } else if (res.status == Utf8DecodeStatus.InvalidSequence) {
                written = u8dWriteReplacement(out, written);
                pos += res.consumed;
            } else {
                // Incomplete tail: buffer 1..3 bytes for the next chunk
                pos += res.consumed;
                let k: usize = 0;
                while (pos < chunk.len && k < 4) {
                    _.pending[k] = chunk[pos];
                    k += 1;
                    pos += 1;
                }
                _.pending_len = k;
                break;
            }
        }

        return written;
    }

    fn finish(*_, out: []u8): usize {
        if (_.pending_len > 0) {
            _.pending_len = 0;
            return u8dWriteReplacement(out, 0);
        }
        return 0;
    }
}

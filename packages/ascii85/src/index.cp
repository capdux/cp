// packages/@cap/ascii85/src/index.cp
// Zero-allocation Adobe Ascii85 (PostScript / PDF) encoder and decoder.
// MIT clean-room pure CP implementation with comptime decode LUT & cap:bin big-endian I/O.

import { readU32BE, writeU32BE } from "cap:bin";

const A85_CODE_Z: u8 = 253;
const A85_CODE_SPACE: u8 = 254;
const A85_CODE_INVALID: u8 = 255;

fn buildA85DecLut(): [256]u8 {
    @setEvalBranchQuota(10000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        if (i >= 33 && i <= 117) {
            tbl[i] = (i - 33) as u8;
        } else if (i == 122) {
            tbl[i] = A85_CODE_Z; // 'z'
        } else if (i == 32 || i == 9 || i == 13 || i == 10 || i == 12 || i == 11) {
            tbl[i] = A85_CODE_SPACE; // ASCII whitespace
        } else {
            tbl[i] = A85_CODE_INVALID;
        }
        i += 1;
    }
    return tbl;
}

const A85_DEC_LUT: [256]u8 = buildA85DecLut();

fn a85IsSpace(c: u8): bool {
    return A85_DEC_LUT[c as usize] == A85_CODE_SPACE;
}

/// Compute the worst-case encoded length for `byte_len` input bytes (with optional `<~...~>` delimiters).
pub fn maxEncodedLen(byte_len: usize, with_delimiters: bool): usize {
    const fullBlocks = byte_len / 4;
    const rem = byte_len % 4;
    let total = fullBlocks * 5;
    if (rem > 0) {
        total += rem + 1;
    }
    if (with_delimiters) {
        total += 4;
    }
    return total;
}

/// Encode raw bytes into Adobe Ascii85 (`!`..`u`) in `out`.
/// - Compresses aligned 4-byte all-zero blocks into `'z'`.
/// - Pads and truncates final 1..3 byte blocks per Adobe Ascii85 spec.
/// - Optionally wraps output in `<~` and `~>`.
/// Returns bytes written, or `0` if `out` is too small (or `data` is empty without delimiters).
pub fn encode(data: []const u8, with_delimiters: bool, out: []u8): usize {
    let pos: usize = 0;

    if (with_delimiters) {
        if (out.len < 4) {
            return 0;
        }
        out[0] = '<' as u8;
        out[1] = '~' as u8;
        pos = 2;
    }

    let i: usize = 0;
    let digits: [5]u8 = undefined;

    while (i + 4 <= data.len) {
        const val: u32 = readU32BE(data, i) ?? (0 as u32);
        if (val == (0 as u32)) {
            if (pos + 1 > out.len) {
                return 0;
            }
            out[pos] = 'z' as u8;
            pos += 1;
        } else {
            if (pos + 5 > out.len) {
                return 0;
            }
            let cur = val;
            out[pos + 4] = (33 as u8) + ((cur % (85 as u32)) as u8);
            cur = cur / (85 as u32);
            out[pos + 3] = (33 as u8) + ((cur % (85 as u32)) as u8);
            cur = cur / (85 as u32);
            out[pos + 2] = (33 as u8) + ((cur % (85 as u32)) as u8);
            cur = cur / (85 as u32);
            out[pos + 1] = (33 as u8) + ((cur % (85 as u32)) as u8);
            cur = cur / (85 as u32);
            out[pos] = (33 as u8) + (cur as u8);
            pos += 5;
        }
        i += 4;
    }

    if (i < data.len) {
        const rem = data.len - i;
        // Final partial block (1..3 bytes): zero-pad on right, emit rem + 1 chars.
        let val: u32 = (data[i] as u32) << (24 as u5);
        if (rem >= 2) {
            val = val | ((data[i + 1] as u32) << (16 as u5));
        }
        if (rem == 3) {
            val = val | ((data[i + 2] as u32) << (8 as u5));
        }

        const emitCount = rem + 1;
        if (pos + emitCount > out.len) {
            return 0;
        }
        let cur = val;
        digits[4] = (33 as u8) + ((cur % (85 as u32)) as u8);
        cur = cur / (85 as u32);
        digits[3] = (33 as u8) + ((cur % (85 as u32)) as u8);
        cur = cur / (85 as u32);
        digits[2] = (33 as u8) + ((cur % (85 as u32)) as u8);
        cur = cur / (85 as u32);
        digits[1] = (33 as u8) + ((cur % (85 as u32)) as u8);
        cur = cur / (85 as u32);
        digits[0] = (33 as u8) + (cur as u8);
        let j: usize = 0;
        while (j < emitCount) {
            out[pos] = digits[j];
            pos += 1;
            j += 1;
        }
    }

    if (with_delimiters) {
        if (pos + 2 > out.len) {
            return 0;
        }
        out[pos] = '~' as u8;
        out[pos + 1] = '>' as u8;
        pos += 2;
    }

    return pos;
}

/// Decode Adobe Ascii85 text into raw bytes in `out`.
/// - Strips optional `<~...~>` delimiters and ignores ASCII whitespace.
/// - Expands `'z'` to 4 zero bytes (rejects `'z'` inside a partial 5-char group).
/// - Rejects `u32` overflow, invalid characters, single-char trailing groups, or insufficient `out`.
/// Returns bytes written, or `-1` on error.
pub fn decode(input: string, out: []u8): i64 {
    let start: usize = 0;
    let end: usize = input.len;

    while (start < end && a85IsSpace(input[start])) {
        start += 1;
    }
    while (end > start && a85IsSpace(input[end - 1])) {
        end -= 1;
    }

    let hasOpen = false;
    if (end - start >= 2 && input[start] == ('<' as u8) && input[start + 1] == ('~' as u8)) {
        start += 2;
        hasOpen = true;
    }
    let hasClose = false;
    if (end - start >= 2 && input[end - 2] == ('~' as u8) && input[end - 1] == ('>' as u8)) {
        end -= 2;
        hasClose = true;
    }
    if (hasOpen && !hasClose) {
        return -1 as i64;
    }

    const maxU32: u64 = 0xFFFFFFFF as u64;
    let pos: usize = 0;
    let acc: u64 = 0 as u64;
    let cnt: usize = 0;

    let i = start;
    while (i < end) {
        const code = A85_DEC_LUT[input[i] as usize];
        i += 1;

        if (code < (85 as u8)) {
            acc = acc * (85 as u64) + (code as u64);
            cnt += 1;
            if (cnt == 5) {
                if (acc > maxU32) {
                    return -1 as i64;
                }
                if (!writeU32BE(out, pos, acc as u32)) {
                    return -1 as i64;
                }
                pos += 4;
                acc = 0 as u64;
                cnt = 0;
            }
            continue;
        }

        if (code == A85_CODE_SPACE) {
            continue;
        }

        if (code == A85_CODE_Z) {
            if (cnt != 0) {
                return -1 as i64;
            }
            if (!writeU32BE(out, pos, 0 as u32)) {
                return -1 as i64;
            }
            pos += 4;
            continue;
        }

        return -1 as i64;
    }

    if (cnt == 1) {
        return -1 as i64;
    }
    if (cnt > 1) {
        let pad = cnt;
        while (pad < 5) {
            acc = acc * (85 as u64) + (84 as u64);
            pad += 1;
        }
        if (acc > maxU32) {
            return -1 as i64;
        }
        const emitBytes = cnt - 1;
        if (pos + emitBytes > out.len) {
            return -1 as i64;
        }
        const v = acc as u32;
        if (emitBytes >= 1) {
            out[pos] = ((v >> (24 as u5)) & (0xFF as u32)) as u8;
            pos += 1;
        }
        if (emitBytes >= 2) {
            out[pos] = ((v >> (16 as u5)) & (0xFF as u32)) as u8;
            pos += 1;
        }
        if (emitBytes >= 3) {
            out[pos] = ((v >> (8 as u5)) & (0xFF as u32)) as u8;
            pos += 1;
        }
    }

    return pos as i64;
}

export {
    encode,
    decode,
    maxEncodedLen,
};

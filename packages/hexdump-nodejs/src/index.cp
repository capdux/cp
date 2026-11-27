// @cap/hex-dump — Zero-allocation Canonical `hexdump -C` Binary Formatter & Diff Locator
// Clean-room MIT implementation in pure CP with cap:fmt + 16-lane SIMD block acceleration.

import { writeHexByte, writeStr } from "cap:fmt";

type HdVec16 = @Vector(16, u8);

fn buildPrintableLut(): [256]u8 {
    @setEvalBranchQuota(10000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        if (i >= 32 && i <= 126) {
            tbl[i] = i as u8;
        } else {
            tbl[i] = 46 as u8; // '.'
        }
        i += 1;
    }
    return tbl;
}

const HD_PRINTABLE_LUT: [256]u8 = buildPrintableLut();

/// Formats up to 16 bytes of `data` as a canonical `hexdump -C` line:
/// `00000000  48 65 6c 6c 6f 20 57 6f  72 6c 64 21 0a           |Hello World!.|`
/// Returns bytes written (`62 + min(data.len, 16)`), or `0` if `data` is empty or `out` is too small.
pub fn formatLine(data: []const u8, base_offset: usize, out: []u8) -> usize {
    if (data.len == 0) {
        return 0;
    }
    let count = data.len;
    if (count > 16) {
        count = 16;
    }
    const needed = count + 62;
    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;

    // 8-digit lowercase hex offset via 4x writeHexByte
    w = writeHexByte(((base_offset >> 24) & 0xFF) as u8, false, out, w);
    w = writeHexByte(((base_offset >> 16) & 0xFF) as u8, false, out, w);
    w = writeHexByte(((base_offset >> 8) & 0xFF) as u8, false, out, w);
    w = writeHexByte((base_offset & 0xFF) as u8, false, out, w);

    // Two spaces after offset
    w = writeStr("  ", out, w);

    // Hex byte columns: fast path for full 16-byte line (branchless)
    if (count == 16) {
        let i: usize = 0;
        while (i < 8) {
            w = writeHexByte(data[i], false, out, w);
            out[w] = 32;
            w += 1;
            i += 1;
        }
        out[w] = 32;
        w += 1;
        while (i < 16) {
            w = writeHexByte(data[i], false, out, w);
            out[w] = 32;
            w += 1;
            i += 1;
        }
    } else {
        let i: usize = 0;
        while (i < 16) {
            if (i == 8) {
                out[w] = 32;
                w += 1;
            }
            if (i < count) {
                w = writeHexByte(data[i], false, out, w);
                out[w] = 32;
                w += 1;
            } else {
                w = writeStr("   ", out, w);
            }
            i += 1;
        }
    }

    // Space + '|' + ASCII preview + '|'
    w = writeStr(" |", out, w);

    let k: usize = 0;
    while (k < count) {
        out[w] = HD_PRINTABLE_LUT[data[k] as usize];
        w += 1;
        k += 1;
    }

    out[w] = 124; // '|'
    w += 1;

    return w;
}

/// Formats `data` as a multi-line `hexdump -C` dump (16 bytes per line, `\n` separated).
/// Returns total bytes written, or `0` if `data` is empty or `out` is too small.
pub fn formatDump(data: []const u8, out: []u8) -> usize {
    if (data.len == 0) {
        return 0;
    }
    let off: usize = 0;
    let w: usize = 0;
    while (off < data.len) {
        if (off > 0) {
            if (w >= out.len) {
                return 0;
            }
            out[w] = 10; // '\n'
            w += 1;
        }
        let end = off + 16;
        if (end > data.len) {
            end = data.len;
        }
        const n = formatLine(data[off..end], off, out[w..out.len]);
        if (n == 0) {
            return 0;
        }
        w += n;
        off = end;
    }
    return w;
}

/// Encodes `data` as space-separated hex bytes (e.g. `"de ad be ef"` or `"DE AD BE EF"`).
/// Returns bytes written (`data.len * 3 - 1`), or `0` if `data` is empty or `out` is too small.
pub fn encodeHexSpaced(data: []const u8, uppercase: bool, out: []u8) -> usize {
    if (data.len == 0) {
        return 0;
    }
    const needed = data.len * 3 - 1;
    if (needed > out.len) {
        return 0;
    }
    let w: usize = writeHexByte(data[0], uppercase, out, 0);
    let i: usize = 1;
    while (i < data.len) {
        out[w] = 32; // ' '
        w = writeHexByte(data[i], uppercase, out, w + 1);
        i += 1;
    }
    return w;
}

/// Returns the byte offset of the first difference between `a` and `b`,
/// or `-1` if both slices are completely identical.
pub fn firstDiffOffset(a: []const u8, b: []const u8) -> i64 {
    let min_len = a.len;
    if (b.len < min_len) {
        min_len = b.len;
    }
    let i: usize = 0;
    // 16-lane SIMD fast-path for identical blocks
    while (i + 16 <= min_len) {
        const va: HdVec16 = a[i .. i + 16][0..16].*;
        const vb: HdVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            break;
        }
        i += 16;
    }
    while (i < min_len) {
        if (a[i] != b[i]) {
            return i as i64;
        }
        i += 1;
    }
    if (a.len != b.len) {
        return min_len as i64;
    }
    return -1;
}

// @cap/hex-dump — Zero-allocation Canonical `hexdump -C` Binary Formatter & Diff Locator
// Clean-room MIT implementation in pure CP.

fn hexDigit(nibble: u8, uppercase: bool) -> u8 {
    if (nibble < 10) {
        return 48 + nibble;
    }
    if (uppercase) {
        return 55 + nibble; // 'A' = 65
    }
    return 87 + nibble; // 'a' = 97
}

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

    // 8-digit lowercase hex offset
    let shift: i32 = 28;
    while (shift >= 0) {
        const nib = ((base_offset >> (shift as u6)) & 0x0F) as u8;
        out[w] = hexDigit(nib, false);
        w += 1;
        shift -= 4;
    }

    // Two spaces after offset
    out[w] = 32;
    out[w + 1] = 32;
    w += 2;

    // First 8 bytes + extra space + second 8 bytes
    let i: usize = 0;
    while (i < 16) {
        if (i == 8) {
            out[w] = 32;
            w += 1;
        }
        if (i < count) {
            const b = data[i];
            out[w] = hexDigit((b >> 4) & 0x0F, false);
            out[w + 1] = hexDigit(b & 0x0F, false);
            out[w + 2] = 32;
        } else {
            out[w] = 32;
            out[w + 1] = 32;
            out[w + 2] = 32;
        }
        w += 3;
        i += 1;
    }

    // Space + '|' + ASCII preview + '|'
    out[w] = 32;
    out[w + 1] = 124; // '|'
    w += 2;

    i = 0;
    while (i < count) {
        const b = data[i];
        if (b >= 32 && b <= 126) {
            out[w] = b;
        } else {
            out[w] = 46; // '.'
        }
        w += 1;
        i += 1;
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
    let w: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        if (i > 0) {
            out[w] = 32; // ' '
            w += 1;
        }
        const b = data[i];
        out[w] = hexDigit((b >> 4) & 0x0F, uppercase);
        out[w + 1] = hexDigit(b & 0x0F, uppercase);
        w += 2;
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

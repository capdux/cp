// @cap/utf16-iter — Zero-allocation bidirectional UTF-16 code point iterator
// Inspired by crates.io:utf16_iter (https://github.com/hsivonen/utf16_iter)

pub const UTF16_REPLACEMENT_CHAR: u32 = 0xFFFD;

fn u16iIsHighSurrogate(unit: u16): bool {
    return unit >= 0xD800 && unit <= 0xDBFF;
}

fn u16iIsLowSurrogate(unit: u16): bool {
    return unit >= 0xDC00 && unit <= 0xDFFF;
}

fn u16iCombineSurrogates(high: u16, low: u16): u32 {
    const hi: u32 = u32(high - 0xD800);
    const lo: u32 = u32(low - 0xDC00);
    return 0x10000 + ((hi << 10) | lo);
}

fn u16iEncodeUtf8(cp: u32, out: []u8, offset: usize): usize {
    if (cp <= 0x7F) {
        if (offset + 1 > out.len) {
            return 0;
        }
        out[offset] = u8(cp);
        return 1;
    }
    if (cp <= 0x7FF) {
        if (offset + 2 > out.len) {
            return 0;
        }
        out[offset] = u8(0xC0 | (cp >> 6));
        out[offset + 1] = u8(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp <= 0xFFFF) {
        if (offset + 3 > out.len) {
            return 0;
        }
        out[offset] = u8(0xE0 | (cp >> 12));
        out[offset + 1] = u8(0x80 | ((cp >> 6) & 0x3F));
        out[offset + 2] = u8(0x80 | (cp & 0x3F));
        return 3;
    }
    if (offset + 4 > out.len) {
        return 0;
    }
    out[offset] = u8(0xF0 | (cp >> 18));
    out[offset + 1] = u8(0x80 | ((cp >> 12) & 0x3F));
    out[offset + 2] = u8(0x80 | ((cp >> 6) & 0x3F));
    out[offset + 3] = u8(0x80 | (cp & 0x3F));
    return 4;
}

/// Bidirectional iterator yielding Unicode scalar values (`u32`) from a UTF-16 slice.
/// Unpaired surrogates yield U+FFFD (`0xFFFD`).
pub struct Utf16Chars {
    slice: []const u16,
    pos: usize,
    end: usize,

    static fn new(slice: []const u16): Utf16Chars {
        return Utf16Chars{
            slice: slice,
            pos: 0,
            end: slice.len,
        };
    }

    fn isEmpty(): bool {
        return _.pos >= _.end;
    }

    fn asSlice(): []const u16 {
        return _.slice[_.pos.._.end];
    }

    fn next(*_): u32? {
        if (_.pos >= _.end) {
            return null;
        }
        const first: u16 = _.slice[_.pos];
        _.pos += 1;

        if (u16iIsHighSurrogate(first)) {
            if (_.pos < _.end) {
                const second: u16 = _.slice[_.pos];
                if (u16iIsLowSurrogate(second)) {
                    _.pos += 1;
                    return u16iCombineSurrogates(first, second);
                }
            }
            return UTF16_REPLACEMENT_CHAR;
        }
        if (u16iIsLowSurrogate(first)) {
            return UTF16_REPLACEMENT_CHAR;
        }
        return u32(first);
    }

    fn nextBack(*_): u32? {
        if (_.pos >= _.end) {
            return null;
        }
        _.end -= 1;
        const last: u16 = _.slice[_.end];

        if (u16iIsLowSurrogate(last)) {
            if (_.end > _.pos) {
                const prev: u16 = _.slice[_.end - 1];
                if (u16iIsHighSurrogate(prev)) {
                    _.end -= 1;
                    return u16iCombineSurrogates(prev, last);
                }
            }
            return UTF16_REPLACEMENT_CHAR;
        }
        if (u16iIsHighSurrogate(last)) {
            return UTF16_REPLACEMENT_CHAR;
        }
        return u32(last);
    }
}

/// Zero-heap Latin-1 (`[]const u8`) to UTF-16 (`u16`) bidirectional iterator & expander.
pub struct Utf16Latin1 {
    bytes: []const u8,
    pos: usize,
    end: usize,

    static fn new(bytes: []const u8): Utf16Latin1 {
        return Utf16Latin1{
            bytes: bytes,
            pos: 0,
            end: bytes.len,
        };
    }

    fn isEmpty(): bool {
        return _.pos >= _.end;
    }

    fn asSlice(): []const u8 {
        return _.bytes[_.pos.._.end];
    }

    fn next(*_): u16? {
        if (_.pos >= _.end) {
            return null;
        }
        const b: u8 = _.bytes[_.pos];
        _.pos += 1;
        return u16(b);
    }

    fn nextBack(*_): u16? {
        if (_.pos >= _.end) {
            return null;
        }
        _.end -= 1;
        return u16(_.bytes[_.end]);
    }

    /// Expands remaining Latin-1 bytes as zero-extended UTF-16 code units into `out`.
    /// Returns the number of `u16` code units written.
    fn writeTo(out: []u16): usize {
        const rem: usize = _.end - _.pos;
        const n: usize = if (rem < out.len) rem else out.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = u16(_.bytes[_.pos + i]);
        }
        return n;
    }

    /// Directly expands `bytes` (Latin-1) into `out` as UTF-16 code units.
    /// Returns the number of `u16` code units written.
    static fn encode(bytes: []const u8, out: []u16): usize {
        const n: usize = if (bytes.len < out.len) bytes.len else out.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = u16(bytes[i]);
        }
        return n;
    }
}

/// Counts the number of Unicode scalar values decoded from a UTF-16 slice.
pub fn countUtf16Chars(slice: []const u16): usize {
    let it: Utf16Chars = Utf16Chars.new(slice);
    let count: usize = 0;
    while (cp := it.next()) {
        * = cp;
        count += 1;
    }
    return count;
}

/// Transcodes a UTF-16 slice into UTF-8 bytes in `out`.
/// Stops cleanly if `out` has insufficient remaining capacity for the next scalar value.
/// Returns total UTF-8 bytes written.
pub fn utf16ToUtf8(slice: []const u16, out: []u8): usize {
    let it: Utf16Chars = Utf16Chars.new(slice);
    let written: usize = 0;
    while (cp := it.next()) {
        const n: usize = u16iEncodeUtf8(cp, out, written);
        if (n == 0) {
            break;
        }
        written += n;
    }
    return written;
}

// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Unicode Braille U+2800..U+28FF Codec, Grade-1 Converter & 2x4 Sub-Pixel Canvas)

/// Unicode Braille Patterns block base code point (`U+2800`).
pub const BRAILLE_BASE_CP: u32 = 0x2800;

/// 8-dot Unicode Braille bit masks (ISO/IEC 10646).
pub const DOT1: u8 = 0x01; // (0, 0) top-left
pub const DOT2: u8 = 0x02; // (0, 1) upper-mid-left
pub const DOT3: u8 = 0x04; // (0, 2) lower-mid-left
pub const DOT4: u8 = 0x08; // (1, 0) top-right
pub const DOT5: u8 = 0x10; // (1, 1) upper-mid-right
pub const DOT6: u8 = 0x20; // (1, 2) lower-mid-right
pub const DOT7: u8 = 0x40; // (0, 3) bottom-left
pub const DOT8: u8 = 0x80; // (1, 3) bottom-right

/// Grade-1 Braille number indicator (`⠼`, dots 3-4-5-6).
pub const BRAILLE_NUMBER_SIGN: u8 = 0x3C;

/// Grade-1 Braille capital letter indicator (`⠠`, dot 6).
pub const BRAILLE_CAPITAL_SIGN: u8 = 0x20;

/// Maps a 2x4 sub-pixel cell coordinate (`x in 0..1`, `y in 0..3`) to its 8-dot bit mask.
/// Returns `null` if `(x, y)` is outside the 2x4 cell.
pub fn dotBitForPixel(x: u8, y: u8): u8? {
    if (x == 0) {
        if (y == 0) { return DOT1; }
        if (y == 1) { return DOT2; }
        if (y == 2) { return DOT3; }
        if (y == 3) { return DOT7; }
        return null;
    }
    if (x == 1) {
        if (y == 0) { return DOT4; }
        if (y == 1) { return DOT5; }
        if (y == 2) { return DOT6; }
        if (y == 3) { return DOT8; }
        return null;
    }
    return null;
}

/// Encodes an 8-dot Braille bit mask (`0x00..0xFF`) into its 3-byte UTF-8 sequence for `U+2800 + mask`.
pub fn brailleToUtf8(mask: u8): [3]u8 {
    return [
        0xE2 as u8,
        (0xA0 | (mask >> 6)) as u8,
        (0x80 | (mask & 0x3F)) as u8,
    ];
}

/// Decodes a 3-byte UTF-8 Braille Pattern (`U+2800..U+28FF`) back into its 8-dot bit mask.
/// Returns `null` if `bytes.len != 3` or if `bytes` is not valid UTF-8 in `U+2800..U+28FF`.
pub fn utf8ToBraille(bytes: []const u8): u8? {
    if (bytes.len != 3) {
        return null;
    }
    if (bytes[0] != 0xE2) {
        return null;
    }
    if (bytes[1] < 0xA0 || bytes[1] > 0xA3) {
        return null;
    }
    if (bytes[2] < 0x80 || bytes[2] > 0xBF) {
        return null;
    }
    const hi = (bytes[1] - 0xA0) << 6;
    const lo = bytes[2] & 0x3F;
    return hi | lo;
}

/// Encodes a slice of 8-dot Braille masks into UTF-8 bytes in `dst`.
/// Returns total bytes written (`masks.len * 3`), or `null` if `dst` is too small.
pub fn encodeBrailleMasks(masks: []const u8, dst: []u8): usize? {
    if (dst.len / 3 < masks.len) {
        return null;
    }
    let out: usize = 0;
    let i: usize = 0;
    while (i < masks.len) {
        const enc = brailleToUtf8(masks[i]);
        dst[out] = enc[0];
        dst[out + 1] = enc[1];
        dst[out + 2] = enc[2];
        out += 3;
        i += 1;
    }
    return out;
}

/// Decodes a UTF-8 stream of Unicode Braille characters (`U+2800..U+28FF`) into 8-dot masks in `dst`.
/// Returns number of masks written, or `null` on invalid UTF-8 / non-Braille bytes / `dst` overflow.
pub fn decodeBrailleUtf8(utf8: []const u8, dst: []u8): usize? {
    if ((utf8.len % 3) != 0) {
        return null;
    }
    const count = utf8.len / 3;
    if (dst.len < count) {
        return null;
    }
    let i: usize = 0;
    let out: usize = 0;
    while (i < utf8.len) {
        const mask = utf8ToBraille(utf8[i..i + 3]) ?? return null;
        dst[out] = mask;
        out += 1;
        i += 3;
    }
    return out;
}

fn letterMask(lower: u8): u8? {
    if (lower == 0x61) { return 0x01; } // a: 1
    if (lower == 0x62) { return 0x03; } // b: 1,2
    if (lower == 0x63) { return 0x09; } // c: 1,4
    if (lower == 0x64) { return 0x19; } // d: 1,4,5
    if (lower == 0x65) { return 0x11; } // e: 1,5
    if (lower == 0x66) { return 0x0B; } // f: 1,2,4
    if (lower == 0x67) { return 0x1B; } // g: 1,2,4,5
    if (lower == 0x68) { return 0x13; } // h: 1,2,5
    if (lower == 0x69) { return 0x0A; } // i: 2,4
    if (lower == 0x6A) { return 0x1A; } // j: 2,4,5
    if (lower == 0x6B) { return 0x05; } // k: 1,3
    if (lower == 0x6C) { return 0x07; } // l: 1,2,3
    if (lower == 0x6D) { return 0x0D; } // m: 1,3,4
    if (lower == 0x6E) { return 0x1D; } // n: 1,3,4,5
    if (lower == 0x6F) { return 0x15; } // o: 1,3,5
    if (lower == 0x70) { return 0x0F; } // p: 1,2,3,4
    if (lower == 0x71) { return 0x1F; } // q: 1,2,3,4,5
    if (lower == 0x72) { return 0x17; } // r: 1,2,3,5
    if (lower == 0x73) { return 0x0E; } // s: 2,3,4
    if (lower == 0x74) { return 0x1E; } // t: 2,3,4,5
    if (lower == 0x75) { return 0x25; } // u: 1,3,6
    if (lower == 0x76) { return 0x27; } // v: 1,2,3,6
    if (lower == 0x77) { return 0x3A; } // w: 2,4,5,6
    if (lower == 0x78) { return 0x2D; } // x: 1,3,4,6
    if (lower == 0x79) { return 0x3D; } // y: 1,3,4,5,6
    if (lower == 0x7A) { return 0x35; } // z: 1,3,5,6
    return null;
}

fn digitMask(ch: u8): u8? {
    if (ch == 0x30) { return 0x1A; } // 0 -> j
    if (ch >= 0x31 && ch <= 0x39) {
        return letterMask(0x61 + (ch - 0x31)); // 1..9 -> a..i
    }
    return null;
}

/// Converts a single ASCII character to its Grade-1 Braille 6-dot mask.
/// Supports `a-z`, `A-Z`, `0-9`, space (`0x00`), and common punctuation `. , ; : ? ! ' -`.
/// Returns `null` for unsupported characters.
pub fn asciiToBrailleMask(ch: u8): u8? {
    if (ch == 0x20) { return 0x00; } // ' '
    if (ch >= 0x61 && ch <= 0x7A) {
        return letterMask(ch);
    }
    if (ch >= 0x41 && ch <= 0x5A) {
        return letterMask(ch + 0x20);
    }
    if (ch >= 0x30 && ch <= 0x39) {
        return digitMask(ch);
    }
    if (ch == 0x2E) { return 0x32; } // '.' dots 2,5,6
    if (ch == 0x2C) { return 0x02; } // ',' dot 2
    if (ch == 0x3B) { return 0x06; } // ';' dots 2,3
    if (ch == 0x3A) { return 0x12; } // ':' dots 2,5
    if (ch == 0x3F) { return 0x26; } // '?' dots 2,3,6
    if (ch == 0x21) { return 0x16; } // '!' dots 2,3,5
    if (ch == 0x27) { return 0x04; } // '\'' dot 3
    if (ch == 0x2D) { return 0x24; } // '-' dots 3,6
    return null;
}

fn writeBrailleCell(dst: []u8, pos: usize, mask: u8): usize? {
    if (dst.len - pos < 3) {
        return null;
    }
    const enc = brailleToUtf8(mask);
    dst[pos] = enc[0];
    dst[pos + 1] = enc[1];
    dst[pos + 2] = enc[2];
    return pos + 3;
}

/// Converts ASCII text into Grade-1 English Braille UTF-8 in `dst`.
/// Automatically inserts `BRAILLE_NUMBER_SIGN` (`⠼`) before digit runs and
/// `BRAILLE_CAPITAL_SIGN` (`⠠`) before uppercase letters.
/// Returns total UTF-8 bytes written, or `null` on unsupported characters or `dst` overflow.
pub fn textToBrailleUtf8(text: []const u8, dst: []u8): usize? {
    let out: usize = 0;
    let in_number: bool = false;
    let i: usize = 0;
    while (i < text.len) {
        const ch = text[i];
        if (ch >= 0x30 && ch <= 0x39) {
            if (!in_number) {
                out = writeBrailleCell(dst, out, BRAILLE_NUMBER_SIGN) ?? return null;
                in_number = true;
            }
            const dm = digitMask(ch) ?? return null;
            out = writeBrailleCell(dst, out, dm) ?? return null;
        } else {
            in_number = false;
            if (ch >= 0x41 && ch <= 0x5A) {
                out = writeBrailleCell(dst, out, BRAILLE_CAPITAL_SIGN) ?? return null;
                const lm = letterMask(ch + 0x20) ?? return null;
                out = writeBrailleCell(dst, out, lm) ?? return null;
            } else {
                const m = asciiToBrailleMask(ch) ?? return null;
                out = writeBrailleCell(dst, out, m) ?? return null;
            }
        }
        i += 1;
    }
    return out;
}

/// Zero-allocation 2x4 sub-pixel terminal graphics canvas backed by caller-owned `cells: []u8`.
/// Each Braille cell covers 2 horizontal pixels by 4 vertical pixels.
pub struct BrailleCanvas {
    cell_cols: usize;
    cell_rows: usize;
    cells: []u8;

    /// Initializes a `BrailleCanvas` and zeroes the backing slice `cells[0..cell_cols * cell_rows]`.
    /// Returns `null` if `cell_cols == 0`, `cell_rows == 0`, or `cells.len < cell_cols * cell_rows`.
    pub fn init(cell_cols: usize, cell_rows: usize, cells: []u8): BrailleCanvas? {
        if (cell_cols == 0 || cell_rows == 0) {
            return null;
        }
        if (cells.len / cell_cols < cell_rows) {
            return null;
        }
        const total = cell_cols * cell_rows;
        let i: usize = 0;
        while (i < total) {
            cells[i] = 0;
            i += 1;
        }
        return BrailleCanvas {
            cell_cols: cell_cols,
            cell_rows: cell_rows,
            cells: cells[0..total],
        };
    }

    /// Clears all pixels in the canvas to 0.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.cells.len) {
            _.cells[i] = 0;
            i += 1;
        }
    }

    /// Returns total horizontal sub-pixel width (`cell_cols * 2`).
    fn pixelWidth(): usize {
        return _.cell_cols * 2;
    }

    /// Returns total vertical sub-pixel height (`cell_rows * 4`).
    fn pixelHeight(): usize {
        return _.cell_rows * 4;
    }

    /// Sets the sub-pixel at `(x, y)`. Returns `false` if out of bounds.
    fn setPixel(*_, x: usize, y: usize): bool {
        if (x >= _.cell_cols * 2 || y >= _.cell_rows * 4) {
            return false;
        }
        const col = x / 2;
        const row = y / 4;
        const bit = dotBitForPixel((x % 2) as u8, (y % 4) as u8) ?? return false;
        const idx = row * _.cell_cols + col;
        _.cells[idx] = _.cells[idx] | bit;
        return true;
    }

    /// Clears the sub-pixel at `(x, y)`. Returns `false` if out of bounds.
    fn clearPixel(*_, x: usize, y: usize): bool {
        if (x >= _.cell_cols * 2 || y >= _.cell_rows * 4) {
            return false;
        }
        const col = x / 2;
        const row = y / 4;
        const bit = dotBitForPixel((x % 2) as u8, (y % 4) as u8) ?? return false;
        const idx = row * _.cell_cols + col;
        _.cells[idx] = _.cells[idx] & (~bit);
        return true;
    }

    /// Returns `true` if the sub-pixel at `(x, y)` is set; returns `false` if unset or out of bounds.
    fn getPixel(x: usize, y: usize): bool {
        if (x >= _.cell_cols * 2 || y >= _.cell_rows * 4) {
            return false;
        }
        const col = x / 2;
        const row = y / 4;
        const bit = dotBitForPixel((x % 2) as u8, (y % 4) as u8) ?? return false;
        const idx = row * _.cell_cols + col;
        return (_.cells[idx] & bit) != 0;
    }

    /// Toggles the sub-pixel at `(x, y)`. Returns `false` if out of bounds.
    fn togglePixel(*_, x: usize, y: usize): bool {
        if (x >= _.cell_cols * 2 || y >= _.cell_rows * 4) {
            return false;
        }
        const col = x / 2;
        const row = y / 4;
        const bit = dotBitForPixel((x % 2) as u8, (y % 4) as u8) ?? return false;
        const idx = row * _.cell_cols + col;
        _.cells[idx] = _.cells[idx] ^ bit;
        return true;
    }

    /// Renders the canvas into `out` as multi-line UTF-8 Unicode Braille art separated by `\n`.
    /// Returns total bytes written, or `null` if `out` is too small.
    fn renderUtf8(out: []u8): usize? {
        if (_.cell_rows == 0 || _.cell_cols == 0) {
            return 0;
        }
        // Required bytes: cell_rows * cell_cols * 3 + (cell_rows - 1)
        const newlines = _.cell_rows - 1;
        if (out.len < newlines) {
            return null;
        }
        if ((out.len - newlines) / 3 < _.cells.len) {
            return null;
        }

        let pos: usize = 0;
        let r: usize = 0;
        while (r < _.cell_rows) {
            if (r > 0) {
                out[pos] = 0x0A; // '\n'
                pos += 1;
            }
            let c: usize = 0;
            while (c < _.cell_cols) {
                const enc = brailleToUtf8(_.cells[r * _.cell_cols + c]);
                out[pos] = enc[0];
                out[pos + 1] = enc[1];
                out[pos + 2] = enc[2];
                pos += 3;
                c += 1;
            }
            r += 1;
        }
        return pos;
    }
}

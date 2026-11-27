// @cap/h3o — Zero-heap Uber H3 64-bit hexagonal hierarchical spatial index.
// Clean-room MIT implementation modeled on crates.io `h3o` v0.6.

pub const H3_MAX_RESOLUTION: u8 = 15;
pub const H3_MAX_BASE_CELL: u8 = 121;
pub const H3_CELL_MODE: u8 = 1;
pub const H3_UNUSED_DIGIT: u8 = 7;

fn h3AllLow63(): u64 {
    return ((1 as u64) << 63) - 1;
}

fn h3DigitShift(r: u8): u6 {
    return ((15 - r) * 3) as u6;
}

fn h3IsPentagonBaseCell(bc: u8): bool {
    return bc == 4 || bc == 14 || bc == 24 || bc == 38
        || bc == 49 || bc == 58 || bc == 63 || bc == 72
        || bc == 83 || bc == 97 || bc == 107 || bc == 117;
}

fn h3HexDigitVal(c: u8): u8 {
    if (c >= 48 && c <= 57) { return c - 48; }
    if (c >= 97 && c <= 102) { return c - 97 + 10; }
    if (c >= 65 && c <= 70) { return c - 65 + 10; }
    return 255;
}

fn h3HexChar(nibble: u8): u8 {
    if (nibble < 10) { return 48 + nibble; }
    return 97 + (nibble - 10);
}

fn h3ClearBits(val: u64, mask: u64): u64 {
    return val & (h3AllLow63() ^ mask);
}

/// 64-bit Uber H3 Cell Index.
///
/// Bit layout (64 bits):
/// - bit 63: reserved (`0`)
/// - bits 62..59: mode (`1` for H3 Cell)
/// - bits 58..56: reserved (`0`)
/// - bits 55..52: resolution (`0..15`)
/// - bits 51..45: base cell (`0..121`)
/// - bits 44..0: 15 x 3-bit hierarchical direction digits (`0..6` active, `7` unused)
pub struct H3Cell {
    pub bits: u64,

    /// Constructs and validates an `H3Cell` from resolution, base cell, and 15 hierarchical digits.
    /// Unused digits beyond `res` are automatically filled with `7`.
    pub static fn fromParts(res: u8, base_cell: u8, digits: [15]u8): H3Cell? {
        if (res > H3_MAX_RESOLUTION || base_cell > H3_MAX_BASE_CELL) {
            return null;
        }
        let is_pent: bool = h3IsPentagonBaseCell(base_cell);
        let first_nonzero_seen: bool = false;
        let bits: u64 = ((H3_CELL_MODE as u64) << 59)
            | ((res as u64) << 52)
            | ((base_cell as u64) << 45);

        let r: u8 = 1;
        while (r <= H3_MAX_RESOLUTION) {
            let d: u8 = H3_UNUSED_DIGIT;
            if (r <= res) {
                d = digits[(r - 1) as usize];
                if (d > 6) { return null; }
                if (is_pent && !first_nonzero_seen && d != 0) {
                    if (d == 1) { return null; }
                    first_nonzero_seen = true;
                }
            }
            let sh: u6 = h3DigitShift(r);
            bits = bits | ((d as u64) << sh);
            r = r + 1;
        }
        return H3Cell { bits: bits };
    }

    /// Validates and wraps a raw 64-bit H3 Cell Index.
    pub static fn fromU64(raw: u64): H3Cell? {
        // Bit 63 must be 0
        if ((raw >> 63) != 0) { return null; }
        // Bits 62..59 must be mode 1
        let mode: u8 = ((raw >> 59) & 15) as u8;
        if (mode != H3_CELL_MODE) { return null; }
        // Bits 58..56 reserved must be 0
        if (((raw >> 56) & 7) != 0) { return null; }

        let res: u8 = ((raw >> 52) & 15) as u8;
        let bc: u8 = ((raw >> 45) & 127) as u8;
        if (res > H3_MAX_RESOLUTION || bc > H3_MAX_BASE_CELL) {
            return null;
        }

        let is_pent: bool = h3IsPentagonBaseCell(bc);
        let first_nonzero_seen: bool = false;
        let r: u8 = 1;
        while (r <= H3_MAX_RESOLUTION) {
            let sh: u6 = h3DigitShift(r);
            let d: u8 = ((raw >> sh) & 7) as u8;
            if (r <= res) {
                if (d > 6) { return null; }
                if (is_pent && !first_nonzero_seen && d != 0) {
                    if (d == 1) { return null; }
                    first_nonzero_seen = true;
                }
            } else {
                if (d != H3_UNUSED_DIGIT) { return null; }
            }
            r = r + 1;
        }
        return H3Cell { bits: raw };
    }

    /// Returns the cell resolution (`0..15`).
    pub fn resolution(): u8 {
        return ((_.bits >> 52) & 15) as u8;
    }

    /// Returns the base cell index (`0..121`).
    pub fn baseCell(): u8 {
        return ((_.bits >> 45) & 127) as u8;
    }

    /// Returns the 3-bit direction digit at 1-based resolution `res` (`1..15`), or `7` if out of range.
    pub fn digitAt(res: u8): u8 {
        if (res == 0 || res > H3_MAX_RESOLUTION) {
            return H3_UNUSED_DIGIT;
        }
        let sh: u6 = h3DigitShift(res);
        return ((_.bits >> sh) & 7) as u8;
    }

    /// Returns `true` if this cell is one of the 12 pentagons at its resolution.
    pub fn isPentagon(): bool {
        let bc: u8 = _.baseCell();
        if (!h3IsPentagonBaseCell(bc)) { return false; }
        let cur_res: u8 = _.resolution();
        let r: u8 = 1;
        while (r <= cur_res) {
            if (_.digitAt(r) != 0) { return false; }
            r = r + 1;
        }
        return true;
    }

    /// Returns the ancestor cell at coarser resolution `target_res <= resolution()`.
    pub fn parent(target_res: u8): H3Cell? {
        let cur_res: u8 = _.resolution();
        if (target_res > cur_res) { return null; }
        let b: u64 = h3ClearBits(_.bits, (15 as u64) << 52);
        b = b | ((target_res as u64) << 52);
        let r: u8 = target_res + 1;
        while (r <= H3_MAX_RESOLUTION) {
            let sh: u6 = h3DigitShift(r);
            b = b | ((H3_UNUSED_DIGIT as u64) << sh);
            r = r + 1;
        }
        return H3Cell { bits: b };
    }

    /// Returns the child cell one resolution finer with direction `digit` (`0..6`).
    pub fn childAt(digit: u8): H3Cell? {
        let cur_res: u8 = _.resolution();
        if (cur_res >= H3_MAX_RESOLUTION || digit > 6) {
            return null;
        }
        if (_.isPentagon() && digit == 1) {
            return null;
        }
        let next_res: u8 = cur_res + 1;
        let b: u64 = h3ClearBits(_.bits, (15 as u64) << 52);
        b = b | ((next_res as u64) << 52);
        let sh: u6 = h3DigitShift(next_res);
        b = h3ClearBits(b, (7 as u64) << sh);
        b = b | ((digit as u64) << sh);
        return H3Cell { bits: b };
    }

    /// Formats the H3 cell index as lowercase hex without leading zeros (15 chars for standard cells).
    pub fn toHex(out: []u8): usize {
        let v: u64 = _.bits;
        let rev: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let n: usize = 0;
        if (v == 0) {
            rev[0] = 48;
            n = 1;
        } else {
            while (v > 0 && n < 16) {
                rev[n] = h3HexChar((v & 15) as u8);
                v = v >> 4;
                n = n + 1;
            }
        }
        let p: usize = 0;
        let i: usize = n;
        while (i > 0 && p < out.len) {
            i = i - 1;
            out[p] = rev[i];
            p = p + 1;
        }
        return p;
    }
}

/// Parses a hexadecimal H3 cell string (with optional `0x`/`0X` prefix) and validates its bit layout.
pub fn parseH3Hex(hex: string): H3Cell? {
    let start: usize = 0;
    if (hex.len >= 2 && hex[0] == 48 && (hex[1] == 120 || hex[1] == 88)) {
        start = 2;
    }
    let len: usize = hex.len - start;
    if (len == 0 || len > 16) { return null; }

    let acc: u64 = 0;
    let i: usize = start;
    while (i < hex.len) {
        let d: u8 = h3HexDigitVal(hex[i]);
        if (d == 255) { return null; }
        acc = (acc << 4) | (d as u64);
        i = i + 1;
    }
    return H3Cell.fromU64(acc);
}

/// Returns the standard Uber H3 average hexagon area in square kilometers for `res` (`0..15`).
pub fn avgHexAreaKm2(res: u8): f64 {
    if (res == 0) { return 4357449.416078381; }
    if (res == 1) { return 609788.4417941332; }
    if (res == 2) { return 86801.7803989972; }
    if (res == 3) { return 12393.43465508816; }
    if (res == 4) { return 1770.347654491307; }
    if (res == 5) { return 252.9038581819449; }
    if (res == 6) { return 36.12906216441245; }
    if (res == 7) { return 5.161293359717191; }
    if (res == 8) { return 0.7373275975941177; }
    if (res == 9) { return 0.1053325134272067; }
    if (res == 10) { return 0.01504750190766435; }
    if (res == 11) { return 0.002149643129451879; }
    if (res == 12) { return 0.000307091875631606; }
    if (res == 13) { return 0.00004387026794728296; }
    if (res == 14) { return 0.000006267181135324313; }
    if (res == 15) { return 0.000000895311590760579; }
    return 0.0;
}

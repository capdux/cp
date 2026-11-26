// @cap/tinystr — Zero-allocation register-packed short ASCII strings (TinyStr4 / TinyStr8) for CP
// Clean-room MIT implementation inspired by Rust ICU4X `tinystr` (835M downloads).

fn isValidAsciiByte(b: u8) -> bool {
    return b >= (1 as u8) && b <= (127 as u8);
}

fn isAlphaByte(b: u8) -> bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z');
}

fn isDigitByte(b: u8) -> bool {
    return b >= '0' && b <= '9';
}

fn toLowerByte(b: u8) -> u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + (32 as u8);
    }
    return b;
}

fn toUpperByte(b: u8) -> u8 {
    if (b >= 'a' && b <= 'z') {
        return b - (32 as u8);
    }
    return b;
}

fn getByte32(raw: u32, idx: usize) -> u8 {
    let v = raw;
    let i: usize = 0;
    while (i < idx) {
        v = v / (256 as u32);
        i += 1;
    }
    return (v & (0xFF as u32)) as u8;
}

fn putByte32(raw: u32, idx: usize, b: u8) -> u32 {
    let shift: u32 = 1;
    let i: usize = 0;
    while (i < idx) {
        shift = shift * (256 as u32);
        i += 1;
    }
    return raw | ((b as u32) * shift);
}

fn getByte64(raw: u64, idx: usize) -> u8 {
    let v = raw;
    let i: usize = 0;
    while (i < idx) {
        v = v / (256 as u64);
        i += 1;
    }
    return (v & (0xFF as u64)) as u8;
}

fn putByte64(raw: u64, idx: usize, b: u8) -> u64 {
    let shift: u64 = 1;
    let i: usize = 0;
    while (i < idx) {
        shift = shift * (256 as u64);
        i += 1;
    }
    return raw | ((b as u64) * shift);
}

pub struct TinyStr4 {
    pub raw: u32,

    pub static fn fromStr(s: string) -> TinyStr4? {
        if (s.len == 0 || s.len > 4) {
            return null;
        }
        let acc: u32 = 0;
        let i: usize = 0;
        while (i < s.len) {
            const b = s[i];
            if (!isValidAsciiByte(b)) {
                return null;
            }
            acc = putByte32(acc, i, b);
            i += 1;
        }
        return TinyStr4{ raw: acc };
    }

    pub fn len() -> usize {
        let n: usize = 0;
        while (n < 4) {
            if (getByte32(_.raw, n) == (0 as u8)) {
                break;
            }
            n += 1;
        }
        return n;
    }

    pub fn equals(other: TinyStr4) -> bool {
        return _.raw == other.raw;
    }

    pub fn toAsciiLower() -> TinyStr4 {
        let out: u32 = 0;
        let i: usize = 0;
        while (i < 4) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            out = putByte32(out, i, toLowerByte(b));
            i += 1;
        }
        return TinyStr4{ raw: out };
    }

    pub fn toAsciiUpper() -> TinyStr4 {
        let out: u32 = 0;
        let i: usize = 0;
        while (i < 4) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            out = putByte32(out, i, toUpperByte(b));
            i += 1;
        }
        return TinyStr4{ raw: out };
    }

    pub fn toAsciiTitle() -> TinyStr4 {
        let out: u32 = 0;
        let i: usize = 0;
        while (i < 4) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            const nb = if (i == 0) { toUpperByte(b) } else { toLowerByte(b) };
            out = putByte32(out, i, nb);
            i += 1;
        }
        return TinyStr4{ raw: out };
    }

    pub fn isAsciiAlpha() -> bool {
        let i: usize = 0;
        let count: usize = 0;
        while (i < 4) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            if (!isAlphaByte(b)) {
                return false;
            }
            count += 1;
            i += 1;
        }
        return count > 0;
    }

    pub fn isAsciiNumeric() -> bool {
        let i: usize = 0;
        let count: usize = 0;
        while (i < 4) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            if (!isDigitByte(b)) {
                return false;
            }
            count += 1;
            i += 1;
        }
        return count > 0;
    }

    pub fn isAsciiAlphanumeric() -> bool {
        let i: usize = 0;
        let count: usize = 0;
        while (i < 4) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            if (!isAlphaByte(b) && !isDigitByte(b)) {
                return false;
            }
            count += 1;
            i += 1;
        }
        return count > 0;
    }

    pub fn writeTo(out: []u8) -> string {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 4 && pos < out.len) {
            const b = getByte32(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            out[pos] = b;
            pos += 1;
            i += 1;
        }
        return out[0..pos];
    }
}

pub struct TinyStr8 {
    pub raw: u64,

    pub static fn fromStr(s: string) -> TinyStr8? {
        if (s.len == 0 || s.len > 8) {
            return null;
        }
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < s.len) {
            const b = s[i];
            if (!isValidAsciiByte(b)) {
                return null;
            }
            acc = putByte64(acc, i, b);
            i += 1;
        }
        return TinyStr8{ raw: acc };
    }

    pub fn len() -> usize {
        let n: usize = 0;
        while (n < 8) {
            if (getByte64(_.raw, n) == (0 as u8)) {
                break;
            }
            n += 1;
        }
        return n;
    }

    pub fn equals(other: TinyStr8) -> bool {
        return _.raw == other.raw;
    }

    pub fn toAsciiLower() -> TinyStr8 {
        let out: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            out = putByte64(out, i, toLowerByte(b));
            i += 1;
        }
        return TinyStr8{ raw: out };
    }

    pub fn toAsciiUpper() -> TinyStr8 {
        let out: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            out = putByte64(out, i, toUpperByte(b));
            i += 1;
        }
        return TinyStr8{ raw: out };
    }

    pub fn toAsciiTitle() -> TinyStr8 {
        let out: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            const nb = if (i == 0) { toUpperByte(b) } else { toLowerByte(b) };
            out = putByte64(out, i, nb);
            i += 1;
        }
        return TinyStr8{ raw: out };
    }

    pub fn isAsciiAlpha() -> bool {
        let i: usize = 0;
        let count: usize = 0;
        while (i < 8) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            if (!isAlphaByte(b)) {
                return false;
            }
            count += 1;
            i += 1;
        }
        return count > 0;
    }

    pub fn isAsciiNumeric() -> bool {
        let i: usize = 0;
        let count: usize = 0;
        while (i < 8) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            if (!isDigitByte(b)) {
                return false;
            }
            count += 1;
            i += 1;
        }
        return count > 0;
    }

    pub fn isAsciiAlphanumeric() -> bool {
        let i: usize = 0;
        let count: usize = 0;
        while (i < 8) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            if (!isAlphaByte(b) && !isDigitByte(b)) {
                return false;
            }
            count += 1;
            i += 1;
        }
        return count > 0;
    }

    pub fn writeTo(out: []u8) -> string {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 8 && pos < out.len) {
            const b = getByte64(_.raw, i);
            if (b == (0 as u8)) {
                break;
            }
            out[pos] = b;
            pos += 1;
            i += 1;
        }
        return out[0..pos];
    }
}

export {
    TinyStr4,
    TinyStr8,
};

// @cap/smol-str — Zero-heap pure-CP small string with 24-byte inline storage (SSO)
// Clean-room MIT equivalent of crates.io `smol_str`.

pub const SMOL_INLINE_CAP: usize = 24;

fn smolGetByte(s: SmolStr, idx: usize): u8 {
    if (s.is_inline) {
        return s.inline_buf[idx];
    }
    return s.ext[idx];
}

fn smolMinUsize(a: usize, b: usize): usize {
    if (a < b) {
        return a;
    }
    return b;
}

pub struct SmolStr {
    pub inline_buf: [24]u8,
    pub inline_len: u8,
    pub is_inline: bool,
    pub ext: string,

    pub static fn empty(): SmolStr {
        let buf: [24]u8 = [0; 24];
        return SmolStr {
            inline_buf: buf,
            inline_len: 0,
            is_inline: true,
            ext: "",
        };
    }

    pub static fn new(s: string): SmolStr {
        let buf: [24]u8 = [0; 24];
        if (s.len <= SMOL_INLINE_CAP) {
            let i: usize = 0;
            while (i < s.len) {
                buf[i] = s[i];
                i = i + 1;
            }
            return SmolStr {
                inline_buf: buf,
                inline_len: s.len as u8,
                is_inline: true,
                ext: "",
            };
        }
        return SmolStr {
            inline_buf: buf,
            inline_len: 0,
            is_inline: false,
            ext: s,
        };
    }

    pub static fn fromInt(v: i64): SmolStr {
        let buf: [24]u8 = [0; 24];
        if (v == 0) {
            buf[0] = 48; // '0'
            return SmolStr {
                inline_buf: buf,
                inline_len: 1,
                is_inline: true,
                ext: "",
            };
        }
        let neg: bool = v < 0;
        let mag: u64 = 0;
        if (neg) {
            // Avoid i64 overflow on i64::MIN (-9223372036854775808)
            mag = ((-(v + 1)) as u64) + 1;
        } else {
            mag = v as u64;
        }
        let rev: [24]u8 = [0; 24];
        let digits: usize = 0;
        while (mag > 0) {
            let rem: u64 = mag % 10;
            rev[digits] = (48 + rem) as u8;
            digits = digits + 1;
            mag = mag / 10;
        }
        let pos: usize = 0;
        if (neg) {
            buf[0] = 45; // '-'
            pos = 1;
        }
        let i: usize = 0;
        while (i < digits) {
            buf[pos + i] = rev[digits - 1 - i];
            i = i + 1;
        }
        return SmolStr {
            inline_buf: buf,
            inline_len: (pos + digits) as u8,
            is_inline: true,
            ext: "",
        };
    }

    pub fn len(): usize {
        if (_.is_inline) {
            return _.inline_len as usize;
        }
        return _.ext.len;
    }

    pub fn isEmpty(): bool {
        return _.len() == 0;
    }

    pub fn isInline(): bool {
        return _.is_inline;
    }

    pub fn byteAt(idx: usize): u8? {
        if (_.is_inline) {
            if (idx >= (_.inline_len as usize)) {
                return null;
            }
            return _.inline_buf[idx];
        }
        if (idx >= _.ext.len) {
            return null;
        }
        return _.ext[idx];
    }

    pub fn eq(other: SmolStr): bool {
        let n: usize = _.len();
        if (n != other.len()) {
            return false;
        }
        let i: usize = 0;
        while (i < n) {
            let a: u8 = if (_.is_inline) { _.inline_buf[i] } else { _.ext[i] };
            let b: u8 = smolGetByte(other, i);
            if (a != b) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn eqStr(s: string): bool {
        let n: usize = _.len();
        if (n != s.len) {
            return false;
        }
        let i: usize = 0;
        while (i < n) {
            let a: u8 = if (_.is_inline) { _.inline_buf[i] } else { _.ext[i] };
            if (a != s[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn cmp(other: SmolStr): i32 {
        let n1: usize = _.len();
        let n2: usize = other.len();
        let common: usize = smolMinUsize(n1, n2);
        let i: usize = 0;
        while (i < common) {
            let a: u8 = if (_.is_inline) { _.inline_buf[i] } else { _.ext[i] };
            let b: u8 = smolGetByte(other, i);
            if (a < b) {
                return -1;
            }
            if (a > b) {
                return 1;
            }
            i = i + 1;
        }
        if (n1 < n2) {
            return -1;
        }
        if (n1 > n2) {
            return 1;
        }
        return 0;
    }

    pub fn hasPrefix(prefix: string): bool {
        if (prefix.len > _.len()) {
            return false;
        }
        let i: usize = 0;
        while (i < prefix.len) {
            let a: u8 = if (_.is_inline) { _.inline_buf[i] } else { _.ext[i] };
            if (a != prefix[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn hasSuffix(suffix: string): bool {
        let n: usize = _.len();
        if (suffix.len > n) {
            return false;
        }
        let start: usize = n - suffix.len;
        let i: usize = 0;
        while (i < suffix.len) {
            let a: u8 = if (_.is_inline) { _.inline_buf[start + i] } else { _.ext[start + i] };
            if (a != suffix[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn writeTo(out: []u8): usize {
        let n: usize = smolMinUsize(_.len(), out.len);
        let i: usize = 0;
        while (i < n) {
            if (_.is_inline) {
                out[i] = _.inline_buf[i];
            } else {
                out[i] = _.ext[i];
            }
            i = i + 1;
        }
        return n;
    }
}

pub fn smolStartsWith(s: SmolStr, prefix: string): bool {
    return s.hasPrefix(prefix);
}

pub fn smolEndsWith(s: SmolStr, suffix: string): bool {
    return s.hasSuffix(suffix);
}

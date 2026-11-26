// @cap/percent-encoding — Zero-allocation WHATWG & RFC 3986 percent-encoding engine for CP
// Clean-room MIT implementation inspired by Rust `percent-encoding` (1.15B downloads).

fn bit64(idx: u8) -> u64 {
    let v: u64 = 1;
    let i: u8 = 0;
    while (i < idx) {
        v = v * (2 as u64);
        i += 1;
    }
    return v;
}

pub struct AsciiSet {
    pub lo: u64,
    pub hi: u64,

    pub static fn empty() -> AsciiSet {
        return AsciiSet{
            lo: 0 as u64,
            hi: 0 as u64,
        };
    }

    pub fn add(*_, b: u8) {
        if (b < 64) {
            const m = bit64(b);
            _.lo = _.lo | m;
        } else if (b < 128) {
            const m = bit64(b - (64 as u8));
            _.hi = _.hi | m;
        }
    }

    pub fn remove(*_, b: u8) {
        if (b < 64) {
            const m = bit64(b);
            if ((_.lo & m) != (0 as u64)) {
                _.lo = _.lo ^ m;
            }
        } else if (b < 128) {
            const m = bit64(b - (64 as u8));
            if ((_.hi & m) != (0 as u64)) {
                _.hi = _.hi ^ m;
            }
        }
    }

    pub fn contains(b: u8) -> bool {
        if (b >= 128) {
            return true;
        }
        if (b < 64) {
            const m = bit64(b);
            return (_.lo & m) != (0 as u64);
        }
        const m = bit64(b - (64 as u8));
        return (_.hi & m) != (0 as u64);
    }
}

pub fn controlsSet() -> AsciiSet {
    let s = AsciiSet.empty();
    let b: u8 = 0;
    while (b <= 0x1F) {
        s.add(b);
        b += 1;
    }
    s.add(0x7F as u8);
    return s;
}

pub fn fragmentSet() -> AsciiSet {
    let s = controlsSet();
    s.add(' ');
    s.add('"');
    s.add('<');
    s.add('>');
    s.add('`');
    return s;
}

pub fn querySet() -> AsciiSet {
    let s = controlsSet();
    s.add(' ');
    s.add('"');
    s.add('#');
    s.add('<');
    s.add('>');
    return s;
}

pub fn pathSegmentSet() -> AsciiSet {
    let s = querySet();
    s.add('?');
    s.add('`');
    s.add('{');
    s.add('}');
    s.add('/');
    return s;
}

pub fn userinfoSet() -> AsciiSet {
    let s = pathSegmentSet();
    s.add(':');
    s.add(';');
    s.add('=');
    s.add('@');
    s.add('[');
    s.add('\\');
    s.add(']');
    s.add('^');
    s.add('|');
    return s;
}

fn isAsciiAlnum(b: u8) -> bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9');
}

pub fn nonAlphanumericSet() -> AsciiSet {
    let s = AsciiSet.empty();
    let b: u8 = 0;
    while (b < 128) {
        if (!isAsciiAlnum(b)) {
            s.add(b);
        }
        b += 1;
    }
    return s;
}

pub fn rfc3986UnreservedSet() -> AsciiSet {
    let s = nonAlphanumericSet();
    s.remove('-');
    s.remove('.');
    s.remove('_');
    s.remove('~');
    return s;
}

fn hexUpperChar(nibble: u8) -> u8 {
    if (nibble < 10) {
        return '0' + nibble;
    }
    return 'A' + (nibble - (10 as u8));
}

fn hexVal(c: u8) -> i16 {
    if (c >= '0' && c <= '9') {
        return (c - '0') as i16;
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + (10 as u8)) as i16;
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + (10 as u8)) as i16;
    }
    return -1 as i16;
}

pub fn percentEncode(input: string, set: AsciiSet, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const b = input[i];
        if (set.contains(b)) {
            if (pos + 3 > out.len) {
                break;
            }
            out[pos] = '%';
            out[pos + 1] = hexUpperChar(b / (16 as u8));
            out[pos + 2] = hexUpperChar(b % (16 as u8));
            pos += 3;
        } else {
            if (pos + 1 > out.len) {
                break;
            }
            out[pos] = b;
            pos += 1;
        }
        i += 1;
    }
    return out[0..pos];
}

pub fn percentDecode(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len && pos < out.len) {
        if (input[i] == '%' && i + 2 < input.len) {
            const hi = hexVal(input[i + 1]);
            const lo = hexVal(input[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out[pos] = ((hi * (16 as i16)) + lo) as u8;
                pos += 1;
                i += 3;
                continue;
            }
        }
        out[pos] = input[i];
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

pub fn percentDecodeStrict(input: string, out: []u8) -> string? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (pos >= out.len) {
            return null;
        }
        if (input[i] == '%') {
            if (i + 2 >= input.len) {
                return null;
            }
            const hi = hexVal(input[i + 1]);
            const lo = hexVal(input[i + 2]);
            if (hi < 0 || lo < 0) {
                return null;
            }
            out[pos] = ((hi * (16 as i16)) + lo) as u8;
            pos += 1;
            i += 3;
        } else {
            out[pos] = input[i];
            pos += 1;
            i += 1;
        }
    }
    return out[0..pos];
}

fn isFormUnescaped(b: u8) -> bool {
    return isAsciiAlnum(b) || b == '*' || b == '-' || b == '.' || b == '_';
}

pub fn formUrlEncode(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const b = input[i];
        if (b == ' ') {
            if (pos + 1 > out.len) {
                break;
            }
            out[pos] = '+';
            pos += 1;
        } else if (isFormUnescaped(b)) {
            if (pos + 1 > out.len) {
                break;
            }
            out[pos] = b;
            pos += 1;
        } else {
            if (pos + 3 > out.len) {
                break;
            }
            out[pos] = '%';
            out[pos + 1] = hexUpperChar(b / (16 as u8));
            out[pos + 2] = hexUpperChar(b % (16 as u8));
            pos += 3;
        }
        i += 1;
    }
    return out[0..pos];
}

pub fn formUrlDecode(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len && pos < out.len) {
        if (input[i] == '+') {
            out[pos] = ' ';
            pos += 1;
            i += 1;
            continue;
        }
        if (input[i] == '%' && i + 2 < input.len) {
            const hi = hexVal(input[i + 1]);
            const lo = hexVal(input[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out[pos] = ((hi * (16 as i16)) + lo) as u8;
                pos += 1;
                i += 3;
                continue;
            }
        }
        out[pos] = input[i];
        pos += 1;
        i += 1;
    }
    return out[0..pos];
}

export {
    AsciiSet,
    controlsSet,
    fragmentSet,
    querySet,
    pathSegmentSet,
    userinfoSet,
    nonAlphanumericSet,
    rfc3986UnreservedSet,
    percentEncode,
    percentDecode,
    percentDecodeStrict,
    formUrlEncode,
    formUrlDecode,
};

pub struct Utf8Error {
    pub valid_up_to: usize,
    pub has_error_len: bool,
    pub error_len: usize,
}

pub struct Utf8CompatResult {
    pub valid: bool,
    pub valid_up_to: usize,
    pub has_error_len: bool,
    pub error_len: usize,
}

type SutfVec16 = @Vector(16, u8);

fn sutfIsCont(b: u8): bool {
    return (b & 192) == 128; // 0b10xxxxxx
}

pub fn isAscii(buf: []u8): bool {
    const hi_mask: SutfVec16 = @splat(0x80 as u8);
    let i: usize = 0;
    while (i + 16 <= buf.len) {
        const v: SutfVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v & hi_mask) != 0) { return false; }
        i = i + 16;
    }
    while (i < buf.len) {
        if ((buf[i] & 128) != 0) { return false; }
        i = i + 1;
    }
    return true;
}

pub fn validateUtf8Compat(buf: []u8): Utf8CompatResult {
    const hi_mask: SutfVec16 = @splat(0x80 as u8);
    let i: usize = 0;
    let n = buf.len;
    while (i < n) {
        // 16-lane hardware SIMD ASCII fast-path
        while (i + 16 <= n) {
            const v: SutfVec16 = buf[i .. i + 16][0..16].*;
            if (@reduce(.Or, v & hi_mask) != 0) { break; }
            i = i + 16;
        }
        if (i >= n) { break; }

        let b0 = buf[i];
        if (b0 < 128) {
            i = i + 1;
            continue;
        }

        // 2-byte sequence: C2..DF 80..BF
        if (b0 >= 194 && b0 <= 223) {
            if (i + 1 >= n) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: false, error_len: 0 };
            }
            if (!sutfIsCont(buf[i + 1])) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 1 };
            }
            i = i + 2;
            continue;
        }

        // 3-byte sequence: E0..EF
        if (b0 >= 224 && b0 <= 239) {
            if (i + 1 >= n) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: false, error_len: 0 };
            }
            let b1 = buf[i + 1];
            let valid_b1 = if (b0 == 224) {
                b1 >= 160 && b1 <= 191 // reject overlong
            } else if (b0 == 237) {
                b1 >= 128 && b1 <= 159 // reject UTF-16 surrogates U+D800..U+DFFF
            } else {
                sutfIsCont(b1)
            };
            if (!valid_b1) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 1 };
            }
            if (i + 2 >= n) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: false, error_len: 0 };
            }
            if (!sutfIsCont(buf[i + 2])) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 2 };
            }
            i = i + 3;
            continue;
        }

        // 4-byte sequence: F0..F4
        if (b0 >= 240 && b0 <= 244) {
            if (i + 1 >= n) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: false, error_len: 0 };
            }
            let b1 = buf[i + 1];
            let valid_b1 = if (b0 == 240) {
                b1 >= 144 && b1 <= 191 // reject overlong
            } else if (b0 == 244) {
                b1 >= 128 && b1 <= 143 // reject > U+10FFFF
            } else {
                sutfIsCont(b1)
            };
            if (!valid_b1) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 1 };
            }
            if (i + 2 >= n) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: false, error_len: 0 };
            }
            if (!sutfIsCont(buf[i + 2])) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 2 };
            }
            if (i + 3 >= n) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: false, error_len: 0 };
            }
            if (!sutfIsCont(buf[i + 3])) {
                return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 3 };
            }
            i = i + 4;
            continue;
        }

        // Invalid lead byte (80..C1, F5..FF)
        return Utf8CompatResult { valid: false, valid_up_to: i, has_error_len: true, error_len: 1 };
    }

    return Utf8CompatResult { valid: true, valid_up_to: n, has_error_len: false, error_len: 0 };
}

pub fn basicIsValidUtf8(buf: []u8): bool {
    return validateUtf8Compat(buf).valid;
}

pub fn countCodepoints(buf: []u8): usize? {
    if (!basicIsValidUtf8(buf)) { return null; }
    const hi_mask: SutfVec16 = @splat(0x80 as u8);
    let count: usize = 0;
    let i: usize = 0;
    while (i + 16 <= buf.len) {
        const v: SutfVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v & hi_mask) == 0) {
            count = count + 16;
            i = i + 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) : (k += 1) {
            if ((buf[i + k] & 192) != 128) {
                count = count + 1;
            }
        }
        i = i + 16;
    }
    while (i < buf.len) {
        if ((buf[i] & 192) != 128) {
            count = count + 1;
        }
        i = i + 1;
    }
    return count;
}

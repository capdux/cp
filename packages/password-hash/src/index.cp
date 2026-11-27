// @cap/password-hash — Zero-allocation pure CP PHC string format parser & serializer
// Parity with RustCrypto crates.io:password-hash (MIT)
// Accelerated with comptime 256-entry bitmask LUT, 16-lane @Vector(16, u8) SIMD scanning & ctEqual, and @memcpy.

type PhcVec16 = @Vector(16, u8);

const PHC_KEY: u8 = 1;
const PHC_VAL: u8 = 2;
const PHC_B64: u8 = 4;

fn phchBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        const b: u8 = c as u8;
        let f: u8 = 0;
        if ((b >= ('a' as u8) && b <= ('z' as u8)) || (b >= ('0' as u8) && b <= ('9' as u8))) {
            f |= PHC_KEY | PHC_VAL | PHC_B64;
        } else if (b >= ('A' as u8) && b <= ('Z' as u8)) {
            f |= PHC_VAL | PHC_B64;
        }
        if (b == ('-' as u8)) { f |= PHC_KEY | PHC_VAL; }
        if (b == ('.' as u8)) { f |= PHC_VAL; }
        if (b == ('/' as u8) || b == ('+' as u8)) { f |= PHC_VAL | PHC_B64; }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const PHC_CHAR_LUT: [256]u8 = phchBuildCharLut();

/// A single key=value parameter in a PHC string.
pub struct PhcParam {
    key: string;
    value: string;
}

/// Parsed representation of a PHC string:
/// `$algorithm[$v=version][$k=v,...][$salt[$hash]]`
pub struct PhcHash {
    algorithm: string;
    version: u32;
    has_version: bool;
    params: [8]PhcParam;
    param_count: usize;
    salt: string;
    hash: string;
    valid: bool;
}

fn phchEmptyHash(): PhcHash {
    let params: [8]PhcParam = undefined;
    let i: usize = 0;
    while (i < 8) {
        params[i] = PhcParam{ key: "", value: "" };
        i += 1;
    }
    return PhcHash{
        algorithm: "",
        version: 0,
        has_version: false,
        params: params,
        param_count: 0,
        salt: "",
        hash: "",
        valid: false,
    };
}

fn phchIsLowerAlnumHyphen(c: u8): bool {
    return (PHC_CHAR_LUT[c as usize] & PHC_KEY) != 0;
}

fn phchIsParamValueChar(c: u8): bool {
    return (PHC_CHAR_LUT[c as usize] & PHC_VAL) != 0;
}

fn phchIsB64Char(c: u8): bool {
    return (PHC_CHAR_LUT[c as usize] & PHC_B64) != 0;
}

fn phchIsValidAlgo(s: string): bool {
    if (s.len == 0 || s.len > 32) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!phchIsLowerAlnumHyphen(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn phchIsValidParamKey(s: string): bool {
    if (s.len == 0 || s.len > 32) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!phchIsLowerAlnumHyphen(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn phchIsValidParamValue(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!phchIsParamValueChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn phchIsValidSalt(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!phchIsParamValueChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn phchIsValidHash(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!phchIsB64Char(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn phchContainsByte(s: string, target: u8): bool {
    let i: usize = 0;
    const len = s.len;
    const vt: PhcVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: PhcVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            return true;
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn phchParseU32(s: string): i64 {
    if (s.len == 0 || s.len > 10) {
        return -1;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            return -1;
        }
        const digit: u64 = (c - ('0' as u8)) as u64;
        acc = acc * 10 + digit;
        if (acc > 4294967295) {
            return -1;
        }
        i += 1;
    }
    return acc as i64;
}

fn phchStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn phchAppendBytes(out: []u8, pos: usize, src: string): usize {
    if (pos + src.len > out.len) {
        return 0;
    }
    if (src.len > 0) {
        @memcpy(out[pos .. pos + src.len], src[0..src.len]);
    }
    return pos + src.len;
}

fn phchAppendU32(out: []u8, pos: usize, val: u32): usize {
    let digits: [10]u8 = undefined;
    let count: usize = 0;
    let v: u32 = val;
    if (v == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        while (v > 0) {
            const rem: u32 = v % 10;
            digits[count] = ('0' as u8) + (rem as u8);
            count += 1;
            v = v / 10;
        }
    }
    if (pos + count > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < count) {
        out[pos + i] = digits[count - 1 - i];
        i += 1;
    }
    return pos + count;
}

/// Parse a PHC string (`$algorithm[$v=version][$k=v,...][$salt[$hash]]`).
/// Returns a `PhcHash` with `valid == true` on success, or `valid == false` on malformed input.
pub fn parsePhc(phc_str: string): PhcHash {
    let result = phchEmptyHash();
    if (phc_str.len < 2 || phc_str[0] != ('$' as u8)) {
        return result;
    }
    if (phc_str[phc_str.len - 1] == ('$' as u8)) {
        return result;
    }

    // Split into up to 5 '$'-delimited fields after the leading '$'
    let segs: [5]string = undefined;
    let segCount: usize = 0;
    let start: usize = 1;
    let i: usize = 1;
    while (i <= phc_str.len) {
        if (i == phc_str.len || phc_str[i] == ('$' as u8)) {
            if (i == start) {
                return result;
            }
            if (segCount >= 5) {
                return result;
            }
            segs[segCount] = phc_str[start..i];
            segCount += 1;
            start = i + 1;
        }
        i += 1;
    }

    if (segCount == 0) {
        return result;
    }

    // Segment 0: algorithm identifier
    if (!phchIsValidAlgo(segs[0])) {
        return result;
    }
    result.algorithm = segs[0];

    let idx: usize = 1;

    // Optional version: v=<digits>
    if (idx < segCount) {
        const seg = segs[idx];
        if (seg.len >= 3 && seg[0] == ('v' as u8) && seg[1] == ('=' as u8)) {
            const verVal: i64 = phchParseU32(seg[2..seg.len]);
            if (verVal < 0) {
                return phchEmptyHash();
            }
            result.version = verVal as u32;
            result.has_version = true;
            idx += 1;
        }
    }

    // Optional params: k=v,k2=v2,...
    if (idx < segCount && phchContainsByte(segs[idx], '=' as u8)) {
        const pseg = segs[idx];
        let pStart: usize = 0;
        let pi: usize = 0;
        while (pi <= pseg.len) {
            if (pi == pseg.len || pseg[pi] == (',' as u8)) {
                if (pi == pStart) {
                    return phchEmptyHash();
                }
                if (result.param_count >= 8) {
                    return phchEmptyHash();
                }
                const pair = pseg[pStart..pi];
                // Find '=' inside pair
                let eqPos: usize = pair.len;
                let j: usize = 0;
                while (j < pair.len) {
                    if (pair[j] == ('=' as u8)) {
                        if (eqPos != pair.len) {
                            return phchEmptyHash();
                        }
                        eqPos = j;
                    }
                    j += 1;
                }
                if (eqPos == pair.len || eqPos == 0 || eqPos + 1 >= pair.len) {
                    return phchEmptyHash();
                }
                const k = pair[0..eqPos];
                const v = pair[(eqPos + 1)..pair.len];
                if (!phchIsValidParamKey(k) || !phchIsValidParamValue(v)) {
                    return phchEmptyHash();
                }
                result.params[result.param_count] = PhcParam{ key: k, value: v };
                result.param_count += 1;
                pStart = pi + 1;
            }
            pi += 1;
        }
        idx += 1;
    }

    // Optional salt
    if (idx < segCount) {
        const saltSeg = segs[idx];
        if (!phchIsValidSalt(saltSeg)) {
            return phchEmptyHash();
        }
        result.salt = saltSeg;
        idx += 1;
    }

    // Optional hash (only valid if salt was present)
    if (idx < segCount) {
        const hashSeg = segs[idx];
        if (!phchIsValidHash(hashSeg)) {
            return phchEmptyHash();
        }
        result.hash = hashSeg;
        idx += 1;
    }

    // No extra trailing segments allowed
    if (idx != segCount) {
        return phchEmptyHash();
    }

    result.valid = true;
    return result;
}

/// Serialize a `PhcHash` back into standard PHC string format.
/// Returns bytes written, or 0 if `h` is invalid or `out` is too small.
pub fn formatPhc(h: PhcHash, out: []u8): usize {
    if (!h.valid || h.algorithm.len == 0 || h.param_count > 8) {
        return 0;
    }
    if (h.hash.len > 0 && h.salt.len == 0) {
        return 0;
    }
    if (out.len < 1) {
        return 0;
    }
    let pos: usize = 0;
    out[pos] = '$' as u8;
    pos += 1;
    pos = phchAppendBytes(out, pos, h.algorithm);
    if (pos == 0) {
        return 0;
    }

    if (h.has_version) {
        pos = phchAppendBytes(out, pos, "$v=");
        if (pos == 0) {
            return 0;
        }
        pos = phchAppendU32(out, pos, h.version);
        if (pos == 0) {
            return 0;
        }
    }

    if (h.param_count > 0) {
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = '$' as u8;
        pos += 1;
        let i: usize = 0;
        while (i < h.param_count) {
            if (i > 0) {
                if (pos >= out.len) {
                    return 0;
                }
                out[pos] = ',' as u8;
                pos += 1;
            }
            pos = phchAppendBytes(out, pos, h.params[i].key);
            if (pos == 0 || pos >= out.len) {
                return 0;
            }
            out[pos] = '=' as u8;
            pos += 1;
            pos = phchAppendBytes(out, pos, h.params[i].value);
            if (pos == 0) {
                return 0;
            }
            i += 1;
        }
    }

    if (h.salt.len > 0) {
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = '$' as u8;
        pos += 1;
        pos = phchAppendBytes(out, pos, h.salt);
        if (pos == 0) {
            return 0;
        }
        if (h.hash.len > 0) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = '$' as u8;
            pos += 1;
            pos = phchAppendBytes(out, pos, h.hash);
            if (pos == 0) {
                return 0;
            }
        }
    }

    return pos;
}

/// Look up a parameter value by key in a parsed `PhcHash`.
/// Returns `null` if `h` is invalid or the key is not found.
pub fn getPhcParam(h: PhcHash, key: string): string? {
    if (!h.valid) {
        return null;
    }
    const limit: usize = if (h.param_count <= 8) h.param_count else 8;
    let i: usize = 0;
    while (i < limit) {
        if (phchStrEq(h.params[i].key, key)) {
            return h.params[i].value;
        }
        i += 1;
    }
    return null;
}

/// Constant-time byte-slice equality comparison for password hash verification.
pub fn ctEqual(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let acc: PhcVec16 = @splat(0 as u8);
    let i: usize = 0;
    const len = a.len;
    while (i + 16 <= len) {
        const va: PhcVec16 = a[i .. i + 16][0..16].*;
        const vb: PhcVec16 = b[i .. i + 16][0..16].*;
        acc = acc | (va ^ vb);
        i += 16;
    }
    const lanes: [16]u8 = acc;
    let diff: u8 = 0;
    let k: usize = 0;
    while (k < 16) {
        diff = diff | lanes[k];
        k += 1;
    }
    while (i < len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return diff == (0 as u8);
}

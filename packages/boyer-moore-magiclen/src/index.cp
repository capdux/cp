// @cap/bmh-search — Zero-heap Boyer-Moore-Horspool sublinear substring search
// MIT License (clean-room CP implementation)
// Accelerated with 16-lane `@Vector(16, u8)` SIMD byte scanning & block comparison,
// 4-lane `@Vector(4, usize)` shift-table fill, and comptime 256-entry ASCII lowercase LUT.

type BmhVec16 = @Vector(16, u8);
type BmhBool16 = @Vector(16, bool);
type BmhVec4Usize = @Vector(4, usize);

fn buildToLowerLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        if (i >= 65 && i <= 90) {
            lut[i] = (i + 32) as u8;
        } else {
            lut[i] = i as u8;
        }
        i += 1;
    }
    return lut;
}

const BMH_LOWER_LUT: [256]u8 = buildToLowerLut();

fn fillShiftTable(tbl: []usize, val: usize) {
    const vv: BmhVec4Usize = @splat(val);
    const v: [4]usize = vv;
    let i: usize = 0;
    while (i < 256) {
        @memcpy(tbl[i .. i + 4], v[0..4]);
        i += 4;
    }
}

fn slicesEqSimd(a: []const u8, b: []const u8) -> bool {
    let i: usize = 0;
    const n: usize = a.len;
    while (i + 16 <= n) {
        const va: BmhVec16 = a[i .. i + 16][0..16].*;
        const vb: BmhVec16 = b[i .. i + 16][0..16].*;
        if (!@reduce(.And, va == vb)) {
            return false;
        }
        i += 16;
    }
    while (i < n) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn slicesEqIgnoreCaseSimd(a: string, b: string) -> bool {
    let i: usize = 0;
    const n: usize = a.len;
    const v_a: BmhVec16 = @splat(65 as u8);
    const v_z: BmhVec16 = @splat(90 as u8);
    const v_bit: BmhVec16 = @splat(32 as u8);
    const v_false: BmhBool16 = @splat(false);
    while (i + 16 <= n) {
        const va: BmhVec16 = a[i .. i + 16][0..16].*;
        const vb: BmhVec16 = b[i .. i + 16][0..16].*;
        const la: BmhVec16 = @select(u8, @select(bool, va >= v_a, va <= v_z, v_false), va | v_bit, va);
        const lb: BmhVec16 = @select(u8, @select(bool, vb >= v_a, vb <= v_z, v_false), vb | v_bit, vb);
        if (!@reduce(.And, la == lb)) {
            return false;
        }
        i += 16;
    }
    while (i < n) {
        if (BMH_LOWER_LUT[a[i] as usize] != BMH_LOWER_LUT[b[i] as usize]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByteSimd(haystack: []const u8, target: u8) -> i64 {
    const vt: BmhVec16 = @splat(target);
    let i: usize = 0;
    while (i + 16 <= haystack.len) {
        const v: BmhVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (haystack[i + k] == target) {
                    return (i + k) as i64;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < haystack.len) {
        if (haystack[i] == target) {
            return i as i64;
        }
        i += 1;
    }
    return -1;
}

fn findLastByteSimd(haystack: []const u8, target: u8) -> i64 {
    const vt: BmhVec16 = @splat(target);
    let end: usize = haystack.len;
    while (end >= 16) {
        const base: usize = end - 16;
        const v: BmhVec16 = haystack[base..end][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                if (haystack[base + k] == target) {
                    return (base + k) as i64;
                }
            }
        }
        end = base;
    }
    while (end > 0) {
        end -= 1;
        if (haystack[end] == target) {
            return end as i64;
        }
    }
    return -1;
}

pub struct BmhPattern {
    pub shift: [256]usize,
    pub pat_len: usize,
}

pub fn compilePattern(needle: []const u8) -> BmhPattern {
    let pat = BmhPattern {
        shift: undefined,
        pat_len: needle.len,
    };
    let def_shift: usize = needle.len;
    if (def_shift == 0) {
        def_shift = 1;
    }
    fillShiftTable(pat.shift[0..256], def_shift);
    if (needle.len > 1) {
        const last: usize = needle.len - 1;
        let i: usize = 0;
        while (i < last) {
            pat.shift[needle[i] as usize] = last - i;
            i += 1;
        }
    }
    return pat;
}

pub fn findWithCompiled(haystack: []const u8, needle: []const u8, compiled: BmhPattern) -> i64 {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len || compiled.pat_len != needle.len) {
        return -1;
    }
    const m: usize = needle.len;
    if (m == 1) {
        return findByteSimd(haystack, needle[0]);
    }
    const last: usize = m - 1;
    const last_byte: u8 = needle[last];
    const first_byte: u8 = needle[0];
    let pos: usize = 0;

    while (pos + m <= haystack.len) {
        const h_last: u8 = haystack[pos + last];
        if (h_last == last_byte && haystack[pos] == first_byte) {
            if (m <= 2 || slicesEqSimd(haystack[pos + 1 .. pos + last], needle[1..last])) {
                return pos as i64;
            }
        }
        pos += compiled.shift[h_last as usize];
    }
    return -1;
}

pub fn findFirst(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return -1;
    }
    if (needle.len == 1) {
        return findByteSimd(haystack, needle[0]);
    }
    const compiled: BmhPattern = compilePattern(needle);
    return findWithCompiled(haystack, needle, compiled);
}

pub fn findLast(haystack: []const u8, needle: []const u8) -> i64 {
    if (needle.len == 0) {
        return haystack.len as i64;
    }
    if (needle.len > haystack.len) {
        return -1;
    }
    const m: usize = needle.len;
    if (m == 1) {
        return findLastByteSimd(haystack, needle[0]);
    }

    // Reverse Horspool shift table indexed by haystack[pos] (first char of window)
    let rshift: [256]usize = undefined;
    fillShiftTable(rshift[0..256], m);
    let k: usize = m;
    while (k > 1) {
        k -= 1;
        rshift[needle[k] as usize] = k;
    }

    const first_byte: u8 = needle[0];
    const last_byte: u8 = needle[m - 1];
    let pos: usize = haystack.len - m;
    while (true) {
        const h_first: u8 = haystack[pos];
        if (h_first == first_byte && haystack[pos + m - 1] == last_byte) {
            if (m <= 2 || slicesEqSimd(haystack[pos + 1 .. pos + m - 1], needle[1 .. m - 1])) {
                return pos as i64;
            }
        }
        const step: usize = rshift[h_first as usize];
        if (pos < step) {
            break;
        }
        pos -= step;
    }
    return -1;
}

pub fn countNonOverlapping(haystack: []const u8, needle: []const u8) -> usize {
    if (needle.len == 0 || needle.len > haystack.len) {
        return 0;
    }
    const compiled: BmhPattern = compilePattern(needle);
    let count: usize = 0;
    let pos: usize = 0;
    while (pos + needle.len <= haystack.len) {
        const idx: i64 = findWithCompiled(haystack[pos..haystack.len], needle, compiled);
        if (idx < 0) {
            break;
        }
        count += 1;
        pos += (idx as usize) + needle.len;
    }
    return count;
}

pub fn findIgnoreCaseAscii(haystack: string, needle: string) -> i64 {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return -1;
    }
    const m: usize = needle.len;
    const last: usize = m - 1;

    let shift: [256]usize = undefined;
    fillShiftTable(shift[0..256], m);
    if (m > 1) {
        let i: usize = 0;
        while (i < last) {
            const lc: u8 = BMH_LOWER_LUT[needle[i] as usize];
            shift[lc as usize] = last - i;
            if (lc >= 'a' as u8 && lc <= 'z' as u8) {
                shift[(lc - 32) as usize] = last - i;
            }
            i += 1;
        }
    }

    const last_lc: u8 = BMH_LOWER_LUT[needle[last] as usize];
    const first_lc: u8 = BMH_LOWER_LUT[needle[0] as usize];
    let pos: usize = 0;
    while (pos + m <= haystack.len) {
        const h_last: u8 = haystack[pos + last];
        if (BMH_LOWER_LUT[h_last as usize] == last_lc && BMH_LOWER_LUT[haystack[pos] as usize] == first_lc) {
            if (m <= 2 || slicesEqIgnoreCaseSimd(haystack[pos + 1 .. pos + last], needle[1..last])) {
                return pos as i64;
            }
        }
        pos += shift[h_last as usize];
    }
    return -1;
}

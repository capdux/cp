import { countByte } from "../../memchr/src/index.cp";

type BcVec16 = @Vector(16, u8);

fn bcSumVec(acc: BcVec16): usize {
    const a: [16]u8 = acc;
    let s: usize = 0;
    let k: usize = 0;
    while (k < 16) {
        s = s + (a[k] as usize);
        k = k + 1;
    }
    return s;
}

pub fn count(haystack: []const u8, needle: u8): usize {
    return countByte(needle, haystack);
}

// 16-lane hardware SIMD byte counting loop
pub fn countFast(haystack: []const u8, needle: u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    const v_needle: BcVec16 = @splat(needle);
    const v_one: BcVec16 = @splat(1 as u8);
    const v_zero: BcVec16 = @splat(0 as u8);
    while (i + 16 <= haystack.len) {
        let acc: BcVec16 = v_zero;
        let step: usize = 0;
        while (step < 255 && i + 16 <= haystack.len) {
            const chunk: BcVec16 = haystack[i .. i + 16][0..16].*;
            acc = acc + @select(u8, chunk == v_needle, v_one, v_zero);
            i = i + 16;
            step = step + 1;
        }
        total = total + bcSumVec(acc);
    }
    while (i < haystack.len) {
        if (haystack[i] == needle) { total = total + 1; }
        i = i + 1;
    }
    return total;
}

// 16-lane SIMD UTF-8 Unicode scalar value counter (bytes where (b & 0xC0) != 0x80)
pub fn numChars(utf8_bytes: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    const v_mask: BcVec16 = @splat(192 as u8);
    const v_cont: BcVec16 = @splat(128 as u8);
    const v_one: BcVec16 = @splat(1 as u8);
    const v_zero: BcVec16 = @splat(0 as u8);
    while (i + 16 <= utf8_bytes.len) {
        let acc: BcVec16 = v_zero;
        let step: usize = 0;
        while (step < 255 && i + 16 <= utf8_bytes.len) {
            const chunk: BcVec16 = utf8_bytes[i .. i + 16][0..16].*;
            acc = acc + @select(u8, (chunk & v_mask) != v_cont, v_one, v_zero);
            i = i + 16;
            step = step + 1;
        }
        total = total + bcSumVec(acc);
    }
    while (i < utf8_bytes.len) {
        if ((utf8_bytes[i] & 192) != 128) {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

pub fn countLines(buf: []const u8): usize {
    if (buf.len == 0) { return 0; }
    let nls = countFast(buf, 10); // '\n'
    if (buf[buf.len - 1] == 10) {
        return nls;
    }
    return nls + 1;
}

pub fn countAsciiWhitespace(buf: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < buf.len) {
        let b = buf[i];
        if (b == 32 || b == 9 || b == 10 || b == 13 || b == 12) {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

pub fn countInRange(buf: []const u8, lo: u8, hi: u8): usize {
    if (lo > hi) { return 0; }
    let total: usize = 0;
    let i: usize = 0;
    const v_lo: BcVec16 = @splat(lo);
    const v_hi: BcVec16 = @splat(hi);
    const v_one: BcVec16 = @splat(1 as u8);
    const v_zero: BcVec16 = @splat(0 as u8);
    while (i + 16 <= buf.len) {
        let acc: BcVec16 = v_zero;
        let step: usize = 0;
        while (step < 255 && i + 16 <= buf.len) {
            const chunk: BcVec16 = buf[i .. i + 16][0..16].*;
            const ge_lo = @select(u8, chunk >= v_lo, v_one, v_zero);
            const le_hi = @select(u8, chunk <= v_hi, v_one, v_zero);
            acc = acc + (ge_lo & le_hi);
            i = i + 16;
            step = step + 1;
        }
        total = total + bcSumVec(acc);
    }
    while (i < buf.len) {
        let b = buf[i];
        if (b >= lo && b <= hi) {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

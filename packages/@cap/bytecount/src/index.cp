import { countByte } from "../../memchr/src/index.cp";

pub fn count(haystack: []u8, needle: u8): usize {
    return countByte(needle, haystack);
}

// Fast SWAR-unrolled 4-byte counting loop
pub fn countFast(haystack: []u8, needle: u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i + 4 <= haystack.len) {
        if (haystack[i] == needle) { total = total + 1; }
        if (haystack[i + 1] == needle) { total = total + 1; }
        if (haystack[i + 2] == needle) { total = total + 1; }
        if (haystack[i + 3] == needle) { total = total + 1; }
        i = i + 4;
    }
    while (i < haystack.len) {
        if (haystack[i] == needle) { total = total + 1; }
        i = i + 1;
    }
    return total;
}

// Counts UTF-8 Unicode scalar values (bytes where (b & 0xC0) != 0x80)
pub fn numChars(utf8_bytes: []u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < utf8_bytes.len) {
        if ((utf8_bytes[i] & 192) != 128) {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

pub fn countLines(buf: []u8): usize {
    if (buf.len == 0) { return 0; }
    let nls = countByte(10, buf); // '\n'
    if (buf[buf.len - 1] == 10) {
        return nls;
    }
    return nls + 1;
}

pub fn countAsciiWhitespace(buf: []u8): usize {
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

pub fn countInRange(buf: []u8, lo: u8, hi: u8): usize {
    if (lo > hi) { return 0; }
    let total: usize = 0;
    let i: usize = 0;
    while (i < buf.len) {
        let b = buf[i];
        if (b >= lo && b <= hi) {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

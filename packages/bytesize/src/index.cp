// @cap/bytesize — Zero-heap integer byte size constants, parser & formatter
// Clean-room pure CP counterpart to crates.io:bytesize

import { isSpace, isDigit, toLowerByte, trim } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

pub const B: u64 = 1;

// SI (decimal, base 1000)
pub const KB: u64 = 1000;
pub const MB: u64 = 1000000;
pub const GB: u64 = 1000000000;
pub const TB: u64 = 1000000000000;
pub const PB: u64 = 1000000000000000;

// IEC (binary, base 1024)
pub const KIB: u64 = 1024;
pub const MIB: u64 = 1048576;
pub const GIB: u64 = 1073741824;
pub const TIB: u64 = 1099511627776;
pub const PIB: u64 = 1125899906842624;

const BSZ_MAX_U64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

fn bszWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn bszUnitMultiplier(s: string): u64? {
    if (s.len == 0) {
        return B;
    }
    if (s.len == 1) {
        const c0 = toLowerByte(s[0]);
        if (c0 == 0x62) return B;   // b
        if (c0 == 0x6B) return KB;  // k
        if (c0 == 0x6D) return MB;  // m
        if (c0 == 0x67) return GB;  // g
        if (c0 == 0x74) return TB;  // t
        if (c0 == 0x70) return PB;  // p
        return null;
    }
    if (s.len == 2) {
        const c0 = toLowerByte(s[0]);
        const c1 = toLowerByte(s[1]);
        if (c1 == 0x62) { // *b
            if (c0 == 0x6B) return KB;
            if (c0 == 0x6D) return MB;
            if (c0 == 0x67) return GB;
            if (c0 == 0x74) return TB;
            if (c0 == 0x70) return PB;
            return null;
        }
        if (c1 == 0x69) { // *i
            if (c0 == 0x6B) return KIB;
            if (c0 == 0x6D) return MIB;
            if (c0 == 0x67) return GIB;
            if (c0 == 0x74) return TIB;
            if (c0 == 0x70) return PIB;
            return null;
        }
        return null;
    }
    if (s.len == 3) {
        const c0 = toLowerByte(s[0]);
        const c1 = toLowerByte(s[1]);
        const c2 = toLowerByte(s[2]);
        if (c1 == 0x69 && c2 == 0x62) { // *ib
            if (c0 == 0x6B) return KIB;
            if (c0 == 0x6D) return MIB;
            if (c0 == 0x67) return GIB;
            if (c0 == 0x74) return TIB;
            if (c0 == 0x70) return PIB;
        }
        return null;
    }
    return null;
}

fn bszFormatScaled(bytes: u64, unit: u64, suffix: string, out: []u8): usize {
    const tenths128 = ((bytes as u128) * (10 as u128) + ((unit as u128) / (2 as u128))) / (unit as u128);
    const whole = (tenths128 / (10 as u128)) as u64;
    const frac = (tenths128 % (10 as u128)) as u8;
    let pos: usize = 0;
    pos = writeU64(whole, out, pos);
    pos = bszWriteByte(out, pos, 0x2E);
    pos = bszWriteByte(out, pos, (0x30 as u8) + frac);
    pos = bszWriteByte(out, pos, 0x20);
    pos = writeStr(suffix, out, pos);
    return pos;
}

/// Format `bytes` using SI decimal units (`B`, `KB`, `MB`, `GB`, `TB`, `PB`).
pub fn formatSi(bytes: u64, out: []u8): usize {
    if (bytes < KB) {
        let pos: usize = 0;
        pos = writeU64(bytes, out, pos);
        pos = writeStr(" B", out, pos);
        return pos;
    }
    if (bytes < MB) {
        return bszFormatScaled(bytes, KB, "KB", out);
    }
    if (bytes < GB) {
        return bszFormatScaled(bytes, MB, "MB", out);
    }
    if (bytes < TB) {
        return bszFormatScaled(bytes, GB, "GB", out);
    }
    if (bytes < PB) {
        return bszFormatScaled(bytes, TB, "TB", out);
    }
    return bszFormatScaled(bytes, PB, "PB", out);
}

/// Format `bytes` using IEC binary units (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`).
pub fn formatIec(bytes: u64, out: []u8): usize {
    if (bytes < KIB) {
        let pos: usize = 0;
        pos = writeU64(bytes, out, pos);
        pos = writeStr(" B", out, pos);
        return pos;
    }
    if (bytes < MIB) {
        return bszFormatScaled(bytes, KIB, "KiB", out);
    }
    if (bytes < GIB) {
        return bszFormatScaled(bytes, MIB, "MiB", out);
    }
    if (bytes < TIB) {
        return bszFormatScaled(bytes, GIB, "GiB", out);
    }
    if (bytes < PIB) {
        return bszFormatScaled(bytes, TIB, "TiB", out);
    }
    return bszFormatScaled(bytes, PIB, "PiB", out);
}

/// Parse a human-readable byte size string (e.g. `"1.5 GiB"`, `"500MB"`, `"2.5 kb"`, `"1024"`).
/// Returns `null` on invalid syntax, unknown unit, or u64 overflow.
pub fn parseByteSize(input: string): u64? {
    const s = trim(input);
    if (s.len == 0) {
        return null;
    }

    let i: usize = 0;
    const end = s.len;
    if (!isDigit(s[i])) {
        return null;
    }

    let whole: u64 = 0;
    while (i < end && isDigit(s[i])) {
        const d = (s[i] - 0x30) as u64;
        if (whole > (BSZ_MAX_U64 - d) / 10) {
            return null;
        }
        whole = whole * 10 + d;
        i += 1;
    }

    let fracNum: u64 = 0;
    let fracScale: u64 = 1;
    if (i < end && s[i] == 0x2E) {
        i += 1;
        if (i >= end || !isDigit(s[i])) {
            return null;
        }
        let fracDigits: usize = 0;
        while (i < end && isDigit(s[i])) {
            if (fracDigits < 9) {
                const d = (s[i] - 0x30) as u64;
                fracNum = fracNum * 10 + d;
                fracScale = fracScale * 10;
                fracDigits += 1;
            }
            i += 1;
        }
    }

    while (i < end && isSpace(s[i])) {
        i += 1;
    }

    const unitOpt = bszUnitMultiplier(s[i..end]);
    let unit: u64 = 1;
    if (u := unitOpt) {
        unit = u;
    } else {
        return null;
    }

    const whole128 = (whole as u128) * (unit as u128);
    const frac128 = ((fracNum as u128) * (unit as u128) + ((fracScale as u128) / (2 as u128))) / (fracScale as u128);
    const total128 = whole128 + frac128;
    if (total128 > (BSZ_MAX_U64 as u128)) {
        return null;
    }
    return total128 as u64;
}

/// Zero-heap value-type byte size wrapper.
pub struct ByteSize {
    val: u64,

    static fn b(n: u64): ByteSize {
        return ByteSize{ val: n };
    }

    static fn kb(n: u64): ByteSize {
        return ByteSize{ val: n * KB };
    }

    static fn mb(n: u64): ByteSize {
        return ByteSize{ val: n * MB };
    }

    static fn gb(n: u64): ByteSize {
        return ByteSize{ val: n * GB };
    }

    static fn tb(n: u64): ByteSize {
        return ByteSize{ val: n * TB };
    }

    static fn pb(n: u64): ByteSize {
        return ByteSize{ val: n * PB };
    }

    static fn kib(n: u64): ByteSize {
        return ByteSize{ val: n * KIB };
    }

    static fn mib(n: u64): ByteSize {
        return ByteSize{ val: n * MIB };
    }

    static fn gib(n: u64): ByteSize {
        return ByteSize{ val: n * GIB };
    }

    static fn tib(n: u64): ByteSize {
        return ByteSize{ val: n * TIB };
    }

    static fn pib(n: u64): ByteSize {
        return ByteSize{ val: n * PIB };
    }

    fn asU64(): u64 {
        return _.val;
    }

    fn add(other: ByteSize): ByteSize {
        return ByteSize{ val: _.val + other.val };
    }

    fn subSaturating(other: ByteSize): ByteSize {
        if (_.val <= other.val) {
            return ByteSize{ val: 0 };
        }
        return ByteSize{ val: _.val - other.val };
    }

    fn displaySi(out: []u8): usize {
        return formatSi(_.val, out);
    }

    fn displayIec(out: []u8): usize {
        return formatIec(_.val, out);
    }
}

// @cap/bytes — Unified Zero-Heap Byte Size Parser/Formatter & Byte Buffer Cursors
// Clean-room MIT superset merging npm `bytes` (IEC/SI size parse & format) and crates.io `bytes` (`BytesReader` / `BytesMut`).

import {
    readU16BE,
    readU16LE,
    readU32BE,
    readU32LE,
    readU64BE,
    readU64LE,
    writeU16BE,
    writeU16LE,
    writeU32BE,
    writeU32LE,
    writeU64BE,
    writeU64LE,
} from "cap:bin";
import { writeU64, writeStr } from "cap:fmt";
import { toLowerByte, isSpace, isDigit } from "cap:ascii";

// ============================================================================
// Part 1: npm `bytes` / Rust `bytesize` — Human-readable byte size parse/format
// ============================================================================

pub const KIB: u64 = 1024;
pub const MIB: u64 = 1048576;
pub const GIB: u64 = 1073741824;
pub const TIB: u64 = 1099511627776;
pub const PIB: u64 = 1125899906842624;

pub const KB: u64 = 1000;
pub const MB: u64 = 1000000;
pub const GB: u64 = 1000000000;
pub const TB: u64 = 1000000000000;
pub const PB: u64 = 1000000000000000;

fn bytUnitMultiplier(unit: string, binary_default: bool): u64 {
    if (unit.len == 0) {
        return 1;
    }
    let c0: u8 = toLowerByte(unit[0]);
    if (unit.len == 1) {
        if (c0 == 'b') { return 1; }
        if (c0 == 'k') { if (binary_default) { return KIB; } return KB; }
        if (c0 == 'm') { if (binary_default) { return MIB; } return MB; }
        if (c0 == 'g') { if (binary_default) { return GIB; } return GB; }
        if (c0 == 't') { if (binary_default) { return TIB; } return TB; }
        if (c0 == 'p') { if (binary_default) { return PIB; } return PB; }
        return 0;
    }
    let c1: u8 = toLowerByte(unit[1]);
    if (unit.len == 2 && c1 == 'b') {
        if (c0 == 'k') { if (binary_default) { return KIB; } return KB; }
        if (c0 == 'm') { if (binary_default) { return MIB; } return MB; }
        if (c0 == 'g') { if (binary_default) { return GIB; } return GB; }
        if (c0 == 't') { if (binary_default) { return TIB; } return TB; }
        if (c0 == 'p') { if (binary_default) { return PIB; } return PB; }
        return 0;
    }
    if (unit.len == 3 && c1 == 'i' && toLowerByte(unit[2]) == 'b') {
        if (c0 == 'k') { return KIB; }
        if (c0 == 'm') { return MIB; }
        if (c0 == 'g') { return GIB; }
        if (c0 == 't') { return TIB; }
        if (c0 == 'p') { return PIB; }
        return 0;
    }
    return 0;
}

fn bytU64Len(val: u64): usize {
    if (val == 0) { return 1; }
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        v = v / 10;
        n += 1;
    }
    return n;
}

/// Parse a human-readable byte string such as `"100KB"`, `"1.5 GiB"`, `"512mb"`, `"2048"`.
/// When `binary_default` is true (npm `bytes` compatible), `KB`/`MB`/`GB` use base 1024;
/// when false (SI / Rust `bytesize` compatible), `KB`/`MB`/`GB` use base 1000 while `KiB`/`MiB` always use 1024.
/// Returns byte count >= 0 on success, or -1 on invalid/overflow input.
pub fn parseBytes(input: string, binary_default: bool): i64 {
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && isSpace(input[start])) {
        start += 1;
    }
    while (end > start && isSpace(input[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return -1;
    }

    let i: usize = start;
    let int_part: u64 = 0;
    let has_digit: bool = false;
    while (i < end && isDigit(input[i])) {
        has_digit = true;
        let d: u64 = (input[i] - '0') as u64;
        if (int_part > 900000000000000000) {
            return -1;
        }
        int_part = int_part * 10 + d;
        i += 1;
    }

    let frac_part: u64 = 0;
    let frac_scale: u64 = 1;
    if (i < end && input[i] == '.') {
        i += 1;
        while (i < end && isDigit(input[i])) {
            has_digit = true;
            if (frac_scale < 1000000) {
                frac_part = frac_part * 10 + ((input[i] - '0') as u64);
                frac_scale = frac_scale * 10;
            }
            i += 1;
        }
    }

    if (!has_digit) {
        return -1;
    }

    while (i < end && isSpace(input[i])) {
        i += 1;
    }
    let unit: string = input[i..end];
    let mult: u64 = bytUnitMultiplier(unit, binary_default);
    if (mult == 0) {
        return -1;
    }

    let val_f: f64 = ((int_part as f64) + ((frac_part as f64) / (frac_scale as f64))) * (mult as f64);
    if (val_f > 9000000000000000000.0) {
        return -1;
    }
    return (val_f + 0.5) as i64;
}

/// Convenience wrapper matching npm `bytes.parse()` (base-1024 default for KB/MB/GB).
pub fn parse(input: string): i64 {
    return parseBytes(input, true);
}

/// Format a byte count into human-readable string (e.g. `"1.50 MiB"` or `"1.50 MB"`).
/// `binary = true` uses base 1024 (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`);
/// `binary = false` uses base 1000 (`B`, `KB`, `MB`, `GB`, `TB`, `PB`).
/// `decimals` specifies 0..3 fractional digits (ignored for bytes `< base`).
pub fn formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize {
    let base: u64 = if (binary) KIB else KB;
    if (bytes < base) {
        let need = bytU64Len(bytes) + 2;
        if (need > out.len) { return 0; }
        let pos = writeU64(bytes, out, 0);
        out[pos] = ' ';
        out[pos + 1] = 'B';
        return pos + 2;
    }

    let unit_idx: usize = 0;
    let divisor: u64 = 1;
    while (unit_idx < 5 && (bytes / divisor) >= base) {
        divisor = divisor * base;
        unit_idx += 1;
    }

    let dec_places: u32 = if (decimals > 3) 3 else decimals;
    let pow10: u64 = 1;
    let d: u32 = 0;
    while (d < dec_places) : (d += 1) {
        pow10 = pow10 * 10;
    }

    let scaled_f: f64 = ((bytes as f64) / (divisor as f64)) * (pow10 as f64) + 0.5;
    let scaled: u64 = scaled_f as u64;
    let whole: u64 = scaled / pow10;
    let frac: u64 = scaled % pow10;

    let whole_len = bytU64Len(whole);
    if (whole_len > out.len) { return 0; }
    let pos: usize = writeU64(whole, out, 0);

    if (dec_places > 0) {
        if (pos + 1 + (dec_places as usize) > out.len) { return 0; }
        out[pos] = '.';
        pos += 1;
        let rem: u64 = frac;
        let div10: u64 = pow10 / 10;
        let k: u32 = 0;
        while (k < dec_places) : (k += 1) {
            let dig: u64 = rem / div10;
            out[pos] = ('0' as u8) + (dig as u8);
            pos += 1;
            rem = rem % div10;
            if (div10 >= 10) {
                div10 = div10 / 10;
            }
        }
    }

    let suffix: string = "B";
    if (binary) {
        if (unit_idx == 1) { suffix = "KiB"; }
        if (unit_idx == 2) { suffix = "MiB"; }
        if (unit_idx == 3) { suffix = "GiB"; }
        if (unit_idx == 4) { suffix = "TiB"; }
        if (unit_idx == 5) { suffix = "PiB"; }
    } else {
        if (unit_idx == 1) { suffix = "KB"; }
        if (unit_idx == 2) { suffix = "MB"; }
        if (unit_idx == 3) { suffix = "GB"; }
        if (unit_idx == 4) { suffix = "TB"; }
        if (unit_idx == 5) { suffix = "PB"; }
    }

    if (pos + 1 + suffix.len > out.len) { return 0; }
    out[pos] = ' ';
    pos += 1;
    return writeStr(suffix, out, pos);
}

/// Format using IEC binary units (`KiB`, `MiB`, `GiB`) with 2 decimal places.
pub fn formatIec(bytes: u64, out: []u8): usize {
    return formatBytes(bytes, true, 2, out);
}

/// Format using SI decimal units (`KB`, `MB`, `GB`) with 2 decimal places.
pub fn formatSi(bytes: u64, out: []u8): usize {
    return formatBytes(bytes, false, 2, out);
}

// ============================================================================
// Part 2: crates.io `bytes` — Zero-copy byte buffer read/write cursors (Buf & BufMut)
// ============================================================================

pub const BYTES_MUT_CAP: usize = 512;

pub struct ReadU8Result { ok: bool, val: u8 }
pub struct ReadU16Result { ok: bool, val: u16 }
pub struct ReadU32Result { ok: bool, val: u32 }
pub struct ReadU64Result { ok: bool, val: u64 }

fn bytbCopyBytes(dst: []u8, src: []const u8, n: usize) {
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Zero-copy read cursor over an immutable byte slice (mirrors `bytes::Buf`).
pub struct BytesReader {
    data: []const u8,
    pos: usize,

    static fn new(data: []const u8): BytesReader {
        return BytesReader { data: data, pos: 0 };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) { return 0; }
        return _.data.len - _.pos;
    }

    fn hasRemaining(): bool {
        return _.pos < _.data.len;
    }

    fn position(): usize {
        return _.pos;
    }

    fn advance(*_, cnt: usize): bool {
        if (cnt > _.remaining()) { return false; }
        _.pos = _.pos + cnt;
        return true;
    }

    fn getU8(*_): ReadU8Result {
        if (_.remaining() < 1) { return ReadU8Result { ok: false, val: 0 }; }
        let v = _.data[_.pos];
        _.pos = _.pos + 1;
        return ReadU8Result { ok: true, val: v };
    }

    fn getU16Be(*_): ReadU16Result {
        if (v := readU16BE(_.data, _.pos)) {
            _.pos = _.pos + 2;
            return ReadU16Result { ok: true, val: v };
        }
        return ReadU16Result { ok: false, val: 0 };
    }

    fn getU16Le(*_): ReadU16Result {
        if (v := readU16LE(_.data, _.pos)) {
            _.pos = _.pos + 2;
            return ReadU16Result { ok: true, val: v };
        }
        return ReadU16Result { ok: false, val: 0 };
    }

    fn getU32Be(*_): ReadU32Result {
        if (v := readU32BE(_.data, _.pos)) {
            _.pos = _.pos + 4;
            return ReadU32Result { ok: true, val: v };
        }
        return ReadU32Result { ok: false, val: 0 };
    }

    fn getU32Le(*_): ReadU32Result {
        if (v := readU32LE(_.data, _.pos)) {
            _.pos = _.pos + 4;
            return ReadU32Result { ok: true, val: v };
        }
        return ReadU32Result { ok: false, val: 0 };
    }

    fn getU64Be(*_): ReadU64Result {
        if (v := readU64BE(_.data, _.pos)) {
            _.pos = _.pos + 8;
            return ReadU64Result { ok: true, val: v };
        }
        return ReadU64Result { ok: false, val: 0 };
    }

    fn getU64Le(*_): ReadU64Result {
        if (v := readU64LE(_.data, _.pos)) {
            _.pos = _.pos + 8;
            return ReadU64Result { ok: true, val: v };
        }
        return ReadU64Result { ok: false, val: 0 };
    }

    fn copyToSlice(*_, dst: []u8): bool {
        if (dst.len > _.remaining()) { return false; }
        bytbCopyBytes(dst, _.data[_.pos..(_.pos + dst.len)], dst.len);
        _.pos = _.pos + dst.len;
        return true;
    }
}

/// Zero-heap mutable byte buffer (capacity 512, mirrors `bytes::BytesMut` / `BufMut`).
pub struct BytesMut {
    buf: [512]u8,
    used: usize,

    static fn new(): BytesMut {
        return BytesMut { buf: undefined, used: 0 };
    }

    fn len(): usize { return _.used; }
    fn capacity(): usize { return BYTES_MUT_CAP; }
    fn remainingMut(): usize { return BYTES_MUT_CAP - _.used; }
    fn isEmpty(): bool { return _.used == 0; }

    fn clear(*_) { _.used = 0; }

    fn truncate(*_, new_len: usize) {
        if (new_len < _.used) { _.used = new_len; }
    }

    fn byteAt(idx: usize): u8 {
        if (idx >= _.used) { return 0; }
        return _.buf[idx];
    }

    fn putU8(*_, v: u8): bool {
        if (_.used + 1 > BYTES_MUT_CAP) { return false; }
        _.buf[_.used] = v;
        _.used = _.used + 1;
        return true;
    }

    fn putU16Be(*_, v: u16): bool {
        if (!writeU16BE(_.buf[0..BYTES_MUT_CAP], _.used, v)) { return false; }
        _.used = _.used + 2;
        return true;
    }

    fn putU16Le(*_, v: u16): bool {
        if (!writeU16LE(_.buf[0..BYTES_MUT_CAP], _.used, v)) { return false; }
        _.used = _.used + 2;
        return true;
    }

    fn putU32Be(*_, v: u32): bool {
        if (!writeU32BE(_.buf[0..BYTES_MUT_CAP], _.used, v)) { return false; }
        _.used = _.used + 4;
        return true;
    }

    fn putU32Le(*_, v: u32): bool {
        if (!writeU32LE(_.buf[0..BYTES_MUT_CAP], _.used, v)) { return false; }
        _.used = _.used + 4;
        return true;
    }

    fn putU64Be(*_, v: u64): bool {
        if (!writeU64BE(_.buf[0..BYTES_MUT_CAP], _.used, v)) { return false; }
        _.used = _.used + 8;
        return true;
    }

    fn putU64Le(*_, v: u64): bool {
        if (!writeU64LE(_.buf[0..BYTES_MUT_CAP], _.used, v)) { return false; }
        _.used = _.used + 8;
        return true;
    }

    fn putSlice(*_, src: []const u8): bool {
        if (_.used + src.len > BYTES_MUT_CAP) { return false; }
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            _.buf[_.used + i] = src[i];
        }
        _.used = _.used + src.len;
        return true;
    }

    /// Copies the first `at` bytes into `out`, shifting the remaining bytes to the front (mirrors `split_to`).
    fn splitTo(*_, at: usize, out: []u8): i64 {
        if (at > _.used || at > out.len) { return -1; }
        let i: usize = 0;
        while (i < at) : (i += 1) {
            out[i] = _.buf[i];
        }
        let rem = _.used - at;
        let j: usize = 0;
        while (j < rem) : (j += 1) {
            _.buf[j] = _.buf[at + j];
        }
        _.used = rem;
        return at as i64;
    }

    /// Copies all written bytes into `out` and returns written length.
    fn copyAll(out: []u8): usize {
        let n = if (_.used <= out.len) _.used else out.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = _.buf[i];
        }
        return n;
    }
}

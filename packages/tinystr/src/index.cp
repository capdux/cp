// packages/@cap/tinystr/src/index.cp
// Zero-allocation stack-inline fixed-capacity ASCII strings in pure CP.
// Clean-room MIT implementation (ICU4X tinystr v0.7/v0.8 parity).

import { toLowerByte, toUpperByte, isAlpha, isDigit, isAlnum, eqIgnoreCase } from "cap:ascii";
import { bytesEq } from "cap:bin";

pub const TINYSTR_OK: i64 = 0;
pub const TINYSTR_ERR_TOO_LONG: i64 = -1;
pub const TINYSTR_ERR_NON_ASCII: i64 = -2;
pub const TINYSTR_ERR_CONTAINS_NUL: i64 = -3;

fn tsIsLower(b: u8): bool {
    return b >= ('a' as u8) && b <= ('z' as u8);
}

fn tsIsUpper(b: u8): bool {
    return b >= ('A' as u8) && b <= ('Z' as u8);
}

fn tsValidateBytes(src: []const u8, max_len: usize): i64 {
    if (src.len > max_len) {
        return TINYSTR_ERR_TOO_LONG;
    }
    let i: usize = 0;
    while (i < src.len) {
        const ch = src[i];
        if (ch == (0 as u8)) {
            return TINYSTR_ERR_CONTAINS_NUL;
        }
        if (ch >= (0x80 as u8)) {
            return TINYSTR_ERR_NON_ASCII;
        }
        i = i + 1;
    }
    return TINYSTR_OK;
}

fn tsValidatePacked(buf: []const u8): i64 {
    let seen_nul = false;
    let i: usize = 0;
    while (i < buf.len) {
        const ch = buf[i];
        if (ch == (0 as u8)) {
            seen_nul = true;
        } else {
            if (seen_nul) {
                return TINYSTR_ERR_CONTAINS_NUL;
            }
            if (ch >= (0x80 as u8)) {
                return TINYSTR_ERR_NON_ASCII;
            }
        }
        i = i + 1;
    }
    return TINYSTR_OK;
}

fn tsSliceLen(buf: []const u8): usize {
    let i: usize = 0;
    while (i < buf.len && buf[i] != (0 as u8)) {
        i = i + 1;
    }
    return i;
}

fn tsCopyOut(src: []const u8, n: usize, out: []u8): usize {
    let take = n;
    if (out.len < take) {
        take = out.len;
    }
    let i: usize = 0;
    while (i < take) {
        out[i] = src[i];
        i = i + 1;
    }
    return take;
}

fn tsSliceCmp(a: []const u8, b: []const u8): i32 {
    let n = a.len;
    if (b.len < n) {
        n = b.len;
    }
    let i: usize = 0;
    while (i < n) {
        if (a[i] < b[i]) {
            return -1 as i32;
        }
        if (a[i] > b[i]) {
            return 1 as i32;
        }
        i = i + 1;
    }
    if (a.len < b.len) {
        return -1 as i32;
    }
    if (a.len > b.len) {
        return 1 as i32;
    }
    return 0 as i32;
}

fn tsAllAlpha(buf: []const u8, n: usize): bool {
    let i: usize = 0;
    while (i < n) {
        if (!isAlpha(buf[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn tsAllAlnum(buf: []const u8, n: usize): bool {
    let i: usize = 0;
    while (i < n) {
        if (!isAlnum(buf[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn tsAllDigit(buf: []const u8, n: usize): bool {
    let i: usize = 0;
    while (i < n) {
        if (!isDigit(buf[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn tsAllLower(buf: []const u8, n: usize): bool {
    let i: usize = 0;
    while (i < n) {
        if (!tsIsLower(buf[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn tsAllUpper(buf: []const u8, n: usize): bool {
    let i: usize = 0;
    while (i < n) {
        if (!tsIsUpper(buf[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// 4-byte stack-inline ASCII string (`0x01..0x7F`, zero-padded), packable as `u32`.
pub struct TinyAsciiStr4 {
    pub bytes: [4]u8,

    pub static fn empty(): TinyAsciiStr4 {
        return TinyAsciiStr4 { bytes: [0; 4] };
    }

    pub static fn fromBytes(src: []const u8): TinyAsciiStr4Result {
        const err = tsValidateBytes(src, 4);
        if (err != TINYSTR_OK) {
            return TinyAsciiStr4Result { ok: false, value: TinyAsciiStr4.empty(), err: err };
        }
        let b: [4]u8 = [0; 4];
        let i: usize = 0;
        while (i < src.len) {
            b[i] = src[i];
            i = i + 1;
        }
        return TinyAsciiStr4Result { ok: true, value: TinyAsciiStr4 { bytes: b }, err: TINYSTR_OK };
    }

    pub static fn fromStr(s: string): TinyAsciiStr4Result {
        return TinyAsciiStr4.fromBytes(s);
    }

    pub static fn fromPackedU32(v: u32): TinyAsciiStr4Result {
        let b: [4]u8 = [
            (v & (0xFF as u32)) as u8,
            ((v >> (8 as u5)) & (0xFF as u32)) as u8,
            ((v >> (16 as u5)) & (0xFF as u32)) as u8,
            ((v >> (24 as u5)) & (0xFF as u32)) as u8,
        ];
        const err = tsValidatePacked(b[0..4]);
        if (err != TINYSTR_OK) {
            return TinyAsciiStr4Result { ok: false, value: TinyAsciiStr4.empty(), err: err };
        }
        return TinyAsciiStr4Result { ok: true, value: TinyAsciiStr4 { bytes: b }, err: TINYSTR_OK };
    }

    pub fn toPackedU32(): u32 {
        return (_.bytes[0] as u32)
            | ((_.bytes[1] as u32) << (8 as u5))
            | ((_.bytes[2] as u32) << (16 as u5))
            | ((_.bytes[3] as u32) << (24 as u5));
    }

    pub fn len(): usize {
        return tsSliceLen(_.bytes[0..4]);
    }

    pub fn isEmpty(): bool {
        return _.bytes[0] == (0 as u8);
    }

    pub fn asBytes(out: []u8): usize {
        return tsCopyOut(_.bytes[0..4], _.len(), out);
    }

    pub fn byteAt(idx: usize): u8 {
        if (idx >= 4) {
            return 0 as u8;
        }
        return _.bytes[idx];
    }

    pub fn eq(other: TinyAsciiStr4): bool {
        return bytesEq(_.bytes[0..4], other.bytes[0..4]);
    }

    pub fn cmp(other: TinyAsciiStr4): i32 {
        return tsSliceCmp(_.bytes[0.._.len()], other.bytes[0..other.len()]);
    }

    pub fn eqIgnoreAsciiCase(other: TinyAsciiStr4): bool {
        return eqIgnoreCase(_.bytes[0.._.len()], other.bytes[0..other.len()]);
    }

    pub fn toAsciiLowercase(): TinyAsciiStr4 {
        let b: [4]u8 = undefined;
        let i: usize = 0;
        while (i < 4) {
            b[i] = toLowerByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr4 { bytes: b };
    }

    pub fn toAsciiUppercase(): TinyAsciiStr4 {
        let b: [4]u8 = undefined;
        let i: usize = 0;
        while (i < 4) {
            b[i] = toUpperByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr4 { bytes: b };
    }

    pub fn toAsciiTitlecase(): TinyAsciiStr4 {
        let b: [4]u8 = undefined;
        b[0] = toUpperByte(_.bytes[0]);
        let i: usize = 1;
        while (i < 4) {
            b[i] = toLowerByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr4 { bytes: b };
    }

    pub fn isAsciiAlphabetic(): bool {
        return tsAllAlpha(_.bytes[0..4], _.len());
    }

    pub fn isAsciiAlphanumeric(): bool {
        return tsAllAlnum(_.bytes[0..4], _.len());
    }

    pub fn isAsciiNumeric(): bool {
        return tsAllDigit(_.bytes[0..4], _.len());
    }

    pub fn isAsciiLowercase(): bool {
        return tsAllLower(_.bytes[0..4], _.len());
    }

    pub fn isAsciiUppercase(): bool {
        return tsAllUpper(_.bytes[0..4], _.len());
    }
}

pub struct TinyAsciiStr4Result {
    pub ok: bool,
    pub value: TinyAsciiStr4,
    pub err: i64,
}

/// 8-byte stack-inline ASCII string (`0x01..0x7F`, zero-padded), packable as `u64`.
pub struct TinyAsciiStr8 {
    pub bytes: [8]u8,

    pub static fn empty(): TinyAsciiStr8 {
        return TinyAsciiStr8 { bytes: [0; 8] };
    }

    pub static fn fromBytes(src: []const u8): TinyAsciiStr8Result {
        const err = tsValidateBytes(src, 8);
        if (err != TINYSTR_OK) {
            return TinyAsciiStr8Result { ok: false, value: TinyAsciiStr8.empty(), err: err };
        }
        let b: [8]u8 = [0; 8];
        let i: usize = 0;
        while (i < src.len) {
            b[i] = src[i];
            i = i + 1;
        }
        return TinyAsciiStr8Result { ok: true, value: TinyAsciiStr8 { bytes: b }, err: TINYSTR_OK };
    }

    pub static fn fromStr(s: string): TinyAsciiStr8Result {
        return TinyAsciiStr8.fromBytes(s);
    }

    pub static fn fromPackedU64(v: u64): TinyAsciiStr8Result {
        let b: [8]u8 = [
            (v & (0xFF as u64)) as u8,
            ((v >> (8 as u6)) & (0xFF as u64)) as u8,
            ((v >> (16 as u6)) & (0xFF as u64)) as u8,
            ((v >> (24 as u6)) & (0xFF as u64)) as u8,
            ((v >> (32 as u6)) & (0xFF as u64)) as u8,
            ((v >> (40 as u6)) & (0xFF as u64)) as u8,
            ((v >> (48 as u6)) & (0xFF as u64)) as u8,
            ((v >> (56 as u6)) & (0xFF as u64)) as u8,
        ];
        const err = tsValidatePacked(b[0..8]);
        if (err != TINYSTR_OK) {
            return TinyAsciiStr8Result { ok: false, value: TinyAsciiStr8.empty(), err: err };
        }
        return TinyAsciiStr8Result { ok: true, value: TinyAsciiStr8 { bytes: b }, err: TINYSTR_OK };
    }

    pub fn toPackedU64(): u64 {
        return (_.bytes[0] as u64)
            | ((_.bytes[1] as u64) << (8 as u6))
            | ((_.bytes[2] as u64) << (16 as u6))
            | ((_.bytes[3] as u64) << (24 as u6))
            | ((_.bytes[4] as u64) << (32 as u6))
            | ((_.bytes[5] as u64) << (40 as u6))
            | ((_.bytes[6] as u64) << (48 as u6))
            | ((_.bytes[7] as u64) << (56 as u6));
    }

    pub fn len(): usize {
        return tsSliceLen(_.bytes[0..8]);
    }

    pub fn isEmpty(): bool {
        return _.bytes[0] == (0 as u8);
    }

    pub fn asBytes(out: []u8): usize {
        return tsCopyOut(_.bytes[0..8], _.len(), out);
    }

    pub fn byteAt(idx: usize): u8 {
        if (idx >= 8) {
            return 0 as u8;
        }
        return _.bytes[idx];
    }

    pub fn eq(other: TinyAsciiStr8): bool {
        return bytesEq(_.bytes[0..8], other.bytes[0..8]);
    }

    pub fn cmp(other: TinyAsciiStr8): i32 {
        return tsSliceCmp(_.bytes[0.._.len()], other.bytes[0..other.len()]);
    }

    pub fn eqIgnoreAsciiCase(other: TinyAsciiStr8): bool {
        return eqIgnoreCase(_.bytes[0.._.len()], other.bytes[0..other.len()]);
    }

    pub fn toAsciiLowercase(): TinyAsciiStr8 {
        let b: [8]u8 = undefined;
        let i: usize = 0;
        while (i < 8) {
            b[i] = toLowerByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr8 { bytes: b };
    }

    pub fn toAsciiUppercase(): TinyAsciiStr8 {
        let b: [8]u8 = undefined;
        let i: usize = 0;
        while (i < 8) {
            b[i] = toUpperByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr8 { bytes: b };
    }

    pub fn toAsciiTitlecase(): TinyAsciiStr8 {
        let b: [8]u8 = undefined;
        b[0] = toUpperByte(_.bytes[0]);
        let i: usize = 1;
        while (i < 8) {
            b[i] = toLowerByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr8 { bytes: b };
    }

    pub fn isAsciiAlphabetic(): bool {
        return tsAllAlpha(_.bytes[0..8], _.len());
    }

    pub fn isAsciiAlphanumeric(): bool {
        return tsAllAlnum(_.bytes[0..8], _.len());
    }

    pub fn isAsciiNumeric(): bool {
        return tsAllDigit(_.bytes[0..8], _.len());
    }

    pub fn isAsciiLowercase(): bool {
        return tsAllLower(_.bytes[0..8], _.len());
    }

    pub fn isAsciiUppercase(): bool {
        return tsAllUpper(_.bytes[0..8], _.len());
    }
}

pub struct TinyAsciiStr8Result {
    pub ok: bool,
    pub value: TinyAsciiStr8,
    pub err: i64,
}

/// 16-byte stack-inline ASCII string (`0x01..0x7F`, zero-padded).
pub struct TinyAsciiStr16 {
    pub bytes: [16]u8,

    pub static fn empty(): TinyAsciiStr16 {
        return TinyAsciiStr16 { bytes: [0; 16] };
    }

    pub static fn fromBytes(src: []const u8): TinyAsciiStr16Result {
        const err = tsValidateBytes(src, 16);
        if (err != TINYSTR_OK) {
            return TinyAsciiStr16Result { ok: false, value: TinyAsciiStr16.empty(), err: err };
        }
        let b: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < src.len) {
            b[i] = src[i];
            i = i + 1;
        }
        return TinyAsciiStr16Result { ok: true, value: TinyAsciiStr16 { bytes: b }, err: TINYSTR_OK };
    }

    pub static fn fromStr(s: string): TinyAsciiStr16Result {
        return TinyAsciiStr16.fromBytes(s);
    }

    pub fn len(): usize {
        return tsSliceLen(_.bytes[0..16]);
    }

    pub fn isEmpty(): bool {
        return _.bytes[0] == (0 as u8);
    }

    pub fn asBytes(out: []u8): usize {
        return tsCopyOut(_.bytes[0..16], _.len(), out);
    }

    pub fn byteAt(idx: usize): u8 {
        if (idx >= 16) {
            return 0 as u8;
        }
        return _.bytes[idx];
    }

    pub fn eq(other: TinyAsciiStr16): bool {
        return bytesEq(_.bytes[0..16], other.bytes[0..16]);
    }

    pub fn cmp(other: TinyAsciiStr16): i32 {
        return tsSliceCmp(_.bytes[0.._.len()], other.bytes[0..other.len()]);
    }

    pub fn eqIgnoreAsciiCase(other: TinyAsciiStr16): bool {
        return eqIgnoreCase(_.bytes[0.._.len()], other.bytes[0..other.len()]);
    }

    pub fn toAsciiLowercase(): TinyAsciiStr16 {
        let b: [16]u8 = undefined;
        let i: usize = 0;
        while (i < 16) {
            b[i] = toLowerByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr16 { bytes: b };
    }

    pub fn toAsciiUppercase(): TinyAsciiStr16 {
        let b: [16]u8 = undefined;
        let i: usize = 0;
        while (i < 16) {
            b[i] = toUpperByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr16 { bytes: b };
    }

    pub fn toAsciiTitlecase(): TinyAsciiStr16 {
        let b: [16]u8 = undefined;
        b[0] = toUpperByte(_.bytes[0]);
        let i: usize = 1;
        while (i < 16) {
            b[i] = toLowerByte(_.bytes[i]);
            i = i + 1;
        }
        return TinyAsciiStr16 { bytes: b };
    }

    pub fn isAsciiAlphabetic(): bool {
        return tsAllAlpha(_.bytes[0..16], _.len());
    }

    pub fn isAsciiAlphanumeric(): bool {
        return tsAllAlnum(_.bytes[0..16], _.len());
    }

    pub fn isAsciiNumeric(): bool {
        return tsAllDigit(_.bytes[0..16], _.len());
    }

    pub fn isAsciiLowercase(): bool {
        return tsAllLower(_.bytes[0..16], _.len());
    }

    pub fn isAsciiUppercase(): bool {
        return tsAllUpper(_.bytes[0..16], _.len());
    }
}

pub struct TinyAsciiStr16Result {
    pub ok: bool,
    pub value: TinyAsciiStr16,
    pub err: i64,
}

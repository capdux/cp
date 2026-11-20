// Source: bun/src/unicode/uucode_lib/src/ascii.cp -> cp/lib/unicode/props.cp (Unicode codepoint properties & case mapping)

const std = @import("std");
import { UnicodeError, decodeUtf8Step, decodeUtf8Lossy, encodeUtf8 } from "./utf.cp";

/// Returns true if `cp` is a 7-bit ASCII codepoint (U+0000..U+007F).
pub fn isAscii(cp: u32): bool {
    return cp < 0x80;
}

/// Returns true if `cp` is a C0 or C1 control character.
pub fn isControl(cp: u32): bool {
    return cp < 0x20 || (cp >= 0x7F && cp <= 0x9F);
}

/// Returns true if `cp` is ASCII or Unicode whitespace (Zs, line/paragraph separators, NEL, BOM).
pub fn isWhitespace(cp: u32): bool {
    if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == 0x0B || cp == 0x0C) {
        return true;
    }
    if (cp < 0x80) {
        return false;
    }
    return cp == 0x0085
        || cp == 0x00A0
        || cp == 0x1680
        || (cp >= 0x2000 && cp <= 0x200A)
        || cp == 0x2028
        || cp == 0x2029
        || cp == 0x202F
        || cp == 0x205F
        || cp == 0x3000
        || cp == 0xFEFF;
}

/// Returns true if `cp` is a valid identifier start character (ASCII + common Unicode letter blocks).
pub fn isIdentifierStart(cp: u32): bool {
    if (cp == 0 || cp > 0x10FFFF) {
        return false;
    }
    if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || cp == '_' || cp == '$') {
        return true;
    }
    if (cp < 0x80) {
        return false;
    }
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x02AF)) {
        return true;
    }
    if ((cp >= 0x0370 && cp <= 0x037D) || (cp >= 0x037F && cp <= 0x052F)) {
        return true;
    }
    if ((cp >= 0x0531 && cp <= 0x0556) || (cp >= 0x0560 && cp <= 0x0588) || (cp >= 0x05D0 && cp <= 0x05EA)) {
        return true;
    }
    if ((cp >= 0x0620 && cp <= 0x064A) || (cp >= 0x066E && cp <= 0x06D3) || (cp >= 0x0904 && cp <= 0x0939)) {
        return true;
    }
    if ((cp >= 0x1E00 && cp <= 0x1FFF) || cp == 0x2118 || cp == 0x212E || (cp >= 0x2160 && cp <= 0x2188)) {
        return true;
    }
    if (cp == 0x3005 || cp == 0x3006 || cp == 0x3007 || (cp >= 0x3021 && cp <= 0x3029)) {
        return true;
    }
    if ((cp >= 0x3041 && cp <= 0x3096) || (cp >= 0x30A1 && cp <= 0x30FA) || (cp >= 0x3105 && cp <= 0x312F)) {
        return true;
    }
    if ((cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xAC00 && cp <= 0xD7AF)) {
        return true;
    }
    if ((cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFF21 && cp <= 0xFF3A) || (cp >= 0xFF41 && cp <= 0xFF5A)) {
        return true;
    }
    if ((cp >= 0xFF66 && cp <= 0xFFBE) || (cp >= 0x20000 && cp <= 0x2EBEF)) {
        return true;
    }
    return false;
}

/// Returns true if `cp` is a valid identifier continuation character.
pub fn isIdentifierContinue(cp: u32): bool {
    if (cp == 0 || cp > 0x10FFFF) {
        return false;
    }
    if ((cp >= '0' && cp <= '9') || cp == 0x200C || cp == 0x200D) {
        return true;
    }
    if (isIdentifierStart(cp)) {
        return true;
    }
    if ((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x0660 && cp <= 0x0669)) {
        return true;
    }
    if (cp == 0x00B7 || cp == 0x203F || cp == 0x2040 || cp == 0xFF3F) {
        return true;
    }
    if ((cp >= 0xFF10 && cp <= 0xFF19) || (cp >= 0xFE00 && cp <= 0xFE0F)) {
        return true;
    }
    return false;
}

/// Maps an uppercase codepoint (ASCII, Latin-1, Latin Ext-A, Greek, Cyrillic, Fullwidth) to lowercase.
pub fn toLowerCodepoint(cp: u32): u32 {
    if (cp >= 'A' && cp <= 'Z') {
        return cp + 32;
    }
    if (cp < 0x80) {
        return cp;
    }
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) {
        return cp + 32;
    }
    if (cp >= 0x0100 && cp <= 0x012E && (cp & 1) == 0) {
        return cp + 1;
    }
    if (cp == 0x0130) {
        return 'i';
    }
    if (cp >= 0x0132 && cp <= 0x0136 && (cp & 1) == 0) {
        return cp + 1;
    }
    if (cp >= 0x0139 && cp <= 0x0147 && (cp & 1) == 1) {
        return cp + 1;
    }
    if (cp >= 0x014A && cp <= 0x0176 && (cp & 1) == 0) {
        return cp + 1;
    }
    if (cp == 0x0178) {
        return 0x00FF;
    }
    if (cp >= 0x0179 && cp <= 0x017D && (cp & 1) == 1) {
        return cp + 1;
    }
    if ((cp >= 0x0391 && cp <= 0x03A1) || (cp >= 0x03A3 && cp <= 0x03A9)) {
        return cp + 32;
    }
    if (cp >= 0x0410 && cp <= 0x042F) {
        return cp + 32;
    }
    if (cp >= 0x0400 && cp <= 0x040F) {
        return cp + 80;
    }
    if (cp >= 0xFF21 && cp <= 0xFF3A) {
        return cp + 32;
    }
    return cp;
}

/// Maps a lowercase codepoint (ASCII, Latin-1, Latin Ext-A, Greek, Cyrillic, Fullwidth) to uppercase.
pub fn toUpperCodepoint(cp: u32): u32 {
    if (cp >= 'a' && cp <= 'z') {
        return cp - 32;
    }
    if (cp < 0x80) {
        return cp;
    }
    if ((cp >= 0x00E0 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x00FE)) {
        return cp - 32;
    }
    if (cp >= 0x0101 && cp <= 0x012F && (cp & 1) == 1) {
        return cp - 1;
    }
    if (cp >= 0x0133 && cp <= 0x0137 && (cp & 1) == 1) {
        return cp - 1;
    }
    if (cp >= 0x013A && cp <= 0x0148 && (cp & 1) == 0) {
        return cp - 1;
    }
    if (cp >= 0x014B && cp <= 0x0177 && (cp & 1) == 1) {
        return cp - 1;
    }
    if (cp == 0x00FF) {
        return 0x0178;
    }
    if (cp >= 0x017A && cp <= 0x017E && (cp & 1) == 0) {
        return cp - 1;
    }
    if ((cp >= 0x03B1 && cp <= 0x03C1) || (cp >= 0x03C3 && cp <= 0x03C9)) {
        return cp - 32;
    }
    if (cp >= 0x0430 && cp <= 0x044F) {
        return cp - 32;
    }
    if (cp >= 0x0450 && cp <= 0x045F) {
        return cp - 80;
    }
    if (cp >= 0xFF41 && cp <= 0xFF5A) {
        return cp - 32;
    }
    return cp;
}

fn utf8ByteLen(cp: u32): usize {
    if (cp < 0x80) { return 1; }
    if (cp < 0x800) { return 2; }
    if (cp < 0x10000) { return 3; }
    return 4;
}

fn mapUtf8Case(alloc: std.mem.Allocator, bytes: []const u8, upper: bool): ([]u8) ! UnicodeError {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        const step = decodeUtf8Step(bytes, i)?;
        i += (step.len as usize);
        const mapped = if (upper) toUpperCodepoint(step.codepoint) else toLowerCodepoint(step.codepoint);
        out_len += utf8ByteLen(mapped);
    }

    let out = alloc.alloc(u8, out_len) catch return UnicodeError.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < bytes.len) {
        const step = decodeUtf8Lossy(bytes, i);
        i += (step.len as usize);
        const mapped = if (upper) toUpperCodepoint(step.codepoint) else toLowerCodepoint(step.codepoint);
        const n = encodeUtf8(mapped, out[pos..]) catch {
            alloc.free(out);
            return UnicodeError.InvalidCodepoint;
        };
        pos += n;
    }
    return out;
}

/// Returns a newly allocated UTF-8 string converted to lowercase.
pub fn toLowerUtf8(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! UnicodeError {
    return mapUtf8Case(alloc, bytes, false);
}

/// Returns a newly allocated UTF-8 string converted to uppercase.
pub fn toUpperUtf8(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! UnicodeError {
    return mapUtf8Case(alloc, bytes, true);
}

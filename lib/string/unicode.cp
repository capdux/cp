// Source: /data/dev/bun/src/unicode/ & /data/dev/bun/src/js_parser/lexer/identifier.cp
/**
 * CP Unicode Classification, Case Folding & Zero-Alloc Codepoint Iterator (cap:string)
 * 对应 Bun src/unicode/ 与 src/js_parser/lexer/identifier.cp
 */

const std = @import("std");
import { Utf8, DecodeResult, REPLACEMENT_CHAR } from "./utf.cp";
import { wcwidth, stringWidth } from "./wcwidth.cp";

pub fn isWhitespaceCodepoint(cp: u32): bool {
    if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == 0x0B || cp == 0x0C) {
        return true;
    }
    if (cp < 0x80) return false;
    return cp == 0x00A0
        || cp == 0x1680
        || (cp >= 0x2000 && cp <= 0x200A)
        || cp == 0x2028
        || cp == 0x2029
        || cp == 0x202F
        || cp == 0x205F
        || cp == 0x3000
        || cp == 0xFEFF;
}

pub fn isNewlineCodepoint(cp: u32): bool {
    return cp == '\n' || cp == '\r' || cp == 0x0085 || cp == 0x2028 || cp == 0x2029;
}

pub fn isLineTerminatorCodepoint(cp: u32): bool {
    return cp == '\n' || cp == '\r' || cp == 0x2028 || cp == 0x2029;
}

pub fn isLineTerminator(cp: u32): bool {
    return isLineTerminatorCodepoint(cp);
}

pub fn isControlCodepoint(cp: u32): bool {
    return cp < 0x20 || (cp >= 0x7F && cp <= 0x9F);
}

pub fn isDigitCodepoint(cp: u32): bool {
    return cp >= '0' && cp <= '9';
}

pub fn isHexDigitCodepoint(cp: u32): bool {
    return (cp >= '0' && cp <= '9') || (cp >= 'a' && cp <= 'f') || (cp >= 'A' && cp <= 'F');
}

pub fn isIdentifierStartCodepoint(codepoint: i32): bool {
    if (codepoint <= 0 || codepoint > 0x10FFFF) return false;
    const cp: u32 = @intCast(codepoint);
    if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || cp == '_' || cp == '$') {
        return true;
    }
    if (cp < 0x80) return false;
    // Latin-1 Supplement letters (excluding multiply 0xD7 and divide 0xF7)
    if (cp >= 0x00C0 && cp <= 0x00D6) return true;
    if (cp >= 0x00D8 && cp <= 0x00F6) return true;
    if (cp >= 0x00F8 && cp <= 0x02AF) return true;
    // Greek, Cyrillic, Armenian, Hebrew, Arabic
    if (cp >= 0x0370 && cp <= 0x037D) return true;
    if (cp >= 0x037F && cp <= 0x052F) return true;
    if (cp >= 0x0531 && cp <= 0x0556) return true;
    if (cp >= 0x0560 && cp <= 0x0588) return true;
    if (cp >= 0x05D0 && cp <= 0x05EA) return true;
    if (cp >= 0x0620 && cp <= 0x064A) return true;
    if (cp >= 0x066E && cp <= 0x06D3) return true;
    // Devanagari & Indic
    if (cp >= 0x0904 && cp <= 0x0939) return true;
    // Latin Extended Additional, Greek Extended, Letterlike
    if (cp >= 0x1E00 && cp <= 0x1FFF) return true;
    if (cp == 0x2118 || cp == 0x212E || (cp >= 0x2160 && cp <= 0x2188)) return true;
    // CJK, Hiragana, Katakana, Hangul
    if (cp == 0x3005 || cp == 0x3006 || cp == 0x3007 || (cp >= 0x3021 && cp <= 0x3029)) return true;
    if (cp >= 0x3041 && cp <= 0x3096) return true;
    if (cp >= 0x30A1 && cp <= 0x30FA) return true;
    if (cp >= 0x3105 && cp <= 0x312F) return true;
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
    if (cp >= 0xAC00 && cp <= 0xD7AF) return true;
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;
    if (cp >= 0xFF21 && cp <= 0xFF3A) return true;
    if (cp >= 0xFF41 && cp <= 0xFF5A) return true;
    if (cp >= 0xFF66 && cp <= 0xFFBE) return true;
    if (cp >= 0x20000 && cp <= 0x2EBEF) return true;
    return false;
}

pub fn isIdentifierStart(codepoint: i32): bool {
    return isIdentifierStartCodepoint(codepoint);
}

pub fn isIdentifierContinueCodepoint(codepoint: i32): bool {
    if (codepoint <= 0 || codepoint > 0x10FFFF) return false;
    const cp: u32 = @intCast(codepoint);
    if ((cp >= '0' && cp <= '9') || cp == 0x200C || cp == 0x200D) {
        return true;
    }
    if (isIdentifierStartCodepoint(codepoint)) return true;
    // Combining marks & connector punctuation
    if (cp >= 0x0300 && cp <= 0x036F) return true;
    if (cp >= 0x0660 && cp <= 0x0669) return true;
    if (cp == 0x00B7 || cp == 0x203F || cp == 0x2040 || cp == 0xFF3F) return true;
    if (cp >= 0xFF10 && cp <= 0xFF19) return true;
    if (cp >= 0xFE00 && cp <= 0xFE0F) return true;
    return false;
}

pub fn isIdentifierContinue(codepoint: i32): bool {
    return isIdentifierContinueCodepoint(codepoint);
}

pub fn isIdentifierPart(codepoint: i32): bool {
    return isIdentifierContinueCodepoint(codepoint);
}

pub fn isAlphabeticCodepoint(cp: u32): bool {
    if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')) return true;
    if (cp < 0x80) return false;
    if (cp == '_' || cp == '$') return false;
    return isIdentifierStartCodepoint(@intCast(cp));
}

pub fn isAlphanumericCodepoint(cp: u32): bool {
    return isDigitCodepoint(cp) || isAlphabeticCodepoint(cp);
}

pub fn toLowerCodepoint(cp: u32): u32 {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    if (cp < 0x80) return cp;
    // Latin-1 Supplement: À..Ö (C0..D6) -> à..ö, Ø..Þ (D8..DE) -> ø..þ
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) {
        return cp + 32;
    }
    // Latin Extended-A alternating pairs (0x0100..0x0136, 0x0139..0x0147, etc.)
    if (cp >= 0x0100 && cp <= 0x012E && (cp & 1) == 0) return cp + 1;
    if (cp == 0x0130) return 'i';
    if (cp >= 0x0132 && cp <= 0x0136 && (cp & 1) == 0) return cp + 1;
    if (cp >= 0x0139 && cp <= 0x0147 && (cp & 1) == 1) return cp + 1;
    if (cp >= 0x014A && cp <= 0x0176 && (cp & 1) == 0) return cp + 1;
    if (cp == 0x0178) return 0x00FF;
    if (cp >= 0x0179 && cp <= 0x017D && (cp & 1) == 1) return cp + 1;
    // Greek uppercase Α..Ω (0x0391..0x03A1, 0x03A3..0x03A9)
    if ((cp >= 0x0391 && cp <= 0x03A1) || (cp >= 0x03A3 && cp <= 0x03A9)) {
        return cp + 32;
    }
    // Cyrillic uppercase А..Я (0x0410..0x042F) -> а..я (0x0430..0x044F), Ё..Џ (0x0400..0x040F)
    if (cp >= 0x0410 && cp <= 0x042F) return cp + 32;
    if (cp >= 0x0400 && cp <= 0x040F) return cp + 80;
    // Fullwidth ASCII A..Z
    if (cp >= 0xFF21 && cp <= 0xFF3A) return cp + 32;
    return cp;
}

pub fn toUpperCodepoint(cp: u32): u32 {
    if (cp >= 'a' && cp <= 'z') return cp - 32;
    if (cp < 0x80) return cp;
    if ((cp >= 0x00E0 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x00FE)) {
        return cp - 32;
    }
    if (cp >= 0x0101 && cp <= 0x012F && (cp & 1) == 1) return cp - 1;
    if (cp >= 0x0133 && cp <= 0x0137 && (cp & 1) == 1) return cp - 1;
    if (cp >= 0x013A && cp <= 0x0148 && (cp & 1) == 0) return cp - 1;
    if (cp >= 0x014B && cp <= 0x0177 && (cp & 1) == 1) return cp - 1;
    if (cp == 0x00FF) return 0x0178;
    if (cp >= 0x017A && cp <= 0x017E && (cp & 1) == 0) return cp - 1;
    if ((cp >= 0x03B1 && cp <= 0x03C1) || (cp >= 0x03C3 && cp <= 0x03C9)) {
        return cp - 32;
    }
    if (cp >= 0x0430 && cp <= 0x044F) return cp - 32;
    if (cp >= 0x0450 && cp <= 0x045F) return cp - 80;
    if (cp >= 0xFF41 && cp <= 0xFF5A) return cp - 32;
    return cp;
}

pub fn caseFoldCodepoint(cp: u32): u32 {
    if (cp == 0x03C2) return 0x03C3; // Greek final sigma -> sigma
    if (cp == 0x00DF) return 0x00DF;
    return toLowerCodepoint(cp);
}

pub struct CodepointCursor {
    i: usize = 0,
    c: i32 = -1,
    width: u8 = 0,
}

pub struct CodepointIterator {
    bytes: []const u8 = "",
    i: usize = 0,
    next_width: usize = 0,
    width: u8 = 0,
    c: i32 = -1,

    pub static fn init(str: []const u8): CodepointIterator {
        return CodepointIterator{
            bytes: str,
            i: 0,
            next_width: 0,
            width: 0,
            c: -1,
        };
    }

    pub static fn initOffset(str: []const u8, offset: usize): CodepointIterator {
        const clamped = if (offset <= str.len) offset else str.len;
        return CodepointIterator{
            bytes: str,
            i: clamped,
            next_width: 0,
            width: 0,
            c: -1,
        };
    }

    pub fn reset(*_) {
        _.i = 0;
        _.next_width = 0;
        _.width = 0;
        _.c = -1;
    }

    pub fn remaining(): []const u8 {
        const pos = _.i + _.next_width;
        if (pos >= _.bytes.len) return "";
        return _.bytes[pos.._.bytes.len];
    }

    /// 推进到下一个码点，返回 `i32`（EOF 返回 `-1`）
    pub fn nextCodepoint(*_): i32 {
        const pos = _.i + _.next_width;
        if (pos >= _.bytes.len) {
            _.i = _.bytes.len;
            _.next_width = 0;
            _.width = 0;
            _.c = -1;
            return -1;
        }
        const decoded: DecodeResult = Utf8.decodeNext(_.bytes, pos);
        const step: usize = if (decoded.bytes_consumed > 0) (decoded.bytes_consumed as usize) else 1;
        _.i = pos;
        _.next_width = step;
        _.width = @intCast(step);
        _.c = @intCast(decoded.codepoint);
        return _.c;
    }

    /// 迭代并返回 `u32?`（EOF 返回 `null`）
    pub fn next(*_): u32? {
        const cp = _.nextCodepoint();
        if (cp < 0) return null;
        return @intCast(cp);
    }

    /// 预读下 n 个码点对应的原始字节切片，不修改迭代器游标
    pub fn peek(*_, n: usize): []const u8 {
        const start = _.i + _.next_width;
        if (start >= _.bytes.len || n == 0) return "";
        let pos = start;
        let count: usize = 0;
        while (count < n && pos < _.bytes.len) {
            const decoded = Utf8.decodeNext(_.bytes, pos);
            const step: usize = if (decoded.bytes_consumed > 0) (decoded.bytes_consumed as usize) else 1;
            pos += step;
            count += 1;
        }
        return _.bytes[start..pos];
    }

    /// 使用外部 Cursor 零状态迭代
    pub fn nextCursor(cursor: *CodepointCursor): bool {
        const pos = cursor.i + (cursor.width as usize);
        if (pos >= _.bytes.len) return false;
        const decoded = Utf8.decodeNext(_.bytes, pos);
        const step: u8 = if (decoded.bytes_consumed > 0) decoded.bytes_consumed else 1;
        cursor.i = pos;
        cursor.c = @intCast(decoded.codepoint);
        cursor.width = step;
        return true;
    }
}

pub struct Unicode {
    pub static fn isWhitespace(cp: u32): bool {
        return isWhitespaceCodepoint(cp);
    }

    pub static fn isNewline(cp: u32): bool {
        return isNewlineCodepoint(cp);
    }

    pub static fn isLineTerminator(cp: u32): bool {
        return isLineTerminatorCodepoint(cp);
    }

    pub static fn isControl(cp: u32): bool {
        return isControlCodepoint(cp);
    }

    pub static fn isDigit(cp: u32): bool {
        return isDigitCodepoint(cp);
    }

    pub static fn isHexDigit(cp: u32): bool {
        return isHexDigitCodepoint(cp);
    }

    pub static fn isAlphabetic(cp: u32): bool {
        return isAlphabeticCodepoint(cp);
    }

    pub static fn isAlphanumeric(cp: u32): bool {
        return isAlphanumericCodepoint(cp);
    }

    pub static fn isIdentifierStart(cp: i32): bool {
        return isIdentifierStartCodepoint(cp);
    }

    pub static fn isIdentifierContinue(cp: i32): bool {
        return isIdentifierContinueCodepoint(cp);
    }

    pub static fn toLower(cp: u32): u32 {
        return toLowerCodepoint(cp);
    }

    pub static fn toUpper(cp: u32): u32 {
        return toUpperCodepoint(cp);
    }

    pub static fn caseFold(cp: u32): u32 {
        return caseFoldCodepoint(cp);
    }

    pub static fn charWidth(cp: u32): u8 {
        return wcwidth(cp);
    }

    pub static fn displayWidth(s: []const u8): usize {
        return stringWidth(s, false);
    }
}

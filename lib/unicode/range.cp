// Source: Absorbed from /data/dev/bun/src/css/rules/font_face.cp | Synced: 2026-11-22T04:58:39Z
//! CSS `@font-face` `<urange>` Unicode codepoint range parser, wildcard `?` optimizer,
//! membership/overlap/merge utilities, and canonical `U+...` serializer.

import std from "std";

export const MAX_CODEPOINT: u32 = 0x10FFFF;

export err UnicodeRangeError {
    InvalidSyntax,
    RangeOutOfOrder,
    CodepointOutOfRange,
    BufferTooSmall,
}

fn hexVal(c: u8): u32? {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

/// Contiguous inclusive Unicode codepoint range `[start, end]` (`0 <= start <= end <= 0x10FFFF`).
export struct UnicodeRange {
    start: u32,
    end: u32,

    pub static fn single(cp: u21): UnicodeRange {
        const v: u32 = @as(u32, cp);
        return UnicodeRange{ start: v, end: v };
    }

    pub static fn init(start: u32, end: u32): UnicodeRange! {
        if (start > end) return err.RangeOutOfOrder;
        if (end > MAX_CODEPOINT) return err.CodepointOutOfRange;
        return UnicodeRange{ start: start, end: end };
    }

    pub fn contains(cp: u21): bool {
        const v: u32 = @as(u32, cp);
        return v >= _.start && v <= _.end;
    }

    pub fn count(): u32 {
        return _.end - _.start + 1;
    }

    pub fn isSingle(): bool {
        return _.start == _.end;
    }

    pub fn eql(other: UnicodeRange): bool {
        return _.start == other.start && _.end == other.end;
    }

    pub fn overlaps(other: UnicodeRange): bool {
        return _.start <= other.end && other.start <= _.end;
    }

    pub fn adjacentOrOverlaps(other: UnicodeRange): bool {
        if (_.overlaps(other)) return true;
        if (_.end < MAX_CODEPOINT && _.end + 1 == other.start) return true;
        if (other.end < MAX_CODEPOINT && other.end + 1 == _.start) return true;
        return false;
    }

    pub fn merge(other: UnicodeRange): UnicodeRange? {
        if (!_.adjacentOrOverlaps(other)) return null;
        const s = if (_.start < other.start) _.start else other.start;
        const e = if (_.end > other.end) _.end else other.end;
        return UnicodeRange{ start: s, end: e };
    }

    /// Parse CSS `<urange>`: `U+0025`, `U+0025-00FF`, `U+4??`, `U+????`, case-insensitive `u+`.
    pub static fn parse(input: []const u8): UnicodeRange! {
        const s = trimAscii(input);
        if (s.len < 3) return err.InvalidSyntax;
        if ((s[0] != 'U' && s[0] != 'u') || s[1] != '+') return err.InvalidSyntax;

        const body = s[2..];
        let pos: usize = 0;
        let start_val: u32 = 0;
        let hex_digits: usize = 0;
        while (pos < body.len && hex_digits < 6) {
            if (d := hexVal(body[pos])) {
                start_val = (start_val << 4) | d;
                hex_digits += 1;
                pos += 1;
            } else {
                break;
            }
        }

        // Wildcard `?` form: e.g. `U+4??` or `U+???`
        let qmarks: usize = 0;
        while (pos < body.len && body[pos] == '?' && hex_digits + qmarks < 6) {
            qmarks += 1;
            pos += 1;
        }

        if (hex_digits == 0 && qmarks == 0) return err.InvalidSyntax;

        if (qmarks > 0) {
            if (pos != body.len) return err.InvalidSyntax;
            let lo = start_val;
            let hi = start_val;
            let k: usize = 0;
            while (k < qmarks) : (k += 1) {
                lo = lo << 4;
                hi = (hi << 4) | 0xF;
            }
            if (hi > MAX_CODEPOINT) hi = MAX_CODEPOINT;
            if (lo > MAX_CODEPOINT) return err.CodepointOutOfRange;
            return UnicodeRange{ start: lo, end: hi };
        }

        // Single codepoint form: `U+0041`
        if (pos == body.len) {
            if (start_val > MAX_CODEPOINT) return err.CodepointOutOfRange;
            return UnicodeRange{ start: start_val, end: start_val };
        }

        // Explicit range form: `U+0025-00FF`
        if (body[pos] != '-') return err.InvalidSyntax;
        pos += 1;
        let end_val: u32 = 0;
        let end_digits: usize = 0;
        while (pos < body.len && end_digits < 6) {
            if (d := hexVal(body[pos])) {
                end_val = (end_val << 4) | d;
                end_digits += 1;
                pos += 1;
            } else {
                break;
            }
        }
        if (end_digits == 0 || pos != body.len) return err.InvalidSyntax;
        return UnicodeRange.init(start_val, end_val);
    }

    /// Format as shortest canonical CSS `<urange>` into `out`, automatically using
    /// wildcard `?` notation when `[start, end]` aligns on hex nibble boundaries.
    pub fn toCss(out: []u8): ([]const u8)! {
        if (_.start != _.end) {
            let shift: u5 = 24;
            let mask: u32 = @as(u32, 0xF) << shift;
            while (shift > 0) {
                if ((_.start & mask) != (_.end & mask)) break;
                mask = mask >> 4;
                shift -= 4;
            }
            shift += 4;
            const rem_mask: u32 = (@as(u32, 1) << shift) - @as(u32, 1);
            if ((_.start & rem_mask) == 0 && (_.end & rem_mask) == rem_mask) {
                const prefix = (_.start & ~rem_mask) >> shift;
                let written: usize = 0;
                if (prefix != 0) {
                    const s1 = std.fmt.bufPrint(out, "U+{X}", .{prefix}) catch return err.BufferTooSmall;
                    written = s1.len;
                } else {
                    if (out.len < 2) return err.BufferTooSmall;
                    out[0] = 'U';
                    out[1] = '+';
                    written = 2;
                }
                let q = shift;
                while (q > 0) : (q -= 4) {
                    if (written >= out.len) return err.BufferTooSmall;
                    out[written] = '?';
                    written += 1;
                }
                return out[0..written];
            }
        }

        if (_.start == _.end) {
            return std.fmt.bufPrint(out, "U+{X}", .{_.start}) catch return err.BufferTooSmall;
        }
        return std.fmt.bufPrint(out, "U+{X}-{X}", .{ _.start, _.end }) catch return err.BufferTooSmall;
    }
}

fn trimAscii(s: []const u8): []const u8 {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) : (a += 1) {}
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r')) : (b -= 1) {}
    return s[a..b];
}

// Source: Absorbed from /data/dev/bun/src/css/values/css_string.cp | Synced: 2026-11-22T16:08:00Z
// ============================================================================
// lib/css/cssString.cp — Zero-Alloc W3C CSSOM String Serializer & Syntax L3 Parser
// ============================================================================
// Implements CSSOM `serialize a string` (double/single quote escaping, NUL ->
// U+FFFD replacement, control-byte hex escaping with trailing space) and CSS
// Syntax Level 3 quoted string parsing (1..6 hex Unicode escapes, line
// continuations, bare-newline rejection, and surrogate/range sanitization).

pub err CssStringError {
    EmptyInput,
    UnterminatedString,
    UnescapedNewline,
    InvalidEscape,
    BufferTooSmall,
}

pub enum CssQuoteStyle: u8 {
    Double = 0,
    Single = 1,

    fn byte(): u8 {
        return switch (_) {
            .Double => '"',
            .Single => '\'',
        };
    }
}

const HEX_LOWER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

fn hexVal(c: u8): u32? {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

fn isCssWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isCssNewline(c: u8): bool {
    return c == '\n' || c == '\r' || c == 0x0C;
}

fn writeByte(out: []u8, pos: usize, b: u8): CssStringError!usize {
    if (pos >= out.len) return CssStringError.BufferTooSmall;
    out[pos] = b;
    return pos + 1;
}

/// Encodes a Unicode scalar value into UTF-8 bytes in `out[pos..]`.
/// Invalid codepoints (0, surrogates 0xD800..0xDFFF, > 0x10FFFF) emit U+FFFD.
fn writeUtf8Codepoint(out: []u8, pos: usize, cp_in: u32): CssStringError!usize {
    let cp = cp_in;
    if (cp == 0 || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
        cp = 0xFFFD;
    }
    if (cp <= 0x7F) {
        if (pos + 1 > out.len) return CssStringError.BufferTooSmall;
        out[pos] = @as(u8, @intCast(cp));
        return pos + 1;
    } else if (cp <= 0x7FF) {
        if (pos + 2 > out.len) return CssStringError.BufferTooSmall;
        out[pos] = @as(u8, @intCast(0xC0 | (cp >> 6)));
        out[pos + 1] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
        return pos + 2;
    } else if (cp <= 0xFFFF) {
        if (pos + 3 > out.len) return CssStringError.BufferTooSmall;
        out[pos] = @as(u8, @intCast(0xE0 | (cp >> 12)));
        out[pos + 1] = @as(u8, @intCast(0x80 | ((cp >> 6) & 0x3F)));
        out[pos + 2] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
        return pos + 3;
    } else {
        if (pos + 4 > out.len) return CssStringError.BufferTooSmall;
        out[pos] = @as(u8, @intCast(0xF0 | (cp >> 18)));
        out[pos + 1] = @as(u8, @intCast(0x80 | ((cp >> 12) & 0x3F)));
        out[pos + 2] = @as(u8, @intCast(0x80 | ((cp >> 6) & 0x3F)));
        out[pos + 3] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
        return pos + 4;
    }
}

fn writeHexEscapeByte(out: []u8, pos: usize, b: u8): CssStringError!usize {
    let p = writeByte(out, pos, '\\')?;
    const hi: usize = @as(usize, (b >> 4) & 0x0F);
    const lo: usize = @as(usize, b & 0x0F);
    if (hi > 0) {
        p = writeByte(out, p, HEX_LOWER[hi])?;
    }
    p = writeByte(out, p, HEX_LOWER[lo])?;
    p = writeByte(out, p, ' ')?;
    return p;
}

/// Zero-allocation W3C CSSOM string serializer & CSS Syntax Level 3 string parser.
pub struct CssString {
    /// Fast-path check: returns true if `raw` contains quote, backslash, NUL, or control bytes.
    static fn needsEscaping(raw: []const u8, quote: CssQuoteStyle): bool {
        const q = quote.byte();
        let i: usize = 0;
        while (i < raw.len) {
            const c = raw[i];
            if (c == q || c == '\\' || c <= 0x1F || c == 0x7F) return true;
            i += 1;
        }
        return false;
    }

    /// Serializes `raw` as a double-quoted CSS string following W3C CSSOM rules.
    static fn serialize(raw: []const u8, out: []u8): CssStringError!usize {
        return CssString.serializeWithQuote(raw, .Double, out);
    }

    /// Serializes `raw` using the chosen `CssQuoteStyle` (`.Double` or `.Single`).
    static fn serializeWithQuote(raw: []const u8, quote: CssQuoteStyle, out: []u8): CssStringError!usize {
        const q = quote.byte();
        let p: usize = 0;
        p = writeByte(out, p, q)?;
        let i: usize = 0;
        while (i < raw.len) {
            const c = raw[i];
            if (c == 0) {
                // CSSOM: U+0000 NULL is serialized as U+FFFD REPLACEMENT CHARACTER
                p = writeUtf8Codepoint(out, p, 0xFFFD)?;
            } else if ((c >= 0x01 && c <= 0x1F) || c == 0x7F) {
                // CSSOM: control characters serialized as `\<hex> `
                p = writeHexEscapeByte(out, p, c)?;
            } else if (c == q || c == '\\') {
                p = writeByte(out, p, '\\')?;
                p = writeByte(out, p, c)?;
            } else {
                p = writeByte(out, p, c)?;
            }
            i += 1;
        }
        p = writeByte(out, p, q)?;
        return p;
    }

    /// Parses a quoted CSS string (`"..."` or `'...'`) into `out`, resolving CSS Syntax L3
    /// escapes (`\\`, `\"`, `\'`, line continuations, and 1..6-digit `\HHHHHH` codepoints).
    static fn parse(quoted: []const u8, out: []u8): CssStringError!usize {
        if (quoted.len < 2) return CssStringError.EmptyInput;
        const quote = quoted[0];
        if (quote != '"' && quote != '\'') return CssStringError.UnterminatedString;

        let i: usize = 1;
        let p: usize = 0;
        let closed = false;

        while (i < quoted.len) {
            const c = quoted[i];
            if (c == quote) {
                closed = true;
                i += 1;
                break;
            }
            if (isCssNewline(c)) {
                return CssStringError.UnescapedNewline;
            }
            if (c == '\\') {
                i += 1;
                if (i >= quoted.len) return CssStringError.InvalidEscape;
                const esc = quoted[i];
                if (esc == '\r') {
                    // Escaped CR or CRLF line continuation -> consumed, emits nothing
                    i += 1;
                    if (i < quoted.len && quoted[i] == '\n') i += 1;
                } else if (esc == '\n' || esc == 0x0C) {
                    // Escaped LF or FF line continuation -> consumed, emits nothing
                    i += 1;
                } else if (first_hex := hexVal(esc)) {
                    let cp: u32 = first_hex;
                    let digits: usize = 1;
                    i += 1;
                    while (i < quoted.len && digits < 6) {
                        if (d := hexVal(quoted[i])) {
                            cp = (cp << 4) | d;
                            digits += 1;
                            i += 1;
                        } else {
                            break;
                        }
                    }
                    // Consume single optional trailing whitespace (CRLF counts as one)
                    if (i < quoted.len && isCssWhitespace(quoted[i])) {
                        if (quoted[i] == '\r' && i + 1 < quoted.len && quoted[i + 1] == '\n') {
                            i += 2;
                        } else {
                            i += 1;
                        }
                    }
                    p = writeUtf8Codepoint(out, p, cp)?;
                } else {
                    p = writeByte(out, p, esc)?;
                    i += 1;
                }
            } else {
                p = writeByte(out, p, c)?;
                i += 1;
            }
        }

        if (!closed || i != quoted.len) {
            return CssStringError.UnterminatedString;
        }
        return p;
    }

    /// Zero-buffer syntax validator for quoted CSS string literals.
    static fn isValidQuoted(quoted: []const u8): bool {
        if (quoted.len < 2) return false;
        const quote = quoted[0];
        if (quote != '"' && quote != '\'') return false;

        let i: usize = 1;
        let closed = false;
        while (i < quoted.len) {
            const c = quoted[i];
            if (c == quote) {
                closed = true;
                i += 1;
                break;
            }
            if (isCssNewline(c)) return false;
            if (c == '\\') {
                i += 1;
                if (i >= quoted.len) return false;
                if (quoted[i] == '\r' && i + 1 < quoted.len && quoted[i + 1] == '\n') {
                    i += 2;
                } else {
                    i += 1;
                }
            } else {
                i += 1;
            }
        }
        return closed && i == quoted.len;
    }
}

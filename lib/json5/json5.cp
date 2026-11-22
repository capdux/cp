// Source: Absorbed from /data/dev/bun/src/interchange/json5.cp | Synced: 2026-11-22T07:42:24Z (Zero-Alloc JSON5 Lexer & JSON Converter)
//! Zero-allocation JSON5 1.0 lexer, validator, and RFC 8259 standard JSON converter.

pub enum Json5TokenKind: u8 {
    lbrace = 0,
    rbrace = 1,
    lbracket = 2,
    rbracket = 3,
    colon = 4,
    comma = 5,
    string = 6,
    ident = 7,
    number = 8,
    true_lit = 9,
    false_lit = 10,
    null_lit = 11,
    invalid = 12,
}

pub struct Json5Token {
    kind: Json5TokenKind,
    lexeme: []const u8,
}

const MAX_NESTING_DEPTH: usize = 64;

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn hexValue(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c >= 0x80;
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || isAsciiDigit(c);
}

fn isAsciiWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

pub struct Json5Iter {
    input: []const u8,
    pos: usize,

    pub static fn init(input: []const u8): Json5Iter {
        return Json5Iter{
            input: input,
            pos: 0,
        };
    }

    fn skipWhitespaceAndComments(*_): bool {
        const s = _.input;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isAsciiWhitespace(c)) {
                _.pos += 1;
                continue;
            }
            // UTF-8 NBSP (U+00A0: 0xC2 0xA0)
            if (c == 0xC2 && _.pos + 1 < s.len && s[_.pos + 1] == 0xA0) {
                _.pos += 2;
                continue;
            }
            // UTF-8 BOM (U+FEFF: 0xEF 0xBB 0xBF)
            if (c == 0xEF && _.pos + 2 < s.len && s[_.pos + 1] == 0xBB && s[_.pos + 2] == 0xBF) {
                _.pos += 3;
                continue;
            }
            // UTF-8 Line/Paragraph Separator (U+2028 / U+2029: 0xE2 0x80 0xA8|0xA9)
            if (c == 0xE2 && _.pos + 2 < s.len && s[_.pos + 1] == 0x80 && (s[_.pos + 2] == 0xA8 || s[_.pos + 2] == 0xA9)) {
                _.pos += 3;
                continue;
            }
            // Comments
            if (c == '/' && _.pos + 1 < s.len) {
                const next_c = s[_.pos + 1];
                if (next_c == '/') {
                    _.pos += 2;
                    while (_.pos < s.len && s[_.pos] != '\n' && s[_.pos] != '\r') {
                        _.pos += 1;
                    }
                    continue;
                }
                if (next_c == '*') {
                    _.pos += 2;
                    let closed = false;
                    while (_.pos + 1 < s.len) {
                        if (s[_.pos] == '*' && s[_.pos + 1] == '/') {
                            _.pos += 2;
                            closed = true;
                            break;
                        }
                        _.pos += 1;
                    }
                    if (!closed) {
                        _.pos = s.len;
                        return false;
                    }
                    continue;
                }
            }
            break;
        }
        return true;
    }

    fn scanString(*_, quote: u8): Json5Token {
        const s = _.input;
        const start = _.pos;
        _.pos += 1; // skip opening quote
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (c == quote) {
                _.pos += 1;
                return Json5Token{ kind: Json5TokenKind.string, lexeme: s[start.._.pos] };
            }
            if (c == '\n' || c == '\r') {
                return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= s.len) {
                    return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                const esc = s[_.pos];
                if (esc == '\r') {
                    _.pos += 1;
                    if (_.pos < s.len && s[_.pos] == '\n') {
                        _.pos += 1;
                    }
                    continue;
                }
                if (esc == '\n') {
                    _.pos += 1;
                    continue;
                }
                if (esc == 'x') {
                    if (_.pos + 2 >= s.len || !isHexDigit(s[_.pos + 1]) || !isHexDigit(s[_.pos + 2])) {
                        _.pos += 1;
                        return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    _.pos += 3;
                    continue;
                }
                if (esc == 'u') {
                    if (_.pos + 4 >= s.len || !isHexDigit(s[_.pos + 1]) || !isHexDigit(s[_.pos + 2]) || !isHexDigit(s[_.pos + 3]) || !isHexDigit(s[_.pos + 4])) {
                        _.pos += 1;
                        return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    _.pos += 5;
                    continue;
                }
                _.pos += 1;
                continue;
            }
            _.pos += 1;
        }
        return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
    }

    fn scanNumber(*_): Json5Token {
        const s = _.input;
        const start = _.pos;

        if (_.pos < s.len && (s[_.pos] == '+' || s[_.pos] == '-')) {
            _.pos += 1;
        }
        if (_.pos >= s.len) {
            return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
        }

        // Signed Infinity / NaN
        if (_.pos + 8 <= s.len && bytesEql(s[_.pos .. _.pos + 8], "Infinity")) {
            if (_.pos + 8 == s.len || !isIdentContinue(s[_.pos + 8])) {
                _.pos += 8;
                return Json5Token{ kind: .number, lexeme: s[start.._.pos] };
            }
            return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
        }
        if (_.pos + 3 <= s.len && bytesEql(s[_.pos .. _.pos + 3], "NaN")) {
            if (_.pos + 3 == s.len || !isIdentContinue(s[_.pos + 3])) {
                _.pos += 3;
                return Json5Token{ kind: .number, lexeme: s[start.._.pos] };
            }
            return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
        }

        // Hexadecimal: 0x... or 0X...
        if (s[_.pos] == '0' && _.pos + 1 < s.len && (s[_.pos + 1] == 'x' || s[_.pos + 1] == 'X')) {
            _.pos += 2;
            const hex_start = _.pos;
            while (_.pos < s.len && isHexDigit(s[_.pos])) {
                _.pos += 1;
            }
            if (_.pos == hex_start) {
                return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            if (_.pos < s.len && isIdentContinue(s[_.pos])) {
                return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            return Json5Token{ kind: .number, lexeme: s[start.._.pos] };
        }

        // Decimal: [digits][.digits] | .digits
        let has_int = false;
        let has_frac = false;

        if (s[_.pos] == '0') {
            has_int = true;
            _.pos += 1;
            // Leading zero followed by another digit is invalid in JSON5
            if (_.pos < s.len && isAsciiDigit(s[_.pos])) {
                return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
            }
        } else if (isAsciiDigit(s[_.pos])) {
            has_int = true;
            while (_.pos < s.len && isAsciiDigit(s[_.pos])) {
                _.pos += 1;
            }
        }

        if (_.pos < s.len && s[_.pos] == '.') {
            _.pos += 1;
            const frac_start = _.pos;
            while (_.pos < s.len && isAsciiDigit(s[_.pos])) {
                _.pos += 1;
            }
            has_frac = _.pos > frac_start;
        }

        if (!has_int && !has_frac) {
            return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
        }

        if (_.pos < s.len && (s[_.pos] == 'e' || s[_.pos] == 'E')) {
            _.pos += 1;
            if (_.pos < s.len && (s[_.pos] == '+' || s[_.pos] == '-')) {
                _.pos += 1;
            }
            const exp_start = _.pos;
            while (_.pos < s.len && isAsciiDigit(s[_.pos])) {
                _.pos += 1;
            }
            if (_.pos == exp_start) {
                return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
            }
        }

        if (_.pos < s.len && isIdentContinue(s[_.pos])) {
            return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
        }

        return Json5Token{ kind: .number, lexeme: s[start.._.pos] };
    }

    pub fn next(*_): Json5Token? {
        const start_before_ws = _.pos;
        if (!_.skipWhitespaceAndComments()) {
            return Json5Token{ kind: .invalid, lexeme: _.input[start_before_ws.._.pos] };
        }
        if (_.pos >= _.input.len) return null;

        const s = _.input;
        const start = _.pos;
        const c = s[_.pos];

        if (c == '{') { _.pos += 1; return Json5Token{ kind: .lbrace, lexeme: s[start.._.pos] }; }
        if (c == '}') { _.pos += 1; return Json5Token{ kind: .rbrace, lexeme: s[start.._.pos] }; }
        if (c == '[') { _.pos += 1; return Json5Token{ kind: .lbracket, lexeme: s[start.._.pos] }; }
        if (c == ']') { _.pos += 1; return Json5Token{ kind: .rbracket, lexeme: s[start.._.pos] }; }
        if (c == ':') { _.pos += 1; return Json5Token{ kind: .colon, lexeme: s[start.._.pos] }; }
        if (c == ',') { _.pos += 1; return Json5Token{ kind: .comma, lexeme: s[start.._.pos] }; }

        if (c == '"' || c == '\'') {
            return _.scanString(c);
        }

        if (isAsciiDigit(c) || c == '+' || c == '-' || c == '.') {
            return _.scanNumber();
        }

        if (isIdentStart(c)) {
            _.pos += 1;
            while (_.pos < s.len && isIdentContinue(s[_.pos])) {
                _.pos += 1;
            }
            const lex = s[start.._.pos];
            if (bytesEql(lex, "true")) return Json5Token{ kind: .true_lit, lexeme: lex };
            if (bytesEql(lex, "false")) return Json5Token{ kind: .false_lit, lexeme: lex };
            if (bytesEql(lex, "null")) return Json5Token{ kind: .null_lit, lexeme: lex };
            if (bytesEql(lex, "Infinity") || bytesEql(lex, "NaN")) {
                return Json5Token{ kind: .number, lexeme: lex };
            }
            return Json5Token{ kind: .ident, lexeme: lex };
        }

        _.pos += 1;
        return Json5Token{ kind: .invalid, lexeme: s[start.._.pos] };
    }
}

struct OutBuf {
    buf: []u8,
    len: usize,

    fn push(*_, c: u8): bool {
        if (_.len >= _.buf.len) return false;
        _.buf[_.len] = c;
        _.len += 1;
        return true;
    }

    fn pushSlice(*_, s: []const u8): bool {
        if (_.len + s.len > _.buf.len) return false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            _.buf[_.len + i] = s[i];
        }
        _.len += s.len;
        return true;
    }

    fn pushU64(*_, val: u64): bool {
        if (val == 0) return _.push('0');
        let tmp: [24]u8 = [0; 24];
        let n: usize = 0;
        let v = val;
        while (v > 0) {
            tmp[n] = @as(u8, @intCast(v % 10)) + '0';
            v /= 10;
            n += 1;
        }
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (!_.push(tmp[n - 1 - i])) return false;
        }
        return true;
    }

    fn pushHexByte(*_, b: u8): bool {
        const hex = "0123456789abcdef";
        if (!_.push(hex[(b >> 4) & 0x0F])) return false;
        return _.push(hex[b & 0x0F]);
    }
}

fn emitJsonStringFromLexeme(out: *OutBuf, lex: []const u8): bool {
    if (lex.len < 2) return false;
    if (!out.push('"')) return false;

    const inner = lex[1 .. lex.len - 1];
    let i: usize = 0;
    while (i < inner.len) {
        const c = inner[i];
        if (c == '\\') {
            if (i + 1 >= inner.len) return false;
            const esc = inner[i + 1];
            // Line continuation: \ + (\r\n | \n | \r)
            if (esc == '\r') {
                i += 2;
                if (i < inner.len && inner[i] == '\n') i += 1;
                continue;
            }
            if (esc == '\n') {
                i += 2;
                continue;
            }
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                if (!out.push('\\')) return false;
                if (!out.push(esc)) return false;
                i += 2;
                continue;
            }
            if (esc == '\'') {
                if (!out.push('\'')) return false;
                i += 2;
                continue;
            }
            if (esc == 'v') {
                if (!out.pushSlice("\\u000b")) return false;
                i += 2;
                continue;
            }
            if (esc == '0') {
                if (!out.pushSlice("\\u0000")) return false;
                i += 2;
                continue;
            }
            if (esc == 'x') {
                if (i + 3 >= inner.len) return false;
                if (!out.pushSlice("\\u00")) return false;
                if (!out.push(inner[i + 2])) return false;
                if (!out.push(inner[i + 3])) return false;
                i += 4;
                continue;
            }
            if (esc == 'u') {
                if (i + 5 >= inner.len) return false;
                if (!out.pushSlice(inner[i .. i + 6])) return false;
                i += 6;
                continue;
            }
            // Non-special escape in JSON5 evaluates to literal character
            if (!out.push(esc)) return false;
            i += 2;
            continue;
        }
        if (c == '"') {
            if (!out.pushSlice("\\\"")) return false;
            i += 1;
            continue;
        }
        if (c < 0x20) {
            if (!out.pushSlice("\\u00")) return false;
            if (!out.pushHexByte(c)) return false;
            i += 1;
            continue;
        }
        if (!out.push(c)) return false;
        i += 1;
    }

    return out.push('"');
}

fn emitJsonNumberFromLexeme(out: *OutBuf, lex: []const u8): bool {
    if (lex.len == 0) return false;

    let neg = false;
    let start: usize = 0;
    if (lex[0] == '+') {
        start = 1;
    } else if (lex[0] == '-') {
        neg = true;
        start = 1;
    }
    if (start >= lex.len) return false;
    const body = lex[start..];

    // Infinity / NaN -> RFC 8259 strict JSON represents non-finite numbers as null
    if (bytesEql(body, "Infinity") || bytesEql(body, "NaN")) {
        return out.pushSlice("null");
    }

    // Hexadecimal integer -> decimal
    if (body.len >= 3 && body[0] == '0' && (body[1] == 'x' || body[1] == 'X')) {
        let acc: u64 = 0;
        let i: usize = 2;
        while (i < body.len) : (i += 1) {
            if (acc > (@as(u64, 0x0FFFFFFFFFFFFFFF))) return false;
            acc = (acc << 4) | @as(u64, hexValue(body[i]));
        }
        if (neg && acc != 0) {
            if (!out.push('-')) return false;
        }
        return out.pushU64(acc);
    }

    if (neg) {
        if (!out.push('-')) return false;
    }

    // Leading dot: .5 -> 0.5
    if (body[0] == '.') {
        if (!out.push('0')) return false;
        return out.pushSlice(body);
    }

    // Check for trailing dot before end or exponent: 5. -> 5.0, 5.e2 -> 5.0e2
    let i: usize = 0;
    while (i < body.len) : (i += 1) {
        const c = body[i];
        if (!out.push(c)) return false;
        if (c == '.') {
            const next_is_digit = (i + 1 < body.len) && isAsciiDigit(body[i + 1]);
            if (!next_is_digit) {
                if (!out.push('0')) return false;
            }
        }
    }
    return true;
}

fn isObjectKeyToken(kind: Json5TokenKind): bool {
    return kind == Json5TokenKind.string || kind == .ident || kind == .true_lit || kind == .false_lit || kind == .null_lit || kind == .number;
}

fn isValidIdentKey(tok: Json5Token): bool {
    if (tok.kind == .ident || tok.kind == .true_lit || tok.kind == .false_lit || tok.kind == .null_lit) {
        return true;
    }
    // Infinity / NaN are valid IdentifierNames in JSON5 object keys
    if (tok.kind == .number && (bytesEql(tok.lexeme, "Infinity") || bytesEql(tok.lexeme, "NaN"))) {
        return true;
    }
    return false;
}

struct Converter {
    iter: Json5Iter,
    out: OutBuf,
    emit: bool,

    fn writeByte(*_, c: u8): bool {
        if (!_.emit) return true;
        return _.out.push(c);
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (!_.emit) return true;
        return _.out.pushSlice(s);
    }

    fn parseValueWithFirst(*_, tok: Json5Token, depth: usize): bool {
        if (depth > MAX_NESTING_DEPTH) return false;
        return switch (tok.kind) {
            .lbrace => _.parseObject(depth + 1),
            .lbracket => _.parseArray(depth + 1),
            Json5TokenKind.string => if (_.emit) emitJsonStringFromLexeme(&_.out, tok.lexeme) else true,
            .number => if (_.emit) emitJsonNumberFromLexeme(&_.out, tok.lexeme) else true,
            .true_lit => _.writeSlice("true"),
            .false_lit => _.writeSlice("false"),
            .null_lit => _.writeSlice("null"),
            else => false,
        };
    }

    fn parseObject(*_, depth: usize): bool {
        if (depth > MAX_NESTING_DEPTH) return false;
        if (!_.writeByte('{')) return false;

        let first = true;
        while (true) {
            const next_tok = _.iter.next() ?? return false;
            if (next_tok.kind == .rbrace) {
                return _.writeByte('}');
            }
            if (!first) return false;

            // Key
            if (next_tok.kind == Json5TokenKind.string) {
                if (_.emit && !emitJsonStringFromLexeme(&_.out, next_tok.lexeme)) return false;
            } else if (isValidIdentKey(next_tok)) {
                if (!_.writeByte('"')) return false;
                if (!_.writeSlice(next_tok.lexeme)) return false;
                if (!_.writeByte('"')) return false;
            } else {
                return false;
            }

            // Colon
            const colon = _.iter.next() ?? return false;
            if (colon.kind != .colon) return false;
            if (!_.writeByte(':')) return false;

            // Value
            const val_tok = _.iter.next() ?? return false;
            if (!_.parseValueWithFirst(val_tok, depth)) return false;

            // Separator: comma or rbrace
            const sep = _.iter.next() ?? return false;
            if (sep.kind == .rbrace) {
                return _.writeByte('}');
            }
            if (sep.kind != .comma) return false;

            // Peek next after comma: allow trailing comma before '}'
            const after_comma = _.iter.next() ?? return false;
            if (after_comma.kind == .rbrace) {
                return _.writeByte('}');
            }

            // Otherwise after_comma is the next key
            if (!_.writeByte(',')) return false;
            if (after_comma.kind == Json5TokenKind.string) {
                if (_.emit && !emitJsonStringFromLexeme(&_.out, after_comma.lexeme)) return false;
            } else if (isValidIdentKey(after_comma)) {
                if (!_.writeByte('"')) return false;
                if (!_.writeSlice(after_comma.lexeme)) return false;
                if (!_.writeByte('"')) return false;
            } else {
                return false;
            }

            const colon2 = _.iter.next() ?? return false;
            if (colon2.kind != .colon) return false;
            if (!_.writeByte(':')) return false;

            const val_tok2 = _.iter.next() ?? return false;
            if (!_.parseValueWithFirst(val_tok2, depth)) return false;

            // Continue loop to handle remaining comma-separated entries
            first = false;
            while (true) {
                const sep2 = _.iter.next() ?? return false;
                if (sep2.kind == .rbrace) {
                    return _.writeByte('}');
                }
                if (sep2.kind != .comma) return false;

                const next_k = _.iter.next() ?? return false;
                if (next_k.kind == .rbrace) {
                    return _.writeByte('}');
                }
                if (!_.writeByte(',')) return false;
                if (next_k.kind == Json5TokenKind.string) {
                    if (_.emit && !emitJsonStringFromLexeme(&_.out, next_k.lexeme)) return false;
                } else if (isValidIdentKey(next_k)) {
                    if (!_.writeByte('"')) return false;
                    if (!_.writeSlice(next_k.lexeme)) return false;
                    if (!_.writeByte('"')) return false;
                } else {
                    return false;
                }

                const c_tok = _.iter.next() ?? return false;
                if (c_tok.kind != .colon) return false;
                if (!_.writeByte(':')) return false;

                const v_tok = _.iter.next() ?? return false;
                if (!_.parseValueWithFirst(v_tok, depth)) return false;
            }
        }
    }

    fn parseArray(*_, depth: usize): bool {
        if (depth > MAX_NESTING_DEPTH) return false;
        if (!_.writeByte('[')) return false;

        const first_tok = _.iter.next() ?? return false;
        if (first_tok.kind == .rbracket) {
            return _.writeByte(']');
        }
        if (!_.parseValueWithFirst(first_tok, depth)) return false;

        while (true) {
            const sep = _.iter.next() ?? return false;
            if (sep.kind == .rbracket) {
                return _.writeByte(']');
            }
            if (sep.kind != .comma) return false;

            const next_tok = _.iter.next() ?? return false;
            if (next_tok.kind == .rbracket) {
                return _.writeByte(']');
            }
            if (!_.writeByte(',')) return false;
            if (!_.parseValueWithFirst(next_tok, depth)) return false;
        }
    }
}

pub fn toStandardJson(input: []const u8, out: []u8): ([]const u8)? {
    let conv = Converter{
        iter: Json5Iter.init(input),
        out: OutBuf{ buf: out, len: 0 },
        emit: true,
    };
    const root = conv.iter.next() ?? return null;
    if (!conv.parseValueWithFirst(root, 0)) return null;
    // Reject trailing garbage after root value
    if (conv.iter.next() != null) return null;
    return out[0..conv.out.len];
}

pub fn isValidJson5(input: []const u8): bool {
    let dummy: [1]u8 = [0; 1];
    let conv = Converter{
        iter: Json5Iter.init(input),
        out: OutBuf{ buf: dummy[0..], len: 0 },
        emit: false,
    };
    const root = conv.iter.next() ?? return false;
    if (!conv.parseValueWithFirst(root, 0)) return false;
    return conv.iter.next() == null;
}

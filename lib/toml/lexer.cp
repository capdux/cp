// Source: Absorbed from /data/dev/bun/src/interchange/toml/lexer.cp | Synced: 2026-11-22T13:46:48Z

/// TOML v1.0 词法 Token 种类
pub enum TomlTokenKind: u8 {
    eof = 0,
    newline = 1,
    equals = 2,
    dot = 3,
    comma = 4,
    lbracket = 5,
    rbracket = 6,
    double_lbracket = 7,
    double_rbracket = 8,
    lbrace = 9,
    rbrace = 10,
    bare_key = 11,
    basic_string = 12,
    multiline_basic_string = 13,
    literal_string = 14,
    multiline_literal_string = 15,
    integer = 16,
    float_val = 17,
    boolean = 18,
    datetime = 19,
    invalid = 20,
}

/// TOML v1.0 词法 Token（零拷贝切片 + 精确行列号 + 预解析整型/布尔值）
pub struct TomlToken {
    kind: TomlTokenKind,
    raw: []const u8,
    start: usize,
    end: usize,
    line: u32,
    col: u32,
    int_val: i64 = 0,
    bool_val: bool = false,
}

type TomlLexVec16 = @Vector(16, u8);

const TOML_FLAG_DIGIT: u8 = 1;
const TOML_FLAG_HEX: u8 = 2;
const TOML_FLAG_OCT: u8 = 4;
const TOML_FLAG_BIN: u8 = 8;
const TOML_FLAG_BARE: u8 = 16;
const TOML_FLAG_DT: u8 = 32;

fn makeTomlCharFlags(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let f: u8 = 0;
        if (i >= '0' && i <= '9') {
            f |= TOML_FLAG_DIGIT | TOML_FLAG_HEX | TOML_FLAG_BARE | TOML_FLAG_DT;
            if (i <= '7') f |= TOML_FLAG_OCT;
            if (i <= '1') f |= TOML_FLAG_BIN;
        }
        if ((i >= 'a' && i <= 'f') || (i >= 'A' && i <= 'F')) {
            f |= TOML_FLAG_HEX;
        }
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_' || i == '-') {
            f |= TOML_FLAG_BARE;
        }
        if (i == '-' || i == ':' || i == '.' || i == 'T' || i == 't' || i == 'Z' || i == 'z' || i == '+') {
            f |= TOML_FLAG_DT;
        }
        lut[i] = f;
    }
    return lut;
}

fn makeTomlLexHexLut(): [i8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [i8; 256] = [-1; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(i8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(i8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(i8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

const TOML_CHAR_FLAGS: [u8; 256] = makeTomlCharFlags();
const TOML_LEX_HEX_LUT: [i8; 256] = makeTomlLexHexLut();

fn isDigit(c: u8): bool {
    return (TOML_CHAR_FLAGS[@as(usize, c)] & TOML_FLAG_DIGIT) != 0;
}

fn isHexDigit(c: u8): bool {
    return (TOML_CHAR_FLAGS[@as(usize, c)] & TOML_FLAG_HEX) != 0;
}

fn isOctDigit(c: u8): bool {
    return (TOML_CHAR_FLAGS[@as(usize, c)] & TOML_FLAG_OCT) != 0;
}

fn isBinDigit(c: u8): bool {
    return (TOML_CHAR_FLAGS[@as(usize, c)] & TOML_FLAG_BIN) != 0;
}

fn isBareKeyChar(c: u8): bool {
    return (TOML_CHAR_FLAGS[@as(usize, c)] & TOML_FLAG_BARE) != 0;
}

fn hexVal(c: u8): i64 {
    return @as(i64, TOML_LEX_HEX_LUT[@as(usize, c)]);
}

fn parseRadixInt(digits: []const u8, base: i64): i64? {
    if (digits.len == 0 || digits[0] == '_' || digits[digits.len - 1] == '_') {
        return null;
    }
    let acc: i64 = 0;
    let prev_us = false;
    for (c of digits) {
        if (c == '_') {
            if (prev_us) { return null; }
            prev_us = true;
            continue;
        }
        prev_us = false;
        let ok = false;
        if (base == 16) { ok = isHexDigit(c); }
        else if (base == 8) { ok = isOctDigit(c); }
        else if (base == 2) { ok = isBinDigit(c); }
        else { ok = isDigit(c); }
        if (!ok) { return null; }
        acc = acc * base + hexVal(c);
    }
    return acc;
}

fn isIsoDatePrefix(s: []const u8): bool {
    return s.len >= 10
        && isDigit(s[0]) && isDigit(s[1]) && isDigit(s[2]) && isDigit(s[3])
        && s[4] == '-'
        && isDigit(s[5]) && isDigit(s[6])
        && s[7] == '-'
        && isDigit(s[8]) && isDigit(s[9]);
}

fn isIsoTimePrefix(s: []const u8): bool {
    return s.len >= 8
        && isDigit(s[0]) && isDigit(s[1])
        && s[2] == ':'
        && isDigit(s[3]) && isDigit(s[4])
        && s[5] == ':'
        && isDigit(s[6]) && isDigit(s[7]);
}

fn isDatetimeContChar(c: u8): bool {
    return (TOML_CHAR_FLAGS[@as(usize, c)] & TOML_FLAG_DT) != 0;
}

/// 纯 CP、零外部依赖、零堆分配 TOML v1.0 词法扫描器
pub struct TomlLexer {
    src: []const u8,
    pos: usize = 0,
    line: u32 = 1,
    col: u32 = 1,

    static fn init(src: []const u8): TomlLexer {
        return TomlLexer{
            src: src,
            pos: 0,
            line: 1,
            col: 1,
        };
    }

    fn advance(*_): u8 {
        const c = _.src[_.pos];
        _.pos += 1;
        if (c == '\n') {
            _.line += 1;
            _.col = 1;
        } else {
            _.col += 1;
        }
        return c;
    }

    fn skipSpacesAndComments(*_) {
        const v_sp: TomlLexVec16 = @splat(' ' as u8);
        const v_tb: TomlLexVec16 = @splat('\t' as u8);
        const v_lf: TomlLexVec16 = @splat('\n' as u8);
        const v_cr: TomlLexVec16 = @splat('\r' as u8);
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t') {
                while (_.pos + 16 <= _.src.len) {
                    const v: TomlLexVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                    if (!@reduce(.And, (v == v_sp) | (v == v_tb))) break;
                    _.pos += 16;
                    _.col += 16;
                }
                while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
                    * = _.advance();
                }
            } else if (c == '#') {
                * = _.advance();
                while (_.pos + 16 <= _.src.len) {
                    const v: TomlLexVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                    if (@reduce(.Or, (v == v_lf) | (v == v_cr))) break;
                    _.pos += 16;
                    _.col += 16;
                }
                while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                    * = _.advance();
                }
            } else {
                break;
            }
        }
    }

    fn makeTok(kind: TomlTokenKind, start: usize, end: usize, line: u32, col: u32): TomlToken {
        return TomlToken{
            kind: kind,
            raw: _.src[start..end],
            start: start,
            end: end,
            line: line,
            col: col,
        };
    }

    fn scanBasicString(*_, start: usize, line: u32, col: u32): TomlToken {
        // Check for multiline `"""`
        if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
            * = _.advance();
            * = _.advance();
            * = _.advance();
            let escaped = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (escaped) {
                    escaped = false;
                    * = _.advance();
                } else if (c == '\\') {
                    escaped = true;
                    * = _.advance();
                } else if (c == '"' && _.pos + 2 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
                    * = _.advance();
                    * = _.advance();
                    * = _.advance();
                    return _.makeTok(.multiline_basic_string, start, _.pos, line, col);
                } else {
                    * = _.advance();
                }
            }
            return _.makeTok(.invalid, start, _.pos, line, col);
        }

        // Single-line `"..."`
        * = _.advance();
        let escaped = false;
        const v_lf: TomlLexVec16 = @splat('\n' as u8);
        const v_cr: TomlLexVec16 = @splat('\r' as u8);
        const v_bs: TomlLexVec16 = @splat('\\' as u8);
        const v_qt: TomlLexVec16 = @splat('"' as u8);
        while (_.pos < _.src.len) {
            if (!escaped) {
                while (_.pos + 16 <= _.src.len) {
                    const v: TomlLexVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                    const stop = (v == v_lf) | (v == v_cr) | (v == v_bs) | (v == v_qt);
                    if (@reduce(.Or, stop)) break;
                    _.pos += 16;
                    _.col += 16;
                }
                if (_.pos >= _.src.len) break;
            }
            const c = _.src[_.pos];
            if (c == '\n' || c == '\r') {
                return _.makeTok(.invalid, start, _.pos, line, col);
            }
            if (escaped) {
                escaped = false;
                * = _.advance();
            } else if (c == '\\') {
                escaped = true;
                * = _.advance();
            } else if (c == '"') {
                * = _.advance();
                return _.makeTok(.basic_string, start, _.pos, line, col);
            } else {
                * = _.advance();
            }
        }
        return _.makeTok(.invalid, start, _.pos, line, col);
    }

    fn scanLiteralString(*_, start: usize, line: u32, col: u32): TomlToken {
        // Check for multiline `'''`
        if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '\'' && _.src[_.pos + 2] == '\'') {
            * = _.advance();
            * = _.advance();
            * = _.advance();
            while (_.pos < _.src.len) {
                if (_.src[_.pos] == '\'' && _.pos + 2 < _.src.len && _.src[_.pos + 1] == '\'' && _.src[_.pos + 2] == '\'') {
                    * = _.advance();
                    * = _.advance();
                    * = _.advance();
                    return _.makeTok(.multiline_literal_string, start, _.pos, line, col);
                }
                * = _.advance();
            }
            return _.makeTok(.invalid, start, _.pos, line, col);
        }

        // Single-line `'...'`
        * = _.advance();
        const v_lf: TomlLexVec16 = @splat('\n' as u8);
        const v_cr: TomlLexVec16 = @splat('\r' as u8);
        const v_sq: TomlLexVec16 = @splat('\'' as u8);
        while (_.pos + 16 <= _.src.len) {
            const v: TomlLexVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
            const stop = (v == v_lf) | (v == v_cr) | (v == v_sq);
            if (@reduce(.Or, stop)) break;
            _.pos += 16;
            _.col += 16;
        }
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\n' || c == '\r') {
                return _.makeTok(.invalid, start, _.pos, line, col);
            }
            if (c == '\'') {
                * = _.advance();
                return _.makeTok(.literal_string, start, _.pos, line, col);
            }
            * = _.advance();
        }
        return _.makeTok(.invalid, start, _.pos, line, col);
    }

    fn scanWordOrNumber(*_, start: usize, line: u32, col: u32): TomlToken {
        // 1. Check ISO 8601 date / datetime / local time
        if (isIsoDatePrefix(_.src[start..]) || isIsoTimePrefix(_.src[start..])) {
            while (_.pos < _.src.len && isDatetimeContChar(_.src[_.pos])) {
                * = _.advance();
            }
            return _.makeTok(.datetime, start, _.pos, line, col);
        }

        // 2. Consume contiguous word/number token (including + / - / . / e / E / _ / alphanumeric)
        const first = _.src[start];
        const starts_signed_or_digit = isDigit(first) || first == '+' || first == '-';
        if (starts_signed_or_digit) {
            * = _.advance();
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (isBareKeyChar(c) || c == '.' || c == '+') {
                    // Stop if '.' is followed by non-digit/non-sign when this looks like a key (e.g. `1.2` is float)
                    * = _.advance();
                } else {
                    break;
                }
            }
        } else {
            while (_.pos < _.src.len && isBareKeyChar(_.src[_.pos])) {
                * = _.advance();
            }
        }

        const raw = _.src[start.._.pos];

        // 3. Booleans
        if (std.mem.eql(u8, raw, "true")) {
            let tok = _.makeTok(.boolean, start, _.pos, line, col);
            tok.bool_val = true;
            return tok;
        }
        if (std.mem.eql(u8, raw, "false")) {
            let tok = _.makeTok(.boolean, start, _.pos, line, col);
            tok.bool_val = false;
            return tok;
        }

        // 4. Special floats: inf / +inf / -inf / nan / +nan / -nan
        if (std.mem.eql(u8, raw, "inf") || std.mem.eql(u8, raw, "+inf") || std.mem.eql(u8, raw, "-inf")
            || std.mem.eql(u8, raw, "nan") || std.mem.eql(u8, raw, "+nan") || std.mem.eql(u8, raw, "-nan"))
        {
            return _.makeTok(.float_val, start, _.pos, line, col);
        }

        // 5. Numeric literals (hex/oct/bin/dec integer or float)
        if (starts_signed_or_digit) {
            // Prefixed radix integers (`0x`, `0o`, `0b`)
            if (raw.len >= 2 && raw[0] == '0' && (raw[1] == 'x' || raw[1] == 'o' || raw[1] == 'b')) {
                const base: i64 = if (raw[1] == 'x') 16 else if (raw[1] == 'o') 8 else 2;
                if (val := parseRadixInt(raw[2..], base)) {
                    let tok = _.makeTok(.integer, start, _.pos, line, col);
                    tok.int_val = val;
                    return tok;
                }
                return _.makeTok(.invalid, start, _.pos, line, col);
            }

            // Decimal float check (contains '.' or 'e' or 'E')
            const has_dot = std.mem.indexOfScalar(u8, raw, '.') != null;
            const has_exp = std.mem.indexOfScalar(u8, raw, 'e') != null || std.mem.indexOfScalar(u8, raw, 'E') != null;
            if (has_dot || has_exp) {
                // Validate by stripping underscores into stack buffer and parsing f64
                let fbuf: [64]u8 = std.mem.zeroes([64]u8);
                let flen: usize = 0;
                let prev_us = false;
                let valid = raw.len > 0 && raw[raw.len - 1] != '_';
                for (c of raw) {
                    if (c == '_') {
                        if (prev_us || flen == 0) { valid = false; break; }
                        prev_us = true;
                        continue;
                    }
                    prev_us = false;
                    if (flen >= 64) { valid = false; break; }
                    fbuf[flen] = c;
                    flen += 1;
                }
                if (valid) {
                    if (_ := std.fmt.parseFloat(f64, fbuf[0..flen]) catch null) {
                        return _.makeTok(.float_val, start, _.pos, line, col);
                    }
                }
                return _.makeTok(.invalid, start, _.pos, line, col);
            }

            // Signed or unsigned decimal integer
            let neg = false;
            let num_slice = raw;
            if (raw[0] == '+' || raw[0] == '-') {
                neg = raw[0] == '-';
                num_slice = raw[1..];
            }
            if (num_slice.len > 0 && isDigit(num_slice[0])) {
                if (val := parseRadixInt(num_slice, 10)) {
                    let tok = _.makeTok(.integer, start, _.pos, line, col);
                    tok.int_val = if (neg) -val else val;
                    return tok;
                }
                return _.makeTok(.invalid, start, _.pos, line, col);
            }

            // Bare key starting with `-` (e.g. `-foo` is a valid TOML bare key!)
            if (first == '-') {
                return _.makeTok(.bare_key, start, _.pos, line, col);
            }
            return _.makeTok(.invalid, start, _.pos, line, col);
        }

        return _.makeTok(.bare_key, start, _.pos, line, col);
    }

    /// 扫描下一个 TOML Token
    fn next(*_): TomlToken {
        _.skipSpacesAndComments();
        const start = _.pos;
        const line = _.line;
        const col = _.col;

        if (_.pos >= _.src.len) {
            return _.makeTok(.eof, start, start, line, col);
        }

        const c = _.src[_.pos];
        if (c == '\r') {
            * = _.advance();
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                * = _.advance();
            } else {
                _.line += 1;
                _.col = 1;
            }
            return _.makeTok(.newline, start, _.pos, line, col);
        }
        if (c == '\n') {
            * = _.advance();
            return _.makeTok(.newline, start, _.pos, line, col);
        }

        if (c == '=') { * = _.advance(); return _.makeTok(.equals, start, _.pos, line, col); }
        if (c == '.') { * = _.advance(); return _.makeTok(.dot, start, _.pos, line, col); }
        if (c == ',') { * = _.advance(); return _.makeTok(.comma, start, _.pos, line, col); }
        if (c == '{') { * = _.advance(); return _.makeTok(.lbrace, start, _.pos, line, col); }
        if (c == '}') { * = _.advance(); return _.makeTok(.rbrace, start, _.pos, line, col); }

        if (c == '[') {
            * = _.advance();
            if (_.pos < _.src.len && _.src[_.pos] == '[') {
                * = _.advance();
                return _.makeTok(.double_lbracket, start, _.pos, line, col);
            }
            return _.makeTok(.lbracket, start, _.pos, line, col);
        }
        if (c == ']') {
            * = _.advance();
            if (_.pos < _.src.len && _.src[_.pos] == ']') {
                * = _.advance();
                return _.makeTok(.double_rbracket, start, _.pos, line, col);
            }
            return _.makeTok(.rbracket, start, _.pos, line, col);
        }

        if (c == '"') {
            return _.scanBasicString(start, line, col);
        }
        if (c == '\'') {
            return _.scanLiteralString(start, line, col);
        }

        if (isBareKeyChar(c) || c == '+') {
            return _.scanWordOrNumber(start, line, col);
        }

        * = _.advance();
        return _.makeTok(.invalid, start, _.pos, line, col);
    }
}

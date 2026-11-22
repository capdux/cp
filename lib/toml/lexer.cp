// Source: Absorbed from /data/dev/bun/src/interchange/toml/lexer.cp | Synced: 2026-11-22T12:49:17Z

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

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isOctDigit(c: u8): bool {
    return c >= '0' && c <= '7';
}

fn isBinDigit(c: u8): bool {
    return c == '0' || c == '1';
}

fn isBareKeyChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || isDigit(c)
        || c == '_'
        || c == '-';
}

fn hexVal(c: u8): i64 {
    if (c >= '0' && c <= '9') { return (c - '0') as i64; }
    if (c >= 'a' && c <= 'f') { return (c - 'a' + 10) as i64; }
    return (c - 'A' + 10) as i64;
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
    return isDigit(c) || c == '-' || c == ':' || c == '.' || c == 'T' || c == 't' || c == 'Z' || c == 'z' || c == '+';
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
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t') {
                * = _.advance();
            } else if (c == '#') {
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
        while (_.pos < _.src.len) {
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

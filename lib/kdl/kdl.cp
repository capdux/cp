// Origin: Pure-CP Stdlib (Zero-Alloc KDL Document Lexer & Validator) | Created: 2026-11-22
//! Zero-allocation KDL configuration document lexer, node-header iterator, and structural validator.

pub enum KdlTokenKind: u8 {
    ident = 0,
    string = 1,
    number = 2,
    bool_lit = 3,
    null_lit = 4,
    equals = 5,
    lbrace = 6,
    rbrace = 7,
    semicolon = 8,
    newline = 9,
    slashdash = 10,
    invalid = 11,
}

pub struct KdlToken {
    kind: KdlTokenKind,
    lexeme: []const u8,
}

pub struct KdlNodeSummary {
    name: []const u8,
    arg_count: u16,
    prop_count: u16,
    has_children: bool,
    depth: u16,
}

const MAX_KDL_DEPTH: u16 = 64;

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isOctalDigit(c: u8): bool {
    return c >= '0' && c <= '7';
}

fn isBinaryDigit(c: u8): bool {
    return c == '0' || c == '1';
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isNewlineByte(c: u8): bool {
    return c == '\n' || c == '\r';
}

fn isDelimiter(c: u8): bool {
    return isHorizSpace(c) || isNewlineByte(c) || c == '{' || c == '}' || c == ';' || c == '=' || c == '(' || c == ')' || c == '[' || c == ']' || c == '/' || c == '"' || c == '\\';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

pub struct KdlIter {
    input: []const u8,
    pos: usize,

    pub static fn init(input: []const u8): KdlIter {
        let start: usize = 0;
        // Skip UTF-8 BOM (0xEF 0xBB 0xBF)
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return KdlIter{
            input: input,
            pos: start,
        };
    }

    fn skipNestedBlockComment(*_): bool {
        const s = _.input;
        // Caller verified s[pos..pos+2] == "/*"
        _.pos += 2;
        let depth: usize = 1;
        while (_.pos < s.len && depth > 0) {
            if (_.pos + 1 < s.len && s[_.pos] == '/' && s[_.pos + 1] == '*') {
                depth += 1;
                _.pos += 2;
                continue;
            }
            if (_.pos + 1 < s.len && s[_.pos] == '*' && s[_.pos + 1] == '/') {
                depth -= 1;
                _.pos += 2;
                continue;
            }
            _.pos += 1;
        }
        return depth == 0;
    }

    fn skipLineContinuation(*_): bool {
        const s = _.input;
        // Caller verified s[pos] == '\\'
        _.pos += 1;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isHorizSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == 0xC2 && _.pos + 1 < s.len && s[_.pos + 1] == 0xA0) {
                _.pos += 2;
                continue;
            }
            if (c == '/' && _.pos + 1 < s.len && s[_.pos + 1] == '*') {
                if (!_.skipNestedBlockComment()) return false;
                continue;
            }
            if (c == '/' && _.pos + 1 < s.len && s[_.pos + 1] == '/') {
                _.pos += 2;
                while (_.pos < s.len && !isNewlineByte(s[_.pos])) {
                    _.pos += 1;
                }
                continue;
            }
            if (c == '\r') {
                _.pos += 1;
                if (_.pos < s.len && s[_.pos] == '\n') _.pos += 1;
                return true;
            }
            if (c == '\n') {
                _.pos += 1;
                return true;
            }
            return false;
        }
        return false;
    }

    fn scanQuotedString(*_): KdlToken {
        const s = _.input;
        const start = _.pos;
        _.pos += 1; // skip opening '"'
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (c == '"') {
                _.pos += 1;
                return KdlToken{ kind: KdlTokenKind.string, lexeme: s[start.._.pos] };
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= s.len) {
                    return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                const esc = s[_.pos];
                if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                    _.pos += 1;
                    continue;
                }
                if (esc == 'u') {
                    _.pos += 1;
                    if (_.pos >= s.len || s[_.pos] != '{') {
                        return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    _.pos += 1;
                    const hex_start = _.pos;
                    while (_.pos < s.len && isHexDigit(s[_.pos])) {
                        _.pos += 1;
                    }
                    const hex_len = _.pos - hex_start;
                    if (hex_len == 0 || hex_len > 6 || _.pos >= s.len || s[_.pos] != '}') {
                        return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    _.pos += 1;
                    continue;
                }
                // Unknown escape in KDL string
                return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            _.pos += 1;
        }
        return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
    }

    fn scanRawString(*_): KdlToken {
        const s = _.input;
        const start = _.pos;
        _.pos += 1; // skip 'r'
        let hashes: usize = 0;
        while (_.pos < s.len && s[_.pos] == '#') {
            hashes += 1;
            _.pos += 1;
        }
        if (_.pos >= s.len || s[_.pos] != '"') {
            return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
        }
        _.pos += 1; // skip opening '"'
        while (_.pos < s.len) {
            if (s[_.pos] == '"') {
                _.pos += 1;
                let matched: usize = 0;
                while (matched < hashes && _.pos < s.len && s[_.pos] == '#') {
                    matched += 1;
                    _.pos += 1;
                }
                if (matched == hashes) {
                    return KdlToken{ kind: KdlTokenKind.string, lexeme: s[start.._.pos] };
                }
                continue;
            }
            _.pos += 1;
        }
        return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
    }

    fn scanNumber(*_): KdlToken {
        const s = _.input;
        const start = _.pos;
        if (_.pos < s.len && (s[_.pos] == '+' || s[_.pos] == '-')) {
            _.pos += 1;
        }
        if (_.pos >= s.len || !isAsciiDigit(s[_.pos])) {
            return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
        }

        // Prefixed base: 0x / 0o / 0b
        if (s[_.pos] == '0' && _.pos + 1 < s.len) {
            const p = s[_.pos + 1];
            if (p == 'x' || p == 'X') {
                _.pos += 2;
                let digits: usize = 0;
                while (_.pos < s.len && (isHexDigit(s[_.pos]) || s[_.pos] == '_')) {
                    if (isHexDigit(s[_.pos])) digits += 1;
                    _.pos += 1;
                }
                if (digits == 0 || (_.pos < s.len && !isDelimiter(s[_.pos]))) {
                    return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                return KdlToken{ kind: .number, lexeme: s[start.._.pos] };
            }
            if (p == 'o' || p == 'O') {
                _.pos += 2;
                let digits: usize = 0;
                while (_.pos < s.len && (isOctalDigit(s[_.pos]) || s[_.pos] == '_')) {
                    if (isOctalDigit(s[_.pos])) digits += 1;
                    _.pos += 1;
                }
                if (digits == 0 || (_.pos < s.len && !isDelimiter(s[_.pos]))) {
                    return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                return KdlToken{ kind: .number, lexeme: s[start.._.pos] };
            }
            if (p == 'b' || p == 'B') {
                _.pos += 2;
                let digits: usize = 0;
                while (_.pos < s.len && (isBinaryDigit(s[_.pos]) || s[_.pos] == '_')) {
                    if (isBinaryDigit(s[_.pos])) digits += 1;
                    _.pos += 1;
                }
                if (digits == 0 || (_.pos < s.len && !isDelimiter(s[_.pos]))) {
                    return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                return KdlToken{ kind: .number, lexeme: s[start.._.pos] };
            }
        }

        // Decimal integer part
        while (_.pos < s.len && (isAsciiDigit(s[_.pos]) || s[_.pos] == '_')) {
            _.pos += 1;
        }

        // Optional fractional part
        if (_.pos < s.len && s[_.pos] == '.') {
            _.pos += 1;
            if (_.pos >= s.len || !isAsciiDigit(s[_.pos])) {
                return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            while (_.pos < s.len && (isAsciiDigit(s[_.pos]) || s[_.pos] == '_')) {
                _.pos += 1;
            }
        }

        // Optional exponent part
        if (_.pos < s.len && (s[_.pos] == 'e' || s[_.pos] == 'E')) {
            _.pos += 1;
            if (_.pos < s.len && (s[_.pos] == '+' || s[_.pos] == '-')) {
                _.pos += 1;
            }
            let exp_digits: usize = 0;
            while (_.pos < s.len && (isAsciiDigit(s[_.pos]) || s[_.pos] == '_')) {
                if (isAsciiDigit(s[_.pos])) exp_digits += 1;
                _.pos += 1;
            }
            if (exp_digits == 0) {
                return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
            }
        }

        if (_.pos < s.len && !isDelimiter(s[_.pos])) {
            return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
        }
        return KdlToken{ kind: .number, lexeme: s[start.._.pos] };
    }

    pub fn next(*_): KdlToken? {
        const s = _.input;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isHorizSpace(c)) {
                _.pos += 1;
                continue;
            }
            // UTF-8 NBSP
            if (c == 0xC2 && _.pos + 1 < s.len && s[_.pos + 1] == 0xA0) {
                _.pos += 2;
                continue;
            }
            // Line continuation '\'
            if (c == '\\') {
                const start = _.pos;
                if (!_.skipLineContinuation()) {
                    return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                continue;
            }
            // Comments: // and nested /* ... */
            if (c == '/' && _.pos + 1 < s.len) {
                const n = s[_.pos + 1];
                if (n == '/') {
                    _.pos += 2;
                    while (_.pos < s.len && !isNewlineByte(s[_.pos])) {
                        _.pos += 1;
                    }
                    continue;
                }
                if (n == '*') {
                    const start = _.pos;
                    if (!_.skipNestedBlockComment()) {
                        return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    continue;
                }
                if (n == '-') {
                    const start = _.pos;
                    _.pos += 2;
                    return KdlToken{ kind: .slashdash, lexeme: s[start.._.pos] };
                }
            }
            break;
        }

        if (_.pos >= s.len) return null;

        const start = _.pos;
        const c = s[_.pos];

        if (c == '\r') {
            _.pos += 1;
            if (_.pos < s.len && s[_.pos] == '\n') _.pos += 1;
            return KdlToken{ kind: .newline, lexeme: s[start.._.pos] };
        }
        if (c == '\n') {
            _.pos += 1;
            return KdlToken{ kind: .newline, lexeme: s[start.._.pos] };
        }
        if (c == '{') { _.pos += 1; return KdlToken{ kind: .lbrace, lexeme: s[start.._.pos] }; }
        if (c == '}') { _.pos += 1; return KdlToken{ kind: .rbrace, lexeme: s[start.._.pos] }; }
        if (c == ';') { _.pos += 1; return KdlToken{ kind: .semicolon, lexeme: s[start.._.pos] }; }
        if (c == '=') { _.pos += 1; return KdlToken{ kind: .equals, lexeme: s[start.._.pos] }; }

        if (c == '"') {
            return _.scanQuotedString();
        }

        // Raw string: r"..." or r#"..."#
        if (c == 'r' && _.pos + 1 < s.len && (s[_.pos + 1] == '"' || s[_.pos + 1] == '#')) {
            return _.scanRawString();
        }

        // Number: digit or +/- followed by digit
        if (isAsciiDigit(c) || ((c == '+' || c == '-') && _.pos + 1 < s.len && isAsciiDigit(s[_.pos + 1]))) {
            return _.scanNumber();
        }

        // Reject stray delimiters like '/', '(', ')', '[', ']'
        if (isDelimiter(c)) {
            _.pos += 1;
            return KdlToken{ kind: .invalid, lexeme: s[start.._.pos] };
        }

        // Bare identifier or keyword
        _.pos += 1;
        while (_.pos < s.len && !isDelimiter(s[_.pos])) {
            _.pos += 1;
        }
        const lex = s[start.._.pos];
        if (bytesEql(lex, "true") || bytesEql(lex, "false")) {
            return KdlToken{ kind: .bool_lit, lexeme: lex };
        }
        if (bytesEql(lex, "null")) {
            return KdlToken{ kind: .null_lit, lexeme: lex };
        }
        return KdlToken{ kind: .ident, lexeme: lex };
    }
}

fn isValueKind(k: KdlTokenKind): bool {
    return k == KdlTokenKind.string || k == .number || k == .bool_lit || k == .null_lit || k == .ident;
}

pub struct KdlNodeIter {
    iter: KdlIter,
    peeked: KdlToken?,
    depth: u16,

    pub static fn init(input: []const u8): KdlNodeIter {
        return KdlNodeIter{
            iter: KdlIter.init(input),
            peeked: null,
            depth: 0,
        };
    }

    fn nextTok(*_): KdlToken? {
        if (p := _.peeked) {
            _.peeked = null;
            return p;
        }
        return _.iter.next();
    }

    fn peekTok(*_): KdlToken? {
        if (_.peeked == null) {
            _.peeked = _.iter.next();
        }
        return _.peeked;
    }

    fn skipBraceBlock(*_): bool {
        // Caller already consumed opening '{'
        let d: usize = 1;
        while (d > 0) {
            const tok = _.nextTok() ?? return false;
            if (tok.kind == .invalid) return false;
            if (tok.kind == .lbrace) {
                d += 1;
            } else if (tok.kind == .rbrace) {
                d -= 1;
            }
        }
        return true;
    }

    fn skipCommentedNode(*_): bool {
        // Skip tokens belonging to a slashdashed node until newline / semicolon / rbrace / EOF.
        // If the node has a child block '{ ... }', skip the entire balanced block too.
        while (tok := _.nextTok()) {
            if (tok.kind == .invalid) return false;
            if (tok.kind == .newline || tok.kind == .semicolon) return true;
            if (tok.kind == .rbrace) {
                if (_.depth > 0) _.depth -= 1;
                return true;
            }
            if (tok.kind == .lbrace) {
                if (!_.skipBraceBlock()) return false;
                return true;
            }
        }
        return true;
    }

    pub fn next(*_): KdlNodeSummary? {
        while (true) {
            const tok = _.nextTok() ?? return null;
            if (tok.kind == .newline || tok.kind == .semicolon) continue;
            if (tok.kind == .rbrace) {
                if (_.depth > 0) {
                    _.depth -= 1;
                    continue;
                }
                return null;
            }
            if (tok.kind == .slashdash) {
                if (!_.skipCommentedNode()) return null;
                continue;
            }
            if (tok.kind != .ident && tok.kind != KdlTokenKind.string) {
                return null;
            }

            const node_depth = _.depth;
            let summary = KdlNodeSummary{
                name: tok.lexeme,
                arg_count: 0,
                prop_count: 0,
                has_children: false,
                depth: node_depth,
            };

            while (true) {
                const item = _.nextTok() ?? return summary;
                if (item.kind == .newline || item.kind == .semicolon) {
                    return summary;
                }
                if (item.kind == .rbrace) {
                    if (_.depth > 0) _.depth -= 1;
                    return summary;
                }
                if (item.kind == .lbrace) {
                    summary.has_children = true;
                    if (_.depth < MAX_KDL_DEPTH) _.depth += 1;
                    return summary;
                }
                if (item.kind == .slashdash) {
                    const skipped = _.nextTok() ?? return summary;
                    if (skipped.kind == .lbrace) {
                        if (!_.skipBraceBlock()) return null;
                        continue;
                    }
                    if (skipped.kind == .ident || skipped.kind == KdlTokenKind.string) {
                        if (pk := _.peekTok()) {
                            if (pk.kind == .equals) {
                                * = _.nextTok(); // consume '='
                                * = _.nextTok(); // consume prop value
                            }
                        }
                    }
                    continue;
                }
                if (item.kind == .ident || item.kind == KdlTokenKind.string) {
                    if (pk := _.peekTok()) {
                        if (pk.kind == .equals) {
                            * = _.nextTok(); // consume '='
                            const val = _.nextTok() ?? return null;
                            if (!isValueKind(val.kind)) return null;
                            summary.prop_count += 1;
                            continue;
                        }
                    }
                    summary.arg_count += 1;
                    continue;
                }
                if (isValueKind(item.kind)) {
                    summary.arg_count += 1;
                    continue;
                }
                return null;
            }
        }
    }
}

pub fn validateKdl(input: []const u8): bool {
    let it = KdlIter.init(input);
    let depth: u16 = 0;
    let at_node_start = true;
    let expect_val_after_eq = false;
    let after_slashdash = false;

    while (tok := it.next()) {
        if (tok.kind == .invalid) return false;

        if (expect_val_after_eq) {
            if (!isValueKind(tok.kind)) return false;
            expect_val_after_eq = false;
            continue;
        }

        if (tok.kind == .slashdash) {
            after_slashdash = true;
            continue;
        }

        if (tok.kind == .newline || tok.kind == .semicolon) {
            if (after_slashdash) return false;
            at_node_start = true;
            continue;
        }

        if (tok.kind == .lbrace) {
            if (at_node_start && !after_slashdash) return false;
            if (depth >= MAX_KDL_DEPTH) return false;
            depth += 1;
            at_node_start = true;
            after_slashdash = false;
            continue;
        }

        if (tok.kind == .rbrace) {
            if (after_slashdash || depth == 0) return false;
            depth -= 1;
            at_node_start = true;
            continue;
        }

        if (tok.kind == .equals) {
            if (at_node_start || after_slashdash) return false;
            expect_val_after_eq = true;
            continue;
        }

        if (at_node_start) {
            if (tok.kind != .ident && tok.kind != KdlTokenKind.string) return false;
            at_node_start = false;
            after_slashdash = false;
            continue;
        }

        after_slashdash = false;
    }

    if (expect_val_after_eq || after_slashdash) return false;
    return depth == 0;
}

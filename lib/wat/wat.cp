// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc WebAssembly Text WAT Lexer & Scanner)
//! Zero-allocation WebAssembly Text Format (.wat) lexer, export extractor, and structural validator.

pub enum WatTokenKind: u8 {
    lparen = 0,
    rparen = 1,
    keyword = 2,
    id = 3,
    string = 4,
    number = 5,
    invalid = 6,
}

pub struct WatToken {
    kind: WatTokenKind,
    lexeme: []const u8,
}

pub struct WatExport {
    name: []const u8,
    kind: []const u8,
    target: []const u8,
}

const MAX_WAT_DEPTH: usize = 64;

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isWatIdChar(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return true;
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' ||
        c == '*' || c == '+' || c == '-' || c == '.' || c == '/' || c == ':' ||
        c == '<' || c == '=' || c == '>' || c == '?' || c == '@' || c == '\\' ||
        c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return bytesEql(s[0..prefix.len], prefix);
}

fn unquoteWatString(lex: []const u8): []const u8 {
    if (lex.len >= 2 && lex[0] == '"' && lex[lex.len - 1] == '"') {
        return lex[1 .. lex.len - 1];
    }
    return lex;
}

fn isExportEntityKind(s: []const u8): bool {
    return bytesEql(s, "func") || bytesEql(s, "memory") || bytesEql(s, "table") || bytesEql(s, "global") || bytesEql(s, "tag");
}

fn indexLabel(idx: usize): []const u8 {
    return switch (idx) {
        0 => "0",
        1 => "1",
        2 => "2",
        3 => "3",
        4 => "4",
        5 => "5",
        6 => "6",
        7 => "7",
        8 => "8",
        9 => "9",
        10 => "10",
        11 => "11",
        12 => "12",
        13 => "13",
        14 => "14",
        15 => "15",
        else => "N",
    };
}

fn isWatNumberLexeme(lex: []const u8): bool {
    if (lex.len == 0) return false;
    let i: usize = 0;
    if (lex[i] == '+' || lex[i] == '-') {
        i += 1;
        if (i >= lex.len) return false;
    }
    const rest = lex[i..];
    if (bytesEql(rest, "inf") || bytesEql(rest, "nan")) return true;
    if (startsWith(rest, "nan:0x") && rest.len > 6) {
        let k: usize = 6;
        let digits: usize = 0;
        while (k < rest.len) : (k += 1) {
            if (isHexDigit(rest[k])) {
                digits += 1;
            } else if (rest[k] != '_') {
                return false;
            }
        }
        return digits > 0;
    }

    // Hex integer / float: 0x...
    if (rest.len >= 3 && rest[0] == '0' && (rest[1] == 'x' || rest[1] == 'X')) {
        let k: usize = 2;
        let digits: usize = 0;
        while (k < rest.len && (isHexDigit(rest[k]) || rest[k] == '_')) {
            if (isHexDigit(rest[k])) digits += 1;
            k += 1;
        }
        if (digits == 0) return false;
        if (k < rest.len && rest[k] == '.') {
            k += 1;
            while (k < rest.len && (isHexDigit(rest[k]) || rest[k] == '_')) k += 1;
        }
        if (k < rest.len && (rest[k] == 'p' || rest[k] == 'P')) {
            k += 1;
            if (k < rest.len && (rest[k] == '+' || rest[k] == '-')) k += 1;
            let ed: usize = 0;
            while (k < rest.len && (isAsciiDigit(rest[k]) || rest[k] == '_')) {
                if (isAsciiDigit(rest[k])) ed += 1;
                k += 1;
            }
            if (ed == 0) return false;
        }
        return k == rest.len;
    }

    // Decimal integer / float
    if (!isAsciiDigit(rest[0])) return false;
    let k: usize = 0;
    while (k < rest.len && (isAsciiDigit(rest[k]) || rest[k] == '_')) k += 1;
    if (k < rest.len && rest[k] == '.') {
        k += 1;
        while (k < rest.len && (isAsciiDigit(rest[k]) || rest[k] == '_')) k += 1;
    }
    if (k < rest.len && (rest[k] == 'e' || rest[k] == 'E')) {
        k += 1;
        if (k < rest.len && (rest[k] == '+' || rest[k] == '-')) k += 1;
        let ed: usize = 0;
        while (k < rest.len && (isAsciiDigit(rest[k]) || rest[k] == '_')) {
            if (isAsciiDigit(rest[k])) ed += 1;
            k += 1;
        }
        if (ed == 0) return false;
    }
    return k == rest.len;
}

pub struct WatIter {
    input: []const u8,
    pos: usize,

    pub static fn init(input: []const u8): WatIter {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return WatIter{
            input: input,
            pos: start,
        };
    }

    fn skipNestedBlockComment(*_): bool {
        const s = _.input;
        // Caller verified s[pos..pos+2] == "(;"
        _.pos += 2;
        let depth: usize = 1;
        while (_.pos < s.len && depth > 0) {
            if (_.pos + 1 < s.len && s[_.pos] == '(' && s[_.pos + 1] == ';') {
                depth += 1;
                _.pos += 2;
                continue;
            }
            if (_.pos + 1 < s.len && s[_.pos] == ';' && s[_.pos + 1] == ')') {
                depth -= 1;
                _.pos += 2;
                continue;
            }
            _.pos += 1;
        }
        return depth == 0;
    }

    fn scanString(*_): WatToken {
        const s = _.input;
        const start = _.pos;
        _.pos += 1; // skip opening '"'
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (c == '"') {
                _.pos += 1;
                return WatToken{ kind: WatTokenKind.string, lexeme: s[start.._.pos] };
            }
            if (c == '\n' || c == '\r') {
                return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= s.len) {
                    return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                const esc = s[_.pos];
                if (esc == 't' || esc == 'n' || esc == 'r' || esc == '"' || esc == '\'' || esc == '\\') {
                    _.pos += 1;
                    continue;
                }
                if (esc == 'u') {
                    _.pos += 1;
                    if (_.pos >= s.len || s[_.pos] != '{') {
                        return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    _.pos += 1;
                    const hs = _.pos;
                    while (_.pos < s.len && (isHexDigit(s[_.pos]) || s[_.pos] == '_')) _.pos += 1;
                    if (_.pos == hs || _.pos >= s.len || s[_.pos] != '}') {
                        return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
                    }
                    _.pos += 1;
                    continue;
                }
                // Two hex digits: \HH
                if (isHexDigit(esc) && _.pos + 1 < s.len && isHexDigit(s[_.pos + 1])) {
                    _.pos += 2;
                    continue;
                }
                return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            _.pos += 1;
        }
        return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
    }

    pub fn next(*_): WatToken? {
        const s = _.input;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isWhitespace(c)) {
                _.pos += 1;
                continue;
            }
            // Line comment: ;; ...
            if (c == ';' && _.pos + 1 < s.len && s[_.pos + 1] == ';') {
                _.pos += 2;
                while (_.pos < s.len && s[_.pos] != '\n' && s[_.pos] != '\r') {
                    _.pos += 1;
                }
                continue;
            }
            // Nested block comment: (; ... ;)
            if (c == '(' && _.pos + 1 < s.len && s[_.pos + 1] == ';') {
                const start = _.pos;
                if (!_.skipNestedBlockComment()) {
                    return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
                }
                continue;
            }
            break;
        }

        if (_.pos >= s.len) return null;

        const start = _.pos;
        const c = s[_.pos];

        if (c == '(') { _.pos += 1; return WatToken{ kind: .lparen, lexeme: s[start.._.pos] }; }
        if (c == ')') { _.pos += 1; return WatToken{ kind: .rparen, lexeme: s[start.._.pos] }; }
        if (c == '"') { return _.scanString(); }

        if (c == '$') {
            _.pos += 1;
            const id_start = _.pos;
            while (_.pos < s.len && isWatIdChar(s[_.pos])) {
                _.pos += 1;
            }
            if (_.pos == id_start) {
                return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
            }
            return WatToken{ kind: .id, lexeme: s[start.._.pos] };
        }

        if (isWatIdChar(c)) {
            _.pos += 1;
            while (_.pos < s.len && isWatIdChar(s[_.pos])) {
                _.pos += 1;
            }
            const lex = s[start.._.pos];
            if (isWatNumberLexeme(lex)) {
                return WatToken{ kind: .number, lexeme: lex };
            }
            // Keywords in WAT start with lowercase ASCII letter
            if (lex[0] >= 'a' && lex[0] <= 'z') {
                return WatToken{ kind: .keyword, lexeme: lex };
            }
            return WatToken{ kind: .invalid, lexeme: lex };
        }

        _.pos += 1;
        return WatToken{ kind: .invalid, lexeme: s[start.._.pos] };
    }
}

pub struct WatExportIter {
    iter: WatIter,
    depth: usize,
    in_module: bool,
    cur_entity_kind: []const u8,
    cur_entity_target: []const u8,
    func_idx: usize,
    mem_idx: usize,
    table_idx: usize,
    global_idx: usize,
    tag_idx: usize,

    pub static fn init(input: []const u8): WatExportIter {
        return WatExportIter{
            iter: WatIter.init(input),
            depth: 0,
            in_module: false,
            cur_entity_kind: "",
            cur_entity_target: "",
            func_idx: 0,
            mem_idx: 0,
            table_idx: 0,
            global_idx: 0,
            tag_idx: 0,
        };
    }

    fn allocEntityIndex(*_, kind: []const u8): []const u8 {
        if (bytesEql(kind, "func")) {
            const idx = _.func_idx;
            _.func_idx += 1;
            return indexLabel(idx);
        }
        if (bytesEql(kind, "memory")) {
            const idx = _.mem_idx;
            _.mem_idx += 1;
            return indexLabel(idx);
        }
        if (bytesEql(kind, "table")) {
            const idx = _.table_idx;
            _.table_idx += 1;
            return indexLabel(idx);
        }
        if (bytesEql(kind, "global")) {
            const idx = _.global_idx;
            _.global_idx += 1;
            return indexLabel(idx);
        }
        const idx = _.tag_idx;
        _.tag_idx += 1;
        return indexLabel(idx);
    }

    pub fn next(*_): WatExport? {
        while (tok := _.iter.next()) {
            if (tok.kind == .invalid) return null;

            if (tok.kind == .rparen) {
                if (_.depth > 0) _.depth -= 1;
                if (_.depth <= 1) {
                    _.cur_entity_kind = "";
                    _.cur_entity_target = "";
                }
                if (_.depth == 0) {
                    _.in_module = false;
                }
                continue;
            }

            if (tok.kind != .lparen) continue;
            _.depth += 1;

            const head = _.iter.next() ?? return null;
            if (head.kind == .invalid) return null;
            if (head.kind == .rparen) {
                if (_.depth > 0) _.depth -= 1;
                continue;
            }

            // Depth 1: (module ...)
            if (_.depth == 1 && head.kind == .keyword && bytesEql(head.lexeme, "module")) {
                _.in_module = true;
                continue;
            }

            if (!_.in_module) continue;

            // Depth 2: top-level module field
            if (_.depth == 2 && head.kind == .keyword) {
                // Standalone: (export "name" (func $id))
                if (bytesEql(head.lexeme, "export")) {
                    const name_tok = _.iter.next() ?? return null;
                    if (name_tok.kind != WatTokenKind.string) continue;
                    const lp = _.iter.next() ?? return null;
                    if (lp.kind != .lparen) continue;
                    _.depth += 1;
                    const kind_tok = _.iter.next() ?? return null;
                    if (kind_tok.kind != .keyword || !isExportEntityKind(kind_tok.lexeme)) continue;
                    const target_tok = _.iter.next() ?? return null;
                    if (target_tok.kind != .id && target_tok.kind != .number) continue;
                    return WatExport{
                        name: unquoteWatString(name_tok.lexeme),
                        kind: kind_tok.lexeme,
                        target: target_tok.lexeme,
                    };
                }

                // Entity declaration: (func [$id] (export "name") ...)
                if (isExportEntityKind(head.lexeme)) {
                    const default_target = _.allocEntityIndex(head.lexeme);
                    _.cur_entity_kind = head.lexeme;
                    _.cur_entity_target = default_target;

                    // Peek next token for optional $id or immediate inline (export "name")
                    const next_t = _.iter.next() ?? return null;
                    if (next_t.kind == .id) {
                        _.cur_entity_target = next_t.lexeme;
                    } else if (next_t.kind == .rparen) {
                        if (_.depth > 0) _.depth -= 1;
                        _.cur_entity_kind = "";
                        _.cur_entity_target = "";
                    } else if (next_t.kind == .lparen) {
                        _.depth += 1;
                        const sub = _.iter.next() ?? return null;
                        if (sub.kind == .keyword && bytesEql(sub.lexeme, "export")) {
                            const name_tok = _.iter.next() ?? return null;
                            if (name_tok.kind == WatTokenKind.string) {
                                return WatExport{
                                    name: unquoteWatString(name_tok.lexeme),
                                    kind: _.cur_entity_kind,
                                    target: _.cur_entity_target,
                                };
                            }
                        } else if (sub.kind == .rparen) {
                            if (_.depth > 0) _.depth -= 1;
                        }
                    }
                    continue;
                }
            }

            // Depth 3 inside entity: inline (export "name")
            if (_.depth == 3 && _.cur_entity_kind.len > 0 && head.kind == .keyword && bytesEql(head.lexeme, "export")) {
                const name_tok = _.iter.next() ?? return null;
                if (name_tok.kind == WatTokenKind.string) {
                    return WatExport{
                        name: unquoteWatString(name_tok.lexeme),
                        kind: _.cur_entity_kind,
                        target: _.cur_entity_target,
                    };
                }
            }
        }
        return null;
    }
}

pub fn validateWatStructure(input: []const u8): bool {
    let it = WatIter.init(input);

    // Must start with '(' 'module'
    const first = it.next() ?? return false;
    if (first.kind != .lparen) return false;
    const kw = it.next() ?? return false;
    if (kw.kind != .keyword || !bytesEql(kw.lexeme, "module")) return false;

    let depth: usize = 1;
    let module_closed = false;

    while (tok := it.next()) {
        if (tok.kind == .invalid) return false;
        if (module_closed) return false;

        if (tok.kind == .lparen) {
            if (depth >= MAX_WAT_DEPTH) return false;
            depth += 1;
            continue;
        }
        if (tok.kind == .rparen) {
            if (depth == 0) return false;
            depth -= 1;
            if (depth == 0) {
                module_closed = true;
            }
            continue;
        }
    }

    return module_closed && depth == 0;
}

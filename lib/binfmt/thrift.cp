// Origin: Pure-CP Stdlib (Zero-Alloc Apache Thrift IDL Lexer & Scanner) | Created: 2026-11-22
//! Zero-allocation Apache Thrift (.thrift) IDL declaration scanner, struct field iterator, and brace validator.

pub enum ThriftDeclKind: u8 {
    namespace_decl = 0,
    include_decl = 1,
    typedef_decl = 2,
    enum_decl = 3,
    struct_decl = 4,
    union_decl = 5,
    exception_decl = 6,
    service_decl = 7,
    const_decl = 8,
}

pub struct ThriftDecl {
    kind: ThriftDeclKind,
    name: []const u8,
    extends_name: ([]const u8)?,
    body: ([]const u8)?,
}

pub struct ThriftField {
    field_id: i32?,
    is_required: bool,
    is_optional: bool,
    ty: []const u8,
    name: []const u8,
}

const MAX_BRACE_DEPTH: usize = 64;

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || isAsciiDigit(c) || c == '.';
}

fn isFieldNameChar(c: u8): bool {
    return isIdentStart(c) || isAsciiDigit(c);
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWhitespace(s[start])) start += 1;
    while (end > start && isWhitespace(s[end - 1])) end -= 1;
    return s[start..end];
}

struct Scanner {
    input: []const u8,
    pos: usize,

    static fn init(input: []const u8): Scanner {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return Scanner{ input: input, pos: start };
    }

    fn skipWsAndComments(*_): bool {
        const s = _.input;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isWhitespace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.pos += 1;
                while (_.pos < s.len && s[_.pos] != '\n' && s[_.pos] != '\r') _.pos += 1;
                continue;
            }
            if (c == '/' && _.pos + 1 < s.len) {
                if (s[_.pos + 1] == '/') {
                    _.pos += 2;
                    while (_.pos < s.len && s[_.pos] != '\n' && s[_.pos] != '\r') _.pos += 1;
                    continue;
                }
                if (s[_.pos + 1] == '*') {
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
                    if (!closed) return false;
                    continue;
                }
            }
            break;
        }
        return true;
    }

    fn skipString(*_): ([]const u8)? {
        const s = _.input;
        if (_.pos >= s.len) return null;
        const quote = s[_.pos];
        if (quote != '"' && quote != '\'') return null;
        const inner_start = _.pos + 1;
        _.pos += 1;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (c == '\\') {
                _.pos += 2;
                if (_.pos > s.len) return null;
                continue;
            }
            if (c == quote) {
                const inner_end = _.pos;
                _.pos += 1;
                return s[inner_start..inner_end];
            }
            _.pos += 1;
        }
        return null;
    }

    fn scanIdent(*_): ([]const u8)? {
        const s = _.input;
        if (_.pos >= s.len || !isIdentStart(s[_.pos])) return null;
        const start = _.pos;
        _.pos += 1;
        while (_.pos < s.len && isIdentContinue(s[_.pos])) _.pos += 1;
        return s[start.._.pos];
    }

    fn scanSimpleName(*_): ([]const u8)? {
        const s = _.input;
        if (_.pos >= s.len || !isIdentStart(s[_.pos])) return null;
        const start = _.pos;
        _.pos += 1;
        while (_.pos < s.len && isFieldNameChar(s[_.pos])) _.pos += 1;
        return s[start.._.pos];
    }

    /// Scans a balanced delimited block (e.g. '{'..'}', '('..')', '['..']', '<'..'>')
    /// and returns the inner slice excluding the outer delimiters.
    fn scanBalanced(*_, open_ch: u8, close_ch: u8): ([]const u8)? {
        const s = _.input;
        if (_.pos >= s.len || s[_.pos] != open_ch) return null;
        _.pos += 1;
        const inner_start = _.pos;
        let depth: usize = 1;
        while (_.pos < s.len && depth > 0) {
            const c = s[_.pos];
            if (c == '"' || c == '\'') {
                if (_.skipString() == null) return null;
                continue;
            }
            if (c == '#' || (c == '/' && _.pos + 1 < s.len && (s[_.pos + 1] == '/' || s[_.pos + 1] == '*'))) {
                if (!_.skipWsAndComments()) return null;
                continue;
            }
            if (c == open_ch) {
                if (depth >= MAX_BRACE_DEPTH) return null;
                depth += 1;
                _.pos += 1;
                continue;
            }
            if (c == close_ch) {
                depth -= 1;
                if (depth == 0) {
                    const inner_end = _.pos;
                    _.pos += 1;
                    return s[inner_start..inner_end];
                }
                _.pos += 1;
                continue;
            }
            _.pos += 1;
        }
        return null;
    }

    /// Scans a Thrift type expression including nested generics (e.g. `map<string, list<i64>>`).
    fn scanType(*_): ([]const u8)? {
        if (!_.skipWsAndComments()) return null;
        const s = _.input;
        const start = _.pos;
        const base = _.scanIdent() ?? return null;
        * = base;

        // Check for optional generic type arguments '<...>'
        const saved = _.pos;
        if (!_.skipWsAndComments()) return null;
        if (_.pos < s.len && s[_.pos] == '<') {
            if (_.scanBalanced('<', '>') == null) return null;
        } else {
            _.pos = saved;
        }

        // Optional cpp_type annotation or type annotations skipped outside type slice
        return trimSpaces(s[start.._.pos]);
    }

    fn skipOptionalAnnotations(*_): bool {
        const s = _.input;
        if (!_.skipWsAndComments()) return false;
        if (_.pos < s.len && s[_.pos] == '(') {
            if (_.scanBalanced('(', ')') == null) return false;
        }
        return true;
    }
}

pub struct ThriftDeclIter {
    sc: Scanner,

    pub static fn init(input: []const u8): ThriftDeclIter {
        return ThriftDeclIter{ sc: Scanner.init(input) };
    }

    pub fn next(*_): ThriftDecl? {
        while (true) {
            if (!_.sc.skipWsAndComments()) return null;
            if (_.sc.pos >= _.sc.input.len) return null;

            const c = _.sc.input[_.sc.pos];
            if (c == ';' || c == ',') {
                _.sc.pos += 1;
                continue;
            }

            const kw = _.sc.scanSimpleName() ?? return null;

            if (bytesEql(kw, "include") || bytesEql(kw, "cpp_include")) {
                if (!_.sc.skipWsAndComments()) return null;
                const inc_path = _.sc.skipString() ?? return null;
                return ThriftDecl{
                    kind: .include_decl,
                    name: inc_path,
                    extends_name: null,
                    body: null,
                };
            }

            if (bytesEql(kw, "namespace")) {
                if (!_.sc.skipWsAndComments()) return null;
                // Scope can be '*' or identifier (e.g. cpp, go, java, py)
                if (_.sc.pos < _.sc.input.len && _.sc.input[_.sc.pos] == '*') {
                    _.sc.pos += 1;
                } else {
                    * = _.sc.scanIdent() ?? return null;
                }
                if (!_.sc.skipWsAndComments()) return null;
                let ns_name: []const u8 = "";
                if (_.sc.pos < _.sc.input.len && (_.sc.input[_.sc.pos] == '"' || _.sc.input[_.sc.pos] == '\'')) {
                    ns_name = _.sc.skipString() ?? return null;
                } else {
                    ns_name = _.sc.scanIdent() ?? return null;
                }
                * = _.sc.skipOptionalAnnotations();
                return ThriftDecl{
                    kind: .namespace_decl,
                    name: ns_name,
                    extends_name: null,
                    body: null,
                };
            }

            if (bytesEql(kw, "typedef")) {
                const ty = _.sc.scanType() ?? return null;
                if (!_.sc.skipWsAndComments()) return null;
                const alias_name = _.sc.scanSimpleName() ?? return null;
                * = _.sc.skipOptionalAnnotations();
                return ThriftDecl{
                    kind: .typedef_decl,
                    name: alias_name,
                    extends_name: null,
                    body: ty,
                };
            }

            if (bytesEql(kw, "const")) {
                const ty = _.sc.scanType() ?? return null;
                if (!_.sc.skipWsAndComments()) return null;
                const const_name = _.sc.scanSimpleName() ?? return null;
                if (!_.sc.skipWsAndComments()) return null;
                if (_.sc.pos < _.sc.input.len && _.sc.input[_.sc.pos] == '=') {
                    _.sc.pos += 1;
                    if (!_.sc.skipWsAndComments()) return null;
                    // Skip constant value literal / list / map
                    if (_.sc.pos < _.sc.input.len) {
                        const vc = _.sc.input[_.sc.pos];
                        if (vc == '{') {
                            if (_.sc.scanBalanced('{', '}') == null) return null;
                        } else if (vc == '[') {
                            if (_.sc.scanBalanced('[', ']') == null) return null;
                        } else if (vc == '"' || vc == '\'') {
                            if (_.sc.skipString() == null) return null;
                        } else {
                            while (_.sc.pos < _.sc.input.len) {
                                const ch = _.sc.input[_.sc.pos];
                                if (isWhitespace(ch) || ch == ';' || ch == ',' || ch == '#') break;
                                if (ch == '/' && _.sc.pos + 1 < _.sc.input.len && (_.sc.input[_.sc.pos + 1] == '/' || _.sc.input[_.sc.pos + 1] == '*')) break;
                                _.sc.pos += 1;
                            }
                        }
                    }
                }
                return ThriftDecl{
                    kind: .const_decl,
                    name: const_name,
                    extends_name: null,
                    body: ty,
                };
            }

            let decl_kind: ThriftDeclKind = .struct_decl;
            let is_block_decl = false;
            if (bytesEql(kw, "struct")) { decl_kind = .struct_decl; is_block_decl = true; }
            else if (bytesEql(kw, "union")) { decl_kind = .union_decl; is_block_decl = true; }
            else if (bytesEql(kw, "exception")) { decl_kind = .exception_decl; is_block_decl = true; }
            else if (bytesEql(kw, "enum")) { decl_kind = .enum_decl; is_block_decl = true; }
            else if (bytesEql(kw, "service")) { decl_kind = .service_decl; is_block_decl = true; }

            if (is_block_decl) {
                if (!_.sc.skipWsAndComments()) return null;
                const decl_name = _.sc.scanSimpleName() ?? return null;
                let ext: ([]const u8)? = null;

                if (!_.sc.skipWsAndComments()) return null;
                if (decl_kind == .service_decl) {
                    const saved = _.sc.pos;
                    if (maybe_ext := _.sc.scanSimpleName()) {
                        if (bytesEql(maybe_ext, "extends")) {
                            if (!_.sc.skipWsAndComments()) return null;
                            ext = _.sc.scanIdent() ?? return null;
                            if (!_.sc.skipWsAndComments()) return null;
                        } else {
                            _.sc.pos = saved;
                        }
                    }
                }

                const body = _.sc.scanBalanced('{', '}') ?? return null;
                * = _.sc.skipOptionalAnnotations();
                return ThriftDecl{
                    kind: decl_kind,
                    name: decl_name,
                    extends_name: ext,
                    body: body,
                };
            }

            return null;
        }
    }
}

fn parseI32Digits(s: []const u8): i32? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
        if (i >= s.len) return null;
    }
    let acc: i64 = 0;
    while (i < s.len) : (i += 1) {
        if (!isAsciiDigit(s[i])) return null;
        acc = acc * 10 + @as(i64, s[i] - '0');
        if (acc > 2147483648) return null;
    }
    const signed_val = if (neg) -acc else acc;
    if (signed_val < -2147483648 || signed_val > 2147483647) return null;
    return @as(i32, @intCast(signed_val));
}

pub struct ThriftFieldIter {
    sc: Scanner,

    pub static fn init(struct_body: []const u8): ThriftFieldIter {
        return ThriftFieldIter{ sc: Scanner.init(struct_body) };
    }

    pub fn next(*_): ThriftField? {
        while (true) {
            if (!_.sc.skipWsAndComments()) return null;
            if (_.sc.pos >= _.sc.input.len) return null;

            const c = _.sc.input[_.sc.pos];
            if (c == ',' || c == ';') {
                _.sc.pos += 1;
                continue;
            }

            // Optional field ID: `<int>:`
            let fid: i32? = null;
            if (isAsciiDigit(c) || c == '-') {
                const id_start = _.sc.pos;
                if (c == '-') _.sc.pos += 1;
                while (_.sc.pos < _.sc.input.len && isAsciiDigit(_.sc.input[_.sc.pos])) {
                    _.sc.pos += 1;
                }
                const id_slice = _.sc.input[id_start.._.sc.pos];
                if (!_.sc.skipWsAndComments()) return null;
                if (_.sc.pos >= _.sc.input.len || _.sc.input[_.sc.pos] != ':') return null;
                _.sc.pos += 1;
                fid = parseI32Digits(id_slice) ?? return null;
                if (!_.sc.skipWsAndComments()) return null;
            }

            // Optional required / optional qualifier
            let is_req = false;
            let is_opt = false;
            const saved = _.sc.pos;
            if (maybe_q := _.sc.scanSimpleName()) {
                if (bytesEql(maybe_q, "required")) {
                    is_req = true;
                } else if (bytesEql(maybe_q, "optional")) {
                    is_opt = true;
                } else {
                    _.sc.pos = saved;
                }
            }

            const ty = _.sc.scanType() ?? return null;
            if (!_.sc.skipWsAndComments()) return null;
            const field_name = _.sc.scanSimpleName() ?? return null;

            // Skip optional default value (`= ...`) and annotations (`(...)`)
            if (!_.sc.skipWsAndComments()) return null;
            if (_.sc.pos < _.sc.input.len && _.sc.input[_.sc.pos] == '=') {
                _.sc.pos += 1;
                if (!_.sc.skipWsAndComments()) return null;
                if (_.sc.pos < _.sc.input.len) {
                    const vc = _.sc.input[_.sc.pos];
                    if (vc == '{') {
                        if (_.sc.scanBalanced('{', '}') == null) return null;
                    } else if (vc == '[') {
                        if (_.sc.scanBalanced('[', ']') == null) return null;
                    } else if (vc == '"' || vc == '\'') {
                        if (_.sc.skipString() == null) return null;
                    } else {
                        while (_.sc.pos < _.sc.input.len) {
                            const ch = _.sc.input[_.sc.pos];
                            if (isWhitespace(ch) || ch == ',' || ch == ';' || ch == '(' || ch == '#') break;
                            if (ch == '/' && _.sc.pos + 1 < _.sc.input.len && (_.sc.input[_.sc.pos + 1] == '/' || _.sc.input[_.sc.pos + 1] == '*')) break;
                            _.sc.pos += 1;
                        }
                    }
                }
            }

            * = _.sc.skipOptionalAnnotations();

            return ThriftField{
                field_id: fid,
                is_required: is_req,
                is_optional: is_opt,
                ty: ty,
                name: field_name,
            };
        }
    }
}

pub fn validateThriftBraces(input: []const u8): bool {
    let stack: [MAX_BRACE_DEPTH]u8 = [0; MAX_BRACE_DEPTH];
    let top: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        const c = input[i];

        // Strings
        if (c == '"' || c == '\'') {
            const quote = c;
            i += 1;
            let closed = false;
            while (i < input.len) {
                if (input[i] == '\\') {
                    i += 2;
                    if (i > input.len) return false;
                    continue;
                }
                if (input[i] == quote) {
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return false;
            continue;
        }

        // '#' line comment
        if (c == '#') {
            i += 1;
            while (i < input.len && input[i] != '\n' && input[i] != '\r') i += 1;
            continue;
        }

        // '//' and '/* */' comments
        if (c == '/' && i + 1 < input.len) {
            if (input[i + 1] == '/') {
                i += 2;
                while (i < input.len && input[i] != '\n' && input[i] != '\r') i += 1;
                continue;
            }
            if (input[i + 1] == '*') {
                i += 2;
                let closed = false;
                while (i + 1 < input.len) {
                    if (input[i] == '*' && input[i + 1] == '/') {
                        i += 2;
                        closed = true;
                        break;
                    }
                    i += 1;
                }
                if (!closed) return false;
                continue;
            }
        }

        // Opening brackets: { ( < [
        if (c == '{' || c == '(' || c == '<' || c == '[') {
            if (top >= MAX_BRACE_DEPTH) return false;
            stack[top] = switch (c) {
                '{' => '}',
                '(' => ')',
                '<' => '>',
                else => ']',
            };
            top += 1;
            i += 1;
            continue;
        }

        // Closing brackets: } ) > ]
        if (c == '}' || c == ')' || c == '>' || c == ']') {
            if (top == 0) return false;
            top -= 1;
            if (stack[top] != c) return false;
            i += 1;
            continue;
        }

        i += 1;
    }

    return top == 0;
}

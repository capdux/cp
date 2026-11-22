// Source: cp/lib/highlight/highlight.cp (Multi-Language Lexical Syntax Highlighter: Tokenize, ANSI & XSS-Safe HTML)
// Pure CP zero-external-dependency lexical syntax highlighter supporting CP, Zig, Rust, TypeScript,
// Python, JSON, SQL, and Bash with lossless tokenization, ANSI SGR output, and XSS-safe HTML output.

const std = @import("std");

pub err HighlightError {
    UnsupportedLanguage,
    OutOfMemory,
}

pub enum Language {
    Cp,
    Zig,
    Rust,
    TypeScript,
    Python,
    Json,
    Sql,
    Bash,

    fn name(): string {
        return switch (_) {
            .Cp => "cp",
            .Zig => "zig",
            .Rust => "rust",
            .TypeScript => "typescript",
            .Python => "python",
            .Json => "json",
            .Sql => "sql",
            .Bash => "bash",
        };
    }
}

pub enum TokenScope {
    Plain,
    Keyword,
    TypeName,
    String,
    Number,
    Comment,
    Operator,
    Punctuation,
    Function,
    Builtin,

    fn cssClass(): string {
        return switch (_) {
            .Plain => "hl-plain",
            .Keyword => "hl-keyword",
            .TypeName => "hl-type",
            .String => "hl-string",
            .Number => "hl-number",
            .Comment => "hl-comment",
            .Operator => "hl-operator",
            .Punctuation => "hl-punctuation",
            .Function => "hl-function",
            .Builtin => "hl-builtin",
        };
    }

    fn ansiCode(): string {
        return switch (_) {
            .Plain => "",
            .Keyword => "\x1b[35m",
            .TypeName => "\x1b[33m",
            .String => "\x1b[32m",
            .Number => "\x1b[36m",
            .Comment => "\x1b[90m",
            .Operator => "\x1b[31m",
            .Punctuation => "\x1b[37m",
            .Function => "\x1b[34m",
            .Builtin => "\x1b[96m",
        };
    }
}

pub struct HighlightToken {
    scope: TokenScope;
    text: string;
}

const ANSI_RESET: string = "\x1b[0m";

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isIdentContinue(c: u8): bool {
    return isAlpha(c) || isDigit(c);
}

fn isOperatorChar(c: u8): bool {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
        c == '=' || c == '!' || c == '<' || c == '>' || c == '&' ||
        c == '|' || c == '^' || c == '~' || c == '?' || c == ':' || c == '$';
}

fn isPunctChar(c: u8): bool {
    return c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}' || c == ',' || c == ';' || c == '.';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlLower(a: string, lower_lit: string): bool {
    if (a.len != lower_lit.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != lower_lit[i]) return false;
    }
    return true;
}

fn inList(word: string, list: []const string): bool {
    for (item of list) {
        if (std.mem.eql(u8, word, item)) return true;
    }
    return false;
}

fn inListIgnoreCase(word: string, lower_list: []const string): bool {
    for (item of lower_list) {
        if (eqlLower(word, item)) return true;
    }
    return false;
}

const CP_KEYWORDS: []const string = [
    "fn", "pub", "priv", "let", "const", "class", "struct", "enum", "err",
    "trait", "impl", "type", "import", "export", "from", "as", "if", "else",
    "switch", "match", "for", "of", "while", "inline", "comptime", "return",
    "break", "continue", "defer", "errdefer", "catch", "orelse", "unsafe",
    "zig", "arena", "fuzz", "async", "await", "spawn", "move", "own",
    "extends", "implements", "super", "static", "new", "test", "try",
    "dec", "meta", "macro", "ref", "extern", "packed", "opaque",
];

const CP_TYPES: []const string = [
    "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "i128", "u128",
    "isize", "usize", "f32", "f64", "bool", "void", "string", "cstring", "cstr",
    "auto", "type", "anyerror", "Self", "Vec", "VecDeque", "HashMap", "HashSet",
    "BTreeMap", "BinaryHeap", "Mutex", "RwLock", "Atomic", "Shared", "Box",
    "Err", "AnyErr", "JSON", "RegExp",
];

const CP_BUILTINS: []const string = [
    "true", "false", "null", "undefined", "unreachable", "print", "expect_eq", "_",
];

const ZIG_KEYWORDS: []const string = [
    "fn", "pub", "const", "var", "struct", "enum", "union", "error", "opaque",
    "if", "else", "switch", "for", "while", "inline", "comptime", "return",
    "break", "continue", "defer", "errdefer", "catch", "orelse", "try",
    "async", "await", "suspend", "resume", "nosuspend", "export", "extern",
    "packed", "align", "callconv", "linksection", "threadlocal", "allowzero",
    "noalias", "volatile", "anyframe", "test", "usingnamespace", "and", "or",
];

const ZIG_TYPES: []const string = [
    "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "i128", "u128",
    "isize", "usize", "f16", "f32", "f64", "f80", "f128", "bool", "void",
    "noreturn", "type", "anyerror", "anyopaque", "anytype", "comptime_int",
    "comptime_float", "c_int", "c_uint", "c_char", "c_void",
];

const ZIG_BUILTINS: []const string = [
    "true", "false", "null", "undefined", "unreachable",
];

const RUST_KEYWORDS: []const string = [
    "fn", "pub", "let", "mut", "const", "static", "struct", "enum", "union",
    "trait", "impl", "type", "mod", "use", "crate", "super", "self", "Self",
    "as", "if", "else", "match", "for", "in", "while", "loop", "return",
    "break", "continue", "async", "await", "move", "dyn", "ref", "unsafe",
    "extern", "where", "box",
];

const RUST_TYPES: []const string = [
    "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "i128", "u128",
    "isize", "usize", "f32", "f64", "bool", "char", "str", "String", "Vec",
    "Option", "Result", "Box", "Rc", "Arc", "HashMap", "HashSet", "BTreeMap",
];

const RUST_BUILTINS: []const string = [
    "true", "false", "Some", "None", "Ok", "Err", "println", "eprintln",
    "format", "vec", "panic", "assert", "assert_eq",
];

const TS_KEYWORDS: []const string = [
    "function", "const", "let", "var", "class", "interface", "type", "enum",
    "namespace", "module", "import", "export", "from", "as", "default",
    "if", "else", "switch", "case", "for", "of", "in", "while", "do",
    "return", "break", "continue", "try", "catch", "finally", "throw",
    "async", "await", "yield", "new", "this", "super", "extends",
    "implements", "static", "public", "private", "protected", "readonly",
    "abstract", "declare", "typeof", "keyof", "instanceof", "void", "delete",
    "satisfies", "infer", "is",
];

const TS_TYPES: []const string = [
    "string", "number", "boolean", "object", "symbol", "bigint", "any",
    "unknown", "never", "Array", "Map", "Set", "Promise", "Record",
    "Partial", "Required", "Readonly", "Pick", "Omit", "Date", "RegExp",
    "Error", "Uint8Array",
];

const TS_BUILTINS: []const string = [
    "true", "false", "null", "undefined", "NaN", "Infinity", "console",
    "JSON", "Math", "Object",
];

const PY_KEYWORDS: []const string = [
    "def", "class", "lambda", "import", "from", "as", "if", "elif", "else",
    "for", "in", "while", "return", "yield", "break", "continue", "pass",
    "try", "except", "finally", "raise", "with", "async", "await", "and",
    "or", "not", "is", "global", "nonlocal", "assert", "del", "match", "case",
];

const PY_TYPES: []const string = [
    "int", "float", "str", "bool", "bytes", "list", "dict", "set", "tuple",
    "object", "type", "Any", "Optional", "Union", "List", "Dict", "Tuple",
    "Callable",
];

const PY_BUILTINS: []const string = [
    "True", "False", "None", "self", "cls", "print", "len", "range",
    "enumerate", "zip", "map", "filter", "isinstance", "super", "open",
];

const JSON_BUILTINS: []const string = [
    "true", "false", "null",
];

const SQL_KEYWORDS_LOWER: []const string = [
    "select", "from", "where", "insert", "into", "values", "update", "set",
    "delete", "create", "alter", "drop", "table", "index", "view", "database",
    "join", "inner", "left", "right", "full", "outer", "on", "as", "and",
    "or", "not", "in", "is", "null", "like", "ilike", "between", "exists",
    "case", "when", "then", "else", "end", "group", "by", "having", "order",
    "asc", "desc", "limit", "offset", "union", "all", "distinct", "primary",
    "key", "foreign", "references", "unique", "check", "default", "begin",
    "commit", "rollback", "transaction", "with", "returning",
];

const SQL_TYPES_LOWER: []const string = [
    "int", "integer", "bigint", "smallint", "tinyint", "varchar", "char",
    "text", "boolean", "bool", "float", "double", "decimal", "numeric",
    "date", "time", "timestamp", "datetime", "blob", "json", "jsonb",
    "uuid", "serial",
];

const SQL_BUILTINS_LOWER: []const string = [
    "count", "sum", "avg", "min", "max", "coalesce", "nullif", "now",
    "current_timestamp", "cast", "true", "false",
];

const BASH_KEYWORDS: []const string = [
    "if", "then", "else", "elif", "fi", "case", "esac", "for", "in",
    "while", "until", "do", "done", "function", "return", "exit", "break",
    "continue", "local", "export", "readonly", "declare", "typeset",
    "unset", "shift", "source",
];

const BASH_BUILTINS: []const string = [
    "echo", "printf", "cd", "pwd", "ls", "cp", "mv", "rm", "mkdir", "cat",
    "grep", "rg", "sed", "awk", "find", "chmod", "chown", "tar", "curl",
    "wget", "git", "true", "false", "test",
];

fn classifyWord(lang: Language, word: string, followed_by_paren: bool): TokenScope {
    switch (lang) {
        .Cp => {
            if (inList(word, CP_KEYWORDS)) return .Keyword;
            if (inList(word, CP_TYPES)) return .TypeName;
            if (inList(word, CP_BUILTINS)) return .Builtin;
        },
        .Zig => {
            if (inList(word, ZIG_KEYWORDS)) return .Keyword;
            if (inList(word, ZIG_TYPES)) return .TypeName;
            if (inList(word, ZIG_BUILTINS)) return .Builtin;
        },
        .Rust => {
            if (inList(word, RUST_KEYWORDS)) return .Keyword;
            if (inList(word, RUST_TYPES)) return .TypeName;
            if (inList(word, RUST_BUILTINS)) return .Builtin;
        },
        .TypeScript => {
            if (inList(word, TS_KEYWORDS)) return .Keyword;
            if (inList(word, TS_TYPES)) return .TypeName;
            if (inList(word, TS_BUILTINS)) return .Builtin;
        },
        .Python => {
            if (inList(word, PY_KEYWORDS)) return .Keyword;
            if (inList(word, PY_TYPES)) return .TypeName;
            if (inList(word, PY_BUILTINS)) return .Builtin;
        },
        .Json => {
            if (inList(word, JSON_BUILTINS)) return .Builtin;
        },
        .Sql => {
            if (inListIgnoreCase(word, SQL_KEYWORDS_LOWER)) return .Keyword;
            if (inListIgnoreCase(word, SQL_TYPES_LOWER)) return .TypeName;
            if (inListIgnoreCase(word, SQL_BUILTINS_LOWER)) return .Builtin;
        },
        .Bash => {
            if (inList(word, BASH_KEYWORDS)) return .Keyword;
            if (inList(word, BASH_BUILTINS)) return .Builtin;
        },
    }
    if (followed_by_paren) return .Function;
    return .Plain;
}

fn supportsSlashSlashComment(lang: Language): bool {
    return lang == .Cp || lang == .Zig || lang == .Rust || lang == .TypeScript || lang == .Json;
}

fn supportsHashComment(lang: Language): bool {
    return lang == .Python || lang == .Bash;
}

fn supportsBlockComment(lang: Language): bool {
    return lang == .Cp || lang == .Zig || lang == .Rust || lang == .TypeScript || lang == .Sql;
}

fn supportsBacktickString(lang: Language): bool {
    return lang == .TypeScript || lang == .Bash || lang == .Sql || lang == .Cp;
}

fn supportsSingleQuoteString(lang: Language): bool {
    return lang == .TypeScript || lang == .Python || lang == .Sql || lang == .Bash;
}

fn scanLineCommentEnd(code: string, start: usize): usize {
    let i = start;
    while (i < code.len && code[i] != '\n') : (i += 1) {}
    return i;
}

fn scanBlockCommentEnd(code: string, start: usize): usize {
    let i = start + 2;
    while (i + 1 < code.len) {
        if (code[i] == '*' && code[i + 1] == '/') {
            return i + 2;
        }
        i += 1;
    }
    return code.len;
}

fn scanQuotedStringEnd(code: string, start: usize, quote: u8, sql_double_escape: bool): usize {
    let i = start + 1;
    while (i < code.len) {
        const c = code[i];
        if (c == '\\' && !sql_double_escape) {
            if (i + 1 < code.len) {
                i += 2;
            } else {
                i += 1;
            }
            continue;
        }
        if (c == quote) {
            if (sql_double_escape && i + 1 < code.len && code[i + 1] == quote) {
                i += 2;
                continue;
            }
            return i + 1;
        }
        i += 1;
    }
    return code.len;
}

fn scanTripleQuoteEnd(code: string, start: usize, quote: u8): usize {
    let i = start + 3;
    while (i + 2 < code.len) {
        if (code[i] == '\\') {
            i += 2;
            continue;
        }
        if (code[i] == quote && code[i + 1] == quote && code[i + 2] == quote) {
            return i + 3;
        }
        i += 1;
    }
    return code.len;
}

fn scanCharLiteralEnd(code: string, start: usize): usize? {
    let i = start + 1;
    if (i >= code.len || code[i] == '\n') return null;
    if (code[i] == '\\') {
        i += 1;
        if (i >= code.len || code[i] == '\n') return null;
        i += 1;
        while (i < code.len && code[i] != '\'' && code[i] != '\n' && i - start <= 6) : (i += 1) {}
        if (i < code.len && code[i] == '\'') return i + 1;
        return null;
    }
    if (code[i] == '\'') return null;
    if (i + 1 < code.len && code[i + 1] == '\'') return i + 2;
    return null;
}

fn scanNumberEnd(code: string, start: usize): usize {
    let i = start;
    if (i + 1 < code.len && code[i] == '0') {
        const p = code[i + 1];
        if (p == 'x' || p == 'X') {
            i += 2;
            while (i < code.len && (isHexDigit(code[i]) || code[i] == '_')) : (i += 1) {}
            return i;
        }
        if (p == 'b' || p == 'B') {
            i += 2;
            while (i < code.len && (code[i] == '0' || code[i] == '1' || code[i] == '_')) : (i += 1) {}
            return i;
        }
        if (p == 'o' || p == 'O') {
            i += 2;
            while (i < code.len && ((code[i] >= '0' && code[i] <= '7') || code[i] == '_')) : (i += 1) {}
            return i;
        }
    }

    while (i < code.len && (isDigit(code[i]) || code[i] == '_')) : (i += 1) {}

    // Decimal fraction: only consume '.' when followed by a digit (preserves `0..10` range syntax)
    if (i + 1 < code.len && code[i] == '.' && isDigit(code[i + 1])) {
        i += 1;
        while (i < code.len && (isDigit(code[i]) || code[i] == '_')) : (i += 1) {}
    }

    // Scientific exponent
    if (i < code.len && (code[i] == 'e' || code[i] == 'E')) {
        let exp_pos = i + 1;
        if (exp_pos < code.len && (code[exp_pos] == '+' || code[exp_pos] == '-')) {
            exp_pos += 1;
        }
        if (exp_pos < code.len && isDigit(code[exp_pos])) {
            i = exp_pos;
            while (i < code.len && (isDigit(code[i]) || code[i] == '_')) : (i += 1) {}
        }
    }

    return i;
}

fn isFollowedByParen(code: string, pos: usize): bool {
    let j = pos;
    while (j < code.len && (code[j] == ' ' || code[j] == '\t')) : (j += 1) {}
    return j < code.len && code[j] == '(';
}

fn appendEscapedHtml(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, text: string) ! HighlightError {
    for (c of text) {
        switch (c) {
            '&' => buf.appendSlice(alloc, "&amp;") catch return HighlightError.OutOfMemory,
            '<' => buf.appendSlice(alloc, "&lt;") catch return HighlightError.OutOfMemory,
            '>' => buf.appendSlice(alloc, "&gt;") catch return HighlightError.OutOfMemory,
            '"' => buf.appendSlice(alloc, "&quot;") catch return HighlightError.OutOfMemory,
            '\'' => buf.appendSlice(alloc, "&#39;") catch return HighlightError.OutOfMemory,
            else => buf.append(alloc, c) catch return HighlightError.OutOfMemory,
        }
    }
}

pub static struct Highlighter {
    static fn languageFromName(name_or_ext: string): Language? {
        const trimmed = std.mem.trim(u8, name_or_ext, " \t\r\n");
        if (trimmed.len == 0) return null;

        if (eqlLower(trimmed, "cp") || eqlLower(trimmed, ".cp")) return .Cp;
        if (eqlLower(trimmed, "zig") || eqlLower(trimmed, ".zig")) return .Zig;
        if (eqlLower(trimmed, "rust") || eqlLower(trimmed, "rs") || eqlLower(trimmed, ".rs")) return .Rust;
        if (
            eqlLower(trimmed, "typescript") || eqlLower(trimmed, "ts") || eqlLower(trimmed, ".ts") ||
            eqlLower(trimmed, "tsx") || eqlLower(trimmed, ".tsx") ||
            eqlLower(trimmed, "mts") || eqlLower(trimmed, ".mts") ||
            eqlLower(trimmed, "cts") || eqlLower(trimmed, ".cts") ||
            eqlLower(trimmed, "javascript") || eqlLower(trimmed, "js") || eqlLower(trimmed, ".js") ||
            eqlLower(trimmed, "jsx") || eqlLower(trimmed, ".jsx")
        ) return .TypeScript;
        if (
            eqlLower(trimmed, "python") || eqlLower(trimmed, "py") || eqlLower(trimmed, ".py") ||
            eqlLower(trimmed, "pyi") || eqlLower(trimmed, ".pyi")
        ) return .Python;
        if (
            eqlLower(trimmed, "json") || eqlLower(trimmed, ".json") ||
            eqlLower(trimmed, "jsonc") || eqlLower(trimmed, ".jsonc")
        ) return .Json;
        if (eqlLower(trimmed, "sql") || eqlLower(trimmed, ".sql")) return .Sql;
        if (
            eqlLower(trimmed, "bash") || eqlLower(trimmed, "sh") || eqlLower(trimmed, ".sh") ||
            eqlLower(trimmed, "zsh") || eqlLower(trimmed, ".zsh") || eqlLower(trimmed, "shell")
        ) return .Bash;

        return null;
    }

    static fn tokenize(alloc: std.mem.Allocator, lang: Language, code: string): ([]HighlightToken) ! HighlightError {
        let tokens: std.ArrayListUnmanaged(HighlightToken) = .empty;
        errdefer tokens.deinit(alloc);

        let i: usize = 0;
        const n = code.len;

        while (i < n) {
            const c = code[i];

            // 1. Whitespace -> Plain
            if (isWhitespace(c)) {
                let end = i + 1;
                while (end < n && isWhitespace(code[end])) : (end += 1) {}
                tokens.append(alloc, HighlightToken{ scope: .Plain, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 2. Single-line comments: //, #, --
            if (supportsSlashSlashComment(lang) && i + 1 < n && c == '/' && code[i + 1] == '/') {
                const end = scanLineCommentEnd(code, i + 2);
                tokens.append(alloc, HighlightToken{ scope: .Comment, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }
            if (supportsHashComment(lang) && c == '#') {
                const end = scanLineCommentEnd(code, i + 1);
                tokens.append(alloc, HighlightToken{ scope: .Comment, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }
            if (lang == .Sql && i + 1 < n && c == '-' && code[i + 1] == '-') {
                const end = scanLineCommentEnd(code, i + 2);
                tokens.append(alloc, HighlightToken{ scope: .Comment, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 3. Block comments: /* ... */
            if (supportsBlockComment(lang) && i + 1 < n && c == '/' && code[i + 1] == '*') {
                const end = scanBlockCommentEnd(code, i);
                tokens.append(alloc, HighlightToken{ scope: .Comment, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 4. Strings & char literals
            if (lang == .Python && i + 2 < n && ((c == '"' && code[i + 1] == '"' && code[i + 2] == '"') || (c == '\'' && code[i + 1] == '\'' && code[i + 2] == '\''))) {
                const end = scanTripleQuoteEnd(code, i, c);
                tokens.append(alloc, HighlightToken{ scope: .String, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }
            if (c == '"') {
                const end = scanQuotedStringEnd(code, i, '"', false);
                tokens.append(alloc, HighlightToken{ scope: .String, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }
            if (c == '\'') {
                if (supportsSingleQuoteString(lang)) {
                    const end = scanQuotedStringEnd(code, i, '\'', lang == .Sql);
                    tokens.append(alloc, HighlightToken{ scope: .String, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                    i = end;
                    continue;
                }
                if (end_char := scanCharLiteralEnd(code, i)) {
                    tokens.append(alloc, HighlightToken{ scope: .String, text: code[i..end_char] }) catch return HighlightError.OutOfMemory;
                    i = end_char;
                    continue;
                }
            }
            if (c == '`' && supportsBacktickString(lang)) {
                const end = scanQuotedStringEnd(code, i, '`', false);
                tokens.append(alloc, HighlightToken{ scope: .String, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 5. @builtin / @decorator
            if (c == '@' && i + 1 < n && isAlpha(code[i + 1])) {
                let end = i + 2;
                while (end < n && isIdentContinue(code[end])) : (end += 1) {}
                tokens.append(alloc, HighlightToken{ scope: .Builtin, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 6. Numbers
            if (isDigit(c)) {
                const end = scanNumberEnd(code, i);
                tokens.append(alloc, HighlightToken{ scope: .Number, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 7. Identifiers, keywords, types, builtins & function calls
            if (isAlpha(c)) {
                let end = i + 1;
                while (end < n && isIdentContinue(code[end])) : (end += 1) {}
                const word = code[i..end];
                const scope = classifyWord(lang, word, isFollowedByParen(code, end));
                tokens.append(alloc, HighlightToken{ scope: scope, text: word }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 8. Operators
            if (isOperatorChar(c)) {
                let end = i + 1;
                while (end < n && isOperatorChar(code[end])) : (end += 1) {}
                tokens.append(alloc, HighlightToken{ scope: .Operator, text: code[i..end] }) catch return HighlightError.OutOfMemory;
                i = end;
                continue;
            }

            // 9. Punctuation
            if (isPunctChar(c)) {
                tokens.append(alloc, HighlightToken{ scope: .Punctuation, text: code[i .. i + 1] }) catch return HighlightError.OutOfMemory;
                i += 1;
                continue;
            }

            // 10. Fallback Plain (preserves UTF-8 and any other bytes losslessly)
            let end = i + 1;
            while (
                end < n &&
                !isWhitespace(code[end]) &&
                !isAlpha(code[end]) &&
                !isDigit(code[end]) &&
                !isOperatorChar(code[end]) &&
                !isPunctChar(code[end]) &&
                code[end] != '"' &&
                code[end] != '\'' &&
                code[end] != '`' &&
                code[end] != '@' &&
                code[end] != '#'
            ) : (end += 1) {}
            tokens.append(alloc, HighlightToken{ scope: .Plain, text: code[i..end] }) catch return HighlightError.OutOfMemory;
            i = end;
        }

        return tokens.toOwnedSlice(alloc) catch return HighlightError.OutOfMemory;
    }

    static fn toAnsi(alloc: std.mem.Allocator, lang: Language, code: string): ([]u8) ! HighlightError {
        const tokens = Highlighter.tokenize(alloc, lang, code)?;
        defer alloc.free(tokens);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        for (tok of tokens) {
            if (tok.scope == .Plain) {
                out.appendSlice(alloc, tok.text) catch return HighlightError.OutOfMemory;
            } else {
                out.appendSlice(alloc, tok.scope.ansiCode()) catch return HighlightError.OutOfMemory;
                out.appendSlice(alloc, tok.text) catch return HighlightError.OutOfMemory;
                out.appendSlice(alloc, ANSI_RESET) catch return HighlightError.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch return HighlightError.OutOfMemory;
    }

    static fn toHtml(alloc: std.mem.Allocator, lang: Language, code: string): ([]u8) ! HighlightError {
        const tokens = Highlighter.tokenize(alloc, lang, code)?;
        defer alloc.free(tokens);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "<pre class=\"cp-hl\"><code>") catch return HighlightError.OutOfMemory;
        for (tok of tokens) {
            if (tok.scope == .Plain) {
                appendEscapedHtml(&out, alloc, tok.text)?;
            } else {
                out.appendSlice(alloc, "<span class=\"") catch return HighlightError.OutOfMemory;
                out.appendSlice(alloc, tok.scope.cssClass()) catch return HighlightError.OutOfMemory;
                out.appendSlice(alloc, "\">") catch return HighlightError.OutOfMemory;
                appendEscapedHtml(&out, alloc, tok.text)?;
                out.appendSlice(alloc, "</span>") catch return HighlightError.OutOfMemory;
            }
        }
        out.appendSlice(alloc, "</code></pre>") catch return HighlightError.OutOfMemory;

        return out.toOwnedSlice(alloc) catch return HighlightError.OutOfMemory;
    }
}

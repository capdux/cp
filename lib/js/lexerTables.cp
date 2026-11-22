// Source: Absorbed from /data/dev/bun/src/js_parser/lexer_tables.cp | Synced: 2026-11-22T19:11:50Z
// ============================================================================
// lib/js/lexerTables.cp — Zero-Alloc JS/TS Lexer Token Tables & JSX Entities
// ============================================================================
// Implements Bun's `js_parser/lexer_tables.cp` token taxonomy, keyword &
// strict-mode reserved word tables, TypeScript property/accessibility modifier
// classifiers, and common JSX named entity codepoint lookup with zero heap
// allocation.

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Complete JS/TS lexer token enum (1:1 port of Bun's `js_lexer.T`).
pub enum JsToken: u8 {
    t_end_of_file = 0,
    t_close_brace = 1,
    t_syntax_error = 2,
    t_hashbang = 3,

    // literals
    t_no_substitution_template_literal = 4,
    t_numeric_literal = 5,
    t_string_literal = 6,
    t_big_integer_literal = 7,

    // pseudo-literals
    t_template_head = 8,
    t_template_middle = 9,
    t_template_tail = 10,

    // punctuation
    t_ampersand = 11,
    t_ampersand_ampersand = 12,
    t_asterisk = 13,
    t_asterisk_asterisk = 14,
    t_at = 15,
    t_bar = 16,
    t_bar_bar = 17,
    t_caret = 18,
    t_close_bracket = 19,
    t_close_paren = 20,
    t_colon = 21,
    t_comma = 22,
    t_dot = 23,
    t_dot_dot_dot = 24,
    t_equals_equals = 25,
    t_equals_equals_equals = 26,
    t_equals_greater_than = 27,
    t_exclamation = 28,
    t_exclamation_equals = 29,
    t_exclamation_equals_equals = 30,
    t_greater_than = 31,
    t_greater_than_equals = 32,
    t_greater_than_greater_than = 33,
    t_greater_than_greater_than_greater_than = 34,
    t_less_than = 35,
    t_less_than_equals = 36,
    t_less_than_less_than = 37,
    t_minus = 38,
    t_minus_minus = 39,
    t_open_brace = 40,
    t_open_bracket = 41,
    t_open_paren = 42,
    t_percent = 43,
    t_plus = 44,
    t_plus_plus = 45,
    t_question = 46,
    t_question_dot = 47,
    t_question_question = 48,
    t_semicolon = 49,
    t_slash = 50,
    t_tilde = 51,

    // assignments (contiguous range for O(1) isAssign)
    t_ampersand_ampersand_equals = 52,
    t_ampersand_equals = 53,
    t_asterisk_asterisk_equals = 54,
    t_asterisk_equals = 55,
    t_bar_bar_equals = 56,
    t_bar_equals = 57,
    t_caret_equals = 58,
    t_equals = 59,
    t_greater_than_greater_than_equals = 60,
    t_greater_than_greater_than_greater_than_equals = 61,
    t_less_than_less_than_equals = 62,
    t_minus_equals = 63,
    t_percent_equals = 64,
    t_plus_equals = 65,
    t_question_question_equals = 66,
    t_slash_equals = 67,

    // identifiers
    t_private_identifier = 68,
    t_identifier = 69,
    t_escaped_keyword = 70,

    // reserved words (contiguous range for O(1) isReservedWord)
    t_break = 71,
    t_case = 72,
    t_catch = 73,
    t_class = 74,
    t_const = 75,
    t_continue = 76,
    t_debugger = 77,
    t_default = 78,
    t_delete = 79,
    t_do = 80,
    t_else = 81,
    t_enum = 82,
    t_export = 83,
    t_extends = 84,
    t_false = 85,
    t_finally = 86,
    t_for = 87,
    t_function = 88,
    t_if = 89,
    t_import = 90,
    t_in = 91,
    t_instanceof = 92,
    t_new = 93,
    t_null = 94,
    t_return = 95,
    t_super = 96,
    t_switch = 97,
    t_this = 98,
    t_throw = 99,
    t_true = 100,
    t_try = 101,
    t_typeof = 102,
    t_var = 103,
    t_void = 104,
    t_while = 105,
    t_with = 106,

    fn isAssign(): bool {
        const v = @intFromEnum(_);
        return v >= @intFromEnum(JsToken.t_ampersand_ampersand_equals) &&
            v <= @intFromEnum(JsToken.t_slash_equals);
    }

    fn isReservedWord(): bool {
        const v = @intFromEnum(_);
        return v >= @intFromEnum(JsToken.t_break) &&
            v <= @intFromEnum(JsToken.t_with);
    }

    fn isString(): bool {
        return _ == JsToken.t_no_substitution_template_literal ||
            _ == JsToken.t_string_literal ||
            _ == JsToken.t_template_head ||
            _ == JsToken.t_template_middle ||
            _ == JsToken.t_template_tail;
    }

    fn isCloseBraceOrEOF(): bool {
        return @intFromEnum(_) <= @intFromEnum(JsToken.t_close_brace);
    }

    /// Returns the human-readable diagnostic display string for this token.
    fn asStr(): []const u8 {
        return switch (_) {
            .t_end_of_file => "end of file",
            .t_close_brace => "}",
            .t_syntax_error => "syntax error",
            .t_hashbang => "hashbang comment",
            .t_no_substitution_template_literal => "template literal",
            .t_numeric_literal => "number",
            .t_string_literal => "string",
            .t_big_integer_literal => "bigint",
            .t_template_head => "template head",
            .t_template_middle => "template middle",
            .t_template_tail => "template tail",
            .t_ampersand => "&",
            .t_ampersand_ampersand => "&&",
            .t_asterisk => "*",
            .t_asterisk_asterisk => "**",
            .t_at => "@",
            .t_bar => "|",
            .t_bar_bar => "||",
            .t_caret => "^",
            .t_close_bracket => "]",
            .t_close_paren => ")",
            .t_colon => ":",
            .t_comma => ",",
            .t_dot => ".",
            .t_dot_dot_dot => "...",
            .t_equals_equals => "==",
            .t_equals_equals_equals => "===",
            .t_equals_greater_than => "=>",
            .t_exclamation => "!",
            .t_exclamation_equals => "!=",
            .t_exclamation_equals_equals => "!==",
            .t_greater_than => ">",
            .t_greater_than_equals => ">=",
            .t_greater_than_greater_than => ">>",
            .t_greater_than_greater_than_greater_than => ">>>",
            .t_less_than => "<",
            .t_less_than_equals => "<=",
            .t_less_than_less_than => "<<",
            .t_minus => "-",
            .t_minus_minus => "--",
            .t_open_brace => "{",
            .t_open_bracket => "[",
            .t_open_paren => "(",
            .t_percent => "%",
            .t_plus => "+",
            .t_plus_plus => "++",
            .t_question => "?",
            .t_question_dot => "?.",
            .t_question_question => "??",
            .t_semicolon => ";",
            .t_slash => "/",
            .t_tilde => "~",
            .t_ampersand_ampersand_equals => "&&=",
            .t_ampersand_equals => "&=",
            .t_asterisk_asterisk_equals => "**=",
            .t_asterisk_equals => "*=",
            .t_bar_bar_equals => "||=",
            .t_bar_equals => "|=",
            .t_caret_equals => "^=",
            .t_equals => "=",
            .t_greater_than_greater_than_equals => ">>=",
            .t_greater_than_greater_than_greater_than_equals => ">>>=",
            .t_less_than_less_than_equals => "<<=",
            .t_minus_equals => "-=",
            .t_percent_equals => "%=",
            .t_plus_equals => "+=",
            .t_question_question_equals => "??=",
            .t_slash_equals => "/=",
            .t_private_identifier => "private identifier",
            .t_identifier => "identifier",
            .t_escaped_keyword => "escaped keyword",
            .t_break => "break",
            .t_case => "case",
            .t_catch => "catch",
            .t_class => "class",
            .t_const => "const",
            .t_continue => "continue",
            .t_debugger => "debugger",
            .t_default => "default",
            .t_delete => "delete",
            .t_do => "do",
            .t_else => "else",
            .t_enum => "enum",
            .t_export => "export",
            .t_extends => "extends",
            .t_false => "false",
            .t_finally => "finally",
            .t_for => "for",
            .t_function => "function",
            .t_if => "if",
            .t_import => "import",
            .t_in => "in",
            .t_instanceof => "instanceof",
            .t_new => "new",
            .t_null => "null",
            .t_return => "return",
            .t_super => "super",
            .t_switch => "switch",
            .t_this => "this",
            .t_throw => "throw",
            .t_true => "true",
            .t_try => "try",
            .t_typeof => "typeof",
            .t_var => "var",
            .t_void => "void",
            .t_while => "while",
            .t_with => "with",
        };
    }
}

/// Looks up an ECMAScript reserved keyword by exact ASCII spelling.
pub fn lookupJsKeyword(s: []const u8): JsToken? {
    if (s.len < 2 || s.len > 10) return null;
    if (strEq(s, "break")) return JsToken.t_break;
    if (strEq(s, "case")) return JsToken.t_case;
    if (strEq(s, "catch")) return JsToken.t_catch;
    if (strEq(s, "class")) return JsToken.t_class;
    if (strEq(s, "const")) return JsToken.t_const;
    if (strEq(s, "continue")) return JsToken.t_continue;
    if (strEq(s, "debugger")) return JsToken.t_debugger;
    if (strEq(s, "default")) return JsToken.t_default;
    if (strEq(s, "delete")) return JsToken.t_delete;
    if (strEq(s, "do")) return JsToken.t_do;
    if (strEq(s, "else")) return JsToken.t_else;
    if (strEq(s, "enum")) return JsToken.t_enum;
    if (strEq(s, "export")) return JsToken.t_export;
    if (strEq(s, "extends")) return JsToken.t_extends;
    if (strEq(s, "false")) return JsToken.t_false;
    if (strEq(s, "finally")) return JsToken.t_finally;
    if (strEq(s, "for")) return JsToken.t_for;
    if (strEq(s, "function")) return JsToken.t_function;
    if (strEq(s, "if")) return JsToken.t_if;
    if (strEq(s, "import")) return JsToken.t_import;
    if (strEq(s, "in")) return JsToken.t_in;
    if (strEq(s, "instanceof")) return JsToken.t_instanceof;
    if (strEq(s, "new")) return JsToken.t_new;
    if (strEq(s, "null")) return JsToken.t_null;
    if (strEq(s, "return")) return JsToken.t_return;
    if (strEq(s, "super")) return JsToken.t_super;
    if (strEq(s, "switch")) return JsToken.t_switch;
    if (strEq(s, "this")) return JsToken.t_this;
    if (strEq(s, "throw")) return JsToken.t_throw;
    if (strEq(s, "true")) return JsToken.t_true;
    if (strEq(s, "try")) return JsToken.t_try;
    if (strEq(s, "typeof")) return JsToken.t_typeof;
    if (strEq(s, "var")) return JsToken.t_var;
    if (strEq(s, "void")) return JsToken.t_void;
    if (strEq(s, "while")) return JsToken.t_while;
    if (strEq(s, "with")) return JsToken.t_with;
    return null;
}

/// Returns true if `s` is one of the 9 ECMAScript strict-mode reserved words.
pub fn isStrictModeReserved(s: []const u8): bool {
    return remapStrictModeReserved(s) != null;
}

/// Maps strict-mode reserved words to safe underscore-prefixed identifiers
/// (aligned with Bun's `StrictModeReservedWordsRemap`).
pub fn remapStrictModeReserved(s: []const u8): ([]const u8)? {
    if (strEq(s, "implements")) return "_implements";
    if (strEq(s, "interface")) return "_interface";
    if (strEq(s, "let")) return "_let";
    if (strEq(s, "package")) return "_package";
    if (strEq(s, "private")) return "_private";
    if (strEq(s, "protected")) return "_protected";
    if (strEq(s, "public")) return "_public";
    if (strEq(s, "static")) return "_static";
    if (strEq(s, "yield")) return "_yield";
    return null;
}

/// Class property / method modifier keywords (aligned with Bun's `PropertyModifierKeyword`).
pub enum JsPropertyModifier: u8 {
    p_abstract = 0,
    p_accessor = 1,
    p_async = 2,
    p_declare = 3,
    p_get = 4,
    p_override = 5,
    p_private = 6,
    p_protected = 7,
    p_public = 8,
    p_readonly = 9,
    p_set = 10,
    p_static = 11,

    static fn parse(s: []const u8): JsPropertyModifier? {
        if (strEq(s, "abstract")) return JsPropertyModifier.p_abstract;
        if (strEq(s, "accessor")) return JsPropertyModifier.p_accessor;
        if (strEq(s, "async")) return JsPropertyModifier.p_async;
        if (strEq(s, "declare")) return JsPropertyModifier.p_declare;
        if (strEq(s, "get")) return JsPropertyModifier.p_get;
        if (strEq(s, "override")) return JsPropertyModifier.p_override;
        if (strEq(s, "private")) return JsPropertyModifier.p_private;
        if (strEq(s, "protected")) return JsPropertyModifier.p_protected;
        if (strEq(s, "public")) return JsPropertyModifier.p_public;
        if (strEq(s, "readonly")) return JsPropertyModifier.p_readonly;
        if (strEq(s, "set")) return JsPropertyModifier.p_set;
        if (strEq(s, "static")) return JsPropertyModifier.p_static;
        return null;
    }

    fn isTsAccessibility(): bool {
        return _ == JsPropertyModifier.p_override ||
            _ == JsPropertyModifier.p_private ||
            _ == JsPropertyModifier.p_protected ||
            _ == JsPropertyModifier.p_public ||
            _ == JsPropertyModifier.p_readonly;
    }
}

pub fn isTsAccessibilityModifier(s: []const u8): bool {
    if (m := JsPropertyModifier.parse(s)) {
        return m.isTsAccessibility();
    }
    return false;
}

/// Decodes high-frequency JSX named HTML entities to Unicode codepoints (without leading `&` or trailing `;`).
pub fn lookupJsxEntity(name: []const u8): u32? {
    if (strEq(name, "amp")) return 0x0026;
    if (strEq(name, "lt")) return 0x003C;
    if (strEq(name, "gt")) return 0x003E;
    if (strEq(name, "quot")) return 0x0022;
    if (strEq(name, "apos")) return 0x0027;
    if (strEq(name, "nbsp")) return 0x00A0;
    if (strEq(name, "copy")) return 0x00A9;
    if (strEq(name, "reg")) return 0x00AE;
    if (strEq(name, "trade")) return 0x2122;
    if (strEq(name, "mdash")) return 0x2014;
    if (strEq(name, "ndash")) return 0x2013;
    if (strEq(name, "hellip")) return 0x2026;
    if (strEq(name, "bull")) return 0x2022;
    if (strEq(name, "middot")) return 0x00B7;
    if (strEq(name, "times")) return 0x00D7;
    if (strEq(name, "divide")) return 0x00F7;
    if (strEq(name, "plusmn")) return 0x00B1;
    if (strEq(name, "larr")) return 0x2190;
    if (strEq(name, "uarr")) return 0x2191;
    if (strEq(name, "rarr")) return 0x2192;
    if (strEq(name, "darr")) return 0x2193;
    if (strEq(name, "laquo")) return 0x00AB;
    if (strEq(name, "raquo")) return 0x00BB;
    if (strEq(name, "lsquo")) return 0x2018;
    if (strEq(name, "rsquo")) return 0x2019;
    if (strEq(name, "ldquo")) return 0x201C;
    if (strEq(name, "rdquo")) return 0x201D;
    if (strEq(name, "euro")) return 0x20AC;
    if (strEq(name, "pound")) return 0x00A3;
    if (strEq(name, "yen")) return 0x00A5;
    if (strEq(name, "cent")) return 0x00A2;
    if (strEq(name, "sect")) return 0x00A7;
    if (strEq(name, "deg")) return 0x00B0;
    return null;
}

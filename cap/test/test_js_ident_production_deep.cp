import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KeywordKind,
    classifyKeyword,
    isReservedWord,
    isValidAsciiIdent,
    isValidPrivateIdent,
    needsPropertyQuoting,
    escapeJsString,
} from "cap:js";

test("1. classifyKeyword covers all 36 ES2024 reserved words, 10 strict reserved, and 10 contextual keywords", () => {
    const reserved_list: [36][]const u8 = [
        "break", "case", "catch", "class", "const", "continue",
        "debugger", "default", "delete", "do", "else", "enum",
        "export", "extends", "false", "finally", "for", "function",
        "if", "import", "in", "instanceof", "new", "null",
        "return", "super", "switch", "this", "throw", "true",
        "try", "typeof", "var", "void", "while", "with",
    ];
    for (kw of reserved_list) {
        expect_eq(classifyKeyword(kw) == KeywordKind.reserved, true);
        expect_eq(isReservedWord(kw, false), true);
        expect_eq(isReservedWord(kw, true), true);
    }

    const strict_list: [10][]const u8 = [
        "implements", "interface", "let", "package", "private",
        "protected", "public", "static", "yield", "await",
    ];
    for (kw of strict_list) {
        expect_eq(classifyKeyword(kw) == KeywordKind.strict_reserved, true);
        expect_eq(isReservedWord(kw, false), false);
        expect_eq(isReservedWord(kw, true), true);
    }

    const contextual_list: [10][]const u8 = [
        "as", "async", "from", "get", "meta",
        "of", "set", "target", "type", "satisfies",
    ];
    for (kw of contextual_list) {
        expect_eq(classifyKeyword(kw) == KeywordKind.contextual, true);
        expect_eq(isReservedWord(kw, false), false);
        expect_eq(isReservedWord(kw, true), false);
    }

    expect_eq(classifyKeyword("identifier") == KeywordKind.none, true);
    expect_eq(classifyKeyword("CLASS") == KeywordKind.none, true);
    expect_eq(classifyKeyword("") == KeywordKind.none, true);
});

test("2. isValidAsciiIdent and isValidPrivateIdent validation", () => {
    expect_eq(isValidAsciiIdent("foo"), true);
    expect_eq(isValidAsciiIdent("_bar123"), true);
    expect_eq(isValidAsciiIdent("$"), true);
    expect_eq(isValidAsciiIdent("$0_aZ"), true);

    expect_eq(isValidAsciiIdent(""), false);
    expect_eq(isValidAsciiIdent("0abc"), false);
    expect_eq(isValidAsciiIdent("foo-bar"), false);
    expect_eq(isValidAsciiIdent("foo bar"), false);
    expect_eq(isValidAsciiIdent("#priv"), false);

    expect_eq(isValidPrivateIdent("#secret"), true);
    expect_eq(isValidPrivateIdent("#$1_x"), true);
    expect_eq(isValidPrivateIdent("#"), false);
    expect_eq(isValidPrivateIdent("#123"), false);
    expect_eq(isValidPrivateIdent("secret"), false);
    expect_eq(isValidPrivateIdent("##double"), false);
});

test("3. needsPropertyQuoting handles identifiers, hyphens, and strict vs non-strict reserved words", () => {
    expect_eq(needsPropertyQuoting("normalKey", false), false);
    expect_eq(needsPropertyQuoting("normalKey", true), false);
    expect_eq(needsPropertyQuoting("content-type", false), true);
    expect_eq(needsPropertyQuoting("123", false), true);
    expect_eq(needsPropertyQuoting("", true), true);

    expect_eq(needsPropertyQuoting("class", false), true);
    expect_eq(needsPropertyQuoting("class", true), true);
    expect_eq(needsPropertyQuoting("let", false), false);
    expect_eq(needsPropertyQuoting("let", true), true);
    expect_eq(needsPropertyQuoting("async", true), false);
});

test("4. escapeJsString escapes quotes, backslashes, control chars, and U+2028/U+2029", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const s1 = escapeJsString("a\"b\\c\n\r\t", '"', &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1, "\"a\\\"b\\\\c\\n\\r\\t\""), true);

    const s2 = escapeJsString("it's ok", '\'', &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s2, "'it\\'s ok'"), true);

    // 控制字符：0x00 后跟数字用 \x00，后跟非数字用 \0，0x01 用 \x01
    const ctrl_raw: [5]u8 = [0x00, '7', 0x00, 'a', 0x01];
    const s3 = escapeJsString(&ctrl_raw, '"', &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s3, "\"\\x007\\0a\\x01\""), true);

    // UTF-8 行分隔符 U+2028 与 U+2029
    const line_seps: [6]u8 = [0xE2, 0x80, 0xA8, 0xE2, 0x80, 0xA9];
    const s4 = escapeJsString(&line_seps, '"', &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s4, "\"\\u2028\\u2029\""), true);
});

test("5. escapeJsString empty input and buffer-too-small guard", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    const empty = escapeJsString("", '"', &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, empty, "\"\""), true);

    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(escapeJsString("ab", '"', &tiny) == null, true);

    let exact: [4]u8 = std.mem.zeroes([4]u8);
    const ok = escapeJsString("ab", '"', &exact) ?? unreachable;
    expect_eq(std.mem.eql(u8, ok, "\"ab\""), true);
});

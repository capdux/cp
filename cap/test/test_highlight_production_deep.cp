// Source: cp/cap/test/test_highlight_production_deep.cp
// Deep Production Verification Suite for cap:highlight & cap.highlight

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { AnsiColorCodec } from "cap:color";
import cap from "cap";
import {
    HighlightError,
    Language,
    TokenScope,
    HighlightToken,
    Highlighter,
} from "cap:text";

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn strContains(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn concatTokensAlloc(alloc: std.mem.Allocator, tokens: []HighlightToken): ([]u8) ! HighlightError {
    let total: usize = 0;
    for (t of tokens) {
        total += t.text.len;
    }
    let buf = alloc.alloc(u8, total) catch return HighlightError.OutOfMemory;
    let pos: usize = 0;
    for (t of tokens) {
        @memcpy(buf[pos .. pos + t.text.len], t.text);
        pos += t.text.len;
    }
    return buf;
}

fn hasToken(tokens: []HighlightToken, scope: TokenScope, text: string): bool {
    for (t of tokens) {
        if (t.scope == scope && strEql(t.text, text)) return true;
    }
    return false;
}

fn testMultiLanguageTokenizeLossless(): !void {
    const alloc = std.testing.allocator;

    // 1. CP: comments, @alloc builtin, fn/let keywords, numbers, range 0..10
    const cp_code =
        "// line comment\n" ++
        "/* block comment */\n" ++
        "pub fn demo(n: usize): i32 {\n" ++
        "    let ptr = @alloc(64);\n" ++
        "    let hex = 0xFF_01;\n" ++
        "    let sci = 1.5e-3;\n" ++
        "    for (i of 0..10) { * = i; }\n" ++
        "    return 42;\n" ++
        "}\n";
    const cp_tokens = Highlighter.tokenize(alloc, .Cp, cp_code)?;
    defer alloc.free(cp_tokens);
    const cp_joined = concatTokensAlloc(alloc, cp_tokens)?;
    defer alloc.free(cp_joined);
    expect_eq(strEql(cp_joined, cp_code), true);
    expect_eq(hasToken(cp_tokens, .Comment, "// line comment"), true);
    expect_eq(hasToken(cp_tokens, .Comment, "/* block comment */"), true);
    expect_eq(hasToken(cp_tokens, .Builtin, "@alloc"), true);
    expect_eq(hasToken(cp_tokens, .Keyword, "fn"), true);
    expect_eq(hasToken(cp_tokens, .Keyword, "let"), true);
    expect_eq(hasToken(cp_tokens, .TypeName, "usize"), true);
    expect_eq(hasToken(cp_tokens, .Number, "0xFF_01"), true);
    expect_eq(hasToken(cp_tokens, .Number, "1.5e-3"), true);
    expect_eq(hasToken(cp_tokens, .Number, "0"), true);

    // 2. Rust: // + /* */ + fn/let + String + println
    const rs_code = "/* rs */\nfn main() {\n    let s: String = \"ok\"; // end\n    println!(\"{}\", s);\n}\n";
    const rs_tokens = Highlighter.tokenize(alloc, .Rust, rs_code)?;
    defer alloc.free(rs_tokens);
    const rs_joined = concatTokensAlloc(alloc, rs_tokens)?;
    defer alloc.free(rs_joined);
    expect_eq(strEql(rs_joined, rs_code), true);
    expect_eq(hasToken(rs_tokens, .Keyword, "fn"), true);
    expect_eq(hasToken(rs_tokens, .Keyword, "let"), true);
    expect_eq(hasToken(rs_tokens, .TypeName, "String"), true);
    expect_eq(hasToken(rs_tokens, .String, "\"ok\""), true);
    expect_eq(hasToken(rs_tokens, .Comment, "// end"), true);

    // 3. Python: # comment + def + triple-quote string
    const py_code = "# py comment\ndef greet(name: str) -> str:\n    \"\"\"doc\"\"\"\n    return f'hi {name}'\n";
    const py_tokens = Highlighter.tokenize(alloc, .Python, py_code)?;
    defer alloc.free(py_tokens);
    const py_joined = concatTokensAlloc(alloc, py_tokens)?;
    defer alloc.free(py_joined);
    expect_eq(strEql(py_joined, py_code), true);
    expect_eq(hasToken(py_tokens, .Comment, "# py comment"), true);
    expect_eq(hasToken(py_tokens, .Keyword, "def"), true);
    expect_eq(hasToken(py_tokens, .TypeName, "str"), true);
    expect_eq(hasToken(py_tokens, .String, "\"\"\"doc\"\"\""), true);

    // 4. SQL: case-insensitive SELECT/from + -- comment
    const sql_code = "-- query users\nSELECT id, name from users WHERE active = TRUE;\n";
    const sql_tokens = Highlighter.tokenize(alloc, .Sql, sql_code)?;
    defer alloc.free(sql_tokens);
    const sql_joined = concatTokensAlloc(alloc, sql_tokens)?;
    defer alloc.free(sql_joined);
    expect_eq(strEql(sql_joined, sql_code), true);
    expect_eq(hasToken(sql_tokens, .Comment, "-- query users"), true);
    expect_eq(hasToken(sql_tokens, .Keyword, "SELECT"), true);
    expect_eq(hasToken(sql_tokens, .Keyword, "from"), true);
    expect_eq(hasToken(sql_tokens, .Keyword, "WHERE"), true);
    expect_eq(hasToken(sql_tokens, .Builtin, "TRUE"), true);

    // 5. Zig, TypeScript, JSON, Bash lossless round-trip
    const zig_code = "const std = @import(\"std\");\npub fn add(a: i32, b: i32) i32 { return a + b; }\n";
    const zig_tokens = Highlighter.tokenize(alloc, .Zig, zig_code)?;
    defer alloc.free(zig_tokens);
    const zig_joined = concatTokensAlloc(alloc, zig_tokens)?;
    defer alloc.free(zig_joined);
    expect_eq(strEql(zig_joined, zig_code), true);
    expect_eq(hasToken(zig_tokens, .Builtin, "@import"), true);

    const ts_code = "export const msg: string = `hello ${name}`;\n";
    const ts_tokens = Highlighter.tokenize(alloc, .TypeScript, ts_code)?;
    defer alloc.free(ts_tokens);
    const ts_joined = concatTokensAlloc(alloc, ts_tokens)?;
    defer alloc.free(ts_joined);
    expect_eq(strEql(ts_joined, ts_code), true);
    expect_eq(hasToken(ts_tokens, .Keyword, "export"), true);
    expect_eq(hasToken(ts_tokens, .TypeName, "string"), true);

    const json_code = "{\"ok\": true, \"count\": 123, \"nil\": null}\n";
    const json_tokens = Highlighter.tokenize(alloc, .Json, json_code)?;
    defer alloc.free(json_tokens);
    const json_joined = concatTokensAlloc(alloc, json_tokens)?;
    defer alloc.free(json_joined);
    expect_eq(strEql(json_joined, json_code), true);
    expect_eq(hasToken(json_tokens, .Builtin, "true"), true);
    expect_eq(hasToken(json_tokens, .Number, "123"), true);

    const bash_code = "#!/usr/bin/env bash\nif [ -f \"$1\" ]; then echo 'ok'; fi\n";
    const bash_tokens = Highlighter.tokenize(alloc, .Bash, bash_code)?;
    defer alloc.free(bash_tokens);
    const bash_joined = concatTokensAlloc(alloc, bash_tokens)?;
    defer alloc.free(bash_joined);
    expect_eq(strEql(bash_joined, bash_code), true);
    expect_eq(hasToken(bash_tokens, .Comment, "#!/usr/bin/env bash"), true);
    expect_eq(hasToken(bash_tokens, .Keyword, "if"), true);
    expect_eq(hasToken(bash_tokens, .Builtin, "echo"), true);
}

fn testAnsiHighlightAndStripRoundTrip(): !void {
    const alloc = std.testing.allocator;
    const code = "pub fn hello(name: string): bool {\n    // greet 中文\n    return true;\n}\n";

    const ansi = Highlighter.toAnsi(alloc, .Cp, code)?;
    defer alloc.free(ansi);

    expect_eq(strContains(ansi, "\x1b["), true);
    expect_eq(strContains(ansi, "\x1b[0m"), true);

    const stripped = AnsiColorCodec.stripAnsi(alloc, ansi) catch return HighlightError.OutOfMemory;
    defer alloc.free(stripped);
    expect_eq(strEql(stripped, code), true);
}

fn testHtmlHighlightAndXssEscaping(): !void {
    const alloc = std.testing.allocator;
    const xss_code = "const payload = \"<script>alert(\\\"x\\\")&'y'</script>\";\n";

    const html = Highlighter.toHtml(alloc, .TypeScript, xss_code)?;
    defer alloc.free(html);

    expect_eq(std.mem.startsWith(u8, html, "<pre class=\"cp-hl\"><code>"), true);
    expect_eq(std.mem.endsWith(u8, html, "</code></pre>"), true);
    expect_eq(strContains(html, "<span class=\"hl-keyword\">const</span>"), true);
    expect_eq(strContains(html, "<span class=\"hl-string\">"), true);

    // Must not contain raw <script> or unescaped & / '
    expect_eq(strContains(html, "<script>"), false);
    expect_eq(strContains(html, "&lt;script&gt;"), true);
    expect_eq(strContains(html, "&amp;"), true);
    expect_eq(strContains(html, "&#39;"), true);
    expect_eq(strContains(html, "&quot;"), true);
}

fn testLanguageFromNameAndLeakFree(): !void {
    const alloc = std.testing.allocator;

    expect_eq(Highlighter.languageFromName("cp").?, Language.Cp);
    expect_eq(Highlighter.languageFromName(".CP").?, Language.Cp);
    expect_eq(Highlighter.languageFromName("zig").?, Language.Zig);
    expect_eq(Highlighter.languageFromName(" .zig ").?, Language.Zig);
    expect_eq(Highlighter.languageFromName("rs").?, Language.Rust);
    expect_eq(Highlighter.languageFromName("Rust").?, Language.Rust);
    expect_eq(Highlighter.languageFromName("ts").?, Language.TypeScript);
    expect_eq(Highlighter.languageFromName(".tsx").?, Language.TypeScript);
    expect_eq(Highlighter.languageFromName("py").?, Language.Python);
    expect_eq(Highlighter.languageFromName("Python").?, Language.Python);
    expect_eq(Highlighter.languageFromName(".json").?, Language.Json);
    expect_eq(Highlighter.languageFromName("SQL").?, Language.Sql);
    expect_eq(Highlighter.languageFromName("sh").?, Language.Bash);
    expect_eq(Highlighter.languageFromName(".bash"), null);
    expect_eq(Highlighter.languageFromName("bash").?, Language.Bash);
    expect_eq(Highlighter.languageFromName("unknown_lang"), null);
    expect_eq(Highlighter.languageFromName(""), null);

    // Exercise tokenize + toAnsi + toHtml under std.testing.allocator
    const sample = "SELECT 1 + 2; -- done\n";
    const toks = Highlighter.tokenize(alloc, .Sql, sample)?;
    defer alloc.free(toks);
    const ansi = Highlighter.toAnsi(alloc, .Sql, sample)?;
    defer alloc.free(ansi);
    const html = Highlighter.toHtml(alloc, .Sql, sample)?;
    defer alloc.free(html);
    expect_eq(toks.len > 0, true);
    expect_eq(ansi.len > sample.len, true);
    expect_eq(html.len > sample.len, true);
}

fn testCapHighlightDynamicMount(): !void {
    const alloc = std.testing.allocator;

    const lang = cap.text.Highlighter.languageFromName("rust") ?? cap.text.Language.Rust;
    expect_eq(lang, cap.text.Language.Rust);

    const code = "pub fn sum(a: u32, b: u32) -> u32 { a + b }";
    const toks = cap.text.Highlighter.tokenize(alloc, lang, code)?;
    defer alloc.free(toks);
    expect_eq(toks.len > 0, true);

    const ansi = cap.text.Highlighter.toAnsi(alloc, lang, code)?;
    defer alloc.free(ansi);
    expect_eq(strContains(ansi, "\x1b["), true);

    const html = cap.text.Highlighter.toHtml(alloc, lang, code)?;
    defer alloc.free(html);
    expect_eq(strContains(html, "<span class=\"hl-keyword\">pub</span>"), true);
}

test("1. Multi-Language Lossless Tokenize (CP/Rust/Python/SQL/Zig/TS/JSON/Bash)") {
    testMultiLanguageTokenizeLossless() catch unreachable;
}

test("2. toAnsi Terminal Highlighting & ANSI Strip Byte-for-Byte Round-Trip") {
    testAnsiHighlightAndStripRoundTrip() catch unreachable;
}

test("3. toHtml <span class=\"hl-...\"> & XSS Entity Escaping") {
    testHtmlHighlightAndXssEscaping() catch unreachable;
}

test("4. languageFromName Aliases/Extensions & std.testing.allocator Zero-Leak") {
    testLanguageFromNameAndLeakFree() catch unreachable;
}

test("5. import cap from \"cap\"; cap.highlight Dynamic Mounting") {
    testCapHighlightDynamicMount() catch unreachable;
}

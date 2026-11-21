// Source: cp/cap/test/test_idl_production_deep.cp (Production deep tests for cap:idl)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WebIdlDeclKind,
    WebIdlDecl,
    WebIdlTokenKind,
    WebIdlToken,
    WebIdlIter,
    WebIdlDeclIter,
    validateWebIdlBraces,
} from "cap:idl";

const SAMPLE_IDL: []const u8 =
    "// W3C Fetch & Streams excerpt\n" ++
    "/* Multi-line\n" ++
    "   block comment with { unmatched [ */\n" ++
    "[Exposed=(Window,Worker), SecureContext]\n" ++
    "interface Response : EventTarget {\n" ++
    "  readonly attribute unsigned short status;\n" ++
    "  Promise<USVString> text();\n" ++
    "};\n" ++
    "partial interface Response {\n" ++
    "  readonly attribute boolean redirected;\n" ++
    "};\n" ++
    "interface mixin Body {\n" ++
    "  readonly attribute boolean bodyUsed;\n" ++
    "};\n" ++
    "dictionary RequestInit : BaseInit {\n" ++
    "  ByteString method = \"GET\";\n" ++
    "};\n" ++
    "enum RequestMode { \"navigate\", \"cors\", \"no-cors\" };\n" ++
    "[Exposed=Window]\n" ++
    "namespace console {\n" ++
    "  undefined log(any... data);\n" ++
    "};\n";

test("1. WebIdlIter lexes comments, strings, numbers, idents, variadic punct, and invalid tokens", () => {
    const src: []const u8 =
        "// line comment\n" ++
        "/* block */ interface Foo { const long X = -0x2A; const double Y = 3.14e-2; " ++
        "undefined log(DOMString msg = \"hi \\\"idl\\\"\", any... rest); };";
    let it = WebIdlIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == WebIdlTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "interface"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.kind == WebIdlTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t1.lexeme, "Foo"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.kind == WebIdlTokenKind.punct, true);
    expect_eq(std.mem.eql(u8, t2.lexeme, "{"), true);

    let found_hex = false;
    let found_float = false;
    let found_str = false;
    let found_ellipsis = false;
    while (tok := it.next()) {
        if (tok.kind == WebIdlTokenKind.number && std.mem.eql(u8, tok.lexeme, "-0x2A")) {
            found_hex = true;
        }
        if (tok.kind == WebIdlTokenKind.number && std.mem.eql(u8, tok.lexeme, "3.14e-2")) {
            found_float = true;
        }
        if (tok.kind == WebIdlTokenKind.string && std.mem.eql(u8, tok.lexeme, "\"hi \\\"idl\\\"\"")) {
            found_str = true;
        }
        if (tok.kind == WebIdlTokenKind.punct && std.mem.eql(u8, tok.lexeme, "...")) {
            found_ellipsis = true;
        }
    }
    expect_eq(found_hex, true);
    expect_eq(found_float, true);
    expect_eq(found_str, true);
    expect_eq(found_ellipsis, true);

    // Unterminated string and block comment produce invalid token
    let bad_str = WebIdlIter.init("\"unterminated");
    const bs = bad_str.next() ?? unreachable;
    expect_eq(bs.kind == WebIdlTokenKind.invalid, true);
    expect_eq(bad_str.next() == null, true);

    let bad_com = WebIdlIter.init("/* unclosed comment");
    const bc = bad_com.next() ?? unreachable;
    expect_eq(bc.kind == WebIdlTokenKind.invalid, true);
    expect_eq(bad_com.next() == null, true);
});

test("2. WebIdlDeclIter extracts interface, partial, mixin, dictionary, enum, namespace with ext_attrs", () => {
    let it = WebIdlDeclIter.init(SAMPLE_IDL);

    // 1) [Exposed=(Window,Worker), SecureContext] interface Response : EventTarget { ... }
    const d0 = it.next() ?? unreachable;
    expect_eq(d0.kind == WebIdlDeclKind.interface_decl, true);
    expect_eq(d0.is_partial, false);
    expect_eq(std.mem.eql(u8, d0.name, "Response"), true);
    expect_eq(std.mem.eql(u8, d0.inherits ?? "", "EventTarget"), true);
    expect_eq(std.mem.eql(u8, d0.ext_attrs ?? "", "[Exposed=(Window,Worker), SecureContext]"), true);
    expect_eq((d0.body ?? "").len > 0, true);

    // 2) partial interface Response
    const d1 = it.next() ?? unreachable;
    expect_eq(d1.kind == WebIdlDeclKind.interface_decl, true);
    expect_eq(d1.is_partial, true);
    expect_eq(std.mem.eql(u8, d1.name, "Response"), true);
    expect_eq(d1.inherits == null, true);
    expect_eq(d1.ext_attrs == null, true);
    expect_eq(std.mem.eql(u8, d1.body ?? "", "readonly attribute boolean redirected;"), true);

    // 3) interface mixin Body
    const d2 = it.next() ?? unreachable;
    expect_eq(d2.kind == WebIdlDeclKind.interface_mixin, true);
    expect_eq(d2.is_partial, false);
    expect_eq(std.mem.eql(u8, d2.name, "Body"), true);
    expect_eq(std.mem.eql(u8, d2.body ?? "", "readonly attribute boolean bodyUsed;"), true);

    // 4) dictionary RequestInit : BaseInit
    const d3 = it.next() ?? unreachable;
    expect_eq(d3.kind == WebIdlDeclKind.dictionary_decl, true);
    expect_eq(std.mem.eql(u8, d3.name, "RequestInit"), true);
    expect_eq(std.mem.eql(u8, d3.inherits ?? "", "BaseInit"), true);
    expect_eq(std.mem.eql(u8, d3.body ?? "", "ByteString method = \"GET\";"), true);

    // 5) enum RequestMode
    const d4 = it.next() ?? unreachable;
    expect_eq(d4.kind == WebIdlDeclKind.enum_decl, true);
    expect_eq(std.mem.eql(u8, d4.name, "RequestMode"), true);
    expect_eq(std.mem.eql(u8, d4.body ?? "", "\"navigate\", \"cors\", \"no-cors\""), true);

    // 6) [Exposed=Window] namespace console
    const d5 = it.next() ?? unreachable;
    expect_eq(d5.kind == WebIdlDeclKind.namespace_decl, true);
    expect_eq(std.mem.eql(u8, d5.name, "console"), true);
    expect_eq(std.mem.eql(u8, d5.ext_attrs ?? "", "[Exposed=Window]"), true);
    expect_eq(std.mem.eql(u8, d5.body ?? "", "undefined log(any... data);"), true);

    expect_eq(it.next() == null, true);
});

test("3. WebIdlDeclIter extracts typedef, callback (fn & interface), and includes statements", () => {
    const src: []const u8 =
        "typedef (USVString or sequence<ByteString>) HeadersInit;\n" ++
        "callback EventHandler = any (Event event);\n" ++
        "callback interface EventListener {\n" ++
        "  undefined handleEvent(Event event);\n" ++
        "};\n" ++
        "Response includes Body;\n";

    let it = WebIdlDeclIter.init(src);

    const d0 = it.next() ?? unreachable;
    expect_eq(d0.kind == WebIdlDeclKind.typedef_decl, true);
    expect_eq(std.mem.eql(u8, d0.name, "HeadersInit"), true);
    expect_eq(std.mem.eql(u8, d0.body ?? "", "(USVString or sequence<ByteString>)"), true);

    const d1 = it.next() ?? unreachable;
    expect_eq(d1.kind == WebIdlDeclKind.callback_decl, true);
    expect_eq(std.mem.eql(u8, d1.name, "EventHandler"), true);
    expect_eq(std.mem.eql(u8, d1.body ?? "", "any (Event event)"), true);

    const d2 = it.next() ?? unreachable;
    expect_eq(d2.kind == WebIdlDeclKind.callback_decl, true);
    expect_eq(std.mem.eql(u8, d2.name, "EventListener"), true);
    expect_eq(std.mem.eql(u8, d2.body ?? "", "undefined handleEvent(Event event);"), true);

    const d3 = it.next() ?? unreachable;
    expect_eq(d3.kind == WebIdlDeclKind.includes_decl, true);
    expect_eq(std.mem.eql(u8, d3.name, "Response"), true);
    expect_eq(std.mem.eql(u8, d3.inherits ?? "", "Body"), true);
    expect_eq(d3.body == null, true);

    expect_eq(it.next() == null, true);
});

test("4. validateWebIdlBraces checks balanced delimiters, string/comment awareness, and 64-depth limit", () => {
    expect_eq(validateWebIdlBraces(SAMPLE_IDL), true);
    expect_eq(validateWebIdlBraces("interface A { Promise<sequence<(long or \"}\")>> f(); };"), true);
    expect_eq(validateWebIdlBraces("// {\n/* [ ( < */ interface Ok {};"), true);

    // Exact 64-level nesting -> valid
    let exact64: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        exact64[i] = '(';
        exact64[127 - i] = ')';
        i += 1;
    }
    expect_eq(validateWebIdlBraces(exact64[0..128]), true);

    // 65-level nesting -> exceeds 64-level limit -> false
    let over65: [130]u8 = undefined;
    let j: usize = 0;
    while (j < 65) {
        over65[j] = '<';
        over65[129 - j] = '>';
        j += 1;
    }
    expect_eq(validateWebIdlBraces(over65[0..130]), false);
});

test("5. Malformed inputs and boundary protection for decl scanner and brace validator", () => {
    // Brace mismatches and unterminated literals
    expect_eq(validateWebIdlBraces("interface Bad { ( [ ) ] };"), false);
    expect_eq(validateWebIdlBraces("interface ExtraClose {}; }"), false);
    expect_eq(validateWebIdlBraces("interface Unclosed { void f();"), false);
    expect_eq(validateWebIdlBraces("const DOMString s = \"unterminated;"), false);
    expect_eq(validateWebIdlBraces("/* unterminated comment"), false);

    // Empty and malformed inputs in WebIdlDeclIter must terminate cleanly
    let it_empty = WebIdlDeclIter.init("   // only comment\n");
    expect_eq(it_empty.next() == null, true);

    let it_unclosed_attr = WebIdlDeclIter.init("[Exposed=Window interface Foo {};");
    expect_eq(it_unclosed_attr.next() == null, true);

    let it_unclosed_body = WebIdlDeclIter.init("interface Broken { readonly attribute long x;");
    expect_eq(it_unclosed_body.next() == null, true);

    let it_unclosed_typedef = WebIdlDeclIter.init("typedef sequence<long> MissingSemi");
    expect_eq(it_unclosed_typedef.next() == null, true);
});

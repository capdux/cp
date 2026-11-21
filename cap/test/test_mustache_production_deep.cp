import {
    MustacheTokenKind,
    MustacheToken,
    MustacheVar,
    MustacheIter,
    validateMustache,
    renderMustache,
} from "cap:mustache";
import { test, expect, expect_eq } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("MustacheIter tokenizes text, escaped_var, raw_var (triple and &), and comments", () => {
    const tpl = "Hello {{ name }}! {{! greeting }}{{{ raw_html }}} and {{& raw_amp }} end";
    let it = MustacheIter.init(tpl);

    const t0 = it.next().?;
    expect(t0.kind == MustacheTokenKind.text);
    expect(strEq(t0.content, "Hello "));

    const t1 = it.next().?;
    expect(t1.kind == MustacheTokenKind.escaped_var);
    expect(strEq(t1.content, "name"));

    const t2 = it.next().?;
    expect(t2.kind == MustacheTokenKind.text);
    expect(strEq(t2.content, "! "));

    const t3 = it.next().?;
    expect(t3.kind == MustacheTokenKind.comment);
    expect(strEq(t3.content, "greeting"));

    const t4 = it.next().?;
    expect(t4.kind == MustacheTokenKind.raw_var);
    expect(strEq(t4.content, "raw_html"));

    const t5 = it.next().?;
    expect(t5.kind == MustacheTokenKind.text);
    expect(strEq(t5.content, " and "));

    const t6 = it.next().?;
    expect(t6.kind == MustacheTokenKind.raw_var);
    expect(strEq(t6.content, "raw_amp"));

    const t7 = it.next().?;
    expect(t7.kind == MustacheTokenKind.text);
    expect(strEq(t7.content, " end"));

    expect(it.next() == null);
    expect(!it.has_error);
});

test("renderMustache performs HTML escaping on {{var}} and raw output on {{{raw}}} / {{& raw}}", () => {
    const tpl = "escaped={{ payload }}|triple={{{ payload }}}|amp={{& payload }}";
    const vars: [1]MustacheVar = [
        MustacheVar { key: "payload", value: "<a href=\"x\" data-k='y'>&</a>" },
    ];
    let buf: [256]u8 = undefined;
    const out = renderMustache(tpl, vars[0..], buf[0..]).?;

    const expected =
        "escaped=&lt;a href=&quot;x&quot; data-k=&#39;y&#39;&gt;&amp;&lt;/a&gt;" ++
        "|triple=<a href=\"x\" data-k='y'>&</a>" ++
        "|amp=<a href=\"x\" data-k='y'>&</a>";
    expect(strEq(out, expected));
});

test("renderMustache omits comments, renders missing vars as empty, and handles single braces", () => {
    const tpl = "{json: {{ val }}}{{! ignore me }}[{{ missing }}]{{!}}";
    const vars: [1]MustacheVar = [
        MustacheVar { key: "val", value: "42" },
    ];
    let buf: [128]u8 = undefined;
    const out = renderMustache(tpl, vars[0..], buf[0..]).?;
    expect(strEq(out, "{json: 42}[]"));

    // Empty template renders to empty slice
    const empty_out = renderMustache("", vars[0..], buf[0..]).?;
    expect_eq(empty_out.len, 0);
});

test("renderMustache enforces exact buffer capacity boundaries without overflow", () => {
    const tpl = "Hi {{user}}!";
    const vars: [1]MustacheVar = [
        MustacheVar { key: "user", value: "Bob" },
    ];

    // Exact output is "Hi Bob!" (7 bytes)
    let small: [6]u8 = undefined;
    expect(renderMustache(tpl, vars[0..], small[0..]) == null);

    let exact: [7]u8 = undefined;
    const ok = renderMustache(tpl, vars[0..], exact[0..]).?;
    expect(strEq(ok, "Hi Bob!"));

    // Escaped expansion exceeds buffer ("<" -> "&lt;" = 4 bytes)
    const esc_vars: [1]MustacheVar = [
        MustacheVar { key: "user", value: "<" },
    ];
    let esc_small: [7]u8 = undefined;
    expect(renderMustache(tpl, esc_vars[0..], esc_small[0..]) == null);

    let esc_exact: [8]u8 = undefined;
    const esc_ok = renderMustache(tpl, esc_vars[0..], esc_exact[0..]).?;
    expect(strEq(esc_ok, "Hi &lt;!"));
});

test("validateMustache and renderMustache reject unclosed, empty, and malformed tags", () => {
    // Valid templates
    expect(validateMustache(""));
    expect(validateMustache("plain text { single } braces"));
    expect(validateMustache("{{a}} {{{b}}} {{& c}} {{! comment }} {{!}}"));

    // Unclosed tags
    expect(!validateMustache("hello {{name"));
    expect(!validateMustache("hello {{{raw}}"));
    expect(!validateMustache("hello {{& raw"));
    expect(!validateMustache("hello {{! unclosed comment"));

    // Empty variable names
    expect(!validateMustache("{{}}"));
    expect(!validateMustache("{{   }}"));
    expect(!validateMustache("{{{ }}}"));
    expect(!validateMustache("{{&  }}"));

    // Nested unclosed tag
    expect(!validateMustache("{{foo {{bar}}"));

    // renderMustache must also return null on malformed template
    const vars: [1]MustacheVar = [
        MustacheVar { key: "name", value: "Alice" },
    ];
    let buf: [64]u8 = undefined;
    expect(renderMustache("hi {{name", vars[0..], buf[0..]) == null);
    expect(renderMustache("hi {{{name}}", vars[0..], buf[0..]) == null);
    expect(renderMustache("hi {{ }}", vars[0..], buf[0..]) == null);
});

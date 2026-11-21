// Source: cp/cap/test/test_caddy_production_deep.cp (Production deep tests for cap:caddy)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CaddyTokenKind,
    CaddyToken,
    CaddyIter,
    CaddySiteBlock,
    CaddySiteIter,
    validateCaddyfile,
} from "cap:caddy";

const SAMPLE_CADDYFILE: []const u8 =
    "# Global Caddy options\n" ++
    "{\n" ++
    "    email ops@example.com\n" ++
    "    admin off\n" ++
    "}\n" ++
    "\n" ++
    "(security_headers) {\n" ++
    "    header {\n" ++
    "        X-Frame-Options \"DENY\"\n" ++
    "        X-Upstream {http.request.host}\n" ++
    "    }\n" ++
    "}\n" ++
    "\n" ++
    "example.com, www.example.com {\n" ++
    "    import security_headers\n" ++
    "    @api path /api/*\n" ++
    "    reverse_proxy @api localhost:8080 {\n" ++
    "        header_up X-Real-IP {remote_host}\n" ++
    "    }\n" ++
    "    respond /health `{\"status\":\"ok\"}` 200\n" ++
    "}\n" ++
    "\n" ++
    ":8080 {\n" ++
    "    respond \"hello \\\"world\\\"\"\n" ++
    "}\n";

test("1. CaddyIter lexes comments, strings, raw backticks, @matchers, placeholders, braces, and newlines", () => {
    const src =
        "# comment with { unbalanced\n" ++
        "@api path /v1/*\n" ++
        "header X-Host {http.request.host} \"val \\\"1\\\"\" `raw {no_brace}` {\n" ++
        "}\n";
    let it = CaddyIter.init(src);

    expect_eq((it.next() ?? unreachable).kind == CaddyTokenKind.newline, true);

    const t_matcher = it.next() ?? unreachable;
    expect_eq(t_matcher.kind == CaddyTokenKind.matcher, true);
    expect_eq(std.mem.eql(u8, t_matcher.lexeme, "@api"), true);

    const t_path = it.next() ?? unreachable;
    expect_eq(t_path.kind == CaddyTokenKind.word, true);
    expect_eq(std.mem.eql(u8, t_path.lexeme, "path"), true);

    const t_glob = it.next() ?? unreachable;
    expect_eq(t_glob.kind == CaddyTokenKind.word, true);
    expect_eq(std.mem.eql(u8, t_glob.lexeme, "/v1/*"), true);

    expect_eq((it.next() ?? unreachable).kind == CaddyTokenKind.newline, true);

    expect_eq(std.mem.eql(u8, (it.next() ?? unreachable).lexeme, "header"), true);
    expect_eq(std.mem.eql(u8, (it.next() ?? unreachable).lexeme, "X-Host"), true);

    const t_ph = it.next() ?? unreachable;
    expect_eq(t_ph.kind == CaddyTokenKind.word, true);
    expect_eq(std.mem.eql(u8, t_ph.lexeme, "{http.request.host}"), true);

    const t_dq = it.next() ?? unreachable;
    expect_eq(t_dq.kind == CaddyTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t_dq.lexeme, "\"val \\\"1\\\"\""), true);

    const t_bt = it.next() ?? unreachable;
    expect_eq(t_bt.kind == CaddyTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t_bt.lexeme, "`raw {no_brace}`"), true);

    expect_eq((it.next() ?? unreachable).kind == CaddyTokenKind.lbrace, true);
    expect_eq((it.next() ?? unreachable).kind == CaddyTokenKind.newline, true);
    expect_eq((it.next() ?? unreachable).kind == CaddyTokenKind.rbrace, true);
    expect_eq((it.next() ?? unreachable).kind == CaddyTokenKind.newline, true);
    expect_eq(it.next() == null, true);
});

test("2. CaddySiteIter extracts global options block, snippet, and multi-address/port site blocks", () => {
    let it = CaddySiteIter.init(SAMPLE_CADDYFILE);

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.is_global, true);
    expect_eq(b0.is_snippet, false);
    expect_eq(b0.addresses.len, 0);
    expect_eq(std.mem.indexOf(u8, b0.body, "email ops@example.com") != null, true);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.is_global, false);
    expect_eq(b1.is_snippet, true);
    expect_eq(std.mem.eql(u8, b1.addresses, "(security_headers)"), true);
    expect_eq(std.mem.indexOf(u8, b1.body, "X-Frame-Options") != null, true);

    const b2 = it.next() ?? unreachable;
    expect_eq(b2.is_global, false);
    expect_eq(b2.is_snippet, false);
    expect_eq(std.mem.eql(u8, b2.addresses, "example.com, www.example.com"), true);
    expect_eq(std.mem.indexOf(u8, b2.body, "reverse_proxy @api localhost:8080") != null, true);

    const b3 = it.next() ?? unreachable;
    expect_eq(b3.is_global, false);
    expect_eq(b3.is_snippet, false);
    expect_eq(std.mem.eql(u8, b3.addresses, ":8080"), true);
    expect_eq(std.mem.indexOf(u8, b3.body, "respond") != null, true);

    expect_eq(it.next() == null, true);
});

test("3. CaddySiteIter handles placeholder-based site addresses and nested blocks cleanly", () => {
    const cfg =
        "{$SITE_DOMAIN:localhost} {\n" ++
        "    route /api/* {\n" ++
        "        uri strip_prefix /api\n" ++
        "        reverse_proxy {$BACKEND_ADDR}\n" ++
        "    }\n" ++
        "}\n";
    let it = CaddySiteIter.init(cfg);

    const site = it.next() ?? unreachable;
    expect_eq(site.is_global, false);
    expect_eq(site.is_snippet, false);
    expect_eq(std.mem.eql(u8, site.addresses, "{$SITE_DOMAIN:localhost}"), true);
    expect_eq(std.mem.indexOf(u8, site.body, "reverse_proxy {$BACKEND_ADDR}") != null, true);
    expect_eq(it.next() == null, true);
});

test("4. CaddyIter flags unterminated double-quoted and backtick strings as invalid", () => {
    let it1 = CaddyIter.init("respond \"unterminated");
    expect_eq((it1.next() ?? unreachable).kind == CaddyTokenKind.word, true);
    expect_eq((it1.next() ?? unreachable).kind == CaddyTokenKind.invalid, true);

    let it2 = CaddyIter.init("respond `unterminated raw");
    expect_eq((it2.next() ?? unreachable).kind == CaddyTokenKind.word, true);
    expect_eq((it2.next() ?? unreachable).kind == CaddyTokenKind.invalid, true);
});

test("5. validateCaddyfile checks string/comment-aware brace balance, quotes, and 64-depth limit", () => {
    expect_eq(validateCaddyfile(SAMPLE_CADDYFILE), true);
    expect_eq(validateCaddyfile("site.com {\n # {{{\n respond \"{{{\" `}}}`\n}\n"), true);

    // Unbalanced braces & unterminated strings
    expect_eq(validateCaddyfile("site.com {\n reverse_proxy a {\n}\n"), false);
    expect_eq(validateCaddyfile("site.com {\n}\n}\n"), false);
    expect_eq(validateCaddyfile("site.com {\n respond \"oops\n}\n"), false);
    expect_eq(validateCaddyfile("site.com {\n respond `oops\n}\n"), false);

    // Exact 64-level block nesting succeeds; 65-level nesting is rejected
    let ok64: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        ok64[i * 2] = '{';
        ok64[i * 2 + 1] = ' ';
        ok64[128 + i * 2] = '}';
        ok64[128 + i * 2 + 1] = ' ';
        i += 1;
    }
    expect_eq(validateCaddyfile(ok64[0..256]), true);

    let bad65: [260]u8 = undefined;
    let j: usize = 0;
    while (j < 65) {
        bad65[j * 2] = '{';
        bad65[j * 2 + 1] = ' ';
        bad65[130 + j * 2] = '}';
        bad65[130 + j * 2 + 1] = ' ';
        j += 1;
    }
    expect_eq(validateCaddyfile(bad65[0..260]), false);
});

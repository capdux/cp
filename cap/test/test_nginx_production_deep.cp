// Source: cp/cap/test/test_nginx_production_deep.cp (Production deep tests for cap:nginx)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NginxTokenKind,
    NginxToken,
    NginxIter,
    NginxDirective,
    NginxBlockIter,
    findNginxDirective,
    validateNginxConfig,
} from "cap:compiler";

const SAMPLE_CONF: []const u8 =
    "# Main nginx configuration\n" ++
    "worker_processes auto;\n" ++
    "events {\n" ++
    "    worker_connections 1024;\n" ++
    "}\n" ++
    "http {\n" ++
    "    upstream api_backend {\n" ++
    "        server 127.0.0.1:8080 weight=3;\n" ++
    "        server 127.0.0.1:8081;\n" ++
    "    }\n" ++
    "    server {\n" ++
    "        listen 443 ssl http2;\n" ++
    "        server_name api.example.com;\n" ++
    "        location ~ ^/v1/users/([0-9]+)$ {\n" ++
    "            proxy_pass http://api_backend;\n" ++
    "            proxy_set_header X-Forwarded-Proto $scheme;\n" ++
    "            add_header X-Msg \"hello \\\"nginx\\\" {not-block}\";\n" ++
    "        }\n" ++
    "    }\n" ++
    "}\n";

test("1. NginxIter lexes comments, quoted strings, $var/${var}, regex paths, and rejects unclosed quotes", () => {
    const src: []const u8 =
        "# header comment\n" ++
        "location ~ ^/api/([a-z]+)$ { set $tag ${host}_v1; return 200 'ok \\'yes\\''; }\n";
    let it = NginxIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == NginxTokenKind.word, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "location"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.kind == NginxTokenKind.word, true);
    expect_eq(std.mem.eql(u8, t1.lexeme, "~"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.kind == NginxTokenKind.word, true);
    expect_eq(std.mem.eql(u8, t2.lexeme, "^/api/([a-z]+)$"), true);

    const t3 = it.next() ?? unreachable;
    expect_eq(t3.kind == NginxTokenKind.lbrace, true);

    let saw_var = false;
    let saw_interp = false;
    let saw_str = false;
    let saw_rbrace = false;
    while (tok := it.next()) {
        if (tok.kind == NginxTokenKind.word && std.mem.eql(u8, tok.lexeme, "$tag")) {
            saw_var = true;
        }
        if (tok.kind == NginxTokenKind.word && std.mem.eql(u8, tok.lexeme, "${host}_v1")) {
            saw_interp = true;
        }
        if (tok.kind == NginxTokenKind.string && std.mem.eql(u8, tok.lexeme, "'ok \\'yes\\''")) {
            saw_str = true;
        }
        if (tok.kind == NginxTokenKind.rbrace) {
            saw_rbrace = true;
        }
    }
    expect_eq(saw_var, true);
    expect_eq(saw_interp, true);
    expect_eq(saw_str, true);
    expect_eq(saw_rbrace, true);

    // Unclosed double quote and single quote yield invalid token
    let bad_dq = NginxIter.init("add_header X \"unclosed");
    const _w0 = bad_dq.next() ?? unreachable;
    const _w1 = bad_dq.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, _w0.lexeme, "add_header"), true);
    expect_eq(std.mem.eql(u8, _w1.lexeme, "X"), true);
    const bad_t = bad_dq.next() ?? unreachable;
    expect_eq(bad_t.kind == NginxTokenKind.invalid, true);
    expect_eq(bad_dq.next() == null, true);
});

test("2. NginxBlockIter parses top-level directives and recursively traverses http/upstream/server/location", () => {
    let top = NginxBlockIter.init(SAMPLE_CONF);

    const d0 = top.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d0.name, "worker_processes"), true);
    expect_eq(std.mem.eql(u8, d0.args, "auto"), true);
    expect_eq(d0.is_block, false);
    expect_eq(d0.body == null, true);
    expect_eq(d0.depth, 0);

    const d1 = top.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d1.name, "events"), true);
    expect_eq(d1.is_block, true);
    expect_eq(std.mem.eql(u8, d1.body ?? "", "worker_connections 1024;"), true);

    const d2 = top.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d2.name, "http"), true);
    expect_eq(d2.is_block, true);
    expect_eq(top.next() == null, true);

    // Traverse inside `http` block (depth = 1)
    let http_it = NginxBlockIter.initWithDepth(d2.body ?? "", d2.depth + 1);
    const up = http_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, up.name, "upstream"), true);
    expect_eq(std.mem.eql(u8, up.args, "api_backend"), true);
    expect_eq(up.is_block, true);
    expect_eq(up.depth, 1);

    const srv = http_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, srv.name, "server"), true);
    expect_eq(srv.is_block, true);
    expect_eq(srv.depth, 1);
    expect_eq(http_it.next() == null, true);

    // Traverse inside `server` block (depth = 2) -> find `location`
    let srv_it = NginxBlockIter.initWithDepth(srv.body ?? "", srv.depth + 1);
    let found_loc = false;
    while (sd := srv_it.next()) {
        if (std.mem.eql(u8, sd.name, "location")) {
            found_loc = true;
            expect_eq(sd.is_block, true);
            expect_eq(sd.depth, 2);
            expect_eq(std.mem.eql(u8, sd.args, "~ ^/v1/users/([0-9]+)$"), true);
            const pp = findNginxDirective(sd.body ?? "", "proxy_pass") ?? "";
            expect_eq(std.mem.eql(u8, pp, "http://api_backend"), true);
        }
    }
    expect_eq(found_loc, true);
});

test("3. findNginxDirective finds first matching directive in current block scope without descending", () => {
    let top = NginxBlockIter.init(SAMPLE_CONF);
    * = top.next(); // worker_processes
    * = top.next(); // events
    const http_blk = top.next() ?? unreachable;
    const http_body = http_blk.body ?? "";

    // `upstream` is at http level; `listen` is nested inside `server`, so not at http level
    expect_eq(std.mem.eql(u8, findNginxDirective(http_body, "upstream") ?? "", "api_backend"), true);
    expect_eq(findNginxDirective(http_body, "listen") == null, true);

    let http_it = NginxBlockIter.init(http_body);
    const up = http_it.next() ?? unreachable;
    const srv = http_it.next() ?? unreachable;

    // First `server` inside upstream
    expect_eq(std.mem.eql(u8, findNginxDirective(up.body ?? "", "server") ?? "", "127.0.0.1:8080 weight=3"), true);
    // Directives inside server block
    expect_eq(std.mem.eql(u8, findNginxDirective(srv.body ?? "", "listen") ?? "", "443 ssl http2"), true);
    expect_eq(std.mem.eql(u8, findNginxDirective(srv.body ?? "", "server_name") ?? "", "api.example.com"), true);
    expect_eq(findNginxDirective(srv.body ?? "", "nonexistent") == null, true);
});

test("4. validateNginxConfig checks balanced braces, string/comment/$var awareness, and 64-depth limit", () => {
    expect_eq(validateNginxConfig(SAMPLE_CONF), true);
    expect_eq(validateNginxConfig("# {\nserver { return 200 \"}\"; set $a ${b}; }\n"), true);

    // Exact 64-level nesting -> valid
    let exact64: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        exact64[i] = '{';
        exact64[127 - i] = '}';
        i += 1;
    }
    expect_eq(validateNginxConfig(exact64[0..128]), true);

    // 65-level nesting -> exceeds 64-level limit -> false
    let over65: [130]u8 = undefined;
    let j: usize = 0;
    while (j < 65) {
        over65[j] = '{';
        over65[129 - j] = '}';
        j += 1;
    }
    expect_eq(validateNginxConfig(over65[0..130]), false);
});

test("5. Malformed config protection: unclosed braces/quotes, extra rbrace, truncated directive, empty input", () => {
    expect_eq(validateNginxConfig("server { listen 80;"), false);
    expect_eq(validateNginxConfig("server { listen 80; } }"), false);
    expect_eq(validateNginxConfig("server { add_header X \"unterminated; }"), false);
    expect_eq(validateNginxConfig("server { add_header X 'unterminated; }"), false);
    expect_eq(validateNginxConfig("server { set $x ${unclosed; }"), false);

    // Empty / comment-only config
    let it_empty = NginxBlockIter.init("   # only comment\n  ");
    expect_eq(it_empty.next() == null, true);

    // Truncated directive without ';' or '{'
    let it_trunc = NginxBlockIter.init("listen 8080");
    expect_eq(it_trunc.next() == null, true);

    // Unclosed block body
    let it_unclosed = NginxBlockIter.init("server { listen 80;");
    expect_eq(it_unclosed.next() == null, true);
});

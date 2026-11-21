import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EarlyHintRel,
    EarlyHintLink,
    EarlyHintIter,
    formatEarlyHintLink,
    formatEarlyHints103Block,
} from "cap:http";

test("1. EarlyHintIter parses multi-link headers with quoted params, URI commas, and flags", () => {
    const hdr =
        "</style.css?v=1,2>; rel=preload; as=style; crossorigin=\"anonymous\"; fetchpriority=high; nopush, "
        ++ "<https://cdn.example.com>; rel=\"preconnect\"; crossorigin, "
        ++ "</app.mjs>; rel=modulepreload, "
        ++ "</privacy>; rel=canonical; title=\"a, b; c\"";

    let it = EarlyHintIter.init(hdr);

    const l0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l0.uri, "/style.css?v=1,2"), true);
    expect_eq(l0.rel == EarlyHintRel.preload, true);
    expect_eq(l0.isPreloadable(), true);
    expect_eq(std.mem.eql(u8, l0.as_type ?? unreachable, "style"), true);
    expect_eq(std.mem.eql(u8, l0.crossorigin ?? unreachable, "anonymous"), true);
    expect_eq(std.mem.eql(u8, l0.fetchpriority ?? unreachable, "high"), true);
    expect_eq(l0.nopush, true);

    const l1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l1.uri, "https://cdn.example.com"), true);
    expect_eq(l1.rel == EarlyHintRel.preconnect, true);
    expect_eq(l1.isPreloadable(), true);
    expect_eq((l1.crossorigin ?? unreachable).len, usize(0));
    expect_eq(l1.nopush, false);

    const l2 = it.next() ?? unreachable;
    expect_eq(l2.rel == EarlyHintRel.modulepreload, true);
    expect_eq(l2.isPreloadable(), true);

    const l3 = it.next() ?? unreachable;
    expect_eq(l3.rel == EarlyHintRel.other, true);
    expect_eq(std.mem.eql(u8, l3.rel_raw, "canonical"), true);
    expect_eq(l3.isPreloadable(), false);

    expect_eq(it.next() == null, true);
});

test("2. formatEarlyHintLink round-trips with EarlyHintIter", () => {
    const link = EarlyHintLink{
        uri: "/fonts/inter.woff2",
        rel: EarlyHintRel.preload,
        rel_raw: "preload",
        as_type: "font",
        crossorigin: "anonymous",
        fetchpriority: "high",
        nopush: true,
    };

    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const formatted = formatEarlyHintLink(link, &buf) ?? unreachable;
    expect_eq(
        std.mem.eql(
            u8,
            formatted,
            "</fonts/inter.woff2>; rel=preload; as=font; crossorigin=anonymous; fetchpriority=high; nopush",
        ),
        true,
    );

    let it = EarlyHintIter.init(formatted);
    const parsed = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, parsed.uri, "/fonts/inter.woff2"), true);
    expect_eq(parsed.rel == EarlyHintRel.preload, true);
    expect_eq(std.mem.eql(u8, parsed.as_type ?? unreachable, "font"), true);
    expect_eq(std.mem.eql(u8, parsed.crossorigin ?? unreachable, "anonymous"), true);
    expect_eq(std.mem.eql(u8, parsed.fetchpriority ?? unreachable, "high"), true);
    expect_eq(parsed.nopush, true);
});

test("3. formatEarlyHints103Block builds complete HTTP/1.1 103 Early Hints block", () => {
    const l1 = EarlyHintLink{
        uri: "/main.css",
        rel: EarlyHintRel.preload,
        rel_raw: "",
        as_type: "style",
        crossorigin: null,
        fetchpriority: null,
        nopush: false,
    };
    const l2 = EarlyHintLink{
        uri: "https://fonts.example.com",
        rel: EarlyHintRel.dns_prefetch,
        rel_raw: "",
        as_type: null,
        crossorigin: "",
        fetchpriority: null,
        nopush: false,
    };
    const links: [2]EarlyHintLink = [l1, l2];

    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const block = formatEarlyHints103Block(links[0..2], &buf) ?? unreachable;
    const expected =
        "HTTP/1.1 103 Early Hints\r\n"
        ++ "Link: </main.css>; rel=preload; as=style\r\n"
        ++ "Link: <https://fonts.example.com>; rel=dns-prefetch; crossorigin\r\n"
        ++ "\r\n";
    expect_eq(std.mem.eql(u8, block, expected), true);
});

test("4. EarlyHintIter skips malformed entries (unclosed angle, missing rel, empty URI)", () => {
    const hdr =
        "</unclosed; rel=preload, "
        ++ "<>; rel=preload, "
        ++ "</no-rel>; as=style, "
        ++ "</bad-quote>; rel=\"unterminated, "
        ++ "</good.js>; rel=preload; as=script";

    let it = EarlyHintIter.init(hdr);
    const ok = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, ok.uri, "/good.js"), true);
    expect_eq(ok.rel == EarlyHintRel.preload, true);
    expect_eq(std.mem.eql(u8, ok.as_type ?? unreachable, "script"), true);
    expect_eq(it.next() == null, true);
});

test("5. Formatters reject empty link slices, CRLF injection, and tiny buffers", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const empty: [0]EarlyHintLink = [];
    expect_eq(formatEarlyHints103Block(empty[0..0], &buf) == null, true);

    const crlf_link = EarlyHintLink{
        uri: "/a\r\nEvil: 1",
        rel: EarlyHintRel.preload,
        rel_raw: "preload",
        as_type: null,
        crossorigin: null,
        fetchpriority: null,
        nopush: false,
    };
    expect_eq(formatEarlyHintLink(crlf_link, &buf) == null, true);

    const good_link = EarlyHintLink{
        uri: "/app.css",
        rel: EarlyHintRel.preload,
        rel_raw: "preload",
        as_type: "style",
        crossorigin: null,
        fetchpriority: null,
        nopush: false,
    };
    let tiny: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(formatEarlyHintLink(good_link, &tiny) == null, true);

    const one: [1]EarlyHintLink = [good_link];
    let tiny32: [32]u8 = std.mem.zeroes([32]u8);
    expect_eq(formatEarlyHints103Block(one[0..1], &tiny32) == null, true);
});

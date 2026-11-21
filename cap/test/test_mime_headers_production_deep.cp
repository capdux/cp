import std from "std";
import { test, expect_eq } from "cap:test";
import {
    splitHeadersAndBody,
    MimeHeaderIter,
    findMimeHeader,
    countMimeHeader,
} from "cap:mime";

test("1. splitHeadersAndBody handles CRLF, LF, leading blank line, and header-only messages", () => {
    const crlf = splitHeadersAndBody("From: a@b\r\nTo: c@d\r\n\r\nHello\r\nBody");
    expect_eq(std.mem.eql(u8, crlf.headers_raw, "From: a@b\r\nTo: c@d"), true);
    expect_eq(std.mem.eql(u8, crlf.body, "Hello\r\nBody"), true);

    const lf = splitHeadersAndBody("From: a@b\n\nPlain body");
    expect_eq(std.mem.eql(u8, lf.headers_raw, "From: a@b"), true);
    expect_eq(std.mem.eql(u8, lf.body, "Plain body"), true);

    // 首行即空行 -> headers_raw 为空
    const no_hdr = splitHeadersAndBody("\r\nOnly body");
    expect_eq(no_hdr.headers_raw.len, usize(0));
    expect_eq(std.mem.eql(u8, no_hdr.body, "Only body"), true);

    const hdr_only = splitHeadersAndBody("X-Test: 1");
    expect_eq(std.mem.eql(u8, hdr_only.headers_raw, "X-Test: 1"), true);
    expect_eq(hdr_only.body.len, usize(0));
});

test("2. MimeHeaderIter unfolds multi-line CRLF and LF folded headers per RFC 5322", () => {
    const raw =
        "Subject: Weekly\r\n"
        ++ "   Status\r\n"
        ++ "\tReport\r\n"
        ++ "Content-Type: text/plain;\n"
        ++ " charset=utf-8\n";

    let it = MimeHeaderIter.init(raw);
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const h0 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, h0.name, "Subject"), true);
    expect_eq(std.mem.eql(u8, h0.value, "Weekly Status Report"), true);

    const h1 = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, h1.name, "Content-Type"), true);
    expect_eq(std.mem.eql(u8, h1.value, "text/plain; charset=utf-8"), true);

    expect_eq(it.next(&buf) == null, true);
    expect_eq(it.hasError(), false);
});

test("3. findMimeHeader case-insensitive lookup and countMimeHeader for repeated headers", () => {
    const raw =
        "Received: from mx1\r\n"
        ++ " by mx2\r\n"
        ++ "Received: from mx0\r\n"
        ++ "X-Trace-Id: abc-123\r\n"
        ++ "RECEIVED: from client\r\n";

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(countMimeHeader(raw, "received"), usize(3));
    expect_eq(countMimeHeader(raw, "X-Trace-Id"), usize(1));
    expect_eq(countMimeHeader(raw, "Missing"), usize(0));

    const first_rcv = findMimeHeader(raw, "RECEIVED", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, first_rcv, "from mx1 by mx2"), true);

    const trace = findMimeHeader(raw, "x-trace-id", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, trace, "abc-123"), true);
    expect_eq(findMimeHeader(raw, "missing", &buf) == null, true);
});

test("4. skips malformed header lines with spaces in field name or missing colon", () => {
    const raw =
        "Bad Name: ignored\r\n"
        ++ "  folded ignored\r\n"
        ++ "NoColonLine\r\n"
        ++ "Valid-Header: ok\r\n";

    let it = MimeHeaderIter.init(raw);
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const h = it.next(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, h.name, "Valid-Header"), true);
    expect_eq(std.mem.eql(u8, h.value, "ok"), true);
    expect_eq(it.next(&buf) == null, true);
});

test("5. buffer-too-small guard sets error flag and returns null", () => {
    const raw = "Subject: very long unfolded header value\r\n";
    let it = MimeHeaderIter.init(raw);
    let tiny: [5]u8 = std.mem.zeroes([5]u8);

    expect_eq(it.next(&tiny) == null, true);
    expect_eq(it.hasError(), true);
    expect_eq(findMimeHeader(raw, "subject", &tiny) == null, true);
});

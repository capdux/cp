// Source: cp/cap/test/test_gemini_production_deep.cp (Production deep tests for cap:gemini)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GEMINI_TCP_PORT,
    GEMINI_MAX_URL_LEN,
    GEMINI_MAX_META_LEN,
    GEMINI_DEFAULT_MIME,
    STATUS_INPUT,
    STATUS_SENSITIVE_INPUT,
    STATUS_SUCCESS,
    STATUS_REDIRECT_TEMP,
    STATUS_REDIRECT_PERM,
    STATUS_TEMP_FAILURE,
    STATUS_SERVER_UNAVAILABLE,
    STATUS_CGI_ERROR,
    STATUS_PROXY_ERROR,
    STATUS_SLOW_DOWN,
    STATUS_PERM_FAILURE,
    STATUS_NOT_FOUND,
    STATUS_GONE,
    STATUS_PROXY_REFUSED,
    STATUS_BAD_REQUEST,
    STATUS_CLIENT_CERT_REQUIRED,
    STATUS_CERT_NOT_AUTHORIZED,
    STATUS_CERT_NOT_VALID,
    GeminiStatusClass,
    GemtextLineKind,
    GeminiRequest,
    parseGeminiRequest,
    encodeGeminiRequest,
    GeminiResponseHeader,
    parseGeminiResponseHeader,
    encodeGeminiResponseHeader,
    GemtextLine,
    GemtextIter,
} from "cap:net";

test("1. Protocol constants, request encode/parse roundtrip, and response header roundtrip", () => {
    expect_eq(GEMINI_TCP_PORT, 1965 as u16);
    expect_eq(GEMINI_MAX_URL_LEN, 1024 as usize);
    expect_eq(GEMINI_MAX_META_LEN, 1024 as usize);
    expect_eq(STATUS_INPUT, 10 as u8);
    expect_eq(STATUS_SENSITIVE_INPUT, 11 as u8);
    expect_eq(STATUS_SUCCESS, 20 as u8);
    expect_eq(STATUS_REDIRECT_TEMP, 30 as u8);
    expect_eq(STATUS_REDIRECT_PERM, 31 as u8);
    expect_eq(STATUS_TEMP_FAILURE, 40 as u8);
    expect_eq(STATUS_SERVER_UNAVAILABLE, 41 as u8);
    expect_eq(STATUS_CGI_ERROR, 42 as u8);
    expect_eq(STATUS_PROXY_ERROR, 43 as u8);
    expect_eq(STATUS_SLOW_DOWN, 44 as u8);
    expect_eq(STATUS_PERM_FAILURE, 50 as u8);
    expect_eq(STATUS_NOT_FOUND, 51 as u8);
    expect_eq(STATUS_GONE, 52 as u8);
    expect_eq(STATUS_PROXY_REFUSED, 53 as u8);
    expect_eq(STATUS_BAD_REQUEST, 59 as u8);
    expect_eq(STATUS_CLIENT_CERT_REQUIRED, 60 as u8);
    expect_eq(STATUS_CERT_NOT_AUTHORIZED, 61 as u8);
    expect_eq(STATUS_CERT_NOT_VALID, 62 as u8);

    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const url = "gemini://geminiprotocol.net/docs/specification.gmi?lang=en";
    const req_len = encodeGeminiRequest(url, buf[0..]) ?? unreachable;
    expect_eq(req_len, url.len + 2);
    expect_eq(buf[req_len - 2], '\r' as u8);
    expect_eq(buf[req_len - 1], '\n' as u8);

    const req = parseGeminiRequest(buf[0..req_len]) ?? unreachable;
    expect_eq(std.mem.eql(u8, req.url, url), true);

    // Response header roundtrip
    const hdr_len = encodeGeminiResponseHeader(STATUS_SUCCESS, "text/gemini; lang=en", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..hdr_len], "20 text/gemini; lang=en\r\n"), true);

    const hdr = parseGeminiResponseHeader(buf[0..hdr_len]) ?? unreachable;
    expect_eq(hdr.status, 20 as u8);
    expect_eq(std.mem.eql(u8, hdr.meta, "text/gemini; lang=en"), true);
    expect_eq(std.mem.eql(u8, hdr.mimeType(), "text/gemini; lang=en"), true);

    // Empty meta on 20 defaults to "text/gemini; charset=utf-8"
    const def_hdr = parseGeminiResponseHeader("20 \r\n") ?? unreachable;
    expect_eq(def_hdr.isSuccess(), true);
    expect_eq(std.mem.eql(u8, def_hdr.mimeType(), GEMINI_DEFAULT_MIME), true);
});

test("2. All 6 Gemini status classes classification and predicates", () => {
    const h1 = parseGeminiResponseHeader("11 Enter password\r\n") ?? unreachable;
    expect_eq(h1.statusClass(), GeminiStatusClass.input);
    expect_eq(h1.isInput(), true);
    expect_eq(h1.isSuccess(), false);
    expect_eq(h1.isRedirect(), false);
    expect_eq(h1.isError(), false);
    expect_eq(h1.mimeType().len, 0 as usize);

    const h2 = parseGeminiResponseHeader("20 text/plain\r\n") ?? unreachable;
    expect_eq(h2.statusClass(), GeminiStatusClass.success);
    expect_eq(h2.isSuccess(), true);
    expect_eq(h2.isError(), false);

    const h3 = parseGeminiResponseHeader("31 gemini://new.example.com/\r\n") ?? unreachable;
    expect_eq(h3.statusClass(), GeminiStatusClass.redirect);
    expect_eq(h3.isRedirect(), true);
    expect_eq(h3.isError(), false);
    expect_eq(std.mem.eql(u8, h3.meta, "gemini://new.example.com/"), true);

    const h4 = parseGeminiResponseHeader("44 30\r\n") ?? unreachable;
    expect_eq(h4.statusClass(), GeminiStatusClass.temp_failure);
    expect_eq(h4.isError(), true);

    const h5 = parseGeminiResponseHeader("51 Not found\r\n") ?? unreachable;
    expect_eq(h5.statusClass(), GeminiStatusClass.perm_failure);
    expect_eq(h5.isError(), true);

    const h6 = parseGeminiResponseHeader("60 Client certificate required\r\n") ?? unreachable;
    expect_eq(h6.statusClass(), GeminiStatusClass.client_cert);
    expect_eq(h6.isError(), true);
});

test("3. GemtextIter parses all 9 line kinds accurately (headings, links, list, quote, text)", () => {
    const doc =
        "# Main Title\r\n" ++
        "## Section Heading\n" ++
        "###   Subsection\r\n" ++
        "Plain paragraph text.\n" ++
        "*bold* is ordinary text without space\n" ++
        "* First bullet\n" ++
        "> Quoted wisdom\r\n" ++
        ">Tight quote\n" ++
        "=> gemini://example.org/   Example Site\r\n" ++
        "=>\t/relative/path.gmi\n" ++
        "";

    let it = GemtextIter.init(doc);

    const l1 = it.next() ?? unreachable;
    expect_eq(l1.kind, GemtextLineKind.heading1);
    expect_eq(std.mem.eql(u8, l1.text, "Main Title"), true);

    const l2 = it.next() ?? unreachable;
    expect_eq(l2.kind, GemtextLineKind.heading2);
    expect_eq(std.mem.eql(u8, l2.text, "Section Heading"), true);

    const l3 = it.next() ?? unreachable;
    expect_eq(l3.kind, GemtextLineKind.heading3);
    expect_eq(std.mem.eql(u8, l3.text, "Subsection"), true);

    const l4 = it.next() ?? unreachable;
    expect_eq(l4.kind, GemtextLineKind.text);
    expect_eq(std.mem.eql(u8, l4.text, "Plain paragraph text."), true);

    const l5 = it.next() ?? unreachable;
    expect_eq(l5.kind, GemtextLineKind.text);
    expect_eq(std.mem.eql(u8, l5.text, "*bold* is ordinary text without space"), true);

    const l6 = it.next() ?? unreachable;
    expect_eq(l6.kind, GemtextLineKind.list_item);
    expect_eq(std.mem.eql(u8, l6.text, "First bullet"), true);

    const l7 = it.next() ?? unreachable;
    expect_eq(l7.kind, GemtextLineKind.quote);
    expect_eq(std.mem.eql(u8, l7.text, "Quoted wisdom"), true);

    const l8 = it.next() ?? unreachable;
    expect_eq(l8.kind, GemtextLineKind.quote);
    expect_eq(std.mem.eql(u8, l8.text, "Tight quote"), true);

    const l9 = it.next() ?? unreachable;
    expect_eq(l9.kind, GemtextLineKind.link);
    expect_eq(std.mem.eql(u8, l9.url, "gemini://example.org/"), true);
    expect_eq(std.mem.eql(u8, l9.text, "Example Site"), true);

    const l10 = it.next() ?? unreachable;
    expect_eq(l10.kind, GemtextLineKind.link);
    expect_eq(std.mem.eql(u8, l10.url, "/relative/path.gmi"), true);
    expect_eq(l10.text.len, 0 as usize);

    expect_eq(it.next() == null, true);
});

test("4. GemtextIter preformatted block toggle prevents misparsing headings/links/lists inside ``` blocks", () => {
    const doc =
        "```cp example\r\n" ++
        "# Not a heading\r\n" ++
        "=> gemini://not-a-link/\n" ++
        "* Not a list item\n" ++
        "> Not a quote\n" ++
        "```ignored closing alt\n" ++
        "# Real Heading After Block";

    let it = GemtextIter.init(doc);
    expect_eq(it.inPreformat(), false);

    const open = it.next() ?? unreachable;
    expect_eq(open.kind, GemtextLineKind.preformat_toggle);
    expect_eq(std.mem.eql(u8, open.alt, "cp example"), true);
    expect_eq(it.inPreformat(), true);

    const p1 = it.next() ?? unreachable;
    expect_eq(p1.kind, GemtextLineKind.preformat_text);
    expect_eq(std.mem.eql(u8, p1.text, "# Not a heading"), true);

    const p2 = it.next() ?? unreachable;
    expect_eq(p2.kind, GemtextLineKind.preformat_text);
    expect_eq(std.mem.eql(u8, p2.text, "=> gemini://not-a-link/"), true);

    const p3 = it.next() ?? unreachable;
    expect_eq(p3.kind, GemtextLineKind.preformat_text);
    expect_eq(std.mem.eql(u8, p3.text, "* Not a list item"), true);

    const p4 = it.next() ?? unreachable;
    expect_eq(p4.kind, GemtextLineKind.preformat_text);
    expect_eq(std.mem.eql(u8, p4.text, "> Not a quote"), true);

    const close = it.next() ?? unreachable;
    expect_eq(close.kind, GemtextLineKind.preformat_toggle);
    expect_eq(close.alt.len, 0 as usize);
    expect_eq(it.inPreformat(), false);

    const after = it.next() ?? unreachable;
    expect_eq(after.kind, GemtextLineKind.heading1);
    expect_eq(std.mem.eql(u8, after.text, "Real Heading After Block"), true);

    expect_eq(it.next() == null, true);
});

test("5. Malformed requests, URL > 1024B, fragments '#', control chars, and missing CRLF rejection", () => {
    // Missing CRLF
    expect_eq(parseGeminiRequest("gemini://example.com/") == null, true);
    expect_eq(parseGeminiRequest("gemini://example.com/\n") == null, true);
    // Missing scheme "://"
    expect_eq(parseGeminiRequest("/relative/path\r\n") == null, true);
    expect_eq(parseGeminiRequest("://no-scheme\r\n") == null, true);
    expect_eq(parseGeminiRequest("gemini://\r\n") == null, true);
    // Fragment '#' forbidden in Gemini requests
    expect_eq(parseGeminiRequest("gemini://example.com/page#section\r\n") == null, true);
    // Space or control chars in URL
    expect_eq(parseGeminiRequest("gemini://example.com/has space\r\n") == null, true);
    expect_eq(parseGeminiRequest("gemini://example.com/a\tb\r\n") == null, true);

    // Boundary: exact 1024-byte URL succeeds, 1025-byte URL rejected
    let url_1024: [1024]u8 = undefined;
    let url_1025: [1025]u8 = undefined;
    const prefix = "gemini://a.co/";
    let i: usize = 0;
    while (i < 1024) {
        const ch = if (i < prefix.len) prefix[i] else ('x' as u8);
        url_1024[i] = ch;
        url_1025[i] = ch;
        i += 1;
    }
    url_1025[1024] = 'x';

    let wire: [1030]u8 = std.mem.zeroes([1030]u8);
    const enc_ok = encodeGeminiRequest(url_1024[0..], wire[0..]) ?? unreachable;
    expect_eq(enc_ok, 1026 as usize);
    const parsed_ok = parseGeminiRequest(wire[0..enc_ok]) ?? unreachable;
    expect_eq(parsed_ok.url.len, 1024 as usize);

    // 1025-byte URL must be rejected by both encoder and parser
    expect_eq(encodeGeminiRequest(url_1025[0..], wire[0..]) == null, true);
    let too_long_wire: [1027]u8 = undefined;
    let j: usize = 0;
    while (j < 1025) {
        too_long_wire[j] = url_1025[j];
        j += 1;
    }
    too_long_wire[1025] = '\r';
    too_long_wire[1026] = '\n';
    expect_eq(parseGeminiRequest(too_long_wire[0..]) == null, true);
});

test("6. Malformed response headers, invalid status codes, missing space, and meta > 1024B rejection", () => {
    // Out-of-range status codes (< 10 or > 69)
    expect_eq(parseGeminiResponseHeader("09 bad\r\n") == null, true);
    expect_eq(parseGeminiResponseHeader("70 bad\r\n") == null, true);
    expect_eq(parseGeminiResponseHeader("99 bad\r\n") == null, true);
    expect_eq(parseGeminiResponseHeader("2a bad\r\n") == null, true);

    // Missing space separator at index 2 (tab or text directly after status)
    expect_eq(parseGeminiResponseHeader("20\ttext/gemini\r\n") == null, true);
    expect_eq(parseGeminiResponseHeader("20text/gemini\r\n") == null, true);
    expect_eq(parseGeminiResponseHeader("20\r\n") == null, true);

    // Missing CRLF or embedded CR/LF in meta
    expect_eq(parseGeminiResponseHeader("20 text/gemini\n") == null, true);
    expect_eq(parseGeminiResponseHeader("20 text/gemini") == null, true);
    expect_eq(parseGeminiResponseHeader("20 line1\r\nline2\r\n") == null, true);

    // Meta > 1024 bytes rejected; exact 1024 bytes accepted
    let meta_1024: [1024]u8 = undefined;
    let meta_1025: [1025]u8 = undefined;
    let i: usize = 0;
    while (i < 1024) {
        meta_1024[i] = 'm';
        meta_1025[i] = 'm';
        i += 1;
    }
    meta_1025[1024] = 'm';

    let out: [1040]u8 = std.mem.zeroes([1040]u8);
    const n_ok = encodeGeminiResponseHeader(STATUS_SUCCESS, meta_1024[0..], out[0..]) ?? unreachable;
    expect_eq(n_ok, 1029 as usize);
    const h_ok = parseGeminiResponseHeader(out[0..n_ok]) ?? unreachable;
    expect_eq(h_ok.meta.len, 1024 as usize);

    expect_eq(encodeGeminiResponseHeader(STATUS_SUCCESS, meta_1025[0..], out[0..]) == null, true);
    expect_eq(encodeGeminiResponseHeader(70, "bad", out[0..]) == null, true);
    expect_eq(encodeGeminiResponseHeader(9, "bad", out[0..]) == null, true);
    expect_eq(encodeGeminiResponseHeader(20, "bad\r\nmeta", out[0..]) == null, true);

    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(encodeGeminiResponseHeader(20, "", tiny[0..]) == null, true);
});

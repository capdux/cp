// cap/test/test_http_web_primitives_deep.cp
// Deep Verification Suite for cap:http Web Primitives:
// 1. WHATWG SSE Incremental Streaming Parser & AI/LLM Encoder
// 2. RFC 7578 Binary-Safe Multipart FormData Parser & Builder
// 3. RFC 6265 Set-Cookie Codec & HttpCookieJar
// 4. Malformed Input & Boundary/DoS Protection
// 5. cap:http Aggregate Import & Zero Regression Compatibility

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    // New SSE primitives
    SseEvent,
    SseParser,
    SseEncoder,
    // New Multipart primitives
    MultipartPart,
    MultipartBoundary,
    MultipartBuilder,
    MultipartParser,
    // New RFC 6265 Cookie primitives
    SameSite,
    HttpCookie,
    CookieCodec,
    CookiePair,
    HttpCookieJar,
    // Existing cap:http primitives (zero-regression check)
    Method,
    Headers,
    CookieSameSite,
    CookieEntry,
    CookieMap,
    FormData,
    HTTPStatus,
    ETag,
    MimeType,
} from "cap:http";

fn testSseStreamingAndAiRoundtrip(): !void {
    const alloc = std.heap.page_allocator;

    let parser = SseParser.init(alloc);
    defer parser.deinit();

    // Feed BOM + multi-line data + CRLF/LF/CR split across arbitrary chunk boundaries
    const c1 = "\xEF\xBB";
    const c2 = "\xBF: keepalive comment\r";
    const c3 = "\nid: evt-101\r\nevent: content_block_delta\r\nretry: 1500\r\ndata: {\"delta\":\"Hel";
    const c4 = "lo\"}\rdata: {\"delta\":\" CP\"}\r\n\r\n";
    const c5 = "data: {\"choices\":[{\"delta\":{\"content\":\"!\"}}]}\n\n" +
        "data: [DONE]\n\n";

    const evs1 = try parser.feed(c1);
    defer parser.freeEvents(evs1);
    expect_eq(evs1.len, 0);

    const evs2 = try parser.feed(c2);
    defer parser.freeEvents(evs2);
    expect_eq(evs2.len, 0);

    const evs3 = try parser.feed(c3);
    defer parser.freeEvents(evs3);
    expect_eq(evs3.len, 0);

    const evs4 = try parser.feed(c4);
    defer parser.freeEvents(evs4);
    expect_eq(evs4.len, 1);
    expect_eq(std.mem.eql(u8, evs4[0].event_type, "content_block_delta"), true);
    expect_eq(std.mem.eql(u8, evs4[0].id, "evt-101"), true);
    expect_eq(evs4[0].retry_ms.?, 1500);
    expect_eq(std.mem.eql(u8, evs4[0].data, "{\"delta\":\"Hello\"}\n{\"delta\":\" CP\"}"), true);
    expect_eq(evs4[0].isDone(), false);
    expect_eq(std.mem.eql(u8, parser.lastEventId(), "evt-101"), true);

    const evs5 = try parser.feed(c5);
    defer parser.freeEvents(evs5);
    expect_eq(evs5.len, 2);
    // Default event_type is "message" and last_event_id persists
    expect_eq(std.mem.eql(u8, evs5[0].event_type, "message"), true);
    expect_eq(std.mem.eql(u8, evs5[0].id, "evt-101"), true);
    expect_eq(evs5[0].isDone(), false);
    // OpenAI [DONE] sentinel
    expect_eq(std.mem.eql(u8, evs5[1].data, "[DONE]"), true);
    expect_eq(evs5[1].isDone(), true);

    // SseEncoder.formatEvent & formatComment roundtrip
    const orig = SseEvent{
        event_type: "token",
        data: "line1\nline2",
        id: "msg-42",
        retry_ms: 3000,
    };
    const wire = try SseEncoder.formatEvent(alloc, orig);
    defer alloc.free(wire);
    const comment_wire = try SseEncoder.formatComment(alloc, "ping");
    defer alloc.free(comment_wire);
    expect_eq(std.mem.eql(u8, comment_wire, ": ping\n"), true);

    let rt_parser = SseParser.init(alloc);
    defer rt_parser.deinit();
    const rt_c = try rt_parser.feed(comment_wire);
    defer rt_parser.freeEvents(rt_c);
    expect_eq(rt_c.len, 0);

    const rt_evs = try rt_parser.feed(wire);
    defer rt_parser.freeEvents(rt_evs);
    expect_eq(rt_evs.len, 1);
    expect_eq(std.mem.eql(u8, rt_evs[0].event_type, "token"), true);
    expect_eq(std.mem.eql(u8, rt_evs[0].id, "msg-42"), true);
    expect_eq(rt_evs[0].retry_ms.?, 3000);
    expect_eq(std.mem.eql(u8, rt_evs[0].data, "line1\nline2"), true);
}

fn testMultipartRoundtripAndBinarySafety(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Content-Type boundary extraction
    const b1 = MultipartBoundary.extractFromContentType("multipart/form-data; boundary=----CPBound99");
    expect_eq(std.mem.eql(u8, b1.?, "----CPBound99"), true);
    const b2 = MultipartBoundary.extractFromContentType("MULTIPART/FORM-DATA; charset=utf-8; BOUNDARY=\"quoted-bnd-123\"");
    expect_eq(std.mem.eql(u8, b2.?, "quoted-bnd-123"), true);
    expect_eq(MultipartBoundary.extractFromContentType("application/json; boundary=ignored") == null, true);

    // 2. Build multipart payload with text field + binary file containing NUL, CRLF, and boundary prefix collision
    const custom_bnd = "CPBound77";
    let builder = try MultipartBuilder.initWithBoundary(alloc, custom_bnd);
    defer builder.deinit();

    expect_eq(std.mem.eql(u8, builder.contentTypeHeader(), "multipart/form-data; boundary=CPBound77"), true);
    try builder.addField("model", "gpt-5");
    try builder.addField("prompt", "Hello\r\nWorld");

    const bin_payload = "\x89PNG\r\n\x1A\n\x00\x01\x02\r\n--CPBound77_not_a_real_boundary\r\n\xFF\xFE\x00";
    try builder.addFile("attachment", "image.png", "image/png", bin_payload);

    const built = try builder.build();
    defer alloc.free(built);

    // 3. Wrap built payload with preamble & epilogue to test skipping
    const preamble = "This is a MIME preamble that must be ignored.\r\n";
    const epilogue = "Trailing epilogue bytes ignored.\r\n";
    const full_len = preamble.len + built.len + epilogue.len;
    let wrapped = try alloc.alloc(u8, full_len);
    defer alloc.free(wrapped);
    @memcpy(wrapped[0..preamble.len], preamble);
    @memcpy(wrapped[preamble.len .. preamble.len + built.len], built);
    @memcpy(wrapped[preamble.len + built.len .. full_len], epilogue);

    const extracted_bnd = MultipartBoundary.extractFromContentType(builder.contentTypeHeader()).?;
    const parts = try MultipartParser.parse(alloc, extracted_bnd, wrapped);
    defer MultipartParser.freeParts(alloc, parts);

    expect_eq(parts.len, 3);
    expect_eq(std.mem.eql(u8, parts[0].name, "model"), true);
    expect_eq(parts[0].filename == null, true);
    expect_eq(std.mem.eql(u8, parts[0].body, "gpt-5"), true);

    expect_eq(std.mem.eql(u8, parts[1].name, "prompt"), true);
    expect_eq(std.mem.eql(u8, parts[1].body, "Hello\r\nWorld"), true);

    expect_eq(std.mem.eql(u8, parts[2].name, "attachment"), true);
    expect_eq(std.mem.eql(u8, parts[2].filename.?, "image.png"), true);
    expect_eq(std.mem.eql(u8, parts[2].content_type.?, "image/png"), true);
    expect_eq(parts[2].body.len, bin_payload.len);
    expect_eq(std.mem.eql(u8, parts[2].body, bin_payload), true);
}

fn testCookieCodecAndCookieJar(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Set-Cookie parse + format roundtrip
    const sc_hdr = "session=tok_abc123; Domain=.example.com; Path=/api; Expires=Wed, 21 Oct 2026 07:28:00 GMT; Max-Age=3600; Secure; HttpOnly; SameSite=Strict";
    const parsed = try CookieCodec.parseSetCookie(sc_hdr);
    expect_eq(std.mem.eql(u8, parsed.name, "session"), true);
    expect_eq(std.mem.eql(u8, parsed.value, "tok_abc123"), true);
    expect_eq(std.mem.eql(u8, parsed.domain, "example.com"), true);
    expect_eq(std.mem.eql(u8, parsed.path, "/api"), true);
    expect_eq(parsed.max_age.?, 3600);
    expect_eq(parsed.expires_epoch_s != null, true);
    expect_eq(parsed.secure, true);
    expect_eq(parsed.http_only, true);
    expect_eq(parsed.same_site == SameSite.Strict, true);
    expect_eq(parsed.host_only, false);

    const formatted = try CookieCodec.formatSetCookie(alloc, parsed);
    defer alloc.free(formatted);
    const reparsed = try CookieCodec.parseSetCookie(formatted);
    expect_eq(std.mem.eql(u8, reparsed.name, "session"), true);
    expect_eq(std.mem.eql(u8, reparsed.value, "tok_abc123"), true);
    expect_eq(std.mem.eql(u8, reparsed.domain, "example.com"), true);
    expect_eq(reparsed.expires_epoch_s.?, parsed.expires_epoch_s.?);
    expect_eq(reparsed.same_site == SameSite.Strict, true);

    // 2. parseCookieHeader
    const pairs = try CookieCodec.parseCookieHeader(alloc, "a=1; b=\"two\"; c=3");
    defer alloc.free(pairs);
    expect_eq(pairs.len, 3);
    expect_eq(std.mem.eql(u8, pairs[0].name, "a"), true);
    expect_eq(std.mem.eql(u8, pairs[1].value, "two"), true);

    // 3. HttpCookieJar: domain suffix, path prefix, Secure, expiry, overwrite & Max-Age<=0 delete
    let jar = HttpCookieJar.init(alloc);
    defer jar.deinit();

    const now: i64 = 1700000000;
    try jar.setFromHeader("api.example.com", "root_tok=v1; Domain=example.com; Path=/; Max-Age=600", now);
    try jar.setFromHeader("api.example.com", "sec_tok=v2; Domain=example.com; Path=/api/v1; Secure; Max-Age=600", now);
    try jar.setFromHeader("api.example.com", "host_only=v3; Path=/api", now);
    expect_eq(jar.count(), 3);

    // HTTPS request to api.example.com/api/v1/users -> matches all 3, longer paths first
    const hdr_https = try jar.cookieHeaderFor(alloc, "api.example.com", "/api/v1/users", true, now + 10);
    defer if (hdr_https.len > 0) alloc.free(hdr_https);
    expect_eq(std.mem.eql(u8, hdr_https, "sec_tok=v2; host_only=v3; root_tok=v1"), true);

    // HTTP (non-HTTPS) request omits Secure cookie; /apple does NOT match /api
    const hdr_http_apple = try jar.cookieHeaderFor(alloc, "api.example.com", "/apple", false, now + 10);
    defer if (hdr_http_apple.len > 0) alloc.free(hdr_http_apple);
    expect_eq(std.mem.eql(u8, hdr_http_apple, "root_tok=v1"), true);

    // Domain boundary: "badexample.com" must NOT match "example.com"
    const hdr_evil = try jar.cookieHeaderFor(alloc, "badexample.com", "/api/v1", true, now + 10);
    defer if (hdr_evil.len > 0) alloc.free(hdr_evil);
    expect_eq(hdr_evil.len, 0);

    // Subdomain web.example.com matches Domain=example.com, NOT host_only for api.example.com
    const hdr_web = try jar.cookieHeaderFor(alloc, "web.example.com", "/api/v1", true, now + 10);
    defer if (hdr_web.len > 0) alloc.free(hdr_web);
    expect_eq(std.mem.eql(u8, hdr_web, "sec_tok=v2; root_tok=v1"), true);

    // Overwrite & Max-Age=0 deletion
    try jar.setFromHeader("api.example.com", "root_tok=v1_updated; Domain=example.com; Path=/; Max-Age=600", now + 20);
    try jar.setFromHeader("api.example.com", "sec_tok=deleted; Domain=example.com; Path=/api/v1; Max-Age=0", now + 20);
    expect_eq(jar.count(), 2);

    const hdr_after = try jar.cookieHeaderFor(alloc, "api.example.com", "/api/v1", true, now + 30);
    defer if (hdr_after.len > 0) alloc.free(hdr_after);
    expect_eq(std.mem.eql(u8, hdr_after, "host_only=v3; root_tok=v1_updated"), true);

    // Expiry check: after now + 600, root_tok expires (host_only is session cookie -> still present)
    const hdr_expired = try jar.cookieHeaderFor(alloc, "api.example.com", "/api", true, now + 700);
    defer if (hdr_expired.len > 0) alloc.free(hdr_expired);
    expect_eq(std.mem.eql(u8, hdr_expired, "host_only=v3"), true);
}

fn testMalformedAndBoundaryProtection(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Empty / oversized / invalid multipart boundary & truncated body
    expect_eq(MultipartBoundary.extractFromContentType("multipart/form-data; boundary=") == null, true);
    expect_eq(MultipartBoundary.extractFromContentType("multipart/form-data; boundary=\"\"") == null, true);

    let err_empty_bnd = false;
    * = MultipartParser.parse(alloc, "", "--\r\n") catch {
        err_empty_bnd = true;
    };
    expect_eq(err_empty_bnd, true);

    let err_trunc = false;
    * = MultipartParser.parse(alloc, "bnd", "--bnd\r\nContent-Disposition: form-data; name=\"x\"\r\n\r\nhello") catch {
        err_trunc = true;
    };
    expect_eq(err_trunc, true);

    // 2. SSE max event size DoS protection
    let small_sse = SseParser.initWithMaxSize(alloc, 32);
    defer small_sse.deinit();
    let sse_too_large = false;
    * = small_sse.feed("data: 0123456789012345678901234567890123456789\n\n") catch {
        sse_too_large = true;
    };
    expect_eq(sse_too_large, true);
}

fn testMalformedCookiesAndSseFields(): !void {
    const alloc = std.heap.page_allocator;

    // 3. SSE encoder CRLF injection guard
    let sse_bad_field = false;
    const bad_ev = SseEvent{ event_type: "bad\r\nid: fake", data: "hi" };
    * = SseEncoder.formatEvent(alloc, bad_ev) catch {
        sse_bad_field = true;
    };
    expect_eq(sse_bad_field, true);
}

fn testCookieSecurityAndCompatibility(): !void {
    const alloc = std.heap.page_allocator;

    // 4. Invalid Set-Cookie & Public Suffix protection
    let bad_sc1 = false;
    * = CookieCodec.parseSetCookie("") catch { bad_sc1 = true; };
    expect_eq(bad_sc1, true);

    let bad_sc2 = false;
    * = CookieCodec.parseSetCookie("no_equals_sign; Path=/") catch { bad_sc2 = true; };
    expect_eq(bad_sc2, true);

    let bad_sc3 = false;
    * = CookieCodec.parseSetCookie("=empty_name") catch { bad_sc3 = true; };
    expect_eq(bad_sc3, true);

    let bad_sc4 = false;
    * = CookieCodec.parseSetCookie("bad name=val") catch { bad_sc4 = true; };
    expect_eq(bad_sc4, true);

    let jar = HttpCookieJar.init(alloc);
    defer jar.deinit();

    let ps_com = false;
    jar.setFromHeader("sub.com", "evil=1; Domain=com", 0) catch { ps_com = true; };
    expect_eq(ps_com, true);

    let ps_couk = false;
    jar.setFromHeader("app.co.uk", "evil=1; Domain=co.uk", 0) catch { ps_couk = true; };
    expect_eq(ps_couk, true);

    let ps_gh = false;
    jar.setFromHeader("user.github.io", "evil=1; Domain=github.io", 0) catch { ps_gh = true; };
    expect_eq(ps_gh, true);

    let dom_mismatch = false;
    jar.setFromHeader("attacker.com", "hijack=1; Domain=victim.com", 0) catch { dom_mismatch = true; };
    expect_eq(dom_mismatch, true);

    // 5. Existing cap:http zero-regression compatibility check
    expect_eq(Method.POST.hasRequestBody(), true);
    expect_eq(std.mem.eql(u8, HTTPStatus.reasonPhrase(200), "OK"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension(".json"), "application/json; charset=utf-8"), true);

    let legacy_cm = CookieMap.parse("sid=\"legacy\"; mode=fast");
    expect_eq(std.mem.eql(u8, legacy_cm.get("sid").?, "legacy"), true);
    let legacy_buf: [128]u8 = undefined;
    const legacy_sc = CookieMap.formatSetCookie(CookieEntry{
        name: "sid",
        value: "legacy",
        same_site: CookieSameSite.Strict,
    }, &legacy_buf);
    expect_eq(std.mem.indexOf(u8, legacy_sc, "SameSite=Strict") != null, true);

    const legacy_fd = FormData.parseUrlEncoded("k1=v1&k2=v2");
    expect_eq(std.mem.eql(u8, legacy_fd.get("k1").?, "v1"), true);

    let hdrs = Headers.init(alloc);
    defer hdrs.deinit();
    hdrs.set("X-Cap-Http", "ok");
    expect_eq(std.mem.eql(u8, hdrs.get("x-cap-http").?, "ok"), true);

    let etag_buf: [32]u8 = undefined;
    const tag = ETag.generate("cp-http", &etag_buf);
    expect_eq(ETag.ifNoneMatch(tag, tag), true);
}

fn main() {
    test("1. WHATWG SSE incremental chunked parsing, multi-line data, AI [DONE] & encoder roundtrip", () => {
        try testSseStreamingAndAiRoundtrip();
    });
    test("2. RFC 7578 Multipart FormData builder, binary-safe payload & preamble/epilogue parser roundtrip", () => {
        try testMultipartRoundtripAndBinarySafety();
    });
    test("3. RFC 6265 Set-Cookie codec & HttpCookieJar domain/path/Secure/expiry/deletion matching", () => {
        try testCookieCodecAndCookieJar();
    });
    test("4. Malformed & DoS boundary protection (empty boundary, truncated multipart, SSE max size & CRLF guard)", () => {
        try testMalformedAndBoundaryProtection();
        try testMalformedCookiesAndSseFields();
    });
    test("5. RFC 6265 security boundaries, cap:http aggregate import & existing HTTP zero-regression", () => {
        try testCookieSecurityAndCompatibility();
    });
}

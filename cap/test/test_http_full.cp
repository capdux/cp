// cap/test/test_http_full.cp
// Comprehensive Offline Regression Test Suite for Industrial HTTP Subsystem (cap:http)
// Covers: HTTP/1.1 Types, Headers, ETag, MimeType, CookieMap, FormData, PicoHttpParser,
//         Request/Response, HTTP/2 Framing & HPACK, HTTP/3 Alt-Svc & Sessions, and Client Engine.

import { test, expect_eq } from "cap:test";
import {
    Method,
    Headers,
    ETag,
    MimeType,
    Request,
    Response,
    Encoding,
    FetchCacheMode,
    FetchRedirect,
    FetchRequestMode,
    HTTPCertError,
    CertificateInfo,
    URLPath,
    SignalStore,
    HeaderValueIterator,
    HTTPRequestBody,
    InternalState,
    CLIENT_PREFACE,
    FrameType,
    ErrorCode,
    SettingsType,
    FrameHeader,
    SettingsPayloadUnit,
    H2ClientSession,
    H2StreamState,
    HpackCodec,
    AltSvc,
    H3ClientSession,
    H3StreamType,
    ProxyTunnel,
    SendFile,
    HTTPContext,
    HTTPClient,
    HTTPClientResult,
    Decompressor,
    PicoHeader,
    PicoHttpParser,
    CookieSameSite,
    CookieEntry,
    CookieMap,
    FormData
} from "cap:http";

const std = @import("std");

fn testStep1_TypesAndInfrastructure(): !void {
    // 1. Method
    expect_eq(Method.GET.isIdempotent(), true);
    expect_eq(Method.GET.hasRequestBody(), false);
    expect_eq(std.mem.eql(u8, Method.GET.toString(), "GET"), true);
    expect_eq(Method.POST.isIdempotent(), false);
    expect_eq(Method.POST.hasRequestBody(), true);
    expect_eq(Method.PUT.isIdempotent(), true);
    expect_eq(Method.fromString("delete").? == Method.DELETE, true);
    expect_eq(Method.fromString("PATCH").? == Method.PATCH, true);
    expect_eq(Method.fromString("UNKNOWN_VERB") == null, true);

    // 2. Encoding
    const enc_gzip = Encoding.Gzip;
    expect_eq(enc_gzip.canUseLibDeflate(), true);
    expect_eq(enc_gzip.isCompressed(), true);

    const enc_br = Encoding.Brotli;
    expect_eq(enc_br.canUseLibDeflate(), false);
    expect_eq(enc_br.isCompressed(), true);

    const enc_identity = Encoding.Identity;
    expect_eq(enc_identity.canUseLibDeflate(), false);
    expect_eq(enc_identity.isCompressed(), false);

    // 3. Fetch Modes
    const cache_mode = FetchCacheMode.fromString("no-store");
    expect_eq(cache_mode != null, true);
    expect_eq(cache_mode.? == .NoStore, true);
    expect_eq(FetchCacheMode.fromString("invalid") == null, true);

    const redirect_mode = FetchRedirect.fromString("follow");
    expect_eq(redirect_mode != null, true);
    expect_eq(redirect_mode.? == .Follow, true);

    const req_mode = FetchRequestMode.fromString("cors");
    expect_eq(req_mode != null, true);
    expect_eq(req_mode.? == .Cors, true);

    // 4. URLPath Parsing
    const path_info = URLPath.parse("/api/v1/users/profile.json?tab=activity&view=compact");
    expect_eq(std.mem.eql(u8, path_info.path, "/api/v1/users/profile.json"), true);
    expect_eq(std.mem.eql(u8, path_info.extname, "json"), true);
    expect_eq(std.mem.eql(u8, path_info.first_segment, "api"), true);
    expect_eq(std.mem.eql(u8, path_info.query_string, "?tab=activity&view=compact"), true);
    expect_eq(path_info.is_source_map, false);

    const map_path = URLPath.parse("/dist/bundle.js.map");
    expect_eq(map_path.is_source_map, true);

    const root_path = URLPath.parse("/");
    expect_eq(root_path.isRoot(), true);

    // 5. Signals
    let store = SignalStore{};
    const sigs = store.toSignals();
    expect_eq(sigs.isAborted(), false);
    expect_eq(sigs.isStreaming(), false);
    expect_eq(sigs.isUpgraded(), false);
    sigs.abort();
    expect_eq(sigs.isAborted(), true);

    // 6. HeaderValueIterator
    let iter = HeaderValueIterator.init("gzip, deflate, br;q=1.0, zstd");
    expect_eq(std.mem.eql(u8, iter.next().?, "gzip"), true);
    expect_eq(std.mem.eql(u8, iter.next().?, "deflate"), true);
    expect_eq(std.mem.eql(u8, iter.next().?, "br;q=1.0"), true);
    expect_eq(std.mem.eql(u8, iter.next().?, "zstd"), true);
    expect_eq(iter.next() == null, true);

    // 7. HTTPRequestBody & InternalState
    const body = HTTPRequestBody.fromBytes("Hello CP Request Body");
    expect_eq(body.len(), 21);
    expect_eq(body.isStream(), false);

    const empty_body = HTTPRequestBody.empty();
    expect_eq(empty_body.len(), 0);

    let st = InternalState{};
    st.content_length = 100;
    st.total_body_received = 50;
    expect_eq(st.isChunkedEncoding(), false);
    expect_eq(st.isDone(), false);
    st.total_body_received = 100;
    expect_eq(st.isDone(), true);

    st.transfer_encoding = Encoding.Chunked;
    expect_eq(st.isChunkedEncoding(), true);
    expect_eq(st.isDone(), false);
    st.flags.received_last_chunk = true;
    expect_eq(st.isDone(), true);

    // 8. Cert types
    const cert_err = HTTPCertError{ error_no: 20, code: "UNABLE_TO_GET_ISSUER_CERT", reason: "issuer missing" };
    expect_eq(cert_err.error_no, 20);
    const cert_info = CertificateInfo{ cert: "", cert_error: cert_err, hostname: "example.com" };
    expect_eq(std.mem.eql(u8, cert_info.hostname, "example.com"), true);
}

fn testStep2_HeadersETagAndMime(): !void {
    const allocator = std.heap.page_allocator;
    let headers = Headers.init(allocator);
    defer headers.deinit();

    // Case-insensitive CRUD
    headers.append("Content-Type", "application/json");
    headers.append("X-Request-Id", "id-1");
    headers.append("Cache-Control", "no-cache");
    expect_eq(headers.count(), 3);
    expect_eq(headers.has("content-type"), true);
    expect_eq(std.mem.eql(u8, headers.get("CONTENT-TYPE").?, "application/json"), true);

    headers.set("x-request-id", "id-2");
    expect_eq(headers.count(), 3);
    expect_eq(std.mem.eql(u8, headers.get("X-REQUEST-ID").?, "id-2"), true);

    headers.delete("cache-control");
    expect_eq(headers.count(), 2);
    expect_eq(headers.has("Cache-Control"), false);

    // Trigger dynamic heap spillover (> 32 inline slots)
    let k: usize = 0;
    while (k < 36) : (k += 1) {
        headers.append("X-Bulk", "v");
    }
    expect_eq(headers.count(), 38);
    expect_eq(headers.has("X-Bulk"), true);
    expect_eq(headers.at(37) != null, true);
    expect_eq(headers.at(38) == null, true);

    // ETag generation and strong/weak If-None-Match
    let tag_buf: [32]u8 = undefined;
    const generated = ETag.generate("cap:http payload v1", &tag_buf);
    expect_eq(generated.len > 2, true);
    expect_eq(ETag.ifNoneMatch(generated, generated), true);
    expect_eq(ETag.ifNoneMatch(generated, "*"), true);

    const parsed_weak = ETag.parse("W/\"abc-99\"");
    expect_eq(parsed_weak.is_weak, true);
    expect_eq(std.mem.eql(u8, parsed_weak.tag, "abc-99"), true);
    expect_eq(ETag.ifNoneMatch("\"abc-99\"", "\"other\", W/\"abc-99\""), true);
    expect_eq(ETag.ifNoneMatch("\"abc-99\"", "\"different\""), false);

    // MimeType extension & path resolution
    expect_eq(std.mem.eql(u8, MimeType.fromExtension("html"), "text/html; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension(".JSON"), "application/json; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension("wasm"), "application/wasm"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromPath("/assets/app.min.js"), "application/javascript; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromPath("/img/logo.svg"), "image/svg+xml"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromPath("/bin/blob"), "application/octet-stream"), true);
}

fn testStep3_CookieMapAndFormData(): !void {
    // 1. CookieMap parsing & updates (RFC 6265)
    let cookies = CookieMap.parse("sid=\"abc123\"; theme=dark; empty=; lang=zh-CN");
    expect_eq(cookies.count, 4);
    expect_eq(cookies.has("sid"), true);
    expect_eq(std.mem.eql(u8, cookies.get("sid").?, "abc123"), true);
    expect_eq(std.mem.eql(u8, cookies.get("theme").?, "dark"), true);
    expect_eq(std.mem.eql(u8, cookies.get("lang").?, "zh-CN"), true);
    expect_eq(cookies.get("missing") == null, true);

    cookies.set("theme", "light");
    expect_eq(cookies.count, 4);
    expect_eq(std.mem.eql(u8, cookies.get("theme").?, "light"), true);

    // 2. Set-Cookie serialization
    const entry = CookieEntry{
        name: "session",
        value: "tok_42",
        path: "/api",
        domain: "example.com",
        http_only: true,
        secure: true,
        same_site: CookieSameSite.Strict,
    };
    let set_buf: [256]u8 = undefined;
    const set_hdr = CookieMap.formatSetCookie(entry, &set_buf);
    expect_eq(
        std.mem.eql(
            u8,
            set_hdr,
            "session=tok_42; Path=/api; Domain=example.com; HttpOnly; Secure; SameSite=Strict",
        ),
        true,
    );

    // 3. FormData urlencoded parsing & multipart boundary extraction
    let form = FormData.parseUrlEncoded("user=alice&role=architect&active&tag=cp");
    expect_eq(form.count, 4);
    expect_eq(form.has("user"), true);
    expect_eq(std.mem.eql(u8, form.get("user").?, "alice"), true);
    expect_eq(std.mem.eql(u8, form.get("role").?, "architect"), true);
    expect_eq(std.mem.eql(u8, form.get("active").?, ""), true);
    expect_eq(form.get("nonexistent") == null, true);

    form.append("extra", "123");
    expect_eq(std.mem.eql(u8, form.get("extra").?, "123"), true);

    const b1 = FormData.extractBoundary("multipart/form-data; boundary=----CPBound001");
    expect_eq(b1 != null, true);
    expect_eq(std.mem.eql(u8, b1.?, "----CPBound001"), true);

    const b2 = FormData.extractBoundary("multipart/form-data; boundary=\"----QuotedBound\"; charset=utf-8");
    expect_eq(b2 != null, true);
    expect_eq(std.mem.eql(u8, b2.?, "----QuotedBound"), true);

    expect_eq(FormData.extractBoundary("application/json") == null, true);
}

fn testStep4_PicoHttpAndRequestResponse(): !void {
    // 1. PicoHttpParser zero-copy request parsing
    const raw_req = "POST /v1/events HTTP/1.1\r\nHost: api.cp.dev\r\nContent-Type: application/json\r\nX-Api-Key: secret-1\r\n\r\n{\"ok\":true}";
    let req_headers: [16]PicoHeader = undefined;
    const req_opt = PicoHttpParser.parseRequest(raw_req, &req_headers);
    expect_eq(req_opt != null, true);
    const req = req_opt.?;
    expect_eq(std.mem.eql(u8, req.method, "POST"), true);
    expect_eq(std.mem.eql(u8, req.path, "/v1/events"), true);
    expect_eq(req.minor_version, 1);
    expect_eq(req.header_count, 3);
    expect_eq(std.mem.eql(u8, raw_req[req.bytes_read..], "{\"ok\":true}"), true);
    expect_eq(
        std.mem.eql(u8, PicoHttpParser.findHeader(&req_headers, req.header_count, "x-api-key").?, "secret-1"),
        true,
    );
    expect_eq(PicoHttpParser.findHeader(&req_headers, req.header_count, "X-Missing") == null, true);

    // Incomplete request returns null safely
    expect_eq(PicoHttpParser.parseRequest("GET /partial HTTP/1.1\r\nHost: a", &req_headers) == null, true);

    // 2. PicoHttpParser zero-copy response parsing
    const raw_resp = "HTTP/1.1 201 Created\r\nContent-Length: 4\r\nETag: \"v1\"\r\n\r\ndone";
    let resp_headers: [16]PicoHeader = undefined;
    const resp_opt = PicoHttpParser.parseResponse(raw_resp, &resp_headers);
    expect_eq(resp_opt != null, true);
    const resp = resp_opt.?;
    expect_eq(resp.status, 201);
    expect_eq(resp.minor_version, 1);
    expect_eq(std.mem.eql(u8, resp.message, "Created"), true);
    expect_eq(resp.header_count, 2);
    expect_eq(std.mem.eql(u8, raw_resp[resp.bytes_read..], "done"), true);
    expect_eq(
        std.mem.eql(u8, PicoHttpParser.findHeader(&resp_headers, resp.header_count, "etag").?, "\"v1\""),
        true,
    );

    // 3. High-level Request & Response objects
    let hi_req = new Request("POST", "/api/echo", "hello-body");
    expect_eq(std.mem.eql(u8, hi_req.text(), "hello-body"), true);

    let hi_res = new Response("created");
    expect_eq(hi_res.ok(), true);
    hi_res.status = 404;
    expect_eq(hi_res.ok(), false);
    expect_eq(std.mem.eql(u8, hi_res.text(), "created"), true);
}

fn testStep5_H2FramingHpackAndH3(): !void {
    // 1. HTTP/2 Connection Preface & Frame Header 9-byte Wire Codec
    expect_eq(CLIENT_PREFACE.len, 24);

    const orig_header = FrameHeader{
        length: 1024,
        type: FrameType.Data,
        flags: 0x01,
        stream_id: 7,
    };

    let wire_buf: [9]u8 = undefined;
    orig_header.encode(&wire_buf);

    const decoded_opt = FrameHeader.decode(&wire_buf);
    expect_eq(decoded_opt != null, true);
    const decoded = decoded_opt.?;
    expect_eq(decoded.length, 1024);
    expect_eq(decoded.type == FrameType.Data, true);
    expect_eq(decoded.flags, 0x01);
    expect_eq(decoded.stream_id, 7);
    expect_eq(FrameHeader.decode(wire_buf[0..4]) == null, true);

    // SettingsPayloadUnit 6-byte big-endian encoding
    const spu = SettingsPayloadUnit{
        type: SettingsType.MaxConcurrentStreams,
        value: 256,
    };
    let spu_buf: [6]u8 = undefined;
    spu.encode(&spu_buf);
    expect_eq(spu_buf[0], 0x00);
    expect_eq(spu_buf[1], 0x03);
    expect_eq(spu_buf[4], 0x01);
    expect_eq(spu_buf[5], 0x00);

    // 2. HTTP/2 Client Session & Stream State Machine
    let h2_session = H2ClientSession{};
    expect_eq(h2_session.canCreateStream(), true);

    let s1 = h2_session.newStream();
    expect_eq(s1.id, 1);
    expect_eq(s1.isOpen(), true);
    s1.sentEndStream();
    expect_eq(s1.state == H2StreamState.HalfClosedLocal, true);
    expect_eq(s1.localClosed(), true);
    expect_eq(s1.remoteClosed(), false);
    s1.recvEndStream();
    expect_eq(s1.state == H2StreamState.Closed, true);
    expect_eq(s1.isOpen(), false);

    let s2 = h2_session.newStream();
    expect_eq(s2.id, 3);
    s2.rst(ErrorCode.Cancel);
    expect_eq(s2.rst_done, true);
    expect_eq(s2.last_error == ErrorCode.Cancel, true);
    expect_eq(s2.isOpen(), false);

    h2_session.onGoaway();
    expect_eq(h2_session.canCreateStream(), false);

    // 3. RFC 7541 HPACK Prefix Integer & Header Block Roundtrip
    let int_buf: [8]u8 = undefined;
    const small_n = HpackCodec.encodePrefixInt(&int_buf, 10, 5, 0x00);
    expect_eq(small_n, 1);
    const small_dec = HpackCodec.decodePrefixInt(int_buf[0..small_n], 5).?;
    expect_eq(small_dec.value, 10);
    expect_eq(small_dec.bytes_read, 1);

    // RFC 7541 §C.1.2 example: value 1337 with 5-bit prefix => 3 bytes
    const big_n = HpackCodec.encodePrefixInt(&int_buf, 1337, 5, 0x00);
    expect_eq(big_n, 3);
    const big_dec = HpackCodec.decodePrefixInt(int_buf[0..big_n], 5).?;
    expect_eq(big_dec.value, 1337);
    expect_eq(big_dec.bytes_read, 3);

    // Static Table + Indexed + Literal + Never-Indexed Header Block
    let hpack_buf: [256]u8 = undefined;
    let hpos: usize = 0;
    const n1 = HpackCodec.encodeHeader(hpack_buf[hpos..], ":method", "GET", false);
    expect_eq(n1, 1); // Indexed static #2 (0x82)
    hpos += n1;

    const n2 = HpackCodec.encodeHeader(hpack_buf[hpos..], ":scheme", "https", false);
    expect_eq(n2, 1); // Indexed static #7 (0x87)
    hpos += n2;

    const n3 = HpackCodec.encodeHeader(hpack_buf[hpos..], "x-custom", "cp-h2", false);
    expect_eq(n3 > 2, true);
    hpos += n3;

    const n4 = HpackCodec.encodeHeader(hpack_buf[hpos..], "authorization", "Bearer tok", true);
    expect_eq(n4 > 2, true);
    hpos += n4;

    // Decode all 4 headers sequentially
    let cur: usize = 0;
    const d1 = HpackCodec.decodeHeader(hpack_buf[cur..hpos]).?;
    expect_eq(std.mem.eql(u8, d1.name, ":method"), true);
    expect_eq(std.mem.eql(u8, d1.value, "GET"), true);
    expect_eq(d1.never_index, false);
    cur += d1.bytes_read;

    const d2 = HpackCodec.decodeHeader(hpack_buf[cur..hpos]).?;
    expect_eq(std.mem.eql(u8, d2.name, ":scheme"), true);
    expect_eq(std.mem.eql(u8, d2.value, "https"), true);
    cur += d2.bytes_read;

    const d3 = HpackCodec.decodeHeader(hpack_buf[cur..hpos]).?;
    expect_eq(std.mem.eql(u8, d3.name, "x-custom"), true);
    expect_eq(std.mem.eql(u8, d3.value, "cp-h2"), true);
    expect_eq(d3.never_index, false);
    cur += d3.bytes_read;

    const d4 = HpackCodec.decodeHeader(hpack_buf[cur..hpos]).?;
    expect_eq(std.mem.eql(u8, d4.name, "authorization"), true);
    expect_eq(std.mem.eql(u8, d4.value, "Bearer tok"), true);
    expect_eq(d4.never_index, true);
    cur += d4.bytes_read;
    expect_eq(cur, hpos);

    // 4. HTTP/3 Alt-Svc Parsing (RFC 7838)
    const alt_opt = AltSvc.parse("h2=\":443\"; ma=1200, h3=\":8443\"; ma=3600");
    expect_eq(alt_opt != null, true);
    const alt = alt_opt.?;
    expect_eq(alt.port, 8443);
    expect_eq(alt.max_age, 3600);
    expect_eq(std.mem.eql(u8, alt.protocol, "h3"), true);
    expect_eq(AltSvc.parse("clear") == null, true);
    expect_eq(AltSvc.parse("") == null, true);

    // 5. HTTP/3 Client Session & QUIC Bidi Stream Allocation
    let h3_session = H3ClientSession{};
    let h3_s1 = h3_session.newRequestStream();
    expect_eq(h3_s1.id, 0);
    expect_eq(h3_s1.stream_type == H3StreamType.RequestBidi, true);
    expect_eq(h3_s1.isOpen(), true);

    let h3_s2 = h3_session.newRequestStream();
    expect_eq(h3_s2.id, 4);

    h3_s1.close();
    expect_eq(h3_s1.isOpen(), false);
}

fn testStep6_CoreClientEngine(): !void {
    // 1. Proxy Tunnel Request Formatting & Validation
    const tunnel = ProxyTunnel.init("127.0.0.1", 8080, "api.example.com", 443);
    let connect_buf: [256]u8 = undefined;
    const connect_req = tunnel.formatConnectRequest(&connect_buf)?;
    expect_eq(std.mem.startsWith(u8, connect_req, "CONNECT api.example.com:443 HTTP/1.1\r\n"), true);
    expect_eq(ProxyTunnel.verifyConnectResponse("HTTP/1.1 200 Connection Established\r\n\r\n"), true);
    expect_eq(ProxyTunnel.verifyConnectResponse("HTTP/1.1 502 Bad Gateway\r\n\r\n"), false);

    // 2. SendFile Kernel Zero-Copy State Machine
    let sf = SendFile.init(3, 4096);
    expect_eq(sf.isDone(), false);
    expect_eq(sf.remain, 4096);

    // 3. HTTPContext Keep-Alive Connection Pooling
    let ctx = HTTPContext.init();
    defer ctx.deinit();

    const released = ctx.releaseSocket(101, "api.example.com", 443, true);
    expect_eq(released, true);

    const acquired = ctx.acquireSocket("api.example.com", 443, true);
    expect_eq(acquired != null, true);
    expect_eq(acquired.?, 101);

    const acquired2 = ctx.acquireSocket("api.example.com", 443, true);
    expect_eq(acquired2 == null, true);

    expect_eq(ctx.releaseSocket(101, "api.example.com", 443, true), true);
    ctx.closeDeadSocket(101);
    expect_eq(ctx.acquireSocket("api.example.com", 443, true) == null, true);

    // 4. HTTPClient Request Serialization with Custom Headers (RFC 7230)
    const allocator = std.heap.page_allocator;
    let custom_hdrs = Headers.init(allocator);
    defer custom_hdrs.deinit();
    custom_hdrs.append("X-Client", "cap-http-test");

    let client = HTTPClient.init(Method.POST, "http://api.myserver.org:9000/v1/chat/completions");
    expect_eq(std.mem.eql(u8, client.hostname, "api.myserver.org"), true);
    expect_eq(client.port, 9000);
    expect_eq(std.mem.eql(u8, client.path, "/v1/chat/completions"), true);
    client.headers = custom_hdrs;
    client.body = "{\"model\":\"cp-gemini\",\"stream\":false}";

    let req_buf: [1024]u8 = undefined;
    const req_str = client.formatRequest(&req_buf)?;
    expect_eq(std.mem.startsWith(u8, req_str, "POST /v1/chat/completions HTTP/1.1\r\n"), true);
    expect_eq(std.mem.indexOf(u8, req_str, "Host: api.myserver.org:9000\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, req_str, "X-Client: cap-http-test\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, req_str, "Content-Length: 36\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, req_str, "{\"model\":\"cp-gemini\",\"stream\":false}") != null, true);

    // 5. Response Status, Body & Result Status Evaluation
    const mock_response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"healthy\"}";
    const status_code = HTTPClient.parseResponseStatus(mock_response);
    expect_eq(status_code != null, true);
    expect_eq(status_code.?, 200);

    const resp_body = HTTPClient.parseResponseBody(mock_response);
    expect_eq(std.mem.eql(u8, resp_body, "{\"status\":\"healthy\"}"), true);

    const res_ok = HTTPClientResult{ status: 200, body: resp_body };
    expect_eq(res_ok.isSuccess(), true);
    expect_eq(res_ok.isTimeout(), false);
    expect_eq(res_ok.isAbort(), false);

    const res_err = HTTPClientResult{ status: 503, is_timeout: true, err_msg: "timeout" };
    expect_eq(res_err.isSuccess(), false);
    expect_eq(res_err.isTimeout(), true);

    // 6. Chunked Transfer Encoding Decoding
    const chunked_payload = "5\r\nHello\r\n8\r\n CP-Lang\r\n1\r\n!\r\n0\r\n\r\n";
    let decoded_buf: [128]u8 = undefined;
    const decoded_len = HTTPClient.decodeChunkedBody(chunked_payload, &decoded_buf)?;
    expect_eq(decoded_len, 14);
    expect_eq(std.mem.eql(u8, decoded_buf[0..decoded_len], "Hello CP-Lang!"), true);

    // 7. Decompressor Lifecycle
    let decomp = Decompressor.init(Encoding.Gzip);
    expect_eq(decomp.is_done, false);
    const duped = decomp.decompressSlice("raw-bytes", allocator)?;
    defer allocator.free(duped);
    expect_eq(std.mem.eql(u8, duped, "raw-bytes"), true);
    decomp.release();
    expect_eq(decomp.is_done, true);
}

fn main() {
    test("HTTP Suite 1: Method, Encoding, FetchModes, URLPath, Signals & InternalState", () => {
        testStep1_TypesAndInfrastructure()?;
    });

    test("HTTP Suite 2: Case-Insensitive Headers (Inline + Dynamic), ETag & MimeType", () => {
        testStep2_HeadersETagAndMime()?;
    });

    test("HTTP Suite 3: RFC 6265 CookieMap & RFC 7578 FormData Codec", () => {
        testStep3_CookieMapAndFormData()?;
    });

    test("HTTP Suite 4: PicoHttpParser Zero-Copy Wire Parsing & Request/Response", () => {
        testStep4_PicoHttpAndRequestResponse()?;
    });

    test("HTTP Suite 5: HTTP/2 Framing, Stream State Machine, HPACK & HTTP/3 Alt-Svc", () => {
        testStep5_H2FramingHpackAndH3()?;
    });

    test("HTTP Suite 6: Industrial HTTP/1.1 Client Engine, Keep-Alive Pool & Chunked Decoding", () => {
        testStep6_CoreClientEngine()?;
    });
}

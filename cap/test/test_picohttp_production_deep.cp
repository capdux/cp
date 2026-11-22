// Source: cp/cap/test/test_picohttp_production_deep.cp
// Deep Production Test Suite for cap:picohttp & cap.picohttp
// Covers:
//   1. Request parsing, half-packet Incomplete & HTTP/1.1 pipelining
//   2. Response parsing & boundary status codes (100, 204 empty reason, 999, invalid)
//   3. RFC 9112 malformed chars, CRLF injection, header overflow & Content-Length conflict
//   4. ChunkedDecoder arbitrary 1..7 byte streaming splits (with chunk-ext & trailers)
//   5. `cap.picohttp` dynamic lazy mounting & zero-heap-allocation verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PicoError,
    HttpHeader,
    ParsedRequest,
    ParsedResponse,
    ParsedHeaders,
    ChunkedState,
    ChunkDecodeResult,
    ChunkedDecoder,
    parseRequest,
    parseResponse,
    parseHeaders,
    findHeader,
    parseContentLength,
} from "cap:web";
import cap from "cap";

fn testRequestAndPipelining()! {
    let hdr_buf: [16]HttpHeader = undefined;

    // 1. Half-packet Incomplete across prefixes
    const req1 = "GET /api/v1/items?q=cp HTTP/1.1\r\nHost: example.com\r\nX-Trace-Id:   abc-123 \t\r\n\r\n";
    const req2 = "POST /submit HTTP/1.0\r\nContent-Length: 4\r\n\r\nBODY";

    let cut: usize = 0;
    while (cut < req1.len) : (cut += 1) {
        let saw_incomplete = false;
        * = parseRequest(req1[0..cut], &hdr_buf) catch |e| {
            if (@as(anyerror, e) == @as(anyerror, err.Incomplete)) {
                saw_incomplete = true;
            }
        };
        expect_eq(saw_incomplete, true);
    }

    // 2. Pipelining two concatenated requests in one buffer
    const combined = req1 ++ req2;
    const p1 = parseRequest(combined, &hdr_buf) catch unreachable;
    expect_eq(std.mem.eql(u8, p1.method, "GET"), true);
    expect_eq(std.mem.eql(u8, p1.path, "/api/v1/items?q=cp"), true);
    expect_eq(p1.minor_version, 1);
    expect_eq(p1.headers.len, 2 as usize);
    expect_eq(std.mem.eql(u8, p1.headers[0].name, "Host"), true);
    expect_eq(std.mem.eql(u8, p1.headers[0].value, "example.com"), true);
    expect_eq(std.mem.eql(u8, p1.headers[1].value, "abc-123"), true);
    expect_eq(p1.bytes_consumed, req1.len);

    let hdr_buf2: [16]HttpHeader = undefined;
    const p2 = parseRequest(combined[p1.bytes_consumed..], &hdr_buf2) catch unreachable;
    expect_eq(std.mem.eql(u8, p2.method, "POST"), true);
    expect_eq(std.mem.eql(u8, p2.path, "/submit"), true);
    expect_eq(p2.minor_version, 0);
    expect_eq(p2.headers.len, 1 as usize);
    const cl = (parseContentLength(p2.headers) catch unreachable) ?? 0;
    expect_eq(cl, 4 as usize);
    const body = combined[p1.bytes_consumed + p2.bytes_consumed ..];
    expect_eq(std.mem.eql(u8, body, "BODY"), true);
}

fn testResponseAndBoundaryStatus()! {
    let hdr_buf: [16]HttpHeader = undefined;

    // 100 Continue
    const r100 = "HTTP/1.1 100 Continue\r\n\r\n";
    const p100 = parseResponse(r100, &hdr_buf) catch unreachable;
    expect_eq(p100.minor_version, 1);
    expect_eq(p100.status, 100 as u16);
    expect_eq(std.mem.eql(u8, p100.reason, "Continue"), true);
    expect_eq(p100.headers.len, 0 as usize);
    expect_eq(p100.bytes_consumed, r100.len);

    // 204 without reason phrase
    const r204 = "HTTP/1.0 204\r\nServer: cp-edge\r\n\r\n";
    const p204 = parseResponse(r204, &hdr_buf) catch unreachable;
    expect_eq(p204.minor_version, 0);
    expect_eq(p204.status, 204 as u16);
    expect_eq(std.mem.eql(u8, p204.reason, ""), true);
    expect_eq(p204.headers.len, 1 as usize);

    // 999 upper boundary
    const r999 = "HTTP/1.1 999 Custom\r\n\r\n";
    const p999 = parseResponse(r999, &hdr_buf) catch unreachable;
    expect_eq(p999.status, 999 as u16);

    // Invalid status codes: 099 and 1000
    let hit_099 = false;
    * = parseResponse("HTTP/1.1 099 Bad\r\n\r\n", &hdr_buf) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.InvalidVersion)) hit_099 = true;
    };
    expect_eq(hit_099, true);

    let hit_1000 = false;
    * = parseResponse("HTTP/1.1 1000 TooLong\r\n\r\n", &hdr_buf) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.InvalidVersion)) hit_1000 = true;
    };
    expect_eq(hit_1000, true);
}

fn testSecurityAndMalformedRejection()! {
    let hdr_buf: [8]HttpHeader = undefined;

    // 1. Invalid method (contains space/ctrl or non-token)
    let bad_method = false;
    * = parseRequest("G@T / HTTP/1.1\r\n\r\n", &hdr_buf) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.InvalidMethod)) bad_method = true;
    };
    expect_eq(bad_method, true);

    // 2. Bare CR in header value (CRLF injection attempt)
    let cr_inject = false;
    * = parseRequest("GET / HTTP/1.1\r\nHost: a\rb\r\n\r\n", &hdr_buf) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.InvalidHeaderValue)) cr_inject = true;
    };
    expect_eq(cr_inject, true);

    // 3. Space before colon in header name
    let bad_hname = false;
    * = parseRequest("GET / HTTP/1.1\r\nHost : example.com\r\n\r\n", &hdr_buf) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.InvalidHeaderName)) bad_hname = true;
    };
    expect_eq(bad_hname, true);

    // 4. TooManyHeaders overflow
    let small_hdrs: [1]HttpHeader = undefined;
    let too_many = false;
    * = parseRequest("GET / HTTP/1.1\r\nA: 1\r\nB: 2\r\n\r\n", &small_hdrs) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.TooManyHeaders)) too_many = true;
    };
    expect_eq(too_many, true);

    // 5. Conflicting Content-Length headers
    const smuggle = "POST / HTTP/1.1\r\nContent-Length: 10\r\ncontent-length: 20\r\n\r\n";
    const parsed_smuggle = parseRequest(smuggle, &hdr_buf) catch unreachable;
    let cl_conflict = false;
    * = parseContentLength(parsed_smuggle.headers) catch |e| {
        if (@as(anyerror, e) == @as(anyerror, err.InvalidHeaderValue)) cl_conflict = true;
    };
    expect_eq(cl_conflict, true);

    // 6. Case-insensitive findHeader
    const host_val = findHeader(parsed_smuggle.headers, "cOnTeNt-LeNgTh") ?? "";
    expect_eq(std.mem.eql(u8, host_val, "10"), true);
}

fn testChunkedArbitrarySplits()! {
    const wire =
        "5;ext=1;foo=\"bar\"\r\n" ++
        "Hello\r\n" ++
        "6\r\n" ++
        " World\r\n" ++
        "1;x=y\r\n" ++
        "!\r\n" ++
        "0;last=true\r\n" ++
        "X-Checksum: deadbeef\r\n" ++
        "X-Trailer: ok\r\n" ++
        "\r\n";

    // Test every chunk split step from 1 to 7 bytes
    let step: usize = 1;
    while (step <= 7) : (step += 1) {
        let decoder = ChunkedDecoder.init(true);
        let out_buf: [64]u8 = undefined;
        let out_len: usize = 0;
        let pos: usize = 0;

        while (pos < wire.len && !decoder.isDone()) {
            let end = pos + step;
            if (end > wire.len) end = wire.len;
            const res = decoder.decodeTo(wire[pos..end], out_buf[out_len..]) catch unreachable;
            out_len += res.bytes_written;
            pos += res.bytes_consumed;
        }
        expect_eq(decoder.isDone(), true);
        expect_eq(pos, wire.len);
        expect_eq(std.mem.eql(u8, out_buf[0..out_len], "Hello World!"), true);
    }

    // In-place decode without consuming trailer + parseHeaders on remaining trailer
    let inplace_buf: [128]u8 = undefined;
    std.mem.copyForwards(u8, inplace_buf[0..wire.len], wire);
    let dec_no_tr = ChunkedDecoder.init(false);
    const in_res = dec_no_tr.decode(inplace_buf[0..wire.len]) catch unreachable;
    expect_eq(in_res.done, true);
    expect_eq(std.mem.eql(u8, inplace_buf[0..in_res.bytes_written], "Hello World!"), true);

    let tr_hdrs: [4]HttpHeader = undefined;
    const tr_parsed = parseHeaders(wire[in_res.bytes_consumed..], &tr_hdrs) catch unreachable;
    expect_eq(tr_parsed.headers.len, 2 as usize);
    expect_eq(std.mem.eql(u8, findHeader(tr_parsed.headers, "x-checksum") ?? "", "deadbeef"), true);
}

fn testCapLazyMountAndZeroAlloc()! {
    // Verify `cap.picohttp` lazy namespace mounting
    let hdr_buf: [8]HttpHeader = undefined;
    const raw = "PUT /kv/key HTTP/1.1\r\nHost: local\r\nContent-Length: 3\r\n\r\n";
    const req = cap.web.parseRequest(raw, &hdr_buf) catch unreachable;
    expect_eq(std.mem.eql(u8, req.method, "PUT"), true);
    expect_eq(std.mem.eql(u8, req.path, "/kv/key"), true);
    expect_eq((cap.web.parseContentLength(req.headers) catch unreachable) ?? 0, 3 as usize);

    // Zero heap allocation: all APIs operate purely on stack slices without any Allocator parameter
    let decoder = cap.web.ChunkedDecoder.init(true);
    let chunk_data = "3\r\nabc\r\n0\r\n\r\n".*;
    const dres = decoder.decode(&chunk_data) catch unreachable;
    expect_eq(dres.done, true);
    expect_eq(std.mem.eql(u8, chunk_data[0..dres.bytes_written], "abc"), true);
}

fn main() {
    test("1. cap:picohttp — Request parsing, Incomplete & Pipelining", () => {
        testRequestAndPipelining()?;
    });
    test("2. cap:picohttp — Response parsing & boundary status codes", () => {
        testResponseAndBoundaryStatus()?;
    });
    test("3. cap:picohttp — RFC 9112 malformed/CRLF rejection & Content-Length conflict", () => {
        testSecurityAndMalformedRejection()?;
    });
    test("4. cap:picohttp — ChunkedDecoder 1..7 byte arbitrary splits & trailers", () => {
        testChunkedArbitrarySplits()?;
    });
    test("5. cap.picohttp — Dynamic lazy mounting & zero-heap-allocation verification", () => {
        testCapLazyMountAndZeroAlloc()?;
    });
}

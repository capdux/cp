// Source: cp/cap/test/testWebHttp1RustRefSuite.cp
// Reference-hardened against seanmonstar/httparse (MIT/Apache-2.0) & RFC 9110 / RFC 9112
// Covers:
// 1. HTTP/1.0 & HTTP/1.1 request/response parsing & incremental partial feeding
// 2. Malformed message rejection (space before colon, bare CR, NUL, invalid method/version/URI, header limit)
// 3. RFC 9112 framing ambiguity guards (TE+CL conflict, duplicate CL mismatch, strict chunk size)
// 4. 16-lane SIMD target/name/value & CRLF scanning vs scalar 0-discrepancy parity
// 5. 1,500-case deterministic PRNG malformed HTTP/1.1 fuzz suite (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    HTTP_MAX_HEADERS,
    PicoError,
    FramingInfo,
    HttpHeader,
    ChunkedDecoder,
    parseRequest,
    parseResponse,
    parseHeaders,
    findHeader,
    parseContentLength,
    validateHttp1Framing,
    parseChunkSizeStrict,
    simdScanTargetBytes,
    simdScanHeaderNameBytes,
    simdScanHeaderValueBytes,
    simdFindCrlf,
} from "../../lib/web/picohttp.cp";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarIsToken(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
           c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
           c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn scalarScanTarget(s: []const u8): usize {
    let i: usize = 0;
    while (i < s.len && s[i] >= 0x21 && s[i] <= 0x7E) : (i += 1) {}
    return i;
}

fn scalarScanHeaderName(s: []const u8): usize {
    let i: usize = 0;
    while (i < s.len && scalarIsToken(s[i])) : (i += 1) {}
    return i;
}

fn scalarScanHeaderValue(s: []const u8): usize {
    let i: usize = 0;
    while (i < s.len && (s[i] == 0x09 || (s[i] >= 0x20 && s[i] != 0x7F))) : (i += 1) {}
    return i;
}

fn scalarFindCrlf(s: []const u8): usize? {
    let i: usize = 0;
    while (i + 1 < s.len) : (i += 1) {
        if (s[i] == '\r' && s[i + 1] == '\n') return i;
    }
    return null;
}

struct Rng {
    state: u64;

    fn nextU64(*_): u64 {
        let x = _.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn nextU8(*_): u8 {
        return ((_.nextU64() >> 32) & 0xFF) as u8;
    }

    fn nextRange(*_, bound: usize): usize {
        return ((_.nextU64() as usize) % bound);
    }
}

// --- Test 1: HTTP/1.0 & 1.1 Request/Response & Progressive Partial Feeding ---

fn testHttp1RequestsResponsesAndPartial(): !void {
    let hdrs: [32]HttpHeader = undefined;

    // 1. Standard HTTP/1.1 request
    const req_raw =
        "POST /api/v1/items?limit=10 HTTP/1.1\r\n" ++
        "Host: api.example.com\r\n" ++
        "Content-Type: application/json; charset=utf-8\r\n" ++
        "Content-Length: 42\r\n" ++
        "X-Custom-Token: abc_123\r\n\r\n";

    // Progressive byte-by-byte partial feeding MUST return Incomplete until full header block arrives
    for (cut of 0..req_raw.len) {
        const cut_u = cut as usize;
        let got_incomplete = false;
        * = parseRequest(req_raw[0..cut_u], &hdrs) catch |e| {
            got_incomplete = (e == PicoError.Incomplete);
        };
        expect_eq(got_incomplete, true);
    }

    const parsed = try parseRequest(req_raw, &hdrs);
    expect_eq(std.mem.eql(u8, parsed.method, "POST"), true);
    expect_eq(std.mem.eql(u8, parsed.path, "/api/v1/items?limit=10"), true);
    expect_eq(parsed.minor_version, 1);
    expect_eq(parsed.headers.len, 4 as usize);
    expect_eq(parsed.bytes_consumed, req_raw.len);
    expect_eq(std.mem.eql(u8, findHeader(parsed.headers, "host").?, "api.example.com"), true);

    const framing = try validateHttp1Framing(parsed.headers);
    expect_eq(framing.is_chunked, false);
    expect_eq(framing.content_length.?, 42 as usize);

    // 2. HTTP/1.0 response & reason phrase
    const resp_raw = "HTTP/1.0 200 OK\r\nServer: cp-web/1.0\r\nContent-Length: 0\r\n\r\n";
    const resp = try parseResponse(resp_raw, &hdrs);
    expect_eq(resp.minor_version, 0);
    expect_eq(resp.status, 200 as u16);
    expect_eq(std.mem.eql(u8, resp.reason, "OK"), true);
    expect_eq(resp.headers.len, 2 as usize);
    expect_eq(resp.bytes_consumed, resp_raw.len);
}

// --- Test 2: Malformed Message Rejection & RFC 9112 Framing Ambiguity Guards ---

fn testMalformedRejectionAndFramingGuards(): !void {
    let hdrs: [32]HttpHeader = undefined;

    // 1. httparse malformed request matrix
    const bad_reqs: []const []const u8 = [
        // Space before colon in header name
        "GET / HTTP/1.1\r\nHost : example.com\r\n\r\n",
        // Bare CR in header value (header splitting guard)
        "GET / HTTP/1.1\r\nHost: ex\rample.com\r\n\r\n",
        // NUL byte in header value
        "GET / HTTP/1.1\r\nHost: ex\x00ample.com\r\n\r\n",
        // Control char in header name
        "GET / HTTP/1.1\r\nHo\x07st: example.com\r\n\r\n",
        // Invalid method characters
        "G@T / HTTP/1.1\r\nHost: a\r\n\r\n",
        " / HTTP/1.1\r\nHost: a\r\n\r\n",
        // Invalid URI (control char / space)
        "GET /\x01 HTTP/1.1\r\nHost: a\r\n\r\n",
        // Invalid HTTP version
        "GET / HTTP/2.0\r\nHost: a\r\n\r\n",
        "GET / HTTP/1.9\r\nHost: a\r\n\r\n",
        "GET / HTP/1.1\r\nHost: a\r\n\r\n",
        // Empty header name
        "GET / HTTP/1.1\r\n: value\r\n\r\n",
    ];

    for (bad of bad_reqs) {
        let rejected = false;
        * = parseRequest(bad, &hdrs) catch { rejected = true; };
        expect_eq(rejected, true);
    }

    // 2. Framing ambiguity guard: Transfer-Encoding + Content-Length coexistence MUST fail
    const te_cl_req = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n";
    const r_te_cl = try parseRequest(te_cl_req, &hdrs);
    let conflict_err = false;
    * = validateHttp1Framing(r_te_cl.headers) catch |e| {
        conflict_err = (e == PicoError.ConflictingFraming);
    };
    expect_eq(conflict_err, true);

    // 3. Conflicting duplicate Content-Length values MUST fail
    const dup_cl_req = "POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\n";
    const r_dup_cl = try parseRequest(dup_cl_req, &hdrs);
    let dup_err = false;
    * = validateHttp1Framing(r_dup_cl.headers) catch { dup_err = true; };
    expect_eq(dup_err, true);

    // Identical duplicate Content-Length values are allowed per RFC 9112
    const same_cl_req = "POST / HTTP/1.1\r\nContent-Length: 7\r\nContent-Length: 7\r\n\r\n";
    const r_same = try parseRequest(same_cl_req, &hdrs);
    expect_eq((try validateHttp1Framing(r_same.headers)).content_length.?, 7 as usize);

    // 4. Duplicate chunked coding rejected; valid chunked accepted
    const dup_te_req = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked, chunked\r\n\r\n";
    const r_dup_te = try parseRequest(dup_te_req, &hdrs);
    let dup_te_err = false;
    * = validateHttp1Framing(r_dup_te.headers) catch { dup_te_err = true; };
    expect_eq(dup_te_err, true);

    const ok_te_req = "POST / HTTP/1.1\r\nTransfer-Encoding: gzip, chunked\r\n\r\n";
    const r_ok_te = try parseRequest(ok_te_req, &hdrs);
    expect_eq((try validateHttp1Framing(r_ok_te.headers)).is_chunked, true);

    // 5. Strict chunk size parsing & ChunkedDecoder roundtrip
    expect_eq(try parseChunkSizeStrict("1A\r\n"), 26 as usize);
    expect_eq(try parseChunkSizeStrict("0;ext=1\r\n"), 0 as usize);
    let bad_cs = false;
    * = parseChunkSizeStrict("-5\r\n") catch { bad_cs = true; };
    expect_eq(bad_cs, true);
    let sp_cs = false;
    * = parseChunkSizeStrict(" 5\r\n") catch { sp_cs = true; };
    expect_eq(sp_cs, true);
    let ovf_cs = false;
    * = parseChunkSizeStrict("FFFFFFFFFFFFFFFFF\r\n") catch |e| {
        ovf_cs = (e == PicoError.ChunkOverflow);
    };
    expect_eq(ovf_cs, true);

    let decoder = ChunkedDecoder.init(true);
    let out_chunk: [64]u8 = undefined;
    const wire = "5\r\nhello\r\n6;ext=val\r\n world\r\n0\r\nX-Trail: 1\r\n\r\n";
    const res = try decoder.decodeTo(wire, &out_chunk);
    expect_eq(res.done, true);
    expect_eq(std.mem.eql(u8, out_chunk[0..res.bytes_written], "hello world"), true);

    // 6. Header count cap enforcement (HTTP_MAX_HEADERS = 128)
    expect_eq(HTTP_MAX_HEADERS, 128 as usize);
    let tiny_hdrs: [2]HttpHeader = undefined;
    let too_many = false;
    * = parseRequest("GET / HTTP/1.1\r\nA: 1\r\nB: 2\r\nC: 3\r\n\r\n", &tiny_hdrs) catch |e| {
        too_many = (e == PicoError.TooManyHeaders);
    };
    expect_eq(too_many, true);
}

// --- Test 3: 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x19C3_7E5A_2B8D_4F01 };
    let buf: [96]u8 = undefined;

    for (len of 0..97) {
        const len_u = len as usize;
        // 1. Target scan parity
        for (i of 0..len) buf[i] = 'a';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s1 = buf[0..len_u];
        expect_eq(simdScanTargetBytes(s1), scalarScanTarget(s1));

        // 2. Header name scan parity
        for (i of 0..len) buf[i] = 'X';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s2 = buf[0..len_u];
        expect_eq(simdScanHeaderNameBytes(s2), scalarScanHeaderName(s2));

        // 3. Header value scan parity
        for (i of 0..len) buf[i] = 'v';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s3 = buf[0..len_u];
        expect_eq(simdScanHeaderValueBytes(s3), scalarScanHeaderValue(s3));

        // 4. CRLF scan parity
        for (i of 0..len) buf[i] = 'z';
        if (len_u >= 2) {
            const p = rng.nextRange(len_u - 1);
            buf[p] = '\r';
            buf[p + 1] = if ((len_u % 2) == 0) '\n' else 'x';
        }
        const s4 = buf[0..len_u];
        expect_eq(simdFindCrlf(s4), scalarFindCrlf(s4));
    }
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed HTTP/1.1 Fuzz Suite ---

fn fuzzCheckRequest(slice: []const u8, hdrs: []HttpHeader): !void {
    const parsed = parseRequest(slice, hdrs) catch return;
    expect_eq(parsed.bytes_consumed <= slice.len, true);
    * = validateHttp1Framing(parsed.headers) catch return;
}

fn fuzzCheckResponse(slice: []const u8, hdrs: []HttpHeader): !void {
    const resp = parseResponse(slice, hdrs) catch return;
    expect_eq(resp.bytes_consumed <= slice.len, true);
    * = validateHttp1Framing(resp.headers) catch return;
}

fn fuzzCheckHeaders(slice: []const u8, hdrs: []HttpHeader) {
    * = parseHeaders(slice, hdrs) catch return;
}

fn fuzzCheckChunkSize(slice: []const u8) {
    * = parseChunkSizeStrict(slice) catch return;
}

fn fuzzCheckChunked(slice: []const u8, chunk_out: []u8) {
    let decoder = ChunkedDecoder.init(true);
    * = decoder.decodeTo(slice, chunk_out) catch return;
}

fn testDeterministicHttp1Fuzz(): !void {
    let rng = Rng { state: 0x7A2B_9C4D_3E1F_6085 };
    let buf: [192]u8 = undefined;
    let hdrs: [32]HttpHeader = undefined;
    let chunk_out: [192]u8 = undefined;
    const tmpl_req = "GET /index.html HTTP/1.1\r\nHost: a.com\r\nContent-Length: 4\r\n\r\n";
    const tmpl_resp = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";

    let total: usize = 0;
    for (case_idx of 0..1500) {
        const strat = (case_idx as usize) % 4;
        let len: usize = 0;
        if (strat == 0) {
            len = rng.nextRange(160);
            for (i of 0..len) buf[i] = rng.nextU8();
        } else if (strat == 1) {
            len = (1 as usize) + rng.nextRange(tmpl_req.len);
            @memcpy(buf[0..len], tmpl_req[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            for (m of 0..muts) {
                * = m;
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else if (strat == 2) {
            len = (1 as usize) + rng.nextRange(tmpl_resp.len);
            @memcpy(buf[0..len], tmpl_resp[0..len]);
            const muts = (1 as usize) + rng.nextRange(3);
            for (m of 0..muts) {
                * = m;
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else {
            // Chunked wire mutation
            const chunk_tmpl = "a\r\n0123456789\r\n0\r\n\r\n";
            len = (1 as usize) + rng.nextRange(chunk_tmpl.len);
            @memcpy(buf[0..len], chunk_tmpl[0..len]);
            buf[rng.nextRange(len)] = rng.nextU8();
        }

        const slice = buf[0..len];
        try fuzzCheckRequest(slice, &hdrs);
        try fuzzCheckResponse(slice, &hdrs);
        fuzzCheckHeaders(slice, &hdrs);
        fuzzCheckChunkSize(slice);
        fuzzCheckChunked(slice, &chunk_out);

        total += 1;
    }
    expect_eq(total, 1500 as usize);
}

fn main() {
    test("1. HTTP/1.0 & HTTP/1.1 Requests, Responses & Progressive Partial Feeding", () => {
        try testHttp1RequestsResponsesAndPartial();
    });
    test("2. Malformed Message Rejection & RFC 9112 Framing Ambiguity Guards", () => {
        try testMalformedRejectionAndFramingGuards();
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        try testSimdVsScalarParity();
    });
    test("4. 1,500-Case Deterministic PRNG Malformed HTTP/1.1 Fuzz Suite", () => {
        try testDeterministicHttp1Fuzz();
    });
}

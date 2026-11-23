// Source: cp/cap/test/testMimeRustRefSuite.cp
// Reference-hardened against hyperium/mime (MIT/Apache-2.0) & RFC 2045 / RFC 6838 / RFC 7578
// Covers:
// 1. MediaType parsing, case normalization, +suffix, quoted-string escapes, toString roundtrip & Accept negotiation
// 2. Malformed MediaType & RFC 7578 multipart/form-data boundary validation, part limits & truncated stream guards
// 3. 16-lane SIMD vs scalar 0-discrepancy differential parity (token, restricted-name, delim, semicolon, dash-boundary)
// 4. 1,500-case deterministic PRNG malformed MIME & multipart fuzz suite (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MIME_MAX_NAME_LEN,
    MIME_MAX_PARAMS,
    MimeError,
    MediaType,
    ContentNegotiator,
    simdIsValidMimeToken,
    simdIsValidRestrictedName,
    simdFindMimeDelim,
    simdIndexOfSemicolon,
} from "../../lib/mime/mime.cp";
import {
    MULTIPART_MAX_PARTS,
    isValidBoundary,
    simdFindDashBoundaryCandidate,
    MultipartParser,
} from "../../lib/mime/multipart.cp";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarIsTokenChar(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return switch (c) {
        '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' => true,
        else => false,
    };
}

fn scalarIsValidMimeToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!scalarIsTokenChar(s[i])) return false;
    }
    return true;
}

fn scalarIsRestrictedNameFirst(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn scalarIsRestrictedNameChar(c: u8): bool {
    if (scalarIsRestrictedNameFirst(c)) return true;
    return switch (c) {
        '!', '#', '$', '&', '-', '^', '_', '.', '+' => true,
        else => false,
    };
}

fn scalarIsValidRestrictedName(s: []const u8): bool {
    if (s.len == 0 || s.len > MIME_MAX_NAME_LEN) return false;
    if (s.len == 1 && s[0] == '*') return true;
    if (!scalarIsRestrictedNameFirst(s[0])) return false;
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!scalarIsRestrictedNameChar(s[i])) return false;
    }
    return true;
}

fn scalarFindMimeDelim(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '/' || c == ';' || c == '=' || c == '"') return i;
    }
    return null;
}

fn scalarIndexOfSemicolon(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ';') return i;
    }
    return null;
}

fn scalarFindDashBoundaryCandidate(data: []const u8, start: usize): usize? {
    let i = start;
    while (i + 1 < data.len) : (i += 1) {
        if (data[i] == '-' && data[i + 1] == '-') return i;
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

// --- Test 1: MediaType Parsing, Normalization, Escapes & Accept Negotiation ---

fn testMediaTypeVectorsAndRoundtrip(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Standard text/plain; charset=utf-8
    let mt1 = try MediaType.parse(alloc, "Text/PLAIN; CHARSET=utf-8");
    defer mt1.deinit();
    expect_eq(std.mem.eql(u8, mt1.type, "text"), true);
    expect_eq(std.mem.eql(u8, mt1.subtype, "plain"), true);
    expect_eq(std.mem.eql(u8, mt1.essence(), "text/plain"), true);
    expect_eq(std.mem.eql(u8, mt1.charset().?, "utf-8"), true);

    // 2. Structured syntax suffix & quoted parameter with escaped quotes
    let mt2 = try MediaType.parse(alloc, "Application/VND.Api+JSON; version=\"1.0\"; note=\"a\\\"b\\\\c\"");
    defer mt2.deinit();
    expect_eq(std.mem.eql(u8, mt2.type, "application"), true);
    expect_eq(std.mem.eql(u8, mt2.subtype, "vnd.api+json"), true);
    expect_eq(std.mem.eql(u8, mt2.suffix, "+json"), true);
    expect_eq(std.mem.eql(u8, mt2.getParam("VERSION").?, "1.0"), true);
    expect_eq(std.mem.eql(u8, mt2.getParam("note").?, "a\"b\\c"), true);
    expect_eq(mt2.matches("+json"), true);
    expect_eq(mt2.matches("application/*+json"), true);

    const formatted = try mt2.toString(alloc);
    defer alloc.free(formatted);
    let mt2_rt = try MediaType.parse(alloc, formatted);
    defer mt2_rt.deinit();
    expect_eq(std.mem.eql(u8, mt2_rt.essence(), "application/vnd.api+json"), true);
    expect_eq(std.mem.eql(u8, mt2_rt.getParam("note").?, "a\"b\\c"), true);

    // 3. Wildcard */*
    let mt3 = try MediaType.parse(alloc, "*/*");
    defer mt3.deinit();
    expect_eq(std.mem.eql(u8, mt3.type, "*"), true);
    expect_eq(std.mem.eql(u8, mt3.subtype, "*"), true);

    // 4. RFC 9110 Accept negotiation
    const avail: []const []const u8 = ["text/plain", "application/json", "text/html"];
    const best = try ContentNegotiator.negotiateAccept(
        alloc,
        "text/plain;q=0, text/*;q=0.8, application/json;q=0.95",
        avail,
    );
    expect_eq(std.mem.eql(u8, best.?, "application/json"), true);
}

// --- Test 2: Malformed MediaType & RFC 7578 Multipart Security Guards ---

fn isRejectedMediaType(alloc: std.mem.Allocator, raw: []const u8): bool {
    let parsed = MediaType.parse(alloc, raw) catch return true;
    parsed.deinit();
    return false;
}

fn testMalformedMimeAndMultipartGuards(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Malformed MediaType matrix
    const bad_mimes: []const []const u8 = [
        "",
        "text",
        "/plain",
        "text/",
        "text/plain/extra",
        "*/plain",
        "text/+json",
        "text/plain+",
        "-bad/plain",
        "text/pl ain",
        "text/plain; charset=\"unterminated",
        "text/plain; charset=\"utf-8\"trailing",
        "text/plain; charset=\"bad\r\nval\"",
        "text/plain; charset=\"bad\x00val\"",
        "text/plain; =value",
        "text/plain; key=",
        "text/plain; key",
    ];
    for (bm of bad_mimes) {
        expect_eq(isRejectedMediaType(alloc, bm), true);
    }

    // Over-length > 127B restricted-name MUST be rejected
    let long_buf: [140]u8 = undefined;
    for (i of 0..128) long_buf[i] = 'a';
    long_buf[128] = '/';
    long_buf[129] = 'b';
    expect_eq(isRejectedMediaType(alloc, long_buf[0..130]), true);
    expect_eq(MIME_MAX_NAME_LEN, 127 as usize);
    expect_eq(MIME_MAX_PARAMS, 64 as usize);

    // 2. RFC 7578 Multipart boundary validation
    expect_eq(isValidBoundary("----WebKitFormBoundary7MA4YWxkTrZu0gW"), true);
    expect_eq(isValidBoundary(""), false);
    expect_eq(isValidBoundary("trailing-space "), false);
    expect_eq(isValidBoundary("bad\r\nboundary"), false);
    let b71: [71]u8 = undefined;
    for (i of 0..71) b71[i] = 'x';
    expect_eq(isValidBoundary(&b71), false);
    expect_eq(isValidBoundary(b71[0..70]), true);

    // 3. Multi-part parsing + preamble + CRLF body + final boundary
    const mp_body =
        "preamble ignored\r\n" ++
        "--bnd42\r\n" ++
        "Content-Disposition: form-data; name=\"field1\"\r\n\r\n" ++
        "hello-world\r\n" ++
        "--bnd42\r\n" ++
        "Content-Disposition: form-data; name=\"file\"; filename=\"a.txt\"\r\n" ++
        "Content-Type: text/plain\r\n\r\n" ++
        "line1\r\n--not-boundary\r\nline2\r\n" ++
        "--bnd42--\r\n";

    let parser = MultipartParser.init(mp_body, "bnd42");
    const p1 = parser.next().?;
    expect_eq(std.mem.eql(u8, p1.body, "hello-world"), true);
    expect_eq(std.mem.eql(u8, p1.getHeader("content-disposition").?, "form-data; name=\"field1\""), true);

    const p2 = parser.next().?;
    expect_eq(std.mem.eql(u8, p2.body, "line1\r\n--not-boundary\r\nline2"), true);
    expect_eq(std.mem.eql(u8, p2.getHeader("Content-Type").?, "text/plain"), true);
    expect_eq(parser.next() == null, true);
    expect_eq(parser.isFinished(), true);

    // 4. Truncated stream without final --boundary-- MUST leave isFinished() == false
    const trunc_body = "--bnd42\r\nContent-Type: text/plain\r\n\r\npartial";
    let trunc_p = MultipartParser.init(trunc_body, "bnd42");
    expect_eq(trunc_p.next() == null, true);
    expect_eq(trunc_p.isFinished(), false);

    // 5. Part count cap enforcement
    expect_eq(MULTIPART_MAX_PARTS, 1024 as usize);
    let capped = MultipartParser.initWithMaxParts(mp_body, "bnd42", 1);
    expect_eq(capped.next() != null, true);
    expect_eq(capped.next() == null, true);
}

// --- Test 3: 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x2B7E_1516_28AE_D2A6 };
    let buf: [96]u8 = undefined;

    for (len of 0..97) {
        const len_u = len as usize;

        // 1. simdIsValidMimeToken parity
        for (i of 0..len) buf[i] = 'a';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s1 = buf[0..len_u];
        expect_eq(simdIsValidMimeToken(s1), scalarIsValidMimeToken(s1));

        // 2. simdIsValidRestrictedName parity
        for (i of 0..len) buf[i] = 'm';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s2 = buf[0..len_u];
        expect_eq(simdIsValidRestrictedName(s2), scalarIsValidRestrictedName(s2));

        // 3. simdFindMimeDelim & simdIndexOfSemicolon parity
        for (i of 0..len) buf[i] = 'z';
        if (len_u > 0) {
            const p = rng.nextRange(len_u);
            const delims: [4]u8 = ['/', ';', '=', '"'];
            buf[p] = delims[len_u % 4];
        }
        const s3 = buf[0..len_u];
        expect_eq(simdFindMimeDelim(s3), scalarFindMimeDelim(s3));
        expect_eq(simdIndexOfSemicolon(s3), scalarIndexOfSemicolon(s3));

        // 4. simdFindDashBoundaryCandidate parity
        for (i of 0..len) buf[i] = 'q';
        if (len_u >= 2) {
            const p = rng.nextRange(len_u - 1);
            buf[p] = '-';
            buf[p + 1] = if ((len_u % 2) == 0) '-' else 'x';
        }
        const s4 = buf[0..len_u];
        expect_eq(simdFindDashBoundaryCandidate(s4, 0), scalarFindDashBoundaryCandidate(s4, 0));
    }
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed MIME & Multipart Fuzz Suite ---

fn fuzzCheckMediaType(alloc: std.mem.Allocator, slice: []const u8) {
    let mt = MediaType.parse(alloc, slice) catch return;
    defer mt.deinit();
    const out = mt.toString(alloc) catch return;
    alloc.free(out);
}

fn fuzzCheckMultipart(slice: []const u8) {
    let parser = MultipartParser.initWithMaxParts(slice, "bnd", 16);
    let steps: usize = 0;
    while (steps < 20) : (steps += 1) {
        const part_opt = parser.next();
        if (part_opt == null) break;
    }
}

fn testDeterministicMimeFuzz(): !void {
    const alloc = std.heap.page_allocator;
    let rng = Rng { state: 0x4F1B_BCDA_1932_60E5 };
    let buf: [192]u8 = undefined;
    const tmpl_mt = "application/vnd.api+json; charset=\"utf-8\"; version=1";
    const tmpl_mp = "--bnd\r\nContent-Type: text/plain\r\n\r\nhello\r\n--bnd--\r\n";

    let total: usize = 0;
    for (case_idx of 0..1500) {
        const strat = (case_idx as usize) % 3;
        let len: usize = 0;
        if (strat == 0) {
            len = rng.nextRange(160);
            for (i of 0..len) buf[i] = rng.nextU8();
        } else if (strat == 1) {
            len = (1 as usize) + rng.nextRange(tmpl_mt.len);
            @memcpy(buf[0..len], tmpl_mt[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            let m: usize = 0;
            while (m < muts) : (m += 1) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else {
            len = (1 as usize) + rng.nextRange(tmpl_mp.len);
            @memcpy(buf[0..len], tmpl_mp[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            let m: usize = 0;
            while (m < muts) : (m += 1) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        }

        const slice = buf[0..len];
        fuzzCheckMediaType(alloc, slice);
        fuzzCheckMultipart(slice);
        total += 1;
    }
    expect_eq(total, 1500 as usize);
}

fn main() {
    test("1. MediaType Parsing, Normalization, Escapes & Accept Negotiation", () => {
        try testMediaTypeVectorsAndRoundtrip();
    });
    test("2. Malformed MediaType & RFC 7578 Multipart Security Guards", () => {
        try testMalformedMimeAndMultipartGuards();
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        try testSimdVsScalarParity();
    });
    test("4. 1,500-Case Deterministic PRNG Malformed MIME & Multipart Fuzz Suite", () => {
        try testDeterministicMimeFuzz();
    });
}

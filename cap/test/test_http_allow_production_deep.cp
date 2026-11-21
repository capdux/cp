import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HttpMethodBit,
    AllowIter,
    parseAllowHeader,
    isMethodAllowed,
    isSafeMethod,
    isIdempotentMethod,
} from "cap:http_allow";

test("1. parseAllowHeader builds standard bitmask, tracks custom methods, and is case-sensitive", () => {
    const hdr = " , OPTIONS , POST , GET , HEAD , PROPFIND , get , ";
    const set = parseAllowHeader(hdr);

    expect_eq(set.isEmpty(), false);
    expect_eq(set.has_custom, true); // PROPFIND and lowercase "get" are non-standard per RFC 9110
    expect_eq(set.countStandard(), u32(4));
    expect_eq(set.containsStandard(HttpMethodBit.GET), true);
    expect_eq(set.containsStandard(HttpMethodBit.HEAD), true);
    expect_eq(set.containsStandard(HttpMethodBit.POST), true);
    expect_eq(set.containsStandard(HttpMethodBit.OPTIONS), true);
    expect_eq(set.containsStandard(HttpMethodBit.GET | HttpMethodBit.POST), true);
    expect_eq(set.containsStandard(HttpMethodBit.DELETE), false);
    expect_eq(set.containsStandard(0), false);

    // Empty header produces empty set
    const empty = parseAllowHeader(" , , \t ");
    expect_eq(empty.isEmpty(), true);
    expect_eq(empty.countStandard(), u32(0));
});

test("2. AllowMethodSet.formatStandard outputs canonical RFC order and checks buffer size", () => {
    const set = parseAllowHeader("PATCH, OPTIONS, POST, HEAD, GET, GET");
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const formatted = set.formatStandard(&buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, formatted, "GET, HEAD, POST, OPTIONS, PATCH"), true);

    // Empty set formats to empty slice
    const empty = parseAllowHeader("");
    const empty_out = empty.formatStandard(&buf) ?? unreachable;
    expect_eq(empty_out.len, usize(0));

    // Tiny buffer returns null
    let tiny: [8]u8 = std.mem.zeroes([8]u8);
    expect_eq(set.formatStandard(&tiny) == null, true);
});

test("3. AllowIter iterates valid RFC 9110 tokens including WebDAV extensions and skips malformed tokens", () => {
    const hdr = "GET, PROPFIND, MKCOL, bad method, \"quoted\", DELETE";
    let it = AllowIter.init(hdr);

    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "GET"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "PROPFIND"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "MKCOL"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "DELETE"), true);
    expect_eq(it.next() == null, true);
});

test("4. isMethodAllowed matches standard and extension methods case-sensitively", () => {
    const hdr = "GET, POST, PROPFIND, LOCK";
    expect_eq(isMethodAllowed(hdr, "GET"), true);
    expect_eq(isMethodAllowed(hdr, "PROPFIND"), true);
    expect_eq(isMethodAllowed(hdr, "LOCK"), true);

    expect_eq(isMethodAllowed(hdr, "get"), false);
    expect_eq(isMethodAllowed(hdr, "DELETE"), false);
    expect_eq(isMethodAllowed(hdr, ""), false);
    expect_eq(isMethodAllowed(hdr, "GET "), false);
});

test("5. isSafeMethod and isIdempotentMethod follow RFC 9110 §9.2.1 and §9.2.2", () => {
    // Safe methods: GET, HEAD, OPTIONS, TRACE
    expect_eq(isSafeMethod("GET"), true);
    expect_eq(isSafeMethod("HEAD"), true);
    expect_eq(isSafeMethod("OPTIONS"), true);
    expect_eq(isSafeMethod("TRACE"), true);
    expect_eq(isSafeMethod("PUT"), false);
    expect_eq(isSafeMethod("DELETE"), false);
    expect_eq(isSafeMethod("POST"), false);
    expect_eq(isSafeMethod("PATCH"), false);
    expect_eq(isSafeMethod("get"), false);

    // Idempotent methods: safe + PUT + DELETE
    expect_eq(isIdempotentMethod("GET"), true);
    expect_eq(isIdempotentMethod("HEAD"), true);
    expect_eq(isIdempotentMethod("OPTIONS"), true);
    expect_eq(isIdempotentMethod("TRACE"), true);
    expect_eq(isIdempotentMethod("PUT"), true);
    expect_eq(isIdempotentMethod("DELETE"), true);
    expect_eq(isIdempotentMethod("POST"), false);
    expect_eq(isIdempotentMethod("PATCH"), false);
    expect_eq(isIdempotentMethod("CONNECT"), false);
});

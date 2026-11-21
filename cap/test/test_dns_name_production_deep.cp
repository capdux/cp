import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isValidDnsLabel,
    isValidHostname,
    encodeWireName,
    decodeWireName,
    isSubdomainOf,
    normalizeHostname,
} from "cap:dns_name";

test("1. isValidDnsLabel and isValidHostname enforce RFC 1035 / RFC 1123 rules", () => {
    expect_eq(isValidDnsLabel("api"), true);
    expect_eq(isValidDnsLabel("123-abc"), true);
    expect_eq(isValidDnsLabel("a12345678901234567890123456789012345678901234567890123456789012"), true); // 63
    expect_eq(isValidDnsLabel("a123456789012345678901234567890123456789012345678901234567890123"), false); // 64
    expect_eq(isValidDnsLabel(""), false);
    expect_eq(isValidDnsLabel("-lead"), false);
    expect_eq(isValidDnsLabel("trail-"), false);
    expect_eq(isValidDnsLabel("under_score"), false);

    expect_eq(isValidHostname("api.cp.dev"), true);
    expect_eq(isValidHostname("api.cp.dev."), true);
    expect_eq(isValidHostname("localhost"), true);
    expect_eq(isValidHostname(""), false);
    expect_eq(isValidHostname("."), false);
    expect_eq(isValidHostname(".leading.dot"), false);
    expect_eq(isValidHostname("double..dot.com"), false);
    expect_eq(isValidHostname("trailing.."), false);
});

test("2. encodeWireName and decodeWireName canonical RFC 1035 round-trip", () => {
    let wire_buf: [64]u8 = std.mem.zeroes([64]u8);
    let host_buf: [64]u8 = std.mem.zeroes([64]u8);

    const wire = encodeWireName("API.Cp.Dev.", &wire_buf) ?? unreachable;
    const expected: [12]u8 = [3, 'a', 'p', 'i', 2, 'c', 'p', 3, 'd', 'e', 'v', 0];
    expect_eq(std.mem.eql(u8, wire, &expected), true);

    const decoded = decodeWireName(wire, &host_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, decoded, "api.cp.dev"), true);

    // 根域 "." -> 单字节 0x00
    const root_wire = encodeWireName(".", &wire_buf) ?? unreachable;
    expect_eq(root_wire.len, usize(1));
    expect_eq(root_wire[0], u8(0));
    expect_eq((decodeWireName(root_wire, &host_buf) ?? unreachable).len, usize(0));
});

test("3. decodeWireName rejects compression pointers, truncated labels, and missing terminator", () => {
    let out: [64]u8 = std.mem.zeroes([64]u8);

    // 压缩指针 0xC0 0x0C
    const ptr_wire: [2]u8 = [0xC0, 0x0C];
    expect_eq(decodeWireName(&ptr_wire, &out) == null, true);

    // 标签截断（声称长度 5，实际只剩 2 字节）
    const trunc: [3]u8 = [5, 'a', 'b'];
    expect_eq(decodeWireName(&trunc, &out) == null, true);

    // 缺少 0x00 终止符
    const no_zero: [4]u8 = [3, 'a', 'b', 'c'];
    expect_eq(decodeWireName(&no_zero, &out) == null, true);

    // 空输入
    const empty: [0]u8 = [];
    expect_eq(decodeWireName(&empty, &out) == null, true);
});

test("4. isSubdomainOf case-insensitive matching, trailing dot, and anti-false-positive", () => {
    expect_eq(isSubdomainOf("a.b.com", "b.com"), true);
    expect_eq(isSubdomainOf("A.B.COM.", "b.com."), true);
    expect_eq(isSubdomainOf("a.b.com", "a.b.com"), true);
    expect_eq(isSubdomainOf("deep.sub.a.b.com", "b.com"), true);

    expect_eq(isSubdomainOf("notb.com", "b.com"), false);
    expect_eq(isSubdomainOf("b.com", "a.b.com"), false);
    expect_eq(isSubdomainOf("bad..host", "host"), false);
});

test("5. normalizeHostname and buffer-too-small safety guards", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const norm = normalizeHostname("Example.COM.", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, norm, "example.com"), true);
    expect_eq(normalizeHostname("bad..host", &buf) == null, true);

    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(normalizeHostname("example.com", &tiny) == null, true);
    expect_eq(encodeWireName("api.cp.dev", &tiny) == null, true);

    const wire: [12]u8 = [3, 'a', 'p', 'i', 2, 'c', 'p', 3, 'd', 'e', 'v', 0];
    expect_eq(decodeWireName(&wire, &tiny) == null, true);
});

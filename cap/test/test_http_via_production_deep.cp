import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ViaIter,
    viaHopCount,
    viaContainsProxy,
    appendVia,
    parseViaMaxForwards as parseMaxForwards,
    decrementMaxForwards,
} from "cap:http";

test("1. ViaIter parses protocol/version, received-by, and comments containing commas", () => {
    const hdr = "1.0 fred, 1.1 p.example.net (Apache/1.1, edge), HTTP/2.0 proxy:8080";
    let it = ViaIter.init(hdr);

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.protocol_name == null, true);
    expect_eq(std.mem.eql(u8, e0.effectiveProtocolName(), "HTTP"), true);
    expect_eq(std.mem.eql(u8, e0.protocol_version, "1.0"), true);
    expect_eq(std.mem.eql(u8, e0.received_by, "fred"), true);
    expect_eq(e0.comment == null, true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.protocol_version, "1.1"), true);
    expect_eq(std.mem.eql(u8, e1.received_by, "p.example.net"), true);
    expect_eq(std.mem.eql(u8, e1.comment ?? unreachable, "Apache/1.1, edge"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.protocol_name ?? unreachable, "HTTP"), true);
    expect_eq(std.mem.eql(u8, e2.protocol_version, "2.0"), true);
    expect_eq(std.mem.eql(u8, e2.received_by, "proxy:8080"), true);
    expect_eq(e2.comment == null, true);

    expect_eq(it.next() == null, true);
});

test("2. viaHopCount and case-insensitive viaContainsProxy loop detection", () => {
    const hdr = "1.1 Edge-1.CP.Dev, 2.0 gateway:8443 (cap-proxy)";
    expect_eq(viaHopCount(hdr), usize(2));
    expect_eq(viaContainsProxy(hdr, "edge-1.cp.dev"), true);
    expect_eq(viaContainsProxy(hdr, "GATEWAY:8443"), true);
    expect_eq(viaContainsProxy(hdr, "edge-2.cp.dev"), false);
    expect_eq(viaHopCount(""), usize(0));
});

test("3. appendVia builds initial and chained Via headers cleanly", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const v1 = appendVia("", "1.1", "edge1", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, v1, "1.1 edge1"), true);

    let buf2: [128]u8 = std.mem.zeroes([128]u8);
    const v2 = appendVia(v1, "HTTP/2.0", "edge2:443", &buf2) ?? unreachable;
    expect_eq(std.mem.eql(u8, v2, "1.1 edge1, HTTP/2.0 edge2:443"), true);
    expect_eq(viaHopCount(v2), usize(2));
});

test("4. parseMaxForwards and decrementMaxForwards enforce non-negative and zero stop", () => {
    expect_eq(parseMaxForwards("0") ?? unreachable, u32(0));
    expect_eq(parseMaxForwards("  10 ") ?? unreachable, u32(10));
    expect_eq(parseMaxForwards("") == null, true);
    expect_eq(parseMaxForwards("-1") == null, true);
    expect_eq(parseMaxForwards("10x") == null, true);
    expect_eq(parseMaxForwards("999999999999") == null, true);

    expect_eq(decrementMaxForwards(5) ?? unreachable, u32(4));
    expect_eq(decrementMaxForwards(1) ?? unreachable, u32(0));
    expect_eq(decrementMaxForwards(0) == null, true);
});

test("5. buffer-too-small and invalid appendVia arguments return null", () => {
    let tiny: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(appendVia("", "1.1", "proxy.local", &tiny) == null, true);
    expect_eq(appendVia("1.1 a", "1.1", "b", &tiny) == null, true);

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(appendVia("", "", "proxy", &buf) == null, true);
    expect_eq(appendVia("", "1.1", "", &buf) == null, true);
});

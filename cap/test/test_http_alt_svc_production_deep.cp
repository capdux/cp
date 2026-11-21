import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AltSvcHeaderEntry as AltSvcEntry,
    isAltSvcClear,
    AltSvcIter,
    formatAltSvcEntry,
} from "cap:http";

test("1. AltSvcIter parses multi-entry HTTP/3, HTTP/2, IPv6, ma, and persist parameters", () => {
    const hdr =
        "h3=\":443\"; ma=2592000; persist=1, "
        ++ "h3-29=\"[::1]:8443\"; ma=3600; persist=2, "
        ++ "h2=\"alt.example.com:8443\"";

    let it = AltSvcIter.init(hdr);

    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.protocol_id, "h3"), true);
    expect_eq(e0.host.len, usize(0));
    expect_eq(e0.port, u16(443));
    expect_eq(e0.max_age, u64(2592000));
    expect_eq(e0.persist, true);
    expect_eq(e0.isHttp3(), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.protocol_id, "h3-29"), true);
    expect_eq(std.mem.eql(u8, e1.host, "[::1]"), true);
    expect_eq(e1.port, u16(8443));
    expect_eq(e1.max_age, u64(3600));
    expect_eq(e1.persist, false); // persist != 1 ignored per RFC 7838
    expect_eq(e1.isHttp3(), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.protocol_id, "h2"), true);
    expect_eq(std.mem.eql(u8, e2.host, "alt.example.com"), true);
    expect_eq(e2.port, u16(8443));
    expect_eq(e2.max_age, u64(86400)); // default RFC 7838 max-age
    expect_eq(e2.persist, false);
    expect_eq(e2.isHttp3(), false);

    expect_eq(it.next() == null, true);
});

test("2. isAltSvcClear matches clear case-insensitively and yields empty AltSvcIter", () => {
    expect_eq(isAltSvcClear("clear"), true);
    expect_eq(isAltSvcClear("  CLEAR \t"), true);
    expect_eq(isAltSvcClear("h3=\":443\""), false);
    expect_eq(isAltSvcClear(""), false);

    let it = AltSvcIter.init("Clear");
    expect_eq(it.next() == null, true);
});

test("3. formatAltSvcEntry round-trips with AltSvcIter for same-origin, host, and IPv6", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const e1 = AltSvcEntry{
        protocol_id: "h3",
        host: "",
        port: 443,
        max_age: 2592000,
        persist: true,
    };
    const s1 = formatAltSvcEntry(e1, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1, "h3=\":443\"; ma=2592000; persist=1"), true);

    let it = AltSvcIter.init(s1);
    const p1 = it.next() ?? unreachable;
    expect_eq(p1.port, u16(443));
    expect_eq(p1.max_age, u64(2592000));
    expect_eq(p1.persist, true);

    // Default max_age=86400 and persist=false omits redundant params
    const e2 = AltSvcEntry{
        protocol_id: "h2",
        host: "[2001:db8::1]",
        port: 8443,
        max_age: 86400,
        persist: false,
    };
    const s2 = formatAltSvcEntry(e2, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s2, "h2=\"[2001:db8::1]:8443\""), true);
});

test("4. AltSvcIter rejects invalid ports (0, >65535, missing) and skips malformed entries", () => {
    const hdr =
        "h3=\":0\", "
        ++ "h3=\":65536\", "
        ++ "h3=\"example.com\", "
        ++ "h3=\"[::1]\", "
        ++ "h3=unquoted, "
        ++ "h2=\":443\"; ext=\"a, b\", "
        ++ "h3=\":8443\"";

    let it = AltSvcIter.init(hdr);

    const ok1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, ok1.protocol_id, "h2"), true);
    expect_eq(ok1.port, u16(443));

    const ok2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, ok2.protocol_id, "h3"), true);
    expect_eq(ok2.port, u16(8443));

    expect_eq(it.next() == null, true);
});

test("5. formatAltSvcEntry rejects invalid fields, zero port, and tiny buffers", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const zero_port = AltSvcEntry{
        protocol_id: "h3",
        host: "",
        port: 0,
        max_age: 86400,
        persist: false,
    };
    expect_eq(formatAltSvcEntry(zero_port, &buf) == null, true);

    const bad_proto = AltSvcEntry{
        protocol_id: "",
        host: "",
        port: 443,
        max_age: 86400,
        persist: false,
    };
    expect_eq(formatAltSvcEntry(bad_proto, &buf) == null, true);

    const good = AltSvcEntry{
        protocol_id: "h3",
        host: "cdn.example.com",
        port: 443,
        max_age: 2592000,
        persist: true,
    };
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(formatAltSvcEntry(good, &tiny) == null, true);
});

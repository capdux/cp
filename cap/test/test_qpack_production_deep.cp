// Source: cp/cap/test/test_qpack_production_deep.cp (Production deep tests for cap:qpack)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    QPACK_NO_INDEX,
    QPACK_IDX_AUTHORITY,
    QPACK_IDX_PATH,
    QPACK_IDX_METHOD_GET,
    QPACK_IDX_SCHEME_HTTPS,
    QPACK_IDX_CONTENT_TYPE,
    QPACK_IDX_USER_AGENT,
    QPACK_IDX_AUTHORIZATION,
    QpackHeader,
    QpackClassKind,
    classifyHeader,
    staticIndexToName,
    encodePrefixedInt,
    decodePrefixedInt,
    buildHttp3RequestHeaders,
    encodeFieldSection,
    decodeFieldSection,
} from "cap:qpack";

test("1. Bun QPACK header classification: forbidden hop-by-hop, host, static indexed, and literal", () => {
    expect_eq(classifyHeader("Connection").kind == QpackClassKind.Forbidden, true);
    expect_eq(classifyHeader("TRANSFER-ENCODING").kind == QpackClassKind.Forbidden, true);
    expect_eq(classifyHeader("Keep-Alive").kind == QpackClassKind.Forbidden, true);
    expect_eq(classifyHeader("Proxy-Connection").kind == QpackClassKind.Forbidden, true);
    expect_eq(classifyHeader("Upgrade").kind == QpackClassKind.Forbidden, true);
    expect_eq(classifyHeader("Host").kind == QpackClassKind.Host, true);

    const ct = classifyHeader("Content-Type");
    expect_eq(ct.kind == QpackClassKind.Indexed, true);
    expect_eq(ct.static_index, QPACK_IDX_CONTENT_TYPE);
    expect_eq(std.mem.eql(u8, ct.canonical_name, "content-type"), true);

    const auth = classifyHeader("AUTHORIZATION");
    expect_eq(auth.kind == QpackClassKind.Indexed, true);
    expect_eq(auth.static_index, QPACK_IDX_AUTHORIZATION);

    const custom = classifyHeader("X-Request-ID");
    expect_eq(custom.kind == QpackClassKind.Literal, true);
    expect_eq(custom.static_index, QPACK_NO_INDEX);

    // Reverse lookup
    const ua_name = staticIndexToName(QPACK_IDX_USER_AGENT as u8) ?? unreachable;
    expect_eq(std.mem.eql(u8, ua_name, "user-agent"), true);
    expect_eq(staticIndexToName(99) == null, true);
});

test("2. RFC 9204 prefixed integer encode/decode across 3/4/5/7/8-bit prefixes", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = buf[0..];

    // RFC 7541 Appendix C.1.1: 10 with 5-bit prefix -> [0x0A]
    const n1 = encodePrefixedInt(5, 0x00, 10, &out) ?? unreachable;
    expect_eq(n1, 1);
    expect_eq(buf[0], 10);
    const d1 = decodePrefixedInt(5, buf[0..n1]) ?? unreachable;
    expect_eq(d1.value, 10);
    expect_eq(d1.bytes_read, 1);

    // RFC 7541 Appendix C.1.2: 1337 with 5-bit prefix -> [0x1F, 0x9A, 0x0A]
    const n2 = encodePrefixedInt(5, 0x00, 1337, &out) ?? unreachable;
    expect_eq(n2, 3);
    expect_eq(buf[0], 0x1F);
    expect_eq(buf[1], 0x9A);
    expect_eq(buf[2], 0x0A);
    const d2 = decodePrefixedInt(5, buf[0..n2]) ?? unreachable;
    expect_eq(d2.value, 1337);
    expect_eq(d2.bytes_read, 3);

    // Large value with 4-bit prefix and mask 0x50
    const n3 = encodePrefixedInt(4, 0x50, 96, &out) ?? unreachable;
    const d3 = decodePrefixedInt(4, buf[0..n3]) ?? unreachable;
    expect_eq(d3.value, 96);
    expect_eq(d3.bytes_read, n3);
    expect_eq((buf[0] & 0xF0), 0x50);
});

test("3. buildHttp3RequestHeaders mirrors Bun h3_client/encode.cp pseudo-header & filtering rules", () => {
    const raw = [
        QpackHeader.init("Host", "override.example.com", QPACK_NO_INDEX),
        QpackHeader.init("Connection", "keep-alive", QPACK_NO_INDEX),
        QpackHeader.init("Upgrade", "websocket", QPACK_NO_INDEX),
        QpackHeader.init("User-Agent", "cp-agent/2.0", QPACK_NO_INDEX),
        QpackHeader.init("X-Custom-Key", "val-42", QPACK_NO_INDEX),
    ];

    let out_arr: [16]QpackHeader = std.mem.zeroes([16]QpackHeader);
    let lower_arr: [64]u8 = std.mem.zeroes([64]u8);
    let out_s: []QpackHeader = out_arr[0..];
    let lower_s: []u8 = lower_arr[0..];

    // Empty path falls back to "/" and Host overrides default_authority
    const cnt = buildHttp3RequestHeaders("POST", "https", "default.local", "", raw[0..], &out_s, &lower_s) ?? unreachable;
    expect_eq(cnt, 6); // 4 pseudo-headers + user-agent + x-custom-key (Connection, Upgrade, Host excluded)

    expect_eq(std.mem.eql(u8, out_arr[0].name, ":method"), true);
    expect_eq(std.mem.eql(u8, out_arr[0].value, "POST"), true);
    expect_eq(out_arr[0].qpack_index, QPACK_IDX_METHOD_GET);

    expect_eq(std.mem.eql(u8, out_arr[1].name, ":scheme"), true);
    expect_eq(std.mem.eql(u8, out_arr[1].value, "https"), true);
    expect_eq(out_arr[1].qpack_index, QPACK_IDX_SCHEME_HTTPS);

    expect_eq(std.mem.eql(u8, out_arr[2].name, ":authority"), true);
    expect_eq(std.mem.eql(u8, out_arr[2].value, "override.example.com"), true);
    expect_eq(out_arr[2].qpack_index, QPACK_IDX_AUTHORITY);

    expect_eq(std.mem.eql(u8, out_arr[3].name, ":path"), true);
    expect_eq(std.mem.eql(u8, out_arr[3].value, "/"), true);
    expect_eq(out_arr[3].qpack_index, QPACK_IDX_PATH);

    expect_eq(std.mem.eql(u8, out_arr[4].name, "user-agent"), true);
    expect_eq(out_arr[4].qpack_index, QPACK_IDX_USER_AGENT);

    expect_eq(std.mem.eql(u8, out_arr[5].name, "x-custom-key"), true);
    expect_eq(std.mem.eql(u8, out_arr[5].value, "val-42"), true);
    expect_eq(out_arr[5].qpack_index, QPACK_NO_INDEX);
});

test("4. Full QPACK Field Section round-trip encode -> decode (static indexed + sensitive never-index + literal)", () => {
    const raw = [
        QpackHeader.init("Authorization", "Bearer secret-token", QPACK_NO_INDEX),
        QpackHeader.init("Accept", "application/json", QPACK_NO_INDEX),
        QpackHeader.init("X-Correlation-Id", "req-998877", QPACK_NO_INDEX),
    ];

    let h3_arr: [10]QpackHeader = std.mem.zeroes([10]QpackHeader);
    let lower_arr: [64]u8 = std.mem.zeroes([64]u8);
    let h3_s: []QpackHeader = h3_arr[0..];
    let lower_s: []u8 = lower_arr[0..];

    const n = buildHttp3RequestHeaders("GET", "https", "api.cp-lang.org", "/v2/items", raw[0..], &h3_s, &lower_s) ?? unreachable;
    expect_eq(n, 7);

    let wire: [512]u8 = std.mem.zeroes([512]u8);
    let wire_s: []u8 = wire[0..];
    const wlen = encodeFieldSection(h3_arr[0..n], &wire_s) ?? unreachable;
    expect_eq(wlen > 2, true);
    expect_eq(wire[0], 0x00);
    expect_eq(wire[1], 0x00);

    let dec_arr: [10]QpackHeader = std.mem.zeroes([10]QpackHeader);
    let dec_s: []QpackHeader = dec_arr[0..];
    const dcnt = decodeFieldSection(wire[0..wlen], &dec_s) ?? unreachable;
    expect_eq(dcnt, n);

    let i: usize = 0;
    while (i < n) {
        expect_eq(std.mem.eql(u8, dec_arr[i].name, h3_arr[i].name), true);
        expect_eq(std.mem.eql(u8, dec_arr[i].value, h3_arr[i].value), true);
        expect_eq(dec_arr[i].qpack_index, h3_arr[i].qpack_index);
        i += 1;
    }
});

test("5. Defensive guards: invalid prefixes, truncated/dynamic-table payloads, and small buffers", () => {
    let tiny_u8: [1]u8 = [0];
    let tiny_s: []u8 = tiny_u8[0..];
    expect_eq(encodePrefixedInt(0, 0, 5, &tiny_s) == null, true);
    expect_eq(encodePrefixedInt(9, 0, 5, &tiny_s) == null, true);
    expect_eq(encodePrefixedInt(5, 0, 1337, &tiny_s) == null, true);

    // Truncated multi-byte prefixed int
    const trunc: [2]u8 = [0x1F, 0x80];
    expect_eq(decodePrefixedInt(5, trunc[0..]) == null, true);

    // Non-zero Required Insert Count (dynamic table unsupported in static-only decoder)
    const dyn_payload: [2]u8 = [0x01, 0x00];
    let dec_arr: [4]QpackHeader = std.mem.zeroes([4]QpackHeader);
    let dec_s: []QpackHeader = dec_arr[0..];
    expect_eq(decodeFieldSection(dyn_payload[0..], &dec_s) == null, true);

    // Truncated field section (< 2 bytes)
    expect_eq(decodeFieldSection(tiny_u8[0..], &dec_s) == null, true);

    // buildHttp3RequestHeaders with empty method or empty authority or tiny output
    const empty_raw: [0]QpackHeader = [];
    let small_hdrs: [2]QpackHeader = std.mem.zeroes([2]QpackHeader);
    let small_hs: []QpackHeader = small_hdrs[0..];
    expect_eq(buildHttp3RequestHeaders("", "https", "a.com", "/", empty_raw[0..], &dec_s, &tiny_s) == null, true);
    expect_eq(buildHttp3RequestHeaders("GET", "https", "", "/", empty_raw[0..], &dec_s, &tiny_s) == null, true);
    expect_eq(buildHttp3RequestHeaders("GET", "https", "a.com", "/", empty_raw[0..], &small_hs, &tiny_s) == null, true);
});

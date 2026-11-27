// cap/test/testCapProxyProto.cp
// Comprehensive unit tests for @cap/proxy-proto (HAProxy PROXY Protocol v1 & v2)

import { test, expect_eq } from "cap:test";
import {
    PP_OK,
    PP_INCOMPLETE,
    PP_INVALID,
    PROTO_UNKNOWN,
    PROTO_TCP4,
    PROTO_TCP6,
    V2_FAM_INET,
    V2_TRANS_STREAM,
    ProxyV1Header,
    ProxyV2Header,
    isV1Prefix,
    isV2Signature,
    parseV1,
    encodeV1,
    parseV2,
    encodeV2Ipv4,
} from "../../packages/proxy-protocol-js/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/proxy-proto — v1 TCP4, TCP6 & UNKNOWN encode/parse round-trip", () => {
        let buf: [128]u8 = undefined;
        let hdr: ProxyV1Header = undefined;

        const n4: usize = encodeV1(PROTO_TCP4, "192.0.2.1", "198.51.100.2", 56324, 443, buf[0..128]);
        expect_eq(sliceEq(buf[0..n4], "PROXY TCP4 192.0.2.1 198.51.100.2 56324 443\r\n"), true);
        expect_eq(isV1Prefix(buf[0..n4]), true);
        expect_eq(parseV1(buf[0..n4], &hdr), PP_OK);
        expect_eq(hdr.proto, PROTO_TCP4);
        expect_eq(sliceEq(hdr.src_ip, "192.0.2.1"), true);
        expect_eq(sliceEq(hdr.dst_ip, "198.51.100.2"), true);
        expect_eq(hdr.src_port, 56324 as u16);
        expect_eq(hdr.dst_port, 443 as u16);
        expect_eq(hdr.consumed, n4);

        const n6: usize = encodeV1(PROTO_TCP6, "2001:db8::1", "2001:db8::2", 12345, 80, buf[0..128]);
        expect_eq(parseV1(buf[0..n6], &hdr), PP_OK);
        expect_eq(hdr.proto, PROTO_TCP6);
        expect_eq(sliceEq(hdr.src_ip, "2001:db8::1"), true);

        const nu: usize = encodeV1(PROTO_UNKNOWN, "", "", 0, 0, buf[0..128]);
        expect_eq(sliceEq(buf[0..nu], "PROXY UNKNOWN\r\n"), true);
        expect_eq(parseV1(buf[0..nu], &hdr), PP_OK);
        expect_eq(hdr.proto, PROTO_UNKNOWN);
    });

    test("2. @cap/proxy-proto — v1 partial frame, >107-byte limit & invalid port rejection", () => {
        let hdr: ProxyV1Header = undefined;
        expect_eq(parseV1("PRO", &hdr), PP_INCOMPLETE);
        expect_eq(parseV1("PROXY TCP4 1.2.3.4 5.6.7.8 80 443", &hdr), PP_INCOMPLETE);
        expect_eq(parseV1("GET / HTTP/1.1\r\n", &hdr), PP_INVALID);
        expect_eq(parseV1("PROXY TCP4 1.2.3.4 5.6.7.8 70000 443\r\n", &hdr), PP_INVALID);
        expect_eq(parseV1("PROXY TCP4 1.2.3.4 5.6.7.8 080 443\r\n", &hdr), PP_INVALID);

        // Over 107 bytes without CRLF -> PP_INVALID
        let big: [110]u8 = undefined;
        const p = "PROXY TCP4 ";
        let i: usize = 0;
        while (i < p.len) {
            big[i] = p[i];
            i += 1;
        }
        while (i < 110) {
            big[i] = 'a' as u8;
            i += 1;
        }
        expect_eq(parseV1(big[0..110], &hdr), PP_INVALID);
    });

    test("3. @cap/proxy-proto — v2 12-byte signature & 28-byte IPv4/TCP encode/parse round-trip", () => {
        const src_ip: [4]u8 = [192, 168, 1, 10];
        const dst_ip: [4]u8 = [10, 0, 0, 1];
        let frame: [32]u8 = undefined;
        const n: usize = encodeV2Ipv4(src_ip, dst_ip, 54321, 443, frame[0..32]);
        expect_eq(n, 28 as usize);
        expect_eq(isV2Signature(frame[0..n]), true);

        let hdr: ProxyV2Header = undefined;
        expect_eq(parseV2(frame[0..n], &hdr), PP_OK);
        expect_eq(hdr.is_proxy, true);
        expect_eq(hdr.family, V2_FAM_INET);
        expect_eq(hdr.transport, V2_TRANS_STREAM);
        expect_eq(hdr.payload_len, 12 as u16);
        expect_eq(hdr.src_ipv4[0], 192 as u8);
        expect_eq(hdr.src_ipv4[3], 10 as u8);
        expect_eq(hdr.dst_ipv4[0], 10 as u8);
        expect_eq(hdr.dst_ipv4[3], 1 as u8);
        expect_eq(hdr.src_port, 54321 as u16);
        expect_eq(hdr.dst_port, 443 as u16);
        expect_eq(hdr.consumed, 28 as usize);
    });

    test("4. @cap/proxy-proto — v2 LOCAL command frame (health check)", () => {
        const local_frame: [16]u8 = [
            0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,
            0x20, 0x00, 0x00, 0x00,
        ];
        let hdr: ProxyV2Header = undefined;
        expect_eq(parseV2(local_frame[0..16], &hdr), PP_OK);
        expect_eq(hdr.is_proxy, false);
        expect_eq(hdr.payload_len, 0 as u16);
        expect_eq(hdr.consumed, 16 as usize);
    });

    test("5. @cap/proxy-proto — v2 partial frames return PP_INCOMPLETE", () => {
        const src_ip: [4]u8 = [1, 2, 3, 4];
        const dst_ip: [4]u8 = [5, 6, 7, 8];
        let frame: [28]u8 = undefined;
        * = encodeV2Ipv4(src_ip, dst_ip, 1000, 80, frame[0..28]);

        let hdr: ProxyV2Header = undefined;
        expect_eq(parseV2(frame[0..5], &hdr), PP_INCOMPLETE);
        expect_eq(parseV2(frame[0..14], &hdr), PP_INCOMPLETE);
        expect_eq(parseV2(frame[0..27], &hdr), PP_INCOMPLETE);
    });

    test("6. @cap/proxy-proto — tiny buffer protection on encoders", () => {
        let tiny: [10]u8 = undefined;
        expect_eq(encodeV1(PROTO_TCP4, "1.2.3.4", "5.6.7.8", 80, 443, tiny[0..10]), 0 as usize);
        expect_eq(encodeV1(PROTO_UNKNOWN, "", "", 0, 0, tiny[0..10]), 0 as usize);
        const ip: [4]u8 = [1, 2, 3, 4];
        expect_eq(encodeV2Ipv4(ip, ip, 80, 443, tiny[0..10]), 0 as usize);
    });
}

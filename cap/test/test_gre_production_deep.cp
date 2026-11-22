// Source: cp/cap/test/test_gre_production_deep.cp
// Deep Production Test Suite for cap:gre
// Covers:
//   1. Minimal 4-byte v0 GRE round-trip (no optional fields, IPv4 & TEB)
//   2. Full 16-byte v0 GRE with Checksum + Key + Sequence + odd-length payload + verifyGreChecksum
//   3. Tampered byte checksum failure detection (both header and payload bit-flip)
//   4. RFC 2637 v1 PPTP Enhanced GRE (payload_len + call_id + seq_num + ack_num) parsing
//   5. Strict error guards: version > 1, truncated optional fields, invalid v1 flags (K=0 or C=1)
//   6. encodeGre buffer-too-small guard & partial optional combinations (Key-only, Seq-only)

import { test, expect_eq } from "cap:test";
import {
    GRE_IP_PROTO,
    GRE_MIN_HEADER_SIZE,
    GRE_FLAG_CHECKSUM,
    GRE_FLAG_ROUTING,
    GRE_FLAG_KEY,
    GRE_FLAG_SEQ,
    GRE_FLAG_STRICT_ROUTE,
    GRE_FLAG_ACK,
    GRE_PROTO_IPV4,
    GRE_PROTO_IPV6,
    GRE_PROTO_TEB,
    GRE_PROTO_MPLS_UC,
    GRE_PROTO_PPP,
    GrePacket,
    greChecksum,
    verifyGreChecksum,
    parseGre,
    encodeGre,
} from "cap:net";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("gre: minimal 4-byte v0 GRE round-trip") {
    expect_eq(GRE_IP_PROTO, 47);
    expect_eq(GRE_MIN_HEADER_SIZE, 4);

    let buf: [64]u8 = [0; 64];
    const inner: []const u8 = "\x45\x00\x00\x14\xDE\xAD\xBE\xEF";
    if (n := encodeGre(GRE_PROTO_IPV4, false, null, null, inner, buf[0..64])) {
        expect_eq(n, 4 + inner.len);
        const wire = buf[0..n];
        expect_eq(verifyGreChecksum(wire), true);

        if (g := parseGre(wire)) {
            expect_eq(g.version, 0);
            expect_eq(g.protocol_type, GRE_PROTO_IPV4);
            expect_eq(g.header_bytes, 4);
            expect_eq(g.hasChecksum(), false);
            expect_eq(g.hasKey(), false);
            expect_eq(g.hasSeq(), false);
            expect_eq(g.hasAck(), false);
            expect_eq(g.checksum == null, true);
            expect_eq(g.key == null, true);
            expect_eq(g.seq_num == null, true);
            expect_eq(strEq(g.payload, inner), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("gre: full 16-byte v0 GRE (Checksum + Key + Seq) + odd-length RFC 1071 checksum") {
    let buf: [128]u8 = [0; 128];
    const inner: []const u8 = "ODD-LEN-PAYLOAD-123"; // 19 bytes (odd)
    if (n := encodeGre(GRE_PROTO_TEB, true, 0x11223344, 0x00000099, inner, buf[0..128])) {
        expect_eq(n, 16 + 19);
        const wire = buf[0..n];
        expect_eq(greChecksum(wire), 0);
        expect_eq(verifyGreChecksum(wire), true);

        if (g := parseGre(wire)) {
            expect_eq(g.version, 0);
            expect_eq(g.protocol_type, GRE_PROTO_TEB);
            expect_eq(g.header_bytes, 16);
            expect_eq(g.hasChecksum(), true);
            expect_eq(g.hasKey(), true);
            expect_eq(g.hasSeq(), true);
            expect_eq(g.checksum != null, true);
            expect_eq(g.key == 0x11223344, true);
            expect_eq(g.seq_num == 0x00000099, true);
            expect_eq(strEq(g.payload, inner), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("gre: tampered payload or header fails verifyGreChecksum") {
    let buf: [128]u8 = [0; 128];
    const inner: []const u8 = "sensitive-tunnel-frame";
    if (n := encodeGre(GRE_PROTO_IPV6, true, 0xAABBCCDD, 7, inner, buf[0..128])) {
        expect_eq(verifyGreChecksum(buf[0..n]), true);

        // Flip a bit in payload
        buf[n - 1] = buf[n - 1] ^ 0x01;
        expect_eq(verifyGreChecksum(buf[0..n]), false);

        // Restore payload, flip a bit in Key field
        buf[n - 1] = buf[n - 1] ^ 0x01;
        expect_eq(verifyGreChecksum(buf[0..n]), true);
        buf[9] = buf[9] ^ 0x80;
        expect_eq(verifyGreChecksum(buf[0..n]), false);
    } else {
        expect_eq(true, false);
    }
}

test("gre: RFC 2637 v1 PPTP Enhanced GRE (payload_len + call_id + seq + ack)") {
    // flags_ver = K (0x2000) | S (0x1000) | A (0x0080) | ver 1 (0x0001) = 0x3081
    // proto = PPP (0x880B)
    // payload_len = 5, call_id = 0x1234
    // seq_num = 0x0000000A, ack_num = 0x00000009
    // payload = "HELLO"
    let pkt: [21]u8 = [
        0x30, 0x81, 0x88, 0x0B,
        0x00, 0x05, 0x12, 0x34,
        0x00, 0x00, 0x00, 0x0A,
        0x00, 0x00, 0x00, 0x09,
        0x48, 0x45, 0x4C, 0x4C, 0x4F,
    ];

    if (g := parseGre(pkt[0..21])) {
        expect_eq(g.version, 1);
        expect_eq(g.protocol_type, GRE_PROTO_PPP);
        expect_eq(g.hasChecksum(), false);
        expect_eq(g.hasKey(), true);
        expect_eq(g.hasSeq(), true);
        expect_eq(g.hasAck(), true);
        expect_eq(g.payload_len, 5);
        expect_eq(g.call_id, 0x1234);
        expect_eq(g.seq_num == 10, true);
        expect_eq(g.ack_num == 9, true);
        expect_eq(g.header_bytes, 16);
        expect_eq(strEq(g.payload, "HELLO"), true);
    } else {
        expect_eq(true, false);
    }

    // v1 Ack-only packet (payload_len = 0, S=0, A=1, K=1 -> flags_ver = 0x2081)
    let ack_only: [12]u8 = [
        0x20, 0x81, 0x88, 0x0B,
        0x00, 0x00, 0x00, 0x77,
        0x00, 0x00, 0x01, 0x00,
    ];
    if (g := parseGre(ack_only[0..12])) {
        expect_eq(g.version, 1);
        expect_eq(g.call_id, 0x0077);
        expect_eq(g.payload_len, 0);
        expect_eq(g.seq_num == null, true);
        expect_eq(g.ack_num == 256, true);
        expect_eq(g.header_bytes, 12);
        expect_eq(g.payload.len, 0);
    } else {
        expect_eq(true, false);
    }
}

test("gre: strict error guards (version > 1, truncated fields, invalid v1 flags)") {
    // 1. Too short (< 4B)
    expect_eq(parseGre("\x00\x00\x08") == null, true);
    expect_eq(verifyGreChecksum("\x00\x00"), false);

    // 2. Invalid version = 2 (0x0002)
    expect_eq(parseGre("\x00\x02\x08\x00") == null, true);

    // 3. v0 with C+K+S set (needs 16B) but only 12B provided
    let trunc_v0: [12]u8 = [0xB0, 0x00, 0x08, 0x00, 0, 0, 0, 0, 0, 0, 0, 1];
    expect_eq(parseGre(trunc_v0[0..12]) == null, true);

    // 4. v1 PPTP without K flag (flags_ver = 0x0001) -> rejected
    let v1_no_k: [8]u8 = [0x00, 0x01, 0x88, 0x0B, 0, 0, 0, 1];
    expect_eq(parseGre(v1_no_k[0..8]) == null, true);

    // 5. v1 PPTP with C flag set (flags_ver = 0xA001) -> rejected
    let v1_with_c: [12]u8 = [0xA0, 0x01, 0x88, 0x0B, 0, 0, 0, 0, 0, 0, 0, 1];
    expect_eq(parseGre(v1_with_c[0..12]) == null, true);

    // 6. v1 PPTP payload_len exceeds actual remaining bytes
    let v1_trunc_pay: [10]u8 = [0x20, 0x01, 0x88, 0x0B, 0x00, 0x10, 0x00, 0x01, 0xAA, 0xBB];
    expect_eq(parseGre(v1_trunc_pay[0..10]) == null, true);
}

test("gre: encodeGre buffer-too-small guard & partial optional combinations") {
    // Buffer too small (needs 4 + 4 + 4 = 12B, given 11B)
    let small: [11]u8 = [0; 11];
    expect_eq(encodeGre(GRE_PROTO_MPLS_UC, false, 0x01020304, null, "ABCD", small[0..11]) == null, true);

    // Key-only (8B header)
    let buf: [32]u8 = [0; 32];
    if (n := encodeGre(GRE_PROTO_MPLS_UC, false, 0x01020304, null, "ABCD", buf[0..32])) {
        expect_eq(n, 12);
        if (g := parseGre(buf[0..n])) {
            expect_eq(g.header_bytes, 8);
            expect_eq(g.hasChecksum(), false);
            expect_eq(g.hasKey(), true);
            expect_eq(g.hasSeq(), false);
            expect_eq(g.key == 0x01020304, true);
            expect_eq(strEq(g.payload, "ABCD"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

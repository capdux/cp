// cap/test/testCapQuinnVarint.cp
// Comprehensive unit tests for @cap/quinn-varint

import { test, expect_eq } from "cap:test";
import {
    MAX_VARINT,
    PKT_INITIAL,
    PKT_ZERO_RTT,
    PKT_HANDSHAKE,
    PKT_RETRY,
    QUIC_VERSION_1,
    QuicVarint,
    QuicLongHeader,
    encodedLen,
    encodeVarint,
    decodeVarint,
    parseLongHeader
} from "../../packages/@cap/quinn-varint/src/index.cp";

fn bytesEq(a: []u8, b: []u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/quinn-varint — RFC 9000 Appendix A.1 official test vectors", () => {
        let buf: [8]u8 = undefined;

        // 37 -> 0x25 (1 byte)
        let n1 = encodeVarint(37, buf[0..8]);
        expect_eq(n1, 1 as usize);
        expect_eq(buf[0], 0x25 as u8);
        let d1 = decodeVarint(buf[0..n1]);
        expect_eq(d1.ok, true);
        expect_eq(d1.bytes_read, 1 as usize);
        expect_eq(d1.val, 37 as u64);

        // 15293 -> 0x7bbd (2 bytes)
        let n2 = encodeVarint(15293, buf[0..8]);
        expect_eq(n2, 2 as usize);
        let exp2: [2]u8 = [0x7b, 0xbd];
        expect_eq(bytesEq(buf[0..n2], exp2[0..2]), true);
        let d2 = decodeVarint(buf[0..n2]);
        expect_eq(d2.ok, true);
        expect_eq(d2.bytes_read, 2 as usize);
        expect_eq(d2.val, 15293 as u64);

        // 494878333 -> 0x9d7f3e7d (4 bytes)
        let n3 = encodeVarint(494878333, buf[0..8]);
        expect_eq(n3, 4 as usize);
        let exp3: [4]u8 = [0x9d, 0x7f, 0x3e, 0x7d];
        expect_eq(bytesEq(buf[0..n3], exp3[0..4]), true);
        let d3 = decodeVarint(buf[0..n3]);
        expect_eq(d3.ok, true);
        expect_eq(d3.bytes_read, 4 as usize);
        expect_eq(d3.val, 494878333 as u64);

        // 151288809941952652 -> 0xc2197c5eff14e88c (8 bytes)
        let n4 = encodeVarint(151288809941952652, buf[0..8]);
        expect_eq(n4, 8 as usize);
        let exp4: [8]u8 = [0xc2, 0x19, 0x7c, 0x5e, 0xff, 0x14, 0xe8, 0x8c];
        expect_eq(bytesEq(buf[0..n4], exp4[0..8]), true);
        let d4 = decodeVarint(buf[0..n4]);
        expect_eq(d4.ok, true);
        expect_eq(d4.bytes_read, 8 as usize);
        expect_eq(d4.val, 151288809941952652 as u64);

        // Non-minimal 2-byte encoding of 37 (0x4025) decodes cleanly per RFC 9000
        let nonmin: [2]u8 = [0x40, 0x25];
        let d5 = decodeVarint(nonmin[0..2]);
        expect_eq(d5.ok, true);
        expect_eq(d5.bytes_read, 2 as usize);
        expect_eq(d5.val, 37 as u64);
    });

    test("2. @cap/quinn-varint — 4-tier boundaries (0, 63, 64, 16383, 16384, 1073741823, MAX_VARINT)", () => {
        expect_eq(encodedLen(0), 1 as usize);
        expect_eq(encodedLen(63), 1 as usize);
        expect_eq(encodedLen(64), 2 as usize);
        expect_eq(encodedLen(16383), 2 as usize);
        expect_eq(encodedLen(16384), 4 as usize);
        expect_eq(encodedLen(1073741823), 4 as usize);
        expect_eq(encodedLen(1073741824), 8 as usize);
        expect_eq(encodedLen(MAX_VARINT), 8 as usize);

        let vals: [7]u64 = [0, 63, 64, 16383, 16384, 1073741823, MAX_VARINT];
        let buf: [8]u8 = undefined;
        let i: usize = 0;
        while (i < 7) {
            let v = vals[i];
            let n = encodeVarint(v, buf[0..8]);
            expect_eq(n, encodedLen(v));
            let d = decodeVarint(buf[0..n]);
            expect_eq(d.ok, true);
            expect_eq(d.bytes_read, n);
            expect_eq(d.val, v);
            i = i + 1;
        }
    });

    test("3. @cap/quinn-varint — Reject values > MAX_VARINT & undersized encode buffer", () => {
        let over: u64 = MAX_VARINT + 1;
        expect_eq(encodedLen(over), 0 as usize);

        let buf: [8]u8 = undefined;
        expect_eq(encodeVarint(over, buf[0..8]), 0 as usize);

        // Buffer too small for 2-byte, 4-byte, 8-byte encodings
        expect_eq(encodeVarint(64, buf[0..1]), 0 as usize);
        expect_eq(encodeVarint(16384, buf[0..3]), 0 as usize);
        expect_eq(encodeVarint(MAX_VARINT, buf[0..7]), 0 as usize);
    });

    test("4. @cap/quinn-varint — Truncated & empty decode buffers fail safely", () => {
        let empty: [1]u8 = [0];
        expect_eq(decodeVarint(empty[0..0]).ok, false);

        // 2-byte prefix (0x40) with only 1 byte available
        let t2: [1]u8 = [0x7b];
        expect_eq(decodeVarint(t2[0..1]).ok, false);

        // 4-byte prefix (0x80) with only 3 bytes available
        let t4: [3]u8 = [0x9d, 0x7f, 0x3e];
        expect_eq(decodeVarint(t4[0..3]).ok, false);

        // 8-byte prefix (0xC0) with only 7 bytes available
        let t8: [7]u8 = [0xc2, 0x19, 0x7c, 0x5e, 0xff, 0x14, 0xe8];
        expect_eq(decodeVarint(t8[0..7]).ok, false);
    });

    test("5. @cap/quinn-varint — parseLongHeader for QUIC v1 Initial & Handshake packets", () => {
        // QUIC v1 Initial packet:
        // byte 0: 0xC3 (Form=1, Fixed=1, Type=00 Initial, Reserved/PNLen=0011)
        // bytes 1..4: 0x00000001 (QUIC v1)
        // byte 5: dcid_len = 8, bytes 6..13: DCID
        // byte 14: scid_len = 4, bytes 15..18: SCID
        // total header_len = 1 + 4 + 1 + 8 + 1 + 4 = 19
        let pkt: [20]u8 = [
            0xC3,
            0x00, 0x00, 0x00, 0x01,
            0x08, 0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08,
            0x04, 0xaa, 0xbb, 0xcc, 0xdd,
            0x00
        ];
        let hdr = parseLongHeader(pkt[0..20]);
        expect_eq(hdr.ok, true);
        expect_eq(hdr.is_long, true);
        expect_eq(hdr.packet_type, PKT_INITIAL);
        expect_eq(hdr.version, QUIC_VERSION_1);
        expect_eq(hdr.dcid_len, 8 as u8);
        expect_eq(hdr.scid_len, 4 as u8);
        expect_eq(hdr.header_len, 19 as usize);

        // QUIC v1 Handshake packet: 0xE0 (Form=1, Fixed=1, Type=10 Handshake)
        let hs: [9]u8 = [0xE0, 0x00, 0x00, 0x00, 0x01, 0x02, 0x11, 0x22, 0x00];
        let hh = parseLongHeader(hs[0..9]);
        expect_eq(hh.ok, true);
        expect_eq(hh.is_long, true);
        expect_eq(hh.packet_type, PKT_HANDSHAKE);
        expect_eq(hh.dcid_len, 2 as u8);
        expect_eq(hh.scid_len, 0 as u8);
        expect_eq(hh.header_len, 9 as usize);
    });

    test("6. @cap/quinn-varint — Short Header discrimination & malformed header rejection", () => {
        // Short Header (1-RTT): Form=0, Fixed=1 -> 0x43
        let short_pkt: [4]u8 = [0x43, 0x01, 0x02, 0x03];
        let sh = parseLongHeader(short_pkt[0..4]);
        expect_eq(sh.ok, true);
        expect_eq(sh.is_long, false);
        expect_eq(sh.header_len, 1 as usize);

        // Short Header with Fixed=0 -> invalid
        let bad_short: [2]u8 = [0x03, 0x01];
        expect_eq(parseLongHeader(bad_short[0..2]).ok, false);

        // Long Header with Fixed=0 on QUIC v1 -> invalid
        let bad_fixed: [8]u8 = [0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00];
        expect_eq(parseLongHeader(bad_fixed[0..8]).ok, false);

        // Truncated DCID -> invalid
        let trunc_dcid: [8]u8 = [0xC0, 0x00, 0x00, 0x00, 0x01, 0x08, 0xaa, 0xbb];
        expect_eq(parseLongHeader(trunc_dcid[0..8]).ok, false);

        // Empty packet -> invalid
        expect_eq(parseLongHeader(short_pkt[0..0]).ok, false);
    });
}

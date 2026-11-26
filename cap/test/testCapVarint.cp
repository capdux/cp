// cap/test/testCapVarint.cp
// Comprehensive unit tests for @cap/varint

import { test, expect_eq } from "cap:test";
import {
    QUIC_VARINT_MAX,
    U64Decode,
    I64Decode,
    uleb128Size,
    encodeUleb128,
    decodeUleb128,
    zigzagEncode,
    zigzagDecode,
    encodeSleb128,
    decodeSleb128,
    encodeZigzagVarint,
    decodeZigzagVarint,
    encodeQuicVarint,
    decodeQuicVarint,
} from "../../packages/@cap/varint/src/index.cp";

fn main() {
    test("1. @cap/varint — ULEB128 encoding, decoding & 64-bit bounds", () => {
        let buf: [16]u8 = undefined;

        // Wikipedia / DWARF standard vector: 624485 -> 0xE5 0x8E 0x26
        expect_eq(uleb128Size(624485), 3 as usize);
        const n = encodeUleb128(624485, buf[0..16]).?;
        expect_eq(n, 3 as usize);
        expect_eq(buf[0], 0xE5 as u8);
        expect_eq(buf[1], 0x8E as u8);
        expect_eq(buf[2], 0x26 as u8);

        const d = decodeUleb128(buf[0..n]);
        expect_eq(d.ok, true);
        expect_eq(d.value, 624485 as u64);
        expect_eq(d.bytes_read, 3 as usize);

        // u64::MAX -> 10 bytes
        const u64_max: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(uleb128Size(u64_max), 10 as usize);
        const nmax = encodeUleb128(u64_max, buf[0..16]).?;
        expect_eq(nmax, 10 as usize);
        const dmax = decodeUleb128(buf[0..nmax]);
        expect_eq(dmax.ok, true);
        expect_eq(dmax.value, u64_max);
    });

    test("2. @cap/varint — ULEB128 overflow & truncated stream rejection", () => {
        // 10th byte > 0x01 overflows 64 bits
        let ov: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
        expect_eq(decodeUleb128(ov[0..10]).ok, false);

        // Truncated stream (MSB still 1 at EOF)
        let trunc: [2]u8 = [0xE5, 0x8E];
        expect_eq(decodeUleb128(trunc[0..2]).ok, false);
        expect_eq(decodeUleb128("").ok, false);
    });

    test("3. @cap/varint — ZigZag full i64 range (0, -1, 1, -2, i64::MAX, i64::MIN)", () => {
        expect_eq(zigzagEncode(0), 0 as u64);
        expect_eq(zigzagEncode(-1), 1 as u64);
        expect_eq(zigzagEncode(1), 2 as u64);
        expect_eq(zigzagEncode(-2), 3 as u64);

        const i64_max: i64 = 9223372036854775807;
        const i64_min: i64 = -9223372036854775807 - 1;
        expect_eq(zigzagDecode(zigzagEncode(i64_max)), i64_max);
        expect_eq(zigzagDecode(zigzagEncode(i64_min)), i64_min);

        let buf: [16]u8 = undefined;
        const n = encodeZigzagVarint(-123456, buf[0..16]).?;
        const d = decodeZigzagVarint(buf[0..n]);
        expect_eq(d.ok, true);
        expect_eq(d.value, -123456 as i64);
    });

    test("4. @cap/varint — SLEB128 negative & extreme 64-bit round-trip", () => {
        let buf: [16]u8 = undefined;

        // DWARF standard vector: -123456 -> 0xC0 0xBB 0x78
        const n = encodeSleb128(-123456, buf[0..16]).?;
        expect_eq(n, 3 as usize);
        expect_eq(buf[0], 0xC0 as u8);
        expect_eq(buf[1], 0xBB as u8);
        expect_eq(buf[2], 0x78 as u8);

        const d = decodeSleb128(buf[0..n]);
        expect_eq(d.ok, true);
        expect_eq(d.value, -123456 as i64);

        const i64_min: i64 = -9223372036854775807 - 1;
        const nmin = encodeSleb128(i64_min, buf[0..16]).?;
        expect_eq(nmin, 10 as usize);
        const dmin = decodeSleb128(buf[0..nmin]);
        expect_eq(dmin.ok, true);
        expect_eq(dmin.value, i64_min);
    });

    test("5. @cap/varint — QUIC RFC 9000 Appendix A.1 official 4 test vectors", () => {
        let buf: [16]u8 = undefined;

        // 1-byte: 37 -> 0x25
        const n1 = encodeQuicVarint(37, buf[0..16]).?;
        expect_eq(n1, 1 as usize);
        expect_eq(buf[0], 0x25 as u8);
        expect_eq(decodeQuicVarint(buf[0..n1]).value, 37 as u64);

        // 2-byte: 15293 -> 0x7b 0xbd
        const n2 = encodeQuicVarint(15293, buf[0..16]).?;
        expect_eq(n2, 2 as usize);
        expect_eq(buf[0], 0x7b as u8);
        expect_eq(buf[1], 0xbd as u8);
        expect_eq(decodeQuicVarint(buf[0..n2]).value, 15293 as u64);

        // 4-byte: 494878333 -> 0x9d 0x7f 0x3e 0x7d
        const n4 = encodeQuicVarint(494878333, buf[0..16]).?;
        expect_eq(n4, 4 as usize);
        expect_eq(buf[0], 0x9d as u8);
        expect_eq(buf[1], 0x7f as u8);
        expect_eq(buf[2], 0x3e as u8);
        expect_eq(buf[3], 0x7d as u8);
        expect_eq(decodeQuicVarint(buf[0..n4]).value, 494878333 as u64);

        // 8-byte: 151288809941952652 -> c2 19 7c 5e ff 14 e8 8c
        const v8: u64 = 151288809941952652;
        const n8 = encodeQuicVarint(v8, buf[0..16]).?;
        expect_eq(n8, 8 as usize);
        expect_eq(buf[0], 0xc2 as u8);
        expect_eq(buf[1], 0x19 as u8);
        expect_eq(buf[2], 0x7c as u8);
        expect_eq(buf[3], 0x5e as u8);
        expect_eq(buf[4], 0xff as u8);
        expect_eq(buf[5], 0x14 as u8);
        expect_eq(buf[6], 0xe8 as u8);
        expect_eq(buf[7], 0x8c as u8);
        expect_eq(decodeQuicVarint(buf[0..n8]).value, v8);
    });

    test("6. @cap/varint — QUIC > 2^62-1 overflow & truncated stream rejection", () => {
        let buf: [16]u8 = undefined;
        expect_eq(encodeQuicVarint(QUIC_VARINT_MAX, buf[0..16]) != null, true);
        expect_eq(encodeQuicVarint(QUIC_VARINT_MAX + 1, buf[0..16]) == null, true);

        // 4-byte prefix (0x80) with only 2 bytes available -> fail
        let short_q: [2]u8 = [0x9d, 0x7f];
        expect_eq(decodeQuicVarint(short_q[0..2]).ok, false);
        expect_eq(decodeQuicVarint("").ok, false);
    });
}

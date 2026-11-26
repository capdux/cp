// cap/test/testCapDnsPacket.cp
// Comprehensive unit tests for @cap/dns-packet (RFC 1035 DNS binary codec)

import { test, expect_eq } from "cap:test";
import {
    TYPE_A,
    TYPE_AAAA,
    TYPE_TXT,
    CLASS_IN,
    DnsHeader,
    NameDecodeResult,
    encodeHeader,
    decodeHeader,
    encodeName,
    decodeName,
    encodeQuery,
} from "../../packages/@cap/dns-packet/src/index.cp";

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
    test("1. @cap/dns-packet — 12-byte big-endian DnsHeader encode & decode", () => {
        const hdr = DnsHeader {
            id: 0xABCD,
            flags: 0x8180,
            qdcount: 1,
            ancount: 2,
            nscount: 0,
            arcount: 1,
        };
        let buf: [16]u8 = undefined;
        const n: usize = encodeHeader(hdr, buf[0..16]);
        expect_eq(n, 12 as usize);
        expect_eq(buf[0], 0xAB as u8);
        expect_eq(buf[1], 0xCD as u8);
        expect_eq(buf[2], 0x81 as u8);
        expect_eq(buf[3], 0x80 as u8);

        const out: DnsHeader = decodeHeader(buf[0..12]);
        expect_eq(out.id, 0xABCD as u16);
        expect_eq(out.flags, 0x8180 as u16);
        expect_eq(out.qdcount, 1 as u16);
        expect_eq(out.ancount, 2 as u16);
        expect_eq(out.nscount, 0 as u16);
        expect_eq(out.arcount, 1 as u16);

        // Too small buffer returns 0
        let tiny: [8]u8 = undefined;
        expect_eq(encodeHeader(hdr, tiny[0..8]), 0 as usize);
    });

    test("2. @cap/dns-packet — domain label encoding with root '.' & trailing dot normalization", () => {
        let buf: [64]u8 = undefined;
        const n1: i64 = encodeName("api.capdux.org", buf[0..64]);
        expect_eq(n1, 16 as i64);
        expect_eq(buf[0], 3 as u8);
        expect_eq(buf[4], 6 as u8);
        expect_eq(buf[11], 3 as u8);
        expect_eq(buf[15], 0 as u8);

        // Trailing dot produces identical wire bytes
        let buf2: [64]u8 = undefined;
        const n2: i64 = encodeName("api.capdux.org.", buf2[0..64]);
        expect_eq(n2, n1);
        expect_eq(sliceEq(buf[0..16], buf2[0..16]), true);

        // Root domain "." and ""
        const n_root: i64 = encodeName(".", buf[0..64]);
        expect_eq(n_root, 1 as i64);
        expect_eq(buf[0], 0 as u8);

        const n_empty: i64 = encodeName("", buf[0..64]);
        expect_eq(n_empty, 1 as i64);
        expect_eq(buf[0], 0 as u8);
    });

    test("3. @cap/dns-packet — RFC 1035 0xC0 multi-hop pointer compression decode", () => {
        // Synthetic packet:
        // offset 0: \x03org\x00                     (5 bytes: 0..5)
        // offset 5: \x06capdux\xC0\x00              (9 bytes: 5..14 -> "capdux.org")
        // offset 14: \x03api\xC0\x05                (6 bytes: 14..20 -> "api.capdux.org")
        const pkt: [20]u8 = [
            3, 'o' as u8, 'r' as u8, 'g' as u8, 0,
            6, 'c' as u8, 'a' as u8, 'p' as u8, 'd' as u8, 'u' as u8, 'x' as u8, 0xC0, 0x00,
            3, 'a' as u8, 'p' as u8, 'i' as u8, 0xC0, 0x05,
        ];
        let out: [64]u8 = undefined;
        const r: NameDecodeResult = decodeName(pkt[0..20], 14, out[0..64]);
        expect_eq(r.ok, true);
        expect_eq(r.next_offset, 20 as usize);
        expect_eq(sliceEq(out[0..r.name_len], "api.capdux.org"), true);
    });

    test("4. @cap/dns-packet — cycle guard rejects self-referencing compression pointer", () => {
        // Offset 0 points directly to offset 0 (0xC0 0x00)
        const loop1: [2]u8 = [0xC0, 0x00];
        let out: [64]u8 = undefined;
        expect_eq(decodeName(loop1[0..2], 0, out[0..64]).ok, false);

        // Two-node mutual pointer loop: 0 -> 2, 2 -> 0
        const loop2: [4]u8 = [0xC0, 0x02, 0xC0, 0x00];
        expect_eq(decodeName(loop2[0..4], 0, out[0..64]).ok, false);
    });

    test("5. @cap/dns-packet — encodeQuery builds standard A/AAAA/TXT query packets", () => {
        let pkt: [128]u8 = undefined;
        const len_a: i64 = encodeQuery(0x4242, "example.com", TYPE_A, true, pkt[0..128]);
        // 12 header + 13 name (\x07example\x03com\x00) + 4 qtype/qclass = 29
        expect_eq(len_a, 29 as i64);

        const hdr: DnsHeader = decodeHeader(pkt[0..12]);
        expect_eq(hdr.id, 0x4242 as u16);
        expect_eq(hdr.flags, 0x0100 as u16);
        expect_eq(hdr.qdcount, 1 as u16);

        let name_buf: [64]u8 = undefined;
        const nr: NameDecodeResult = decodeName(pkt[0..(len_a as usize)], 12, name_buf[0..64]);
        expect_eq(nr.ok, true);
        expect_eq(sliceEq(name_buf[0..nr.name_len], "example.com"), true);
        expect_eq(pkt[nr.next_offset], 0 as u8);
        expect_eq(pkt[nr.next_offset + 1], TYPE_A as u8);
        expect_eq(pkt[nr.next_offset + 2], 0 as u8);
        expect_eq(pkt[nr.next_offset + 3], CLASS_IN as u8);

        // AAAA & TXT with rd=false
        const len_aaaa: i64 = encodeQuery(0x0001, "v6.test", TYPE_AAAA, false, pkt[0..128]);
        expect_eq(len_aaaa > 16, true);
        expect_eq(decodeHeader(pkt[0..12]).flags, 0 as u16);
        expect_eq(pkt[(len_aaaa as usize) - 3], TYPE_AAAA as u8);

        const len_txt: i64 = encodeQuery(0x0002, "txt.test", TYPE_TXT, true, pkt[0..128]);
        expect_eq(len_txt > 16, true);
        expect_eq(pkt[(len_txt as usize) - 3], TYPE_TXT as u8);
    });

    test("6. @cap/dns-packet — rejects >63-byte labels, empty labels & tiny buffers", () => {
        let long_label: [64]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            long_label[i] = 'a' as u8;
            i += 1;
        }
        let dst: [128]u8 = undefined;
        expect_eq(encodeName(long_label[0..64], dst[0..128]), -1 as i64);
        expect_eq(encodeName("a..b", dst[0..128]), -1 as i64);
        expect_eq(encodeName(".leading", dst[0..128]), -1 as i64);

        let tiny: [5]u8 = undefined;
        expect_eq(encodeName("example.com", tiny[0..5]), -1 as i64);
        expect_eq(encodeQuery(1, "example.com", TYPE_A, true, tiny[0..5]), -1 as i64);
    });
}

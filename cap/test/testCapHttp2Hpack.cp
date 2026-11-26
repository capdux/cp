// cap/test/testCapHttp2Hpack.cp
// Comprehensive unit tests for @cap/http2-hpack

import { test, expect_eq } from "cap:test";
import {
    staticTableGet,
    staticTableFind,
    staticTableFindName,
    encodePrefixedInt,
    decodePrefixedInt,
    encodeIndexedHeader,
} from "../../packages/@cap/http2-hpack/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/http2-hpack — Static table key entries (1, 2, 3, 4, 8, 16, 61)", () => {
        const e1 = staticTableGet(1);
        expect_eq(e1.valid, true);
        expect_eq(sliceEq(e1.name, ":authority"), true);
        expect_eq(e1.value.len, 0 as usize);

        const e2 = staticTableGet(2);
        expect_eq(sliceEq(e2.name, ":method"), true);
        expect_eq(sliceEq(e2.value, "GET"), true);

        const e3 = staticTableGet(3);
        expect_eq(sliceEq(e3.name, ":method"), true);
        expect_eq(sliceEq(e3.value, "POST"), true);

        const e4 = staticTableGet(4);
        expect_eq(sliceEq(e4.name, ":path"), true);
        expect_eq(sliceEq(e4.value, "/"), true);

        const e8 = staticTableGet(8);
        expect_eq(sliceEq(e8.name, ":status"), true);
        expect_eq(sliceEq(e8.value, "200"), true);

        const e16 = staticTableGet(16);
        expect_eq(sliceEq(e16.name, "accept-encoding"), true);
        expect_eq(sliceEq(e16.value, "gzip, deflate"), true);

        const e61 = staticTableGet(61);
        expect_eq(e61.valid, true);
        expect_eq(sliceEq(e61.name, "www-authenticate"), true);

        expect_eq(staticTableGet(0).valid, false);
        expect_eq(staticTableGet(62).valid, false);
    });

    test("2. @cap/http2-hpack — staticTableFind & staticTableFindName", () => {
        expect_eq(staticTableFind(":method", "POST"), 3 as usize);
        expect_eq(staticTableFind(":status", "404"), 13 as usize);
        expect_eq(staticTableFind(":scheme", "https"), 7 as usize);
        expect_eq(staticTableFind(":status", "418"), 0 as usize);

        expect_eq(staticTableFindName(":status"), 8 as usize);
        expect_eq(staticTableFindName("content-type"), 31 as usize);
        expect_eq(staticTableFindName("x-custom-header"), 0 as usize);
    });

    test("3. @cap/http2-hpack — RFC 7541 C.1.1 (10 in 5-bit) & C.1.2 (1337 in 5-bit)", () => {
        let buf1: [8]u8 = undefined;
        const n1 = encodePrefixedInt(10 as u64, 5 as u8, 0x00 as u8, buf1[0..8]);
        expect_eq(n1, 1 as usize);
        expect_eq(buf1[0], 0x0A as u8);

        let buf2: [8]u8 = undefined;
        const n2 = encodePrefixedInt(1337 as u64, 5 as u8, 0x00 as u8, buf2[0..8]);
        expect_eq(n2, 3 as usize);
        expect_eq(buf2[0], 0x1F as u8);
        expect_eq(buf2[1], 0x9A as u8);
        expect_eq(buf2[2], 0x0A as u8);

        const d2 = decodePrefixedInt(buf2[0..n2], 5 as u8);
        expect_eq(d2.valid, true);
        expect_eq(d2.value, 1337 as u64);
        expect_eq(d2.consumed, 3 as usize);
    });

    test("4. @cap/http2-hpack — RFC 7541 C.1.3 (42 in 8-bit) & large integer round-trip", () => {
        let buf1: [8]u8 = undefined;
        const n1 = encodePrefixedInt(42 as u64, 8 as u8, 0x00 as u8, buf1[0..8]);
        expect_eq(n1, 1 as usize);
        expect_eq(buf1[0], 42 as u8);
        const d1 = decodePrefixedInt(buf1[0..n1], 8 as u8);
        expect_eq(d1.valid, true);
        expect_eq(d1.value, 42 as u64);

        let buf2: [16]u8 = undefined;
        const n2 = encodePrefixedInt(1000000 as u64, 4 as u8, 0xF0 as u8, buf2[0..16]);
        expect_eq(n2 > (1 as usize), true);
        expect_eq((buf2[0] & (0xF0 as u8)), 0xF0 as u8);
        const d2 = decodePrefixedInt(buf2[0..n2], 4 as u8);
        expect_eq(d2.valid, true);
        expect_eq(d2.value, 1000000 as u64);
        expect_eq(d2.consumed, n2);
    });

    test("5. @cap/http2-hpack — encodeIndexedHeader for small & multi-byte indices", () => {
        let buf: [8]u8 = undefined;
        // Index 2 (:method: GET) -> 0x82
        const n1 = encodeIndexedHeader(2, buf[0..8]);
        expect_eq(n1, 1 as usize);
        expect_eq(buf[0], 0x82 as u8);

        // Index 130 (>= 127) -> [0xFF, 0x03]
        const n2 = encodeIndexedHeader(130, buf[0..8]);
        expect_eq(n2, 2 as usize);
        expect_eq(buf[0], 0xFF as u8);
        expect_eq(buf[1], 0x03 as u8);

        const d2 = decodePrefixedInt(buf[0..n2], 7 as u8);
        expect_eq(d2.valid, true);
        expect_eq(d2.value, 130 as u64);

        // Index 0 is invalid.
        expect_eq(encodeIndexedHeader(0, buf[0..8]), 0 as usize);
    });

    test("6. @cap/http2-hpack — Truncated, out-of-range prefix & tiny buffer rejection", () => {
        let trunc: [2]u8 = undefined;
        trunc[0] = 0x1F as u8;
        trunc[1] = 0x9A as u8; // continuation bit still set, missing last byte
        expect_eq(decodePrefixedInt(trunc[0..2], 5 as u8).valid, false);
        expect_eq(decodePrefixedInt(trunc[0..0], 5 as u8).valid, false);
        expect_eq(decodePrefixedInt(trunc[0..2], 0 as u8).valid, false);
        expect_eq(decodePrefixedInt(trunc[0..2], 9 as u8).valid, false);

        let tiny: [2]u8 = undefined;
        // 1337 in 5-bit needs 3 bytes, tiny buffer has 2.
        expect_eq(encodePrefixedInt(1337 as u64, 5 as u8, 0x00 as u8, tiny[0..2]), 0 as usize);
        expect_eq(encodePrefixedInt(10 as u64, 0 as u8, 0x00 as u8, tiny[0..2]), 0 as usize);
    });
}

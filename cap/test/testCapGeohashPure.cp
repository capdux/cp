// cap/test/testCapGeohashPure.cp
// Comprehensive unit tests for @cap/geohash-pure

import { test, expect_eq } from "cap:test";
import {
    encode,
    decodeBounds,
    decode,
    isValidGeohash,
} from "../../packages/@cap/geohash-pure/src/index.cp";

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

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/geohash-pure — Classic Wikipedia vector encode(42.6, -5.6, 5) == 'ezs42'", () => {
        let buf: [12]u8 = undefined;
        const n = encode(42.6 as f64, -5.6 as f64, 5, buf[0..12]);
        expect_eq(n, 5 as usize);
        expect_eq(sliceEq(buf[0..5], "ezs42"), true);

        const pt = decode("ezs42");
        expect_eq(pt.valid, true);
        expect_eq(near(pt.lat, 42.6 as f64, 0.03 as f64), true);
        expect_eq(near(pt.lon, -5.6 as f64, 0.03 as f64), true);
    });

    test("2. @cap/geohash-pure — San Francisco 9-char high-precision round-trip", () => {
        let buf: [12]u8 = undefined;
        const n = encode(37.7749 as f64, -122.4194 as f64, 9, buf[0..12]);
        expect_eq(n, 9 as usize);
        expect_eq(sliceEq(buf[0..9], "9q8yyk8yt"), true);

        const pt = decode(buf[0..9]);
        expect_eq(pt.valid, true);
        expect_eq(near(pt.lat, 37.7749 as f64, 0.001 as f64), true);
        expect_eq(near(pt.lon, -122.4194 as f64, 0.001 as f64), true);
    });

    test("3. @cap/geohash-pure — Equator origin (0, 0) encodes to 's00000'", () => {
        let buf: [12]u8 = undefined;
        const n = encode(0.0 as f64, 0.0 as f64, 6, buf[0..12]);
        expect_eq(sliceEq(buf[0..n], "s00000"), true);

        const b = decodeBounds(buf[0..n]);
        expect_eq(b.valid, true);
        expect_eq(b.min_lat <= (0.0 as f64) && b.max_lat >= (0.0 as f64), true);
        expect_eq(b.min_lon <= (0.0 as f64) && b.max_lon >= (0.0 as f64), true);
    });

    test("4. @cap/geohash-pure — High-precision cell is nested inside low-precision prefix bounds", () => {
        const coarse = decodeBounds("9q8y");
        const fine = decodeBounds("9q8yyk8yu");
        expect_eq(coarse.valid && fine.valid, true);
        expect_eq(fine.min_lat >= coarse.min_lat && fine.max_lat <= coarse.max_lat, true);
        expect_eq(fine.min_lon >= coarse.min_lon && fine.max_lon <= coarse.max_lon, true);
    });

    test("5. @cap/geohash-pure — Reject invalid characters ('a', 'i', 'l', 'o') & empty hash", () => {
        expect_eq(isValidGeohash("ezs42"), true);
        expect_eq(isValidGeohash("EZS42"), true);
        expect_eq(isValidGeohash(""), false);
        expect_eq(isValidGeohash("ezsa2"), false);
        expect_eq(isValidGeohash("ezsi2"), false);
        expect_eq(isValidGeohash("ezsl2"), false);
        expect_eq(isValidGeohash("ezso2"), false);
        expect_eq(decode("invalid!").valid, false);
    });

    test("6. @cap/geohash-pure — Out-of-range coordinates & small buffer return 0 safely", () => {
        let buf: [12]u8 = undefined;
        expect_eq(encode(91.0 as f64, 0.0 as f64, 5, buf[0..12]), 0 as usize);
        expect_eq(encode(-91.0 as f64, 0.0 as f64, 5, buf[0..12]), 0 as usize);
        expect_eq(encode(0.0 as f64, 181.0 as f64, 5, buf[0..12]), 0 as usize);
        expect_eq(encode(0.0 as f64, -181.0 as f64, 5, buf[0..12]), 0 as usize);
        expect_eq(encode(0.0 as f64, 0.0 as f64, 0, buf[0..12]), 0 as usize);

        let tiny: [3]u8 = undefined;
        expect_eq(encode(42.6 as f64, -5.6 as f64, 5, tiny[0..3]), 0 as usize);
    });
}

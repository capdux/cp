// cap/test/testCapGeohash.cp
// Comprehensive unit tests for @cap/geohash

import { test, expect_eq } from "cap:test";
import {
    encode,
    decodeBBox,
    decode,
    neighbor,
    isValid,
} from "../../packages/@cap/geohash/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/geohash — Standard Geohash test vectors (Jutland & Beijing)", () => {
        let b1: [16]u8 = undefined;
        const n1 = encode(57.64911, 10.40744, 11 as usize, b1[0..16]);
        expect_eq(n1, 11 as usize);
        expect_eq(b1[0..n1], "u4pruydqqvj");

        let b2: [16]u8 = undefined;
        const n2 = encode(39.9042, 116.4074, 7 as usize, b2[0..16]);
        expect_eq(n2, 7 as usize);
        expect_eq(b2[0..n2], "wx4g0bm");
    });

    test("2. @cap/geohash — decodeBBox precision convergence & decode center", () => {
        const bb = decodeBBox("u4pruydqqvj");
        expect_eq(bb.valid, true);
        expect_eq(bb.min_lat <= 57.64911 && bb.max_lat >= 57.64911, true);
        expect_eq(bb.min_lon <= 10.40744 && bb.max_lon >= 10.40744, true);

        const pt = decode("u4pruydqqvj");
        expect_eq(approxEq(pt.lat, 57.64911, 0.00001), true);
        expect_eq(approxEq(pt.lon, 10.40744, 0.00001), true);

        const bbShort = decodeBBox("wx4g");
        const bbLong = decodeBBox("wx4g0b7");
        expect_eq((bbLong.max_lat - bbLong.min_lat) < (bbShort.max_lat - bbShort.min_lat), true);
    });

    test("3. @cap/geohash — 8-direction neighbor symmetry (N<->S, E<->W, NE<->SW)", () => {
        const origin = "wx4g0b7";
        let north: [16]u8 = undefined;
        const nn = neighbor(origin, 1 as i32, 0 as i32, north[0..16]);
        expect_eq(nn, 7 as usize);
        expect_eq(north[0..nn] != origin, true);

        let backS: [16]u8 = undefined;
        const ns = neighbor(north[0..nn], -1 as i32, 0 as i32, backS[0..16]);
        expect_eq(backS[0..ns], origin);

        let ne: [16]u8 = undefined;
        const nne = neighbor(origin, 1 as i32, 1 as i32, ne[0..16]);
        expect_eq(nne, 7 as usize);

        let backSw: [16]u8 = undefined;
        const nsw = neighbor(ne[0..nne], -1 as i32, -1 as i32, backSw[0..16]);
        expect_eq(backSw[0..nsw], origin);
    });

    test("4. @cap/geohash — Antimeridian (±180° longitude) wrap-around neighbor", () => {
        let eastEdge: [16]u8 = undefined;
        const ne = encode(0.0, 179.99, 5 as usize, eastEdge[0..16]);
        expect_eq(ne, 5 as usize);

        let wrapped: [16]u8 = undefined;
        const nw = neighbor(eastEdge[0..ne], 0 as i32, 1 as i32, wrapped[0..16]);
        expect_eq(nw, 5 as usize);
        const ptWrapped = decode(wrapped[0..nw]);
        expect_eq(ptWrapped.lon < -170.0, true);

        // Going west from wrapped should return to eastEdge
        let back: [16]u8 = undefined;
        const nb = neighbor(wrapped[0..nw], 0 as i32, -1 as i32, back[0..16]);
        expect_eq(back[0..nb], eastEdge[0..ne]);
    });

    test("5. @cap/geohash — isValid alphabet & length checks", () => {
        expect_eq(isValid("u4pruydqqvj"), true);
        expect_eq(isValid("0"), true);
        expect_eq(isValid(""), false);
        expect_eq(isValid("u4pruydqqvj12"), false); // > 12 chars
        // 'a', 'i', 'l', 'o' are excluded from Base32 Geohash alphabet
        expect_eq(isValid("abc"), false);
        expect_eq(isValid("123i"), false);
        expect_eq(isValid("123l"), false);
        expect_eq(isValid("123o"), false);
    });

    test("6. @cap/geohash — Out-of-bounds coordinates, zero precision & small buffer guards", () => {
        let buf: [16]u8 = undefined;
        expect_eq(encode(91.0, 0.0, 5 as usize, buf[0..16]), 0 as usize);
        expect_eq(encode(-91.0, 0.0, 5 as usize, buf[0..16]), 0 as usize);
        expect_eq(encode(0.0, 181.0, 5 as usize, buf[0..16]), 0 as usize);
        expect_eq(encode(0.0, -181.0, 5 as usize, buf[0..16]), 0 as usize);
        expect_eq(encode(0.0, 0.0, 0 as usize, buf[0..16]), 0 as usize);
        expect_eq(encode(0.0, 0.0, 13 as usize, buf[0..16]), 0 as usize);

        let tiny: [3]u8 = undefined;
        expect_eq(encode(39.9, 116.4, 5 as usize, tiny[0..3]), 0 as usize);
        expect_eq(decodeBBox("invalid!").valid, false);
        expect_eq(neighbor("wx4g0b7", 2 as i32, 0 as i32, buf[0..16]), 0 as usize);
    });
}

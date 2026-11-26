import { test, expect_eq } from "cap:test";
import {
    LatLng,
    encodeSigned,
    decodeSigned,
    encodePoints,
    decodePoints,
    countPoints,
    isValid,
} from "../../packages/@cap/polyline/src/index.cp";

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return 0.0 - x;
    }
    return x;
}

fn strEq(a: []const u8, b: string): bool {
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
    test("1. @cap/polyline — Google reference example (precision 5)", () => {
        let pts: [3]LatLng = undefined;
        pts[0] = LatLng { lat: 38.5, lng: -120.2 };
        pts[1] = LatLng { lat: 40.7, lng: -120.95 };
        pts[2] = LatLng { lat: 43.252, lng: -126.453 };

        let buf: [128]u8 = undefined;
        let n: usize = encodePoints(pts[0..3], 5, buf[0..]);
        expect_eq(strEq(buf[0..n], "_p~iF~ps|U_ulLnnqC_mqNvxq`@"), true);

        let decoded: [8]LatLng = undefined;
        let cnt: usize = decodePoints("_p~iF~ps|U_ulLnnqC_mqNvxq`@", 5, decoded[0..]);
        expect_eq(cnt, 3);
        expect_eq(absF64(decoded[0].lat - 38.5) < 0.00001, true);
        expect_eq(absF64(decoded[0].lng - (-120.2)) < 0.00001, true);
        expect_eq(absF64(decoded[1].lat - 40.7) < 0.00001, true);
        expect_eq(absF64(decoded[1].lng - (-120.95)) < 0.00001, true);
        expect_eq(absF64(decoded[2].lat - 43.252) < 0.00001, true);
        expect_eq(absF64(decoded[2].lng - (-126.453)) < 0.00001, true);
    });

    test("2. @cap/polyline — precision 6 (OSRM / Valhalla) roundtrip", () => {
        let pts: [2]LatLng = undefined;
        pts[0] = LatLng { lat: 37.774929, lng: -122.419416 };
        pts[1] = LatLng { lat: 34.052235, lng: -118.243683 };

        let buf: [128]u8 = undefined;
        let n: usize = encodePoints(pts[0..2], 6, buf[0..]);
        expect_eq(n > 0, true);

        let decoded: [4]LatLng = undefined;
        let cnt: usize = decodePoints(buf[0..n], 6, decoded[0..]);
        expect_eq(cnt, 2);
        expect_eq(absF64(decoded[0].lat - 37.774929) < 0.000002, true);
        expect_eq(absF64(decoded[0].lng - (-122.419416)) < 0.000002, true);
        expect_eq(absF64(decoded[1].lat - 34.052235) < 0.000002, true);
        expect_eq(absF64(decoded[1].lng - (-118.243683)) < 0.000002, true);
    });

    test("3. @cap/polyline — signed delta encode/decode edge cases", () => {
        let vals: [5]i64 = undefined;
        vals[0] = 0;
        vals[1] = 1;
        vals[2] = -1;
        vals[3] = -17998321;
        vals[4] = 90000000;

        let i: usize = 0;
        while (i < 5) {
            let buf: [32]u8 = undefined;
            let n: usize = encodeSigned(vals[i], buf[0..]);
            expect_eq(n > 0, true);
            let out_v: i64 = 0;
            let consumed: usize = decodeSigned(buf[0..n], 0, &out_v);
            expect_eq(consumed, n);
            expect_eq(out_v, vals[i]);
            i += 1;
        }
    });

    test("4. @cap/polyline — countPoints and isValid", () => {
        expect_eq(isValid(""), true);
        expect_eq(countPoints(""), 0);
        expect_eq(isValid("_p~iF~ps|U_ulLnnqC_mqNvxq`@"), true);
        expect_eq(countPoints("_p~iF~ps|U_ulLnnqC_mqNvxq`@"), 3);
    });

    test("5. @cap/polyline — invalid, truncated, and odd-value rejection", () => {
        expect_eq(isValid("_p~iF"), false);
        expect_eq(isValid("_p~i"), false);
        expect_eq(isValid("  "), false);
        expect_eq(countPoints("_p~iF"), 0);
    });

    test("6. @cap/polyline — buffer overflow protection", () => {
        let pts: [2]LatLng = undefined;
        pts[0] = LatLng { lat: 38.5, lng: -120.2 };
        pts[1] = LatLng { lat: 40.7, lng: -120.95 };

        let tiny: [3]u8 = undefined;
        let n: usize = encodePoints(pts[0..2], 5, tiny[0..]);
        expect_eq(n, 0);

        let one: [1]LatLng = undefined;
        let cnt: usize = decodePoints("_p~iF~ps|U_ulLnnqC_mqNvxq`@", 5, one[0..]);
        expect_eq(cnt, 1);
        expect_eq(absF64(one[0].lat - 38.5) < 0.00001, true);
    });
}

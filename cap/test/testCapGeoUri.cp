// cap/test/testCapGeoUri.cp
// Comprehensive unit tests for @cap/geo-uri

import { test, expect_eq } from "cap:test";
import {
    parseGeoUri,
    formatGeoUri,
    isValidLatLng,
} from "../../packages/geo-uri/src/index.cp";

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

fn nearEq(a: f64, b: f64): bool {
    let diff: f64 = a - b;
    if (diff < (0.0 as f64)) {
        diff = -diff;
    }
    return diff < (0.000001 as f64);
}

fn main() {
    test("1. @cap/geo-uri — 2D geo:lat,lng parsing with default crs=wgs84", () => {
        const g = parseGeoUri("geo:37.786971,-122.399677");
        expect_eq(g.valid, true);
        expect_eq(nearEq(g.lat, 37.786971 as f64), true);
        expect_eq(nearEq(g.lng, -122.399677 as f64), true);
        expect_eq(g.has_alt, false);
        expect_eq(g.has_uncertainty, false);
        expect_eq(sliceEq(g.crs, "wgs84"), true);
    });

    test("2. @cap/geo-uri — 3D geo:lat,lng,alt parsing", () => {
        const g = parseGeoUri("GEO:48.198634,16.371648,171.5");
        expect_eq(g.valid, true);
        expect_eq(nearEq(g.lat, 48.198634 as f64), true);
        expect_eq(nearEq(g.lng, 16.371648 as f64), true);
        expect_eq(g.has_alt, true);
        expect_eq(nearEq(g.alt, 171.5 as f64), true);
    });

    test("3. @cap/geo-uri — Parsing ;crs=wgs84;u=35.5 parameters", () => {
        const g = parseGeoUri("geo:37.786971,-122.399677,15.5;crs=wgs84;u=35.5");
        expect_eq(g.valid, true);
        expect_eq(g.has_alt, true);
        expect_eq(nearEq(g.alt, 15.5 as f64), true);
        expect_eq(sliceEq(g.crs, "wgs84"), true);
        expect_eq(g.has_uncertainty, true);
        expect_eq(nearEq(g.uncertainty, 35.5 as f64), true);
    });

    test("4. @cap/geo-uri — formatGeoUri formatting & parseGeoUri round-trip", () => {
        let buf: [64]u8 = undefined;
        const n = formatGeoUri(37.786971 as f64, -122.399677 as f64, true, 15.5 as f64, true, 35.0 as f64, buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "geo:37.786971,-122.399677,15.5;u=35"), true);

        const reparsed = parseGeoUri(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(nearEq(reparsed.lat, 37.786971 as f64), true);
        expect_eq(nearEq(reparsed.lng, -122.399677 as f64), true);
        expect_eq(reparsed.has_alt, true);
        expect_eq(nearEq(reparsed.alt, 15.5 as f64), true);
        expect_eq(reparsed.has_uncertainty, true);
        expect_eq(nearEq(reparsed.uncertainty, 35.0 as f64), true);
    });

    test("5. @cap/geo-uri — Reject out-of-range lat/lng & negative uncertainty", () => {
        expect_eq(isValidLatLng(90.0 as f64, 180.0 as f64), true);
        expect_eq(isValidLatLng(-90.0 as f64, -180.0 as f64), true);
        expect_eq(isValidLatLng(95.0 as f64, 0.0 as f64), false);
        expect_eq(isValidLatLng(0.0 as f64, 200.0 as f64), false);

        expect_eq(parseGeoUri("geo:95.0,10.0").valid, false);
        expect_eq(parseGeoUri("geo:10.0,-200.0").valid, false);
        expect_eq(parseGeoUri("geo:10.0,20.0;u=-5").valid, false);

        let buf: [64]u8 = undefined;
        expect_eq(formatGeoUri(95.0 as f64, 0.0 as f64, false, 0.0 as f64, false, 0.0 as f64, buf[0..64]), 0 as usize);
        expect_eq(formatGeoUri(10.0 as f64, 20.0 as f64, false, 0.0 as f64, true, -1.0 as f64, buf[0..64]), 0 as usize);
    });

    test("6. @cap/geo-uri — Reject non-geo scheme, malformed coords & tiny output buffer", () => {
        expect_eq(parseGeoUri("https://37.7,-122.4").valid, false);
        expect_eq(parseGeoUri("geo:37.7").valid, false);
        expect_eq(parseGeoUri("geo:1,2,3,4").valid, false);
        expect_eq(parseGeoUri("geo:abc,def").valid, false);

        let tiny: [6]u8 = undefined;
        expect_eq(formatGeoUri(37.786971 as f64, -122.399677 as f64, false, 0.0 as f64, false, 0.0 as f64, tiny[0..6]), 0 as usize);
    });
}

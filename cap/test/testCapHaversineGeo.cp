// cap/test/testCapHaversineGeo.cp
// Comprehensive unit tests for @cap/haversine-geo

import { test, expect_eq } from "cap:test";
import {
    LatLon,
    distanceMeters,
    distanceKm,
    distanceMiles,
    initialBearing,
    midpoint,
    destinationPoint,
    isValidCoord,
} from "../../packages/@cap/haversine-geo/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= eps;
}

fn main() {
    test("1. @cap/haversine-geo — Identical point distance is zero", () => {
        const p = LatLon{ lat: 37.7749 as f64, lon: -122.4194 as f64 };
        expect_eq(near(distanceMeters(p, p), 0.0 as f64, 0.001 as f64), true);
        expect_eq(near(distanceKm(p, p), 0.0 as f64, 0.001 as f64), true);
        expect_eq(near(distanceMiles(p, p), 0.0 as f64, 0.001 as f64), true);
    });

    test("2. @cap/haversine-geo — Classic New York -> London great-circle distance (~5570 km)", () => {
        const nyc = LatLon{ lat: 40.7128 as f64, lon: -74.0060 as f64 };
        const london = LatLon{ lat: 51.5074 as f64, lon: -0.1278 as f64 };

        const km: f64 = distanceKm(nyc, london);
        expect_eq(km > (5565.0 as f64) && km < (5575.0 as f64), true);

        const mi: f64 = distanceMiles(nyc, london);
        expect_eq(mi > (3455.0 as f64) && mi < (3465.0 as f64), true);
    });

    test("3. @cap/haversine-geo — Due North (0 deg) & due East (90 deg) initialBearing", () => {
        const eq0 = LatLon{ lat: 0.0 as f64, lon: 0.0 as f64 };
        const north = LatLon{ lat: 10.0 as f64, lon: 0.0 as f64 };
        const east = LatLon{ lat: 0.0 as f64, lon: 10.0 as f64 };
        const west = LatLon{ lat: 0.0 as f64, lon: -10.0 as f64 };

        expect_eq(near(initialBearing(eq0, north), 0.0 as f64, 0.001 as f64), true);
        expect_eq(near(initialBearing(eq0, east), 90.0 as f64, 0.001 as f64), true);
        expect_eq(near(initialBearing(eq0, west), 270.0 as f64, 0.001 as f64), true);
    });

    test("4. @cap/haversine-geo — Equatorial midpoint precision", () => {
        const a = LatLon{ lat: 0.0 as f64, lon: 10.0 as f64 };
        const b = LatLon{ lat: 0.0 as f64, lon: 30.0 as f64 };
        const mid = midpoint(a, b);
        expect_eq(near(mid.lat, 0.0 as f64, 0.001 as f64), true);
        expect_eq(near(mid.lon, 20.0 as f64, 0.001 as f64), true);
    });

    test("5. @cap/haversine-geo — destinationPoint round-trip distance verification", () => {
        const start = LatLon{ lat: 35.6762 as f64, lon: 139.6503 as f64 };
        const targetDistM: f64 = 123456.0;
        const dest = destinationPoint(start, 45.0 as f64, targetDistM);
        expect_eq(isValidCoord(dest), true);

        const measured: f64 = distanceMeters(start, dest);
        expect_eq(near(measured, targetDistM, 1.0 as f64), true);
    });

    test("6. @cap/haversine-geo — isValidCoord boundary validation", () => {
        expect_eq(isValidCoord(LatLon{ lat: 90.0 as f64, lon: 180.0 as f64 }), true);
        expect_eq(isValidCoord(LatLon{ lat: -90.0 as f64, lon: -180.0 as f64 }), true);
        expect_eq(isValidCoord(LatLon{ lat: 90.1 as f64, lon: 0.0 as f64 }), false);
        expect_eq(isValidCoord(LatLon{ lat: -91.0 as f64, lon: 0.0 as f64 }), false);
        expect_eq(isValidCoord(LatLon{ lat: 0.0 as f64, lon: 180.5 as f64 }), false);
        expect_eq(isValidCoord(LatLon{ lat: 0.0 as f64, lon: -181.0 as f64 }), false);
    });
}

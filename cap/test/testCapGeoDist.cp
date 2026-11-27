// cap/test/testCapGeoDist.cp
// Comprehensive unit tests for @cap/geo-dist

import { test, expect_eq } from "cap:test";
import {
    Coord,
    haversineMeters,
    initialBearing,
    destinationPoint,
    midpoint,
    boundingBox,
    pointInPolygon,
} from "../../packages/geo/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/geo-dist — Zero distance, 1° equator (~111.195km) & NYC-London (~5570km)", () => {
        const origin = Coord{ lat: 0.0, lon: 0.0 };
        expect_eq(approxEq(haversineMeters(origin, origin), 0.0, 0.001), true);

        const eq1 = Coord{ lat: 0.0, lon: 1.0 };
        const dEq = haversineMeters(origin, eq1);
        expect_eq(approxEq(dEq, 111195.0, 5.0), true);

        const nyc = Coord{ lat: 40.7128, lon: -74.0060 };
        const london = Coord{ lat: 51.5074, lon: -0.1278 };
        const dNycLon = haversineMeters(nyc, london);
        expect_eq(dNycLon > 5565000.0 && dNycLon < 5575000.0, true);
    });

    test("2. @cap/geo-dist — initialBearing cardinal directions (N/E/S/W)", () => {
        const c = Coord{ lat: 0.0, lon: 0.0 };
        expect_eq(approxEq(initialBearing(c, Coord{ lat: 10.0, lon: 0.0 }), 0.0, 0.01), true);
        expect_eq(approxEq(initialBearing(c, Coord{ lat: 0.0, lon: 10.0 }), 90.0, 0.01), true);
        expect_eq(approxEq(initialBearing(c, Coord{ lat: -10.0, lon: 0.0 }), 180.0, 0.01), true);
        expect_eq(approxEq(initialBearing(c, Coord{ lat: 0.0, lon: -10.0 }), 270.0, 0.01), true);
    });

    test("3. @cap/geo-dist — destinationPoint & haversineMeters round-trip consistency", () => {
        const start = Coord{ lat: 39.9042, lon: 116.4074 };
        const targetDist: f64 = 150000.0; // 150 km
        const dest = destinationPoint(start, 65.0, targetDist);
        const measured = haversineMeters(start, dest);
        expect_eq(approxEq(measured, targetDist, 1.0), true);
        expect_eq(approxEq(initialBearing(start, dest), 65.0, 0.05), true);
    });

    test("4. @cap/geo-dist — Spherical midpoint across hemispheres", () => {
        const a = Coord{ lat: 0.0, lon: -10.0 };
        const b = Coord{ lat: 0.0, lon: 10.0 };
        const m1 = midpoint(a, b);
        expect_eq(approxEq(m1.lat, 0.0, 0.0001), true);
        expect_eq(approxEq(m1.lon, 0.0, 0.0001), true);

        const p1 = Coord{ lat: 40.0, lon: -70.0 };
        const p2 = Coord{ lat: 50.0, lon: 10.0 };
        const m2 = midpoint(p1, p2);
        const d1 = haversineMeters(p1, m2);
        const d2 = haversineMeters(m2, p2);
        expect_eq(approxEq(d1, d2, 10.0), true);
    });

    test("5. @cap/geo-dist — boundingBox contains destination points at radius", () => {
        const center = Coord{ lat: 35.6762, lon: 139.6503 };
        const radius: f64 = 10000.0; // 10 km
        const bb = boundingBox(center, radius);

        const north = destinationPoint(center, 0.0, radius);
        const south = destinationPoint(center, 180.0, radius);
        const east = destinationPoint(center, 90.0, radius);
        const west = destinationPoint(center, 270.0, radius);

        expect_eq(bb.max_lat >= north.lat - 0.0001, true);
        expect_eq(bb.min_lat <= south.lat + 0.0001, true);
        expect_eq(bb.max_lon >= east.lon - 0.0001, true);
        expect_eq(bb.min_lon <= west.lon + 0.0001, true);
    });

    test("6. @cap/geo-dist — pointInPolygon for convex, concave (L-shape) & degenerate polygons", () => {
        // Concave L-shaped polygon: (0,0)-(10,0)-(10,4)-(4,4)-(4,10)-(0,10)
        const lShape: [6]Coord = [
            Coord{ lat: 0.0, lon: 0.0 },
            Coord{ lat: 0.0, lon: 10.0 },
            Coord{ lat: 4.0, lon: 10.0 },
            Coord{ lat: 4.0, lon: 4.0 },
            Coord{ lat: 10.0, lon: 4.0 },
            Coord{ lat: 10.0, lon: 0.0 },
        ];
        expect_eq(pointInPolygon(Coord{ lat: 2.0, lon: 2.0 }, lShape[0..6]), true);
        expect_eq(pointInPolygon(Coord{ lat: 2.0, lon: 8.0 }, lShape[0..6]), true);
        expect_eq(pointInPolygon(Coord{ lat: 8.0, lon: 2.0 }, lShape[0..6]), true);
        // Inside concave cutout -> MUST be false
        expect_eq(pointInPolygon(Coord{ lat: 7.0, lon: 7.0 }, lShape[0..6]), false);
        // Outside bounding box
        expect_eq(pointInPolygon(Coord{ lat: 12.0, lon: 12.0 }, lShape[0..6]), false);
        // Degenerate (< 3 vertices) -> false
        expect_eq(pointInPolygon(Coord{ lat: 1.0, lon: 1.0 }, lShape[0..2]), false);
    });
}

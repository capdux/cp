// Source: cp/cap/test/test_voronoi_production_deep.cp (Production deep tests for cap:voronoi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VoronoiPoint2 as Point2,
    VoronoiBounds2 as Bounds2,
    CellMetrics,
    clipPolygonHalfPlane,
    polygonArea,
    polygonCentroid,
    computeVoronoiCell,
    lloydRelaxationStep,
    nearestSite,
} from "cap:graph";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

test("1. Two-site perpendicular bisection: axis-aligned & diagonal split exact areas and centroids", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    expect_eq(bounds.isValid(), true);
    expect_eq(bounds.area(), 100.0);

    let buf_a: [16]Point2 = std.mem.zeroes([16]Point2);
    let buf_b: [16]Point2 = std.mem.zeroes([16]Point2);

    // Axis-aligned pair: bisector is x = 5
    const pair_x: [2]Point2 = [
        Point2 { x: 2.0, y: 5.0 },
        Point2 { x: 8.0, y: 5.0 },
    ];
    const c0 = computeVoronoiCell(pair_x[0..], 0, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
    const c1 = computeVoronoiCell(pair_x[0..], 1, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
    expect_eq(c0.vertex_count, 4 as usize);
    expect_eq(c1.vertex_count, 4 as usize);
    expect_eq(absDiff(c0.area, 50.0) < 1.0e-11, true);
    expect_eq(absDiff(c1.area, 50.0) < 1.0e-11, true);
    expect_eq(absDiff(c0.centroid.x, 2.5) < 1.0e-11, true);
    expect_eq(absDiff(c0.centroid.y, 5.0) < 1.0e-11, true);
    expect_eq(absDiff(c1.centroid.x, 7.5) < 1.0e-11, true);

    // Diagonal pair: bisector is x + y = 10 -> two right triangles of area 50 each
    const pair_d: [2]Point2 = [
        Point2 { x: 2.0, y: 2.0 },
        Point2 { x: 8.0, y: 8.0 },
    ];
    const d0 = computeVoronoiCell(pair_d[0..], 0, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
    const d1 = computeVoronoiCell(pair_d[0..], 1, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
    expect_eq(d0.vertex_count, 3 as usize);
    expect_eq(d1.vertex_count, 3 as usize);
    expect_eq(absDiff(d0.area, 50.0) < 1.0e-11, true);
    expect_eq(absDiff(d1.area, 50.0) < 1.0e-11, true);
    // Triangle (0,0)-(10,0)-(0,10) centroid is (10/3, 10/3)
    expect_eq(absDiff(d0.centroid.x, 10.0 / 3.0) < 1.0e-11, true);
    expect_eq(absDiff(d0.centroid.y, 10.0 / 3.0) < 1.0e-11, true);
});

test("2. 4-site symmetric grid exact quartering & coincident duplicate site resilience", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const sites: [4]Point2 = [
        Point2 { x: 2.5, y: 2.5 },
        Point2 { x: 7.5, y: 2.5 },
        Point2 { x: 2.5, y: 7.5 },
        Point2 { x: 7.5, y: 7.5 },
    ];
    let buf_a: [16]Point2 = std.mem.zeroes([16]Point2);
    let buf_b: [16]Point2 = std.mem.zeroes([16]Point2);

    let i: usize = 0;
    while (i < 4) {
        const m = computeVoronoiCell(sites[0..], i, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
        expect_eq(m.vertex_count, 4 as usize);
        expect_eq(absDiff(m.area, 25.0) < 1.0e-11, true);
        // Symmetric sites are already at their exact CVT centroids!
        expect_eq(absDiff(m.centroid.x, sites[i].x) < 1.0e-11, true);
        expect_eq(absDiff(m.centroid.y, sites[i].y) < 1.0e-11, true);
        i += 1;
    }

    // Coincident duplicate site is skipped cleanly without NaN
    const dup_sites: [3]Point2 = [
        Point2 { x: 2.0, y: 5.0 },
        Point2 { x: 2.0, y: 5.0 },
        Point2 { x: 8.0, y: 5.0 },
    ];
    const m0 = computeVoronoiCell(dup_sites[0..], 0, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
    expect_eq(absDiff(m0.area, 50.0) < 1.0e-11, true);
});

test("3. 12 irregular sites: sum of Voronoi cell areas == bounding box area & centroid nearest-site invariant", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 100.0, max_y: 80.0 };
    const sites: [12]Point2 = [
        Point2 { x: 12.3, y: 15.7 },
        Point2 { x: 37.8, y: 9.4 },
        Point2 { x: 68.2, y: 18.1 },
        Point2 { x: 91.5, y: 11.2 },
        Point2 { x: 8.6, y: 42.9 },
        Point2 { x: 31.4, y: 38.5 },
        Point2 { x: 55.9, y: 44.3 },
        Point2 { x: 84.7, y: 47.6 },
        Point2 { x: 17.2, y: 71.8 },
        Point2 { x: 43.5, y: 66.4 },
        Point2 { x: 69.1, y: 73.2 },
        Point2 { x: 93.4, y: 69.5 },
    ];
    let buf_a: [32]Point2 = std.mem.zeroes([32]Point2);
    let buf_b: [32]Point2 = std.mem.zeroes([32]Point2);

    let area_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < 12) {
        const m = computeVoronoiCell(sites[0..], i, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
        expect_eq(m.vertex_count >= 3, true);
        expect_eq(m.area > 0.0, true);
        expect_eq(bounds.contains(m.centroid), true);

        // Every cell centroid must strictly have site i as its nearest site
        const nn = nearestSite(sites[0..], m.centroid) ?? unreachable;
        expect_eq(nn, i);

        // All cell vertices must lie within bounding box (within 1e-9 tolerance)
        let v: usize = 0;
        while (v < m.vertex_count) {
            expect_eq(buf_a[v].x >= bounds.min_x - 1.0e-9 && buf_a[v].x <= bounds.max_x + 1.0e-9, true);
            expect_eq(buf_a[v].y >= bounds.min_y - 1.0e-9 && buf_a[v].y <= bounds.max_y + 1.0e-9, true);
            v += 1;
        }

        area_sum = area_sum + m.area;
        i += 1;
    }

    expect_eq(absDiff(area_sum, bounds.area()) < 1.0e-9, true);
});

test("4. Multi-step Lloyd relaxation converges smoothly & keeps all sites inside bounds", () => {
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    // Initially clustered near bottom-left corner
    let cur: [8]Point2 = [
        Point2 { x: 1.0, y: 1.0 },
        Point2 { x: 2.0, y: 1.2 },
        Point2 { x: 1.3, y: 2.1 },
        Point2 { x: 2.4, y: 2.3 },
        Point2 { x: 3.1, y: 1.5 },
        Point2 { x: 1.7, y: 3.2 },
        Point2 { x: 2.8, y: 3.0 },
        Point2 { x: 3.5, y: 2.6 },
    ];
    let next: [8]Point2 = std.mem.zeroes([8]Point2);
    let buf_a: [32]Point2 = std.mem.zeroes([32]Point2);
    let buf_b: [32]Point2 = std.mem.zeroes([32]Point2);

    const disp_first = lloydRelaxationStep(cur[0..], bounds, next[0..], buf_a[0..], buf_b[0..]) ?? unreachable;
    expect_eq(disp_first > 0.5, true);

    let step: usize = 0;
    let last_disp = disp_first;
    while (step < 20) {
        let j: usize = 0;
        while (j < 8) {
            cur[j] = next[j];
            expect_eq(bounds.contains(cur[j]), true);
            j += 1;
        }
        last_disp = lloydRelaxationStep(cur[0..], bounds, next[0..], buf_a[0..], buf_b[0..]) ?? unreachable;
        step += 1;
    }

    // Displacement decays sharply after 20 Lloyd iterations
    expect_eq(last_disp < disp_first * 0.2, true);

    // Total area remains conserved after relaxation
    let area_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < 8) {
        const m = computeVoronoiCell(next[0..], i, bounds, buf_a[0..], buf_b[0..]) ?? unreachable;
        area_sum = area_sum + m.area;
        i += 1;
    }
    expect_eq(absDiff(area_sum, 100.0) < 1.0e-9, true);
});

test("5. Half-plane clipping, Shoelace area & polygon centroid primitives", () => {
    const square: [4]Point2 = [
        Point2 { x: 0.0, y: 0.0 },
        Point2 { x: 4.0, y: 0.0 },
        Point2 { x: 4.0, y: 4.0 },
        Point2 { x: 0.0, y: 4.0 },
    ];
    let out: [8]Point2 = std.mem.zeroes([8]Point2);

    // Clip to x + y <= 4 -> right triangle (0,0)-(4,0)-(0,4), area = 8, centroid = (4/3, 4/3)
    const n1 = clipPolygonHalfPlane(square[0..], 1.0, 1.0, 4.0, out[0..]) ?? unreachable;
    expect_eq(n1, 3 as usize);
    expect_eq(absDiff(polygonArea(out[0..n1]), 8.0) < 1.0e-12, true);
    const cen = polygonCentroid(out[0..n1]) ?? unreachable;
    expect_eq(absDiff(cen.x, 4.0 / 3.0) < 1.0e-12, true);
    expect_eq(absDiff(cen.y, 4.0 / 3.0) < 1.0e-12, true);

    // Clip completely outside: x <= -1 -> 0 vertices
    const n2 = clipPolygonHalfPlane(square[0..], 1.0, 0.0, -1.0, out[0..]) ?? unreachable;
    expect_eq(n2, 0 as usize);
    expect_eq(polygonArea(out[0..n2]), 0.0);
    expect_eq(polygonCentroid(out[0..n2]) == null, true);

    // Clip completely inside: x <= 10 -> retains all 4 vertices
    const n3 = clipPolygonHalfPlane(square[0..], 1.0, 0.0, 10.0, out[0..]) ?? unreachable;
    expect_eq(n3, 4 as usize);
    expect_eq(absDiff(polygonArea(out[0..n3]), 16.0) < 1.0e-12, true);
});

test("6. Boundary guards: empty sites, OOB index, invalid bounds, undersized buffers & NaN rejection", () => {
    const empty_sites: [0]Point2 = [];
    const ok_sites: [2]Point2 = [
        Point2 { x: 1.0, y: 1.0 },
        Point2 { x: 9.0, y: 9.0 },
    ];
    const nan_sites: [2]Point2 = [
        Point2 { x: std.math.nan(f64), y: 1.0 },
        Point2 { x: 9.0, y: 9.0 },
    ];
    const ok_bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const bad_bounds = Bounds2 { min_x: 10.0, min_y: 0.0, max_x: 5.0, max_y: 10.0 };
    const nan_bounds = Bounds2 { min_x: 0.0, min_y: std.math.nan(f64), max_x: 10.0, max_y: 10.0 };

    let buf_a: [16]Point2 = std.mem.zeroes([16]Point2);
    let buf_b: [16]Point2 = std.mem.zeroes([16]Point2);
    let small3: [3]Point2 = std.mem.zeroes([3]Point2);
    let next1: [1]Point2 = std.mem.zeroes([1]Point2);

    expect_eq(bad_bounds.isValid(), false);
    expect_eq(nan_bounds.isValid(), false);
    expect_eq(bad_bounds.area(), 0.0);

    expect_eq(computeVoronoiCell(empty_sites[0..], 0, ok_bounds, buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(computeVoronoiCell(ok_sites[0..], 2, ok_bounds, buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(computeVoronoiCell(ok_sites[0..], 0, bad_bounds, buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(computeVoronoiCell(ok_sites[0..], 0, nan_bounds, buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(computeVoronoiCell(nan_sites[0..], 0, ok_bounds, buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(computeVoronoiCell(ok_sites[0..], 0, ok_bounds, small3[0..], buf_b[0..]) == null, true);
    expect_eq(computeVoronoiCell(ok_sites[0..], 0, ok_bounds, buf_a[0..], small3[0..]) == null, true);

    expect_eq(lloydRelaxationStep(empty_sites[0..], ok_bounds, next1[0..], buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(lloydRelaxationStep(ok_sites[0..], ok_bounds, next1[0..], buf_a[0..], buf_b[0..]) == null, true);
    expect_eq(lloydRelaxationStep(ok_sites[0..], bad_bounds, buf_a[0..], buf_a[0..], buf_b[0..]) == null, true);

    expect_eq(nearestSite(empty_sites[0..], Point2 { x: 0.0, y: 0.0 }) == null, true);
    expect_eq(nearestSite(ok_sites[0..], Point2 { x: std.math.nan(f64), y: 0.0 }) == null, true);
    expect_eq(nearestSite(nan_sites[0..], Point2 { x: 0.0, y: 0.0 }) == null, true);
    expect_eq(clipPolygonHalfPlane(ok_sites[0..], std.math.nan(f64), 1.0, 0.0, buf_a[0..]) == null, true);
});

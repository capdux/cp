// Source: cp/cap/test/test_geom2d_production_deep.cp
// Deep Production Verification Suite for cap:geom2d & cap.geom2d
// Covers:
//   1. segmentsIntersect & segmentIntersectionPoint: orthogonal cross, T-junction, collinear overlap/touch, parallel & disjoint
//   2. polygonArea, polygonCentroid & pointInPolygon (concave U-shape, boundary edge/vertex, error guards)
//   3. convexHull Andrew's Monotone Chain (interior & collinear filtering, duplicates, degenerate <3 / collinear)
//   4. Aabb2, Circle2, circleAabbIntersects, pointToSegmentDist & Point2 vector primitives
//   5. `import cap from "cap"; cap.geom2d` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GeomError,
    Orientation,
    Point2,
    Segment2,
    Aabb2,
    Circle2,
    Geom2D,
} from "cap:geo";

fn expectNear(actual: f64, expected: f64, tol: f64) {
    expect_eq(@abs(actual - expected) <= tol, true);
}

test("cap:geom2d - segmentsIntersect & segmentIntersectionPoint boundaries") {
    const tol: f64 = 0.000000001;

    // 1. Orthogonal / asymmetric cross intersection
    const s1 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(4.0, 4.0));
    const s2 = Segment2.init(Point2.init(0.0, 4.0), Point2.init(4.0, 0.0));
    expect_eq(Geom2D.segmentsIntersect(s1, s2), true);
    if (p := Geom2D.segmentIntersectionPoint(s1, s2)) {
        expectNear(p.x, 2.0, tol);
        expectNear(p.y, 2.0, tol);
    } else {
        expect_eq(true, false);
    }

    const s3 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(6.0, 3.0));
    const s4 = Segment2.init(Point2.init(2.0, 0.0), Point2.init(2.0, 5.0));
    expect_eq(Geom2D.segmentsIntersect(s3, s4), true);
    if (p := Geom2D.segmentIntersectionPoint(s3, s4)) {
        expectNear(p.x, 2.0, tol);
        expectNear(p.y, 1.0, tol);
    } else {
        expect_eq(true, false);
    }

    // 2. T-junction endpoint touch
    const base = Segment2.init(Point2.init(0.0, 0.0), Point2.init(4.0, 0.0));
    const stem = Segment2.init(Point2.init(2.0, 0.0), Point2.init(2.0, 3.0));
    expect_eq(Geom2D.segmentsIntersect(base, stem), true);
    if (p := Geom2D.segmentIntersectionPoint(base, stem)) {
        expectNear(p.x, 2.0, tol);
        expectNear(p.y, 0.0, tol);
    } else {
        expect_eq(true, false);
    }

    // 3. Collinear overlap & collinear endpoint touch
    const c1 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(4.0, 0.0));
    const c2 = Segment2.init(Point2.init(2.0, 0.0), Point2.init(6.0, 0.0));
    expect_eq(Geom2D.segmentsIntersect(c1, c2), true);
    if (p := Geom2D.segmentIntersectionPoint(c1, c2)) {
        expectNear(p.x, 2.0, tol);
        expectNear(p.y, 0.0, tol);
    } else {
        expect_eq(true, false);
    }

    const touch1 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(2.0, 0.0));
    const touch2 = Segment2.init(Point2.init(2.0, 0.0), Point2.init(5.0, 0.0));
    expect_eq(Geom2D.segmentsIntersect(touch1, touch2), true);
    if (p := Geom2D.segmentIntersectionPoint(touch1, touch2)) {
        expectNear(p.x, 2.0, tol);
        expectNear(p.y, 0.0, tol);
    } else {
        expect_eq(true, false);
    }

    // 4. Collinear disjoint, parallel disjoint, and non-parallel non-intersecting
    const col_far = Segment2.init(Point2.init(5.0, 0.0), Point2.init(7.0, 0.0));
    expect_eq(Geom2D.segmentsIntersect(touch1, col_far), false);
    expect_eq(Geom2D.segmentIntersectionPoint(touch1, col_far) == null, true);

    const par1 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(4.0, 0.0));
    const par2 = Segment2.init(Point2.init(0.0, 1.0), Point2.init(4.0, 1.0));
    expect_eq(Geom2D.segmentsIntersect(par1, par2), false);
    expect_eq(Geom2D.segmentIntersectionPoint(par1, par2) == null, true);

    const skew1 = Segment2.init(Point2.init(0.0, 0.0), Point2.init(1.0, 1.0));
    const skew2 = Segment2.init(Point2.init(2.0, 0.0), Point2.init(2.0, 5.0));
    expect_eq(Geom2D.segmentsIntersect(skew1, skew2), false);
    expect_eq(Geom2D.segmentIntersectionPoint(skew1, skew2) == null, true);
}

test("cap:geom2d - polygonArea, polygonCentroid & pointInPolygon") {
    const tol: f64 = 0.000000001;

    // Right triangle (0,0)-(4,0)-(0,3): area = 6.0, centroid = (4/3, 1.0)
    const tri = [
        Point2.init(0.0, 0.0),
        Point2.init(4.0, 0.0),
        Point2.init(0.0, 3.0),
    ];
    expectNear(Geom2D.polygonArea(&tri), 6.0, tol);
    const tri_c = Geom2D.polygonCentroid(&tri) catch unreachable;
    expectNear(tri_c.x, 4.0 / 3.0, tol);
    expectNear(tri_c.y, 1.0, tol);

    // Concave U-shaped polygon: 10x10 square with 4x7 top notch cut out
    // Area = 100 - 28 = 72
    const u_poly = [
        Point2.init(0.0, 0.0),
        Point2.init(10.0, 0.0),
        Point2.init(10.0, 10.0),
        Point2.init(7.0, 10.0),
        Point2.init(7.0, 3.0),
        Point2.init(3.0, 3.0),
        Point2.init(3.0, 10.0),
        Point2.init(0.0, 10.0),
    ];
    expectNear(Geom2D.polygonArea(&u_poly), 72.0, tol);
    const u_c = Geom2D.polygonCentroid(&u_poly) catch unreachable;
    // Symmetric about x = 5.0
    expectNear(u_c.x, 5.0, tol);

    // Inside pillars & bottom bridge
    expect_eq(Geom2D.pointInPolygon(Point2.init(1.5, 7.0), &u_poly), true);
    expect_eq(Geom2D.pointInPolygon(Point2.init(8.5, 7.0), &u_poly), true);
    expect_eq(Geom2D.pointInPolygon(Point2.init(5.0, 1.5), &u_poly), true);
    // Inside concave notch -> MUST be outside!
    expect_eq(Geom2D.pointInPolygon(Point2.init(5.0, 6.0), &u_poly), false);
    // Far outside
    expect_eq(Geom2D.pointInPolygon(Point2.init(12.0, 5.0), &u_poly), false);
    // Boundary edge & vertex -> MUST be inside!
    expect_eq(Geom2D.pointInPolygon(Point2.init(10.0, 5.0), &u_poly), true);
    expect_eq(Geom2D.pointInPolygon(Point2.init(0.0, 0.0), &u_poly), true);
    expect_eq(Geom2D.pointInPolygon(Point2.init(5.0, 3.0), &u_poly), true);

    // Error interception for degenerate polygons
    const two_pts = [Point2.init(0.0, 0.0), Point2.init(1.0, 1.0)];
    expect_eq(Geom2D.polygonArea(&two_pts), 0.0);
    expect_eq(Geom2D.pointInPolygon(Point2.init(0.5, 0.5), &two_pts), false);
    expect_eq(Geom2D.polygonCentroid(&two_pts) == GeomError.InsufficientPoints, true);

    const col_pts = [
        Point2.init(0.0, 0.0),
        Point2.init(1.0, 1.0),
        Point2.init(2.0, 2.0),
    ];
    expect_eq(Geom2D.polygonCentroid(&col_pts) == GeomError.DegenerateGeometry, true);
}

test("cap:geom2d - convexHull Andrew Monotone Chain & degenerate handling") {
    const alloc = std.testing.allocator;
    const tol: f64 = 0.000000001;

    // 3x3 grid + interior points + duplicate corners -> hull must be the 4 outer corners CCW
    const cloud = [
        Point2.init(1.0, 1.0),
        Point2.init(0.0, 0.0),
        Point2.init(2.0, 0.0),
        Point2.init(1.0, 0.0),
        Point2.init(2.0, 2.0),
        Point2.init(0.0, 1.0),
        Point2.init(2.0, 1.0),
        Point2.init(0.0, 2.0),
        Point2.init(1.0, 2.0),
        Point2.init(0.5, 0.5),
        Point2.init(2.0, 0.0),
    ];
    const hull = Geom2D.convexHull(alloc, &cloud) catch unreachable;
    defer alloc.free(hull);

    expect_eq(hull.len, 4);
    // Starting from lexicographically smallest (0, 0) in CCW order
    expectNear(hull[0].x, 0.0, tol);
    expectNear(hull[0].y, 0.0, tol);
    expectNear(hull[1].x, 2.0, tol);
    expectNear(hull[1].y, 0.0, tol);
    expectNear(hull[2].x, 2.0, tol);
    expectNear(hull[2].y, 2.0, tol);
    expectNear(hull[3].x, 0.0, tol);
    expectNear(hull[3].y, 2.0, tol);

    // Verify strict CCW turn at every vertex
    for (i of 0..hull.len) {
        const p0 = hull[i];
        const p1 = hull[(i + 1) % hull.len];
        const p2 = hull[(i + 2) % hull.len];
        expect_eq(Geom2D.orientation(p0, p1, p2) == Orientation.CounterClockwise, true);
    }

    // Degenerate cases
    const too_few = [Point2.init(0.0, 0.0), Point2.init(1.0, 1.0)];
    expect_eq(Geom2D.convexHull(alloc, &too_few) == GeomError.InsufficientPoints, true);

    const all_collinear = [
        Point2.init(0.0, 0.0),
        Point2.init(1.0, 1.0),
        Point2.init(2.0, 2.0),
        Point2.init(3.0, 3.0),
    ];
    expect_eq(Geom2D.convexHull(alloc, &all_collinear) == GeomError.DegenerateGeometry, true);
}

test("cap:geom2d - Aabb2, Circle2, circleAabbIntersects, pointToSegmentDist & Point2 ops") {
    const tol: f64 = 0.000000001;

    // Point2 vector ops & rotation
    const p1 = Point2.init(1.0, 0.0);
    const p2 = Point2.init(0.0, 1.0);
    expectNear(p1.dot(p2), 0.0, tol);
    expectNear(p1.cross(p2), 1.0, tol);
    const rotated = p1.rotate(std.math.pi * 0.5);
    expectNear(rotated.x, 0.0, tol);
    expectNear(rotated.y, 1.0, tol);
    expectNear(p1.add(p2).dist(Point2.zero()), @sqrt(2.0), tol);

    // Aabb2 collision
    const box_a = Aabb2.init(0.0, 0.0, 4.0, 4.0);
    const box_b = Aabb2.init(3.0, 3.0, 6.0, 6.0);
    const box_c = Aabb2.init(5.0, 0.0, 7.0, 2.0);
    expect_eq(Geom2D.aabbIntersects(box_a, box_b), true);
    expect_eq(Geom2D.aabbIntersects(box_a, box_c), false);
    expect_eq(box_a.contains(Point2.init(2.0, 2.0)), true);
    expectNear(box_a.area(), 16.0, tol);

    // Circle2 collision
    const c1 = Circle2.init(Point2.init(0.0, 0.0), 3.0);
    const c2 = Circle2.init(Point2.init(4.0, 0.0), 2.0);
    const c3 = Circle2.init(Point2.init(6.0, 0.0), 2.0);
    expect_eq(Geom2D.circleIntersects(c1, c2), true);
    expect_eq(Geom2D.circleIntersects(c1, c3), false);

    // circleAabbIntersects: corner distance edge cases
    // Corner at (4, 4); distance from (5, 5) to corner is sqrt(2) ~= 1.41421356
    const c_hit_corner = Circle2.init(Point2.init(5.0, 5.0), 1.5);
    const c_miss_corner = Circle2.init(Point2.init(5.0, 5.0), 1.4);
    expect_eq(Geom2D.circleAabbIntersects(c_hit_corner, box_a), true);
    expect_eq(Geom2D.circleAabbIntersects(c_miss_corner, box_a), false);

    // pointToSegmentDist
    const seg = Segment2.init(Point2.init(0.0, 0.0), Point2.init(4.0, 0.0));
    expectNear(Geom2D.pointToSegmentDist(Point2.init(2.0, 3.0), seg), 3.0, tol);
    expectNear(Geom2D.pointToSegmentDist(Point2.init(-3.0, 4.0), seg), 5.0, tol);
    expectNear(Geom2D.pointToSegmentDist(Point2.init(2.5, 0.0), seg), 0.0, tol);

    const deg_seg = Segment2.init(Point2.init(1.0, 1.0), Point2.init(1.0, 1.0));
    expectNear(Geom2D.pointToSegmentDist(Point2.init(4.0, 5.0), deg_seg), 5.0, tol);
}

test("cap.geom2d - dynamic lazy-mount equivalence via import cap") {
    const alloc = std.testing.allocator;
    const tol: f64 = 0.000000001;

    const p1 = cap.geo.Point2.init(0.0, 0.0);
    const p2 = cap.geo.Point2.init(3.0, 4.0);
    expectNear(p1.dist(p2), 5.0, tol);

    const box = cap.geo.Aabb2.init(0.0, 0.0, 10.0, 10.0);
    const cir = cap.geo.Circle2.init(cap.geo.Point2.init(11.0, 5.0), 1.5);
    expect_eq(cap.geo.Geom2D.circleAabbIntersects(cir, box), true);

    const pts = [
        cap.geo.Point2.init(0.0, 0.0),
        cap.geo.Point2.init(4.0, 0.0),
        cap.geo.Point2.init(2.0, 1.0),
        cap.geo.Point2.init(2.0, 3.0),
    ];
    const hull = cap.geo.Geom2D.convexHull(alloc, &pts) catch unreachable;
    defer alloc.free(hull);
    expect_eq(hull.len, 3);
}

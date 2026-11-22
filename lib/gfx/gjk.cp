// Origin: Pure-CP Stdlib (Zero-Alloc 2D GJK Convex Collision, Minkowski Support & SAT MTV Physics) | Created: 2026-11-22

import std from "std";

import { Vec2 } from "./bezier.cp";

const GJK_MAX_ITERATIONS: u32 = 64;

/// Separating Axis Theorem contact manifold with unit separation normal (A -> B) and depth.
pub struct SatContact {
    colliding: bool,
    normal: Vec2,
    depth: f64,

    /// Minimum Translation Vector to push shape B out of shape A (`normal * depth`).
    fn mtv(): Vec2 {
        if (!_.colliding) {
            return Vec2 { x: 0.0, y: 0.0 };
        }
        return _.normal.scale(_.depth);
    }
}

/// Result returned by `gjkIntersects`.
pub struct GjkResult {
    colliding: bool,
    iterations: u32,
}

fn tripleProduct(a: Vec2, b: Vec2, c: Vec2): Vec2 {
    const ac = a.dot(c);
    const bc = b.dot(c);
    return Vec2 {
        x: b.x * ac - a.x * bc,
        y: b.y * ac - a.y * bc,
    };
}

/// Computes the arithmetic centroid of `verts`. Returns `null` if `verts.len == 0`.
pub fn polygonCentroid(verts: []const Vec2): Vec2? {
    if (verts.len == 0) {
        return null;
    }
    let sx: f64 = 0.0;
    let sy: f64 = 0.0;
    let i: usize = 0;
    while (i < verts.len) {
        sx += verts[i].x;
        sy += verts[i].y;
        i += 1;
    }
    const inv = 1.0 / (verts.len as f64);
    return Vec2 { x: sx * inv, y: sy * inv };
}

/// Translates every vertex of `src` by `delta` into `dst` (zero-alloc).
/// Returns `false` if `dst.len < src.len`.
pub fn translatePolygon(src: []const Vec2, delta: Vec2, dst: []Vec2): bool {
    if (dst.len < src.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i].add(delta);
        i += 1;
    }
    return true;
}

/// Returns the support vertex of convex polygon `verts` furthest along `dir`.
/// Returns `null` if `verts.len == 0`.
pub fn polygonSupport(verts: []const Vec2, dir: Vec2): Vec2? {
    if (verts.len == 0) {
        return null;
    }
    let best = verts[0];
    let best_dot = best.dot(dir);
    let i: usize = 1;
    while (i < verts.len) {
        const d = verts[i].dot(dir);
        if (d > best_dot) {
            best_dot = d;
            best = verts[i];
        }
        i += 1;
    }
    return best;
}

/// Computes the Minkowski difference support point `support(A, dir) - support(B, -dir)`.
pub fn minkowskiSupport(poly_a: []const Vec2, poly_b: []const Vec2, dir: Vec2): Vec2? {
    const sa = polygonSupport(poly_a, dir) ?? return null;
    const sb = polygonSupport(poly_b, dir.neg()) ?? return null;
    return sa.sub(sb);
}

/// Tests intersection of two 2D convex polygons using Gilbert-Johnson-Keerthi (GJK)
/// with 2D Line/Triangle simplex evolution and a 64-iteration convergence guard.
pub fn gjkIntersects(poly_a: []const Vec2, poly_b: []const Vec2): GjkResult {
    if (poly_a.len < 3 || poly_b.len < 3) {
        return GjkResult { colliding: false, iterations: 0 };
    }

    const ca = polygonCentroid(poly_a) ?? return GjkResult { colliding: false, iterations: 0 };
    const cb = polygonCentroid(poly_b) ?? return GjkResult { colliding: false, iterations: 0 };
    let dir = cb.sub(ca);
    if (dir.lenSq() <= 1.0e-18) {
        dir = Vec2 { x: 1.0, y: 0.0 };
    }

    const s0 = minkowskiSupport(poly_a, poly_b, dir) ?? return GjkResult { colliding: false, iterations: 0 };
    if (s0.lenSq() <= 1.0e-18) {
        return GjkResult { colliding: true, iterations: 1 };
    }

    let simplex: [3]Vec2 = [s0, Vec2 { x: 0.0, y: 0.0 }, Vec2 { x: 0.0, y: 0.0 }];
    let count: usize = 1;
    dir = s0.neg();

    let iter: u32 = 1;
    while (iter <= GJK_MAX_ITERATIONS) {
        const a = minkowskiSupport(poly_a, poly_b, dir) ?? return GjkResult { colliding: false, iterations: iter };
        if (a.dot(dir) < 0.0) {
            return GjkResult { colliding: false, iterations: iter };
        }

        simplex[count] = a;
        count += 1;

        if (count == 2) {
            const b = simplex[0];
            const ab = b.sub(a);
            const ao = a.neg();
            if (ab.dot(ao) > 0.0) {
                const perp = tripleProduct(ab, ao, ab);
                if (perp.lenSq() <= 1.0e-18) {
                    // Origin lies colinearly on segment AB
                    return GjkResult { colliding: true, iterations: iter };
                }
                dir = perp;
            } else {
                simplex[0] = a;
                count = 1;
                dir = ao;
            }
        } else {
            // Triangle simplex: C = simplex[0], B = simplex[1], A = simplex[2]
            const c = simplex[0];
            const b = simplex[1];
            const ab = b.sub(a);
            const ac = c.sub(a);
            const ao = a.neg();

            const ab_perp = tripleProduct(ac, ab, ab);
            const ac_perp = tripleProduct(ab, ac, ac);

            if (ab_perp.dot(ao) > 0.0) {
                simplex[0] = b;
                simplex[1] = a;
                count = 2;
                dir = ab_perp;
            } else if (ac_perp.dot(ao) > 0.0) {
                simplex[0] = c;
                simplex[1] = a;
                count = 2;
                dir = ac_perp;
            } else {
                return GjkResult { colliding: true, iterations: iter };
            }
        }

        iter += 1;
    }

    return GjkResult { colliding: false, iterations: GJK_MAX_ITERATIONS };
}

struct ProjRange {
    min: f64,
    max: f64,
}

fn projectPolygon(verts: []const Vec2, axis: Vec2): ProjRange {
    let mn = verts[0].dot(axis);
    let mx = mn;
    let i: usize = 1;
    while (i < verts.len) {
        const p = verts[i].dot(axis);
        if (p < mn) { mn = p; }
        if (p > mx) { mx = p; }
        i += 1;
    }
    return ProjRange { min: mn, max: mx };
}

/// Separating Axis Theorem (SAT) collision detection between two 2D convex polygons.
/// Returns `SatContact` with unit separation `normal` pointing from A to B and minimum `depth`.
pub fn satCollide(poly_a: []const Vec2, poly_b: []const Vec2): SatContact {
    const no_hit = SatContact { colliding: false, normal: Vec2 { x: 0.0, y: 0.0 }, depth: 0.0 };
    if (poly_a.len < 3 || poly_b.len < 3) {
        return no_hit;
    }

    let best_depth: f64 = 1.0e300;
    let best_axis = Vec2 { x: 1.0, y: 0.0 };
    let tested_axis = false;

    // Test edge normals of polygon A
    let i: usize = 0;
    while (i < poly_a.len) {
        const next = if (i + 1 < poly_a.len) i + 1 else 0;
        const edge = poly_a[next].sub(poly_a[i]);
        i += 1;
        if (edge.lenSq() <= 1.0e-18) {
            continue;
        }
        const axis = edge.perp().normalize();
        const pa = projectPolygon(poly_a, axis);
        const pb = projectPolygon(poly_b, axis);
        if (pa.max < pb.min || pb.max < pa.min) {
            return no_hit;
        }
        const hi = if (pa.max < pb.max) pa.max else pb.max;
        const lo = if (pa.min > pb.min) pa.min else pb.min;
        const overlap = hi - lo;
        if (!tested_axis || overlap < best_depth) {
            best_depth = overlap;
            best_axis = axis;
            tested_axis = true;
        }
    }

    // Test edge normals of polygon B
    i = 0;
    while (i < poly_b.len) {
        const next = if (i + 1 < poly_b.len) i + 1 else 0;
        const edge = poly_b[next].sub(poly_b[i]);
        i += 1;
        if (edge.lenSq() <= 1.0e-18) {
            continue;
        }
        const axis = edge.perp().normalize();
        const pa = projectPolygon(poly_a, axis);
        const pb = projectPolygon(poly_b, axis);
        if (pa.max < pb.min || pb.max < pa.min) {
            return no_hit;
        }
        const hi = if (pa.max < pb.max) pa.max else pb.max;
        const lo = if (pa.min > pb.min) pa.min else pb.min;
        const overlap = hi - lo;
        if (!tested_axis || overlap < best_depth) {
            best_depth = overlap;
            best_axis = axis;
            tested_axis = true;
        }
    }

    if (!tested_axis) {
        return no_hit;
    }

    // Ensure normal points from A toward B
    const ca = polygonCentroid(poly_a) ?? return no_hit;
    const cb = polygonCentroid(poly_b) ?? return no_hit;
    if (best_axis.dot(cb.sub(ca)) < 0.0) {
        best_axis = best_axis.neg();
    }

    return SatContact {
        colliding: true,
        normal: best_axis,
        depth: best_depth,
    };
}

/// Exact SAT collision detection between a circle `(center, radius)` and a 2D convex polygon.
/// Separation normal points from the circle toward the polygon.
pub fn circlePolygonCollide(center: Vec2, radius: f64, poly: []const Vec2): SatContact {
    const no_hit = SatContact { colliding: false, normal: Vec2 { x: 0.0, y: 0.0 }, depth: 0.0 };
    if (radius < 0.0 || poly.len < 3) {
        return no_hit;
    }

    let best_depth: f64 = 1.0e300;
    let best_axis = Vec2 { x: 1.0, y: 0.0 };
    let tested_axis = false;

    // 1. Test all polygon edge normals
    let i: usize = 0;
    while (i < poly.len) {
        const next = if (i + 1 < poly.len) i + 1 else 0;
        const edge = poly[next].sub(poly[i]);
        i += 1;
        if (edge.lenSq() <= 1.0e-18) {
            continue;
        }
        const axis = edge.perp().normalize();
        const c_proj = center.dot(axis);
        const c_min = c_proj - radius;
        const c_max = c_proj + radius;
        const pp = projectPolygon(poly, axis);

        if (c_max < pp.min || pp.max < c_min) {
            return no_hit;
        }
        const hi = if (c_max < pp.max) c_max else pp.max;
        const lo = if (c_min > pp.min) c_min else pp.min;
        const overlap = hi - lo;
        if (!tested_axis || overlap < best_depth) {
            best_depth = overlap;
            best_axis = axis;
            tested_axis = true;
        }
    }

    // 2. Test Voronoi axis from circle center to closest polygon vertex
    let closest = poly[0];
    let best_dist_sq = closest.sub(center).lenSq();
    i = 1;
    while (i < poly.len) {
        const dsq = poly[i].sub(center).lenSq();
        if (dsq < best_dist_sq) {
            best_dist_sq = dsq;
            closest = poly[i];
        }
        i += 1;
    }

    if (best_dist_sq > 1.0e-18) {
        const axis = closest.sub(center).normalize();
        const c_proj = center.dot(axis);
        const c_min = c_proj - radius;
        const c_max = c_proj + radius;
        const pp = projectPolygon(poly, axis);

        if (c_max < pp.min || pp.max < c_min) {
            return no_hit;
        }
        const hi = if (c_max < pp.max) c_max else pp.max;
        const lo = if (c_min > pp.min) c_min else pp.min;
        const overlap = hi - lo;
        if (!tested_axis || overlap < best_depth) {
            best_depth = overlap;
            best_axis = axis;
            tested_axis = true;
        }
    }

    if (!tested_axis) {
        return no_hit;
    }

    const cp = polygonCentroid(poly) ?? return no_hit;
    if (best_axis.dot(cp.sub(center)) < 0.0) {
        best_axis = best_axis.neg();
    }

    return SatContact {
        colliding: true,
        normal: best_axis,
        depth: best_depth,
    };
}

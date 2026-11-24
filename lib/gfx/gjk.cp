// Reference-hardened against donbright/earcutr (ISC) & dimforge/parry (Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc 2D GJK Convex Collision, 4-Lane SIMD Support & SAT MTV Physics) | Created: 2026-11-22

import std from "std";

import { Vec2 } from "./bezier.cp";

type GjkVec4f64 = @Vector(4, f64);

pub const GJK_MAX_ITERATIONS: u32 = 64;

/// Separating Axis Theorem contact manifold with unit separation normal (A -> B) and depth.
pub struct SatContact {
    colliding: bool,
    normal: Vec2,
    depth: f64,

    /// Minimum Translation Vector to push shape B out of shape A (`normal * depth`).
    pub fn mtv(): Vec2 {
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

fn gjkIsFiniteF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn isFiniteVec2(v: Vec2): bool {
    return gjkIsFiniteF64(v.x) && gjkIsFiniteF64(v.y);
}

fn allFiniteVec2(verts: []const Vec2): bool {
    for (v of verts) {
        if (!isFiniteVec2(v)) return false;
    }
    return true;
}

/// 4-lane `@Vector(4, f64)` SIMD support-vertex index finder maximizing `dot(verts[i], dir)`.
/// Returns `null` on empty `verts` or any `NaN`/`±Inf` in `dir` or `verts`.
pub fn simdGjkSupportIndex(verts: []const Vec2, dir: Vec2): usize? {
    if (verts.len == 0 || !isFiniteVec2(dir) || !allFiniteVec2(verts)) {
        return null;
    }
    let best_idx: usize = 0;
    let best_dot: f64 = verts[0].dot(dir);

    const dx_v: GjkVec4f64 = @splat(dir.x);
    const dy_v: GjkVec4f64 = @splat(dir.y);

    let i: usize = 0;
    while (i + 4 <= verts.len) {
        const vx: GjkVec4f64 = [verts[i].x, verts[i + 1].x, verts[i + 2].x, verts[i + 3].x];
        const vy: GjkVec4f64 = [verts[i].y, verts[i + 1].y, verts[i + 2].y, verts[i + 3].y];
        const dots = vx * dx_v + vy * dy_v;
        if (@reduce(.Max, dots) > best_dot) {
            let j: usize = 0;
            while (j < 4) : (j += 1) {
                const d = verts[i + j].dot(dir);
                if (d > best_dot) {
                    best_dot = d;
                    best_idx = i + j;
                }
            }
        }
        i += 4;
    }
    while (i < verts.len) : (i += 1) {
        const d = verts[i].dot(dir);
        if (d > best_dot) {
            best_dot = d;
            best_idx = i;
        }
    }
    return best_idx;
}

/// Scalar reference for `simdGjkSupportIndex` (differential verification).
pub fn scalarGjkSupportIndex(verts: []const Vec2, dir: Vec2): usize? {
    if (verts.len == 0 || !isFiniteVec2(dir) || !allFiniteVec2(verts)) {
        return null;
    }
    let best_idx: usize = 0;
    let best_dot: f64 = verts[0].dot(dir);
    let i: usize = 1;
    while (i < verts.len) : (i += 1) {
        const d = verts[i].dot(dir);
        if (d > best_dot) {
            best_dot = d;
            best_idx = i;
        }
    }
    return best_idx;
}

fn tripleProduct(a: Vec2, b: Vec2, c: Vec2): Vec2 {
    const ac = a.dot(c);
    const bc = b.dot(c);
    return Vec2 {
        x: b.x * ac - a.x * bc,
        y: b.y * ac - a.y * bc,
    };
}

fn aabbDisjoint(poly_a: []const Vec2, poly_b: []const Vec2): bool {
    let ax0 = poly_a[0].x;
    let ax1 = ax0;
    let ay0 = poly_a[0].y;
    let ay1 = ay0;
    let i: usize = 1;
    while (i < poly_a.len) : (i += 1) {
        if (poly_a[i].x < ax0) ax0 = poly_a[i].x;
        if (poly_a[i].x > ax1) ax1 = poly_a[i].x;
        if (poly_a[i].y < ay0) ay0 = poly_a[i].y;
        if (poly_a[i].y > ay1) ay1 = poly_a[i].y;
    }
    let bx0 = poly_b[0].x;
    let bx1 = bx0;
    let by0 = poly_b[0].y;
    let by1 = by0;
    i = 1;
    while (i < poly_b.len) : (i += 1) {
        if (poly_b[i].x < bx0) bx0 = poly_b[i].x;
        if (poly_b[i].x > bx1) bx1 = poly_b[i].x;
        if (poly_b[i].y < by0) by0 = poly_b[i].y;
        if (poly_b[i].y > by1) by1 = poly_b[i].y;
    }
    return ax1 < bx0 || bx1 < ax0 || ay1 < by0 || by1 < ay0;
}

/// Computes the arithmetic centroid of `verts`. Returns `null` if empty or non-finite.
pub fn polygonCentroid(verts: []const Vec2): Vec2? {
    if (verts.len == 0 || !allFiniteVec2(verts)) {
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
    const out = Vec2 { x: sx * inv, y: sy * inv };
    if (!isFiniteVec2(out)) return null;
    return out;
}

/// Translates every vertex of `src` by `delta` into `dst` (zero-alloc).
/// Returns `false` if `dst.len < src.len` or `delta` is non-finite.
pub fn translatePolygon(src: []const Vec2, delta: Vec2, dst: []Vec2): bool {
    if (dst.len < src.len || !isFiniteVec2(delta)) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i].add(delta);
        i += 1;
    }
    return true;
}

/// Returns the support vertex of convex polygon `verts` furthest along `dir` via 4-lane SIMD.
/// Returns `null` if `verts.len == 0` or any coordinate is non-finite.
pub fn polygonSupport(verts: []const Vec2, dir: Vec2): Vec2? {
    const idx = simdGjkSupportIndex(verts, dir) ?? return null;
    return verts[idx];
}

/// Computes the Minkowski difference support point `support(A, dir) - support(B, -dir)`.
pub fn minkowskiSupport(poly_a: []const Vec2, poly_b: []const Vec2, dir: Vec2): Vec2? {
    if (!isFiniteVec2(dir) || dir.lenSq() <= 1.0e-24) return null;
    const sa = polygonSupport(poly_a, dir) ?? return null;
    const sb = polygonSupport(poly_b, dir.neg()) ?? return null;
    return sa.sub(sb);
}

/// Tests intersection of two 2D convex polygons using Gilbert-Johnson-Keerthi (GJK)
/// with 4-lane SIMD support mapping, strict `NaN`/`±Inf`/degenerate-simplex guards,
/// symmetric AABB broadphase, and a 64-iteration convergence guard.
pub fn gjkIntersects(poly_a: []const Vec2, poly_b: []const Vec2): GjkResult {
    if (poly_a.len < 3 || poly_b.len < 3) {
        return GjkResult { colliding: false, iterations: 0 };
    }
    if (!allFiniteVec2(poly_a) || !allFiniteVec2(poly_b)) {
        return GjkResult { colliding: false, iterations: 0 };
    }
    if (aabbDisjoint(poly_a, poly_b)) {
        return GjkResult { colliding: false, iterations: 0 };
    }

    const ca = polygonCentroid(poly_a) ?? return GjkResult { colliding: false, iterations: 0 };
    const cb = polygonCentroid(poly_b) ?? return GjkResult { colliding: false, iterations: 0 };
    let dir = cb.sub(ca);
    if (!isFiniteVec2(dir) || dir.lenSq() <= 1.0e-18) {
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
        if (!isFiniteVec2(dir) || dir.lenSq() <= 1.0e-24) {
            return GjkResult { colliding: false, iterations: iter };
        }
        const a = minkowskiSupport(poly_a, poly_b, dir) ?? return GjkResult { colliding: false, iterations: iter };
        if (!isFiniteVec2(a)) {
            return GjkResult { colliding: false, iterations: iter };
        }
        if (a.dot(dir) < 0.0) {
            return GjkResult { colliding: false, iterations: iter };
        }

        // Degenerate simplex stagnation guard (prevents infinite oscillation on touching/duplicate vertices)
        let dup_hit = false;
        let si: usize = 0;
        while (si < count) : (si += 1) {
            if (a.sub(simplex[si]).lenSq() <= 1.0e-24) {
                dup_hit = true;
                break;
            }
        }
        if (dup_hit) {
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
                    return GjkResult { colliding: true, iterations: iter };
                }
                dir = perp;
            } else {
                simplex[0] = a;
                count = 1;
                dir = ao;
            }
        } else {
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
    if (poly_a.len < 3 || poly_b.len < 3 || !allFiniteVec2(poly_a) || !allFiniteVec2(poly_b)) {
        return no_hit;
    }

    let best_depth: f64 = 1.0e300;
    let best_axis = Vec2 { x: 1.0, y: 0.0 };
    let tested_axis = false;

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
    if (!isFiniteVec2(center) || !gjkIsFiniteF64(radius) || radius < 0.0 || poly.len < 3 || !allFiniteVec2(poly)) {
        return no_hit;
    }

    let best_depth: f64 = 1.0e300;
    let best_axis = Vec2 { x: 1.0, y: 0.0 };
    let tested_axis = false;

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

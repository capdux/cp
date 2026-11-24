// Reference-hardened against donbright/earcutr (ISC) & dimforge/parry (Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc 2D Polygon Ear-Clipping Triangulation with Holes & 4-Lane SIMD AABB) | Created: 2026-11-22

const std = @import("std");

type EarcutVec4f64 = @Vector(4, f64);

const EPS: f64 = 1.0e-12;
pub const EARCUT_MAX_ITERATIONS: usize = 65536;

/// 2D floating-point coordinate.
pub struct Point2D {
    x: f64;
    y: f64;
}

/// Indexed triangle referencing vertex indices `(a, b, c)` in counter-clockwise order.
pub struct TriangleIdx {
    a: usize;
    b: usize;
    c: usize;
}

/// 2D axis-aligned bounding box computed by `simdPolyBounds2d` / `scalarPolyBounds2d`.
pub struct PolyBounds2D {
    min_x: f64 = 0.0;
    min_y: f64 = 0.0;
    max_x: f64 = 0.0;
    max_y: f64 = 0.0;
    valid: bool = false;
}

fn earcutIsFiniteF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn isFinitePt(p: Point2D): bool {
    return earcutIsFiniteF64(p.x) && earcutIsFiniteF64(p.y);
}

fn allFinitePts(pts: []const Point2D): bool {
    for (p of pts) {
        if (!isFinitePt(p)) return false;
    }
    return true;
}

/// 4-lane `@Vector(4, f64)` SIMD 2D polygon AABB scanner.
/// Returns `valid: false` if `pts` is empty or contains any `NaN`/`±Inf`.
pub fn simdPolyBounds2d(pts: []const Point2D): PolyBounds2D {
    if (pts.len == 0 || !allFinitePts(pts)) {
        return PolyBounds2D{};
    }
    let min_xv: EarcutVec4f64 = @splat(pts[0].x);
    let max_xv: EarcutVec4f64 = @splat(pts[0].x);
    let min_yv: EarcutVec4f64 = @splat(pts[0].y);
    let max_yv: EarcutVec4f64 = @splat(pts[0].y);

    let i: usize = 0;
    while (i + 4 <= pts.len) {
        const vx: EarcutVec4f64 = [pts[i].x, pts[i + 1].x, pts[i + 2].x, pts[i + 3].x];
        const vy: EarcutVec4f64 = [pts[i].y, pts[i + 1].y, pts[i + 2].y, pts[i + 3].y];
        min_xv = @min(min_xv, vx);
        max_xv = @max(max_xv, vx);
        min_yv = @min(min_yv, vy);
        max_yv = @max(max_yv, vy);
        i += 4;
    }

    let min_x: f64 = @reduce(.Min, min_xv);
    let max_x: f64 = @reduce(.Max, max_xv);
    let min_y: f64 = @reduce(.Min, min_yv);
    let max_y: f64 = @reduce(.Max, max_yv);

    while (i < pts.len) : (i += 1) {
        if (pts[i].x < min_x) min_x = pts[i].x;
        if (pts[i].x > max_x) max_x = pts[i].x;
        if (pts[i].y < min_y) min_y = pts[i].y;
        if (pts[i].y > max_y) max_y = pts[i].y;
    }

    return PolyBounds2D{
        min_x: min_x,
        min_y: min_y,
        max_x: max_x,
        max_y: max_y,
        valid: true,
    };
}

/// Scalar reference for `simdPolyBounds2d` (differential verification).
pub fn scalarPolyBounds2d(pts: []const Point2D): PolyBounds2D {
    if (pts.len == 0 || !allFinitePts(pts)) {
        return PolyBounds2D{};
    }
    let min_x = pts[0].x;
    let max_x = pts[0].x;
    let min_y = pts[0].y;
    let max_y = pts[0].y;
    let i: usize = 1;
    while (i < pts.len) : (i += 1) {
        if (pts[i].x < min_x) min_x = pts[i].x;
        if (pts[i].x > max_x) max_x = pts[i].x;
        if (pts[i].y < min_y) min_y = pts[i].y;
        if (pts[i].y > max_y) max_y = pts[i].y;
    }
    return PolyBounds2D{
        min_x: min_x,
        min_y: min_y,
        max_x: max_x,
        max_y: max_y,
        valid: true,
    };
}

fn earcutAbsF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn cross2d(a: Point2D, b: Point2D, c: Point2D): f64 {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

fn samePoint(a: Point2D, b: Point2D): bool {
    return earcutAbsF64(a.x - b.x) <= EPS && earcutAbsF64(a.y - b.y) <= EPS;
}

/// Computes the signed area of a 2D polygon using the shoelace formula.
/// Positive for counter-clockwise (CCW) winding, negative for clockwise (CW) winding.
/// Returns `0.0` on `< 3` vertices or non-finite coordinates.
pub fn signedArea(pts: []const Point2D): f64 {
    if (pts.len < 3 || !allFinitePts(pts)) {
        return 0.0;
    }
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < pts.len) {
        const j = if (i + 1 == pts.len) 0 as usize else i + 1;
        acc = acc + (pts[i].x * pts[j].y - pts[j].x * pts[i].y);
        i += 1;
    }
    const res = acc * 0.5;
    return if (earcutIsFiniteF64(res)) res else 0.0;
}

/// Computes the non-negative area of a 2D polygon.
pub fn polygonArea(pts: []const Point2D): f64 {
    return earcutAbsF64(signedArea(pts));
}

/// Returns `true` if `pts` has strictly positive counter-clockwise (CCW) signed area.
pub fn isCounterClockwise(pts: []const Point2D): bool {
    return signedArea(pts) > 0.0;
}

/// Returns `true` if `pts` forms a non-degenerate convex polygon (all consecutive turns have the same sign).
pub fn isConvex(pts: []const Point2D): bool {
    if (pts.len < 3 || !allFinitePts(pts)) {
        return false;
    }
    let has_pos: bool = false;
    let has_neg: bool = false;
    let i: usize = 0;
    while (i < pts.len) {
        const a = pts[i];
        const b = pts[(i + 1) % pts.len];
        const c = pts[(i + 2) % pts.len];
        const cr = cross2d(a, b, c);
        if (cr > EPS) {
            has_pos = true;
        } else if (cr < 0.0 - EPS) {
            has_neg = true;
        }
        if (has_pos && has_neg) {
            return false;
        }
        i += 1;
    }
    return has_pos || has_neg;
}

/// Returns `true` if point `p` lies inside or on the boundary of non-degenerate triangle `(a, b, c)`.
pub fn pointInTriangle(p: Point2D, a: Point2D, b: Point2D, c: Point2D): bool {
    if (!isFinitePt(p) || !isFinitePt(a) || !isFinitePt(b) || !isFinitePt(c)) {
        return false;
    }
    if (earcutAbsF64(cross2d(a, b, c)) <= 1.0e-15) {
        return false;
    }
    const c1 = cross2d(a, b, p);
    const c2 = cross2d(b, c, p);
    const c3 = cross2d(c, a, p);

    const has_neg = (c1 < 0.0 - EPS) || (c2 < 0.0 - EPS) || (c3 < 0.0 - EPS);
    const has_pos = (c1 > EPS) || (c2 > EPS) || (c3 > EPS);
    return !(has_neg && has_pos);
}

/// Ray-casting point-in-polygon test for arbitrary simple (convex or concave) polygons.
pub fn pointInPolygon(p: Point2D, pts: []const Point2D): bool {
    if (pts.len < 3 || !isFinitePt(p) || !allFinitePts(pts)) {
        return false;
    }
    const bounds = simdPolyBounds2d(pts);
    if (!bounds.valid || p.x < bounds.min_x - EPS || p.x > bounds.max_x + EPS ||
        p.y < bounds.min_y - EPS || p.y > bounds.max_y + EPS)
    {
        return false;
    }
    let inside: bool = false;
    let i: usize = 0;
    let j: usize = pts.len - 1;
    while (i < pts.len) {
        const xi = pts[i].x;
        const yi = pts[i].y;
        const xj = pts[j].x;
        const yj = pts[j].y;

        const crosses_y = (yi > p.y) != (yj > p.y);
        if (crosses_y) {
            const x_intersect = (xj - xi) * (p.y - yi) / (yj - yi) + xi;
            if (p.x < x_intersect) {
                inside = !inside;
            }
        }
        j = i;
        i += 1;
    }
    return inside;
}

fn isEar(pts: []const Point2D, ring: []const usize, rem: usize, k: usize, allow_collinear: bool): bool {
    const prev_pos = if (k == 0) rem - 1 else k - 1;
    const next_pos = if (k + 1 == rem) 0 as usize else k + 1;

    const a = pts[ring[prev_pos]];
    const b = pts[ring[k]];
    const c = pts[ring[next_pos]];

    const cr = cross2d(a, b, c);
    if (allow_collinear) {
        if (cr < 0.0 - EPS) {
            return false;
        }
    } else {
        if (cr <= EPS) {
            return false;
        }
    }

    let m: usize = 0;
    while (m < rem) {
        if (m != prev_pos && m != k && m != next_pos) {
            const p = pts[ring[m]];
            if (!samePoint(p, a) && !samePoint(p, b) && !samePoint(p, c)) {
                const c1 = cross2d(a, b, p);
                const c2 = cross2d(b, c, p);
                const c3 = cross2d(c, a, p);
                if (c1 >= 0.0 - EPS && c2 >= 0.0 - EPS && c3 >= 0.0 - EPS) {
                    return false;
                }
            }
        }
        m += 1;
    }
    return true;
}

fn earclipRing(pts: []const Point2D, ring: []usize, ring_len: usize, out_tris: []TriangleIdx): usize? {
    // Strip consecutive duplicate vertices from working ring
    let rem: usize = 0;
    let i: usize = 0;
    while (i < ring_len) : (i += 1) {
        const idx = ring[i];
        if (rem > 0 && samePoint(pts[ring[rem - 1]], pts[idx])) {
            continue;
        }
        ring[rem] = idx;
        rem += 1;
    }
    while (rem > 1 && samePoint(pts[ring[0]], pts[ring[rem - 1]])) {
        rem -= 1;
    }
    if (rem < 3 || out_tris.len < rem - 2) {
        return null;
    }

    let tri_count: usize = 0;
    let iters: usize = 0;

    while (rem > 3) {
        iters += 1;
        if (iters > EARCUT_MAX_ITERATIONS) {
            return null;
        }
        let found: bool = false;
        let ear_k: usize = 0;

        let k: usize = 0;
        while (k < rem) {
            if (isEar(pts, ring[0..rem], rem, k, false)) {
                ear_k = k;
                found = true;
                break;
            }
            k += 1;
        }

        if (!found) {
            let k2: usize = 0;
            while (k2 < rem) {
                if (isEar(pts, ring[0..rem], rem, k2, true)) {
                    ear_k = k2;
                    found = true;
                    break;
                }
                k2 += 1;
            }
        }

        if (!found) {
            return null;
        }

        const prev_pos = if (ear_k == 0) rem - 1 else ear_k - 1;
        const next_pos = if (ear_k + 1 == rem) 0 as usize else ear_k + 1;

        out_tris[tri_count] = TriangleIdx {
            a: ring[prev_pos],
            b: ring[ear_k],
            c: ring[next_pos],
        };
        tri_count += 1;

        let shift = ear_k;
        while (shift + 1 < rem) {
            ring[shift] = ring[shift + 1];
            shift += 1;
        }
        rem -= 1;
    }

    out_tris[tri_count] = TriangleIdx {
        a: ring[0],
        b: ring[1],
        c: ring[2],
    };
    tri_count += 1;

    return tri_count;
}

/// Triangulates a simple 2D polygon (`pts`) via zero-allocation ear-clipping.
/// Strictly rejects `NaN`/`±Inf`, `< 3` vertices, degenerate zero-area/collinear polygons,
/// and enforces `EARCUT_MAX_ITERATIONS`.
pub fn triangulate(pts: []const Point2D, out_tris: []TriangleIdx, scratch_idx: []usize): usize? {
    const n = pts.len;
    if (n < 3 || out_tris.len < n - 2 || scratch_idx.len < n) {
        return null;
    }
    const bounds = simdPolyBounds2d(pts);
    if (!bounds.valid || (bounds.max_x - bounds.min_x) <= EPS || (bounds.max_y - bounds.min_y) <= EPS) {
        return null;
    }
    const area = signedArea(pts);
    if (earcutAbsF64(area) <= EPS) {
        return null;
    }

    let i: usize = 0;
    if (area > 0.0) {
        while (i < n) : (i += 1) {
            scratch_idx[i] = i;
        }
    } else {
        while (i < n) : (i += 1) {
            scratch_idx[i] = n - 1 - i;
        }
    }

    return earclipRing(pts, scratch_idx, n, out_tris);
}

fn findBridgeVertex(pts: []const Point2D, ring: []const usize, ring_len: usize, hole_pt: Point2D): usize? {
    // Cast horizontal ray (+X) from hole_pt to find closest visible edge intersection on outer ring
    let best_x: f64 = std.math.inf(f64);
    let best_edge_pos: usize = 0;
    let found_edge = false;

    let i: usize = 0;
    while (i < ring_len) : (i += 1) {
        const j = if (i + 1 == ring_len) 0 as usize else i + 1;
        const a = pts[ring[i]];
        const b = pts[ring[j]];
        const min_y = if (a.y < b.y) a.y else b.y;
        const max_y = if (a.y > b.y) a.y else b.y;
        if (hole_pt.y < min_y - EPS || hole_pt.y > max_y + EPS) continue;
        if (earcutAbsF64(a.y - b.y) <= EPS) {
            // Horizontal edge
            const ex = if (a.x > b.x) a.x else b.x;
            if (ex >= hole_pt.x - EPS && ex < best_x) {
                best_x = ex;
                best_edge_pos = if (a.x >= b.x) i else j;
                found_edge = true;
            }
            continue;
        }
        const t = (hole_pt.y - a.y) / (b.y - a.y);
        if (t < 0.0 - EPS || t > 1.0 + EPS) continue;
        const ix = a.x + t * (b.x - a.x);
        if (ix >= hole_pt.x - EPS && ix < best_x) {
            best_x = ix;
            best_edge_pos = if (a.x >= b.x) i else j;
            found_edge = true;
        }
    }
    if (!found_edge) return null;

    // Check if any reflex vertex inside triangle (hole_pt, I, P) is closer in angle
    const intersect_pt = Point2D{ x: best_x, y: hole_pt.y };
    let bridge_pos = best_edge_pos;
    const p_pt = pts[ring[bridge_pos]];
    let best_tan: f64 = std.math.inf(f64);
    let best_dist_sq: f64 = std.math.inf(f64);
    let found_reflex = false;

    let k_ref: usize = 0;
    while (k_ref < ring_len) : (k_ref += 1) {
        if (k_ref == best_edge_pos) continue;
        const v = pts[ring[k_ref]];
        if (v.x < hole_pt.x - EPS) continue;
        if (pointInTriangle(v, hole_pt, intersect_pt, p_pt)) {
            const dx = v.x - hole_pt.x;
            const dy = earcutAbsF64(v.y - hole_pt.y);
            const tan_val = if (dx > EPS) dy / dx else std.math.inf(f64);
            const dsq = dx * dx + dy * dy;
            if (!found_reflex || tan_val < best_tan - EPS || (earcutAbsF64(tan_val - best_tan) <= EPS && dsq < best_dist_sq)) {
                found_reflex = true;
                best_tan = tan_val;
                best_dist_sq = dsq;
                bridge_pos = k_ref;
            }
        }
    }
    return bridge_pos;
}

/// Triangulates a 2D polygon with optional holes (`hole_indices` specifies the strictly ascending
/// start indices of each hole ring in `pts`).
/// - Rejects out-of-bounds, unsorted, or `< 3`-vertex outer/hole rings and non-finite coordinates.
/// - Requires `scratch_idx.len >= pts.len + 2 * hole_indices.len`.
pub fn triangulateWithHoles(
    pts: []const Point2D,
    hole_indices: []const usize,
    out_tris: []TriangleIdx,
    scratch_idx: []usize,
): usize? {
    if (hole_indices.len == 0) {
        return triangulate(pts, out_tris, scratch_idx);
    }
    if (pts.len < 6 || !allFinitePts(pts)) {
        return null;
    }

    // Validate hole_indices strictly ascending & every ring has >= 3 vertices
    const outer_end = hole_indices[0];
    if (outer_end < 3 || outer_end >= pts.len) {
        return null;
    }
    let hi: usize = 0;
    while (hi < hole_indices.len) : (hi += 1) {
        const h_start = hole_indices[hi];
        const h_end = if (hi + 1 < hole_indices.len) hole_indices[hi + 1] else pts.len;
        if (h_end <= h_start || (h_end - h_start) < 3 || h_end > pts.len) {
            return null;
        }
    }

    const needed_scratch = pts.len + 2 * hole_indices.len;
    if (scratch_idx.len < needed_scratch) {
        return null;
    }

    // Initialize outer ring in CCW order
    const outer_area = signedArea(pts[0..outer_end]);
    if (earcutAbsF64(outer_area) <= EPS) {
        return null;
    }
    let ring_len: usize = outer_end;
    let i: usize = 0;
    if (outer_area > 0.0) {
        while (i < outer_end) : (i += 1) {
            scratch_idx[i] = i;
        }
    } else {
        while (i < outer_end) : (i += 1) {
            scratch_idx[i] = outer_end - 1 - i;
        }
    }

    // Splice each hole (in order of rightmost vertex X descending; up to 32 holes inline)
    if (hole_indices.len > 32) return null;
    let hole_order: [32]usize = [0; 32];
    let hole_max_x: [32]f64 = [0.0; 32];
    let hole_right_v: [32]usize = [0; 32];

    let h_scan: usize = 0;
    while (h_scan < hole_indices.len) : (h_scan += 1) {
        const h_start = hole_indices[h_scan];
        const h_end = if (h_scan + 1 < hole_indices.len) hole_indices[h_scan + 1] else pts.len;
        if (earcutAbsF64(signedArea(pts[h_start..h_end])) <= EPS) return null;
        let max_x = pts[h_start].x;
        let rv = h_start;
        let k = h_start + 1;
        while (k < h_end) : (k += 1) {
            if (pts[k].x > max_x) {
                max_x = pts[k].x;
                rv = k;
            }
        }
        hole_order[h_scan] = h_scan;
        hole_max_x[h_scan] = max_x;
        hole_right_v[h_scan] = rv;
    }

    // Insertion sort holes by max_x descending
    let a: usize = 1;
    while (a < hole_indices.len) : (a += 1) {
        const key_o = hole_order[a];
        const key_x = hole_max_x[key_o];
        let b = a;
        while (b > 0 && hole_max_x[hole_order[b - 1]] < key_x) {
            hole_order[b] = hole_order[b - 1];
            b -= 1;
        }
        hole_order[b] = key_o;
    }

    hi = 0;
    while (hi < hole_indices.len) : (hi += 1) {
        const h_id = hole_order[hi];
        const h_start = hole_indices[h_id];
        const h_end = if (h_id + 1 < hole_indices.len) hole_indices[h_id + 1] else pts.len;
        const h_len = h_end - h_start;
        const rv = hole_right_v[h_id];

        const bridge_pos = findBridgeVertex(pts, scratch_idx[0..ring_len], ring_len, pts[rv]) ?? return null;
        const bridge_v = scratch_idx[bridge_pos];
        const hole_cw = signedArea(pts[h_start..h_end]) < 0.0;

        // Insert (h_len + 2) vertices right after bridge_pos: hole ring in CW order starting at rv, closing rv, then bridge_v
        const insert_n = h_len + 2;
        if (ring_len + insert_n > scratch_idx.len) return null;
        let sh = ring_len;
        while (sh > bridge_pos + 1) {
            sh -= 1;
            scratch_idx[sh + insert_n] = scratch_idx[sh];
        }

        let w = bridge_pos + 1;
        const rv_rel = rv - h_start;
        let step: usize = 0;
        while (step < h_len) : (step += 1) {
            const rel = if (hole_cw)
                (rv_rel + step) % h_len
            else
                (rv_rel + h_len - (step % h_len)) % h_len;
            scratch_idx[w] = h_start + rel;
            w += 1;
        }
        scratch_idx[w] = rv;
        w += 1;
        scratch_idx[w] = bridge_v;
        ring_len += insert_n;
    }

    return earclipRing(pts, scratch_idx, ring_len, out_tris);
}

/// Computes the sum of unsigned areas of all indexed triangles in `tris`.
/// For a valid triangulation, `triangulationTotalArea(pts, tris) == polygonArea(pts)`.
pub fn triangulationTotalArea(pts: []const Point2D, tris: []const TriangleIdx): f64 {
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < tris.len) {
        const t = tris[i];
        if (t.a < pts.len && t.b < pts.len && t.c < pts.len) {
            total = total + 0.5 * earcutAbsF64(cross2d(pts[t.a], pts[t.b], pts[t.c]));
        }
        i += 1;
    }
    return total;
}

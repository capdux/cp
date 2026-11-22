// Origin: Pure-CP Stdlib (Zero-Alloc 2D Bowyer-Watson Incremental Delaunay Triangulation) | Created: 2026-11-22

const DEGEN_EPS: f64 = 1.0e-18;
const IN_TRI_EPS: f64 = 1.0e-12;

/// 2D Cartesian point.
pub struct Point2 {
    x: f64,
    y: f64,
}

/// Counter-clockwise (CCW) triangle referencing vertex indices in the input point slice.
pub struct Triangle {
    a: usize,
    b: usize,
    c: usize,
}

/// Directed / undirected 2D edge between two vertex indices.
pub struct Edge2 {
    u: usize,
    v: usize,
}

/// Circumcircle of a non-degenerate triangle.
pub struct Circumcircle {
    cx: f64,
    cy: f64,
    radiusSq: f64,
}

/// Caller-owned zero-allocation workspace for Bowyer-Watson triangulation.
/// Recommended sizes for `n` points:
/// - `tris.len >= delaunayMinTriPool(n)` (`2 * n + 16`)
/// - `bad.len >= tris.len`
/// - `edges.len >= delaunayMinEdgePool(n)` (`6 * n + 48`)
/// - `edge_dup.len >= edges.len`
pub struct DelaunayWorkspace {
    tris: []Triangle,
    bad: []u8,
    edges: []Edge2,
    edge_dup: []u8,
}

/// Recommended minimum triangle pool capacity for `n` input points.
pub fn delaunayMinTriPool(n: usize): usize {
    return (2 as usize) * n + 16;
}

/// Recommended minimum boundary edge pool capacity for `n` input points.
pub fn delaunayMinEdgePool(n: usize): usize {
    return (6 as usize) * n + 48;
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

/// Signed double area `orient2d(a, b, c) = cross(b - a, c - a)`.
/// Positive if `(a, b, c)` is counter-clockwise (CCW), negative if clockwise, 0 if collinear.
pub fn orient2d(a: Point2, b: Point2, c: Point2): f64 {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/// Computes the circumcircle of triangle `(a, b, c)`.
/// Returns `null` if the points are collinear or contain non-finite coordinates.
pub fn circumcircle(a: Point2, b: Point2, c: Point2): Circumcircle? {
    if (!isFiniteF64(a.x) || !isFiniteF64(a.y)
        || !isFiniteF64(b.x) || !isFiniteF64(b.y)
        || !isFiniteF64(c.x) || !isFiniteF64(c.y)) {
        return null;
    }
    const ax: f64 = b.x - a.x;
    const ay: f64 = b.y - a.y;
    const bx: f64 = c.x - a.x;
    const by: f64 = c.y - a.y;
    const d: f64 = 2.0 * (ax * by - ay * bx);
    if (absF64(d) <= DEGEN_EPS) {
        return null;
    }
    const a2: f64 = ax * ax + ay * ay;
    const b2: f64 = bx * bx + by * by;
    const ux: f64 = (by * a2 - ay * b2) / d;
    const uy: f64 = (ax * b2 - bx * a2) / d;
    const rsq: f64 = ux * ux + uy * uy;
    if (!isFiniteF64(ux) || !isFiniteF64(uy) || !isFiniteF64(rsq)) {
        return null;
    }
    return Circumcircle {
        cx: a.x + ux,
        cy: a.y + uy,
        radiusSq: rsq,
    };
}

/// Returns `true` if point `p` lies strictly inside the circumcircle of triangle `(a, b, c)`.
pub fn inCircumcircle(a: Point2, b: Point2, c: Point2, p: Point2): bool {
    const orient: f64 = orient2d(a, b, c);
    if (absF64(orient) <= DEGEN_EPS) {
        return false;
    }
    const adx: f64 = a.x - p.x;
    const ady: f64 = a.y - p.y;
    const bdx: f64 = b.x - p.x;
    const bdy: f64 = b.y - p.y;
    const cdx: f64 = c.x - p.x;
    const cdy: f64 = c.y - p.y;

    const abdet: f64 = adx * bdy - bdx * ady;
    const bcdet: f64 = bdx * cdy - cdx * bdy;
    const cadet: f64 = cdx * ady - adx * cdy;

    const alift: f64 = adx * adx + ady * ady;
    const blift: f64 = bdx * bdx + bdy * bdy;
    const clift: f64 = cdx * cdx + cdy * cdy;

    const det: f64 = alift * bcdet + blift * cadet + clift * abdet;
    return if (orient > 0.0) (det > 0.0) else (det < 0.0);
}

/// Returns `true` if point `p` lies inside or on the boundary of triangle `(a, b, c)`.
pub fn pointInTriangle(a: Point2, b: Point2, c: Point2, p: Point2): bool {
    const area2: f64 = orient2d(a, b, c);
    const abs_area: f64 = absF64(area2);
    if (abs_area <= DEGEN_EPS) {
        return false;
    }
    const tol: f64 = IN_TRI_EPS * abs_area;
    const o1: f64 = orient2d(a, b, p);
    const o2: f64 = orient2d(b, c, p);
    const o3: f64 = orient2d(c, a, p);
    if (area2 > 0.0) {
        return o1 >= (0.0 - tol) && o2 >= (0.0 - tol) && o3 >= (0.0 - tol);
    }
    return o1 <= tol && o2 <= tol && o3 <= tol;
}

fn fetchPoint(points: []const Point2, super_pts: []const Point2, idx: usize): Point2 {
    if (idx < points.len) {
        return points[idx];
    }
    return super_pts[idx - points.len];
}

fn edgesUndirectedEqual(e1: Edge2, e2: Edge2): bool {
    return (e1.u == e2.u && e1.v == e2.v) || (e1.u == e2.v && e1.v == e2.u);
}

/// Performs zero-allocation 2D Bowyer-Watson incremental Delaunay triangulation.
/// - Writes resulting CCW triangles into `out_triangles[0..count]` and returns `count`.
/// - Automatically skips duplicate/coincident points; returns `0` for `< 3` points or all-collinear sets.
/// - Returns `null` if any coordinate is NaN/Inf or caller buffers/workspace are insufficient.
pub fn triangulate(
    points: []const Point2,
    out_triangles: *[]Triangle,
    ws: *DelaunayWorkspace,
): usize? {
    const n: usize = points.len;
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(points[chk].x) || !isFiniteF64(points[chk].y)) {
            return null;
        }
        chk += 1;
    }

    if (n < 3) {
        return 0 as usize;
    }
    if (ws.*.tris.len < 4 || ws.*.bad.len < 4 || ws.*.edges.len < 6 || ws.*.edge_dup.len < 6) {
        return null;
    }

    // Bounding box of input points
    let min_x: f64 = points[0].x;
    let max_x: f64 = points[0].x;
    let min_y: f64 = points[0].y;
    let max_y: f64 = points[0].y;
    let bi: usize = 1;
    while (bi < n) {
        const pt: Point2 = points[bi];
        if (pt.x < min_x) { min_x = pt.x; }
        if (pt.x > max_x) { max_x = pt.x; }
        if (pt.y < min_y) { min_y = pt.y; }
        if (pt.y > max_y) { max_y = pt.y; }
        bi += 1;
    }

    const dx: f64 = max_x - min_x;
    const dy: f64 = max_y - min_y;
    const dmax: f64 = if (dx > dy) dx else dy;
    if (dmax <= 1.0e-15) {
        return 0 as usize;
    }

    // Construct oversized CCW super-triangle enclosing all points
    const mid_x: f64 = 0.5 * (min_x + max_x);
    const mid_y: f64 = 0.5 * (min_y + max_y);
    const scale: f64 = 20.0 * dmax;
    const super_pts: [3]Point2 = [
        Point2 { x: mid_x - scale, y: mid_y - scale },
        Point2 { x: mid_x + scale, y: mid_y - scale },
        Point2 { x: mid_x,         y: mid_y + 2.0 * scale },
    ];
    const super_slice: []const Point2 = super_pts[0..];

    ws.*.tris[0] = Triangle { a: n, b: n + 1, c: n + 2 };
    let tri_count: usize = 1;
    const dup_tol_sq: f64 = (1.0e-12 * dmax) * (1.0e-12 * dmax);

    let pi: usize = 0;
    while (pi < n) {
        const p: Point2 = points[pi];

        // Skip duplicate / coincident points
        let is_dup: bool = false;
        let pj: usize = 0;
        while (pj < pi) {
            const ddx: f64 = p.x - points[pj].x;
            const ddy: f64 = p.y - points[pj].y;
            if ((ddx * ddx + ddy * ddy) <= dup_tol_sq) {
                is_dup = true;
                break;
            }
            pj += 1;
        }
        if (is_dup) {
            pi += 1;
            continue;
        }

        if (tri_count > ws.*.bad.len) {
            return null;
        }

        // Find all bad triangles whose circumcircle contains p
        let edge_count: usize = 0;
        let t: usize = 0;
        while (t < tri_count) {
            const tri: Triangle = ws.*.tris[t];
            const pa: Point2 = fetchPoint(points, super_slice, tri.a);
            const pb: Point2 = fetchPoint(points, super_slice, tri.b);
            const pc: Point2 = fetchPoint(points, super_slice, tri.c);
            if (inCircumcircle(pa, pb, pc, p)) {
                ws.*.bad[t] = 1;
                if (edge_count + 3 > ws.*.edges.len || edge_count + 3 > ws.*.edge_dup.len) {
                    return null;
                }
                ws.*.edges[edge_count] = Edge2 { u: tri.a, v: tri.b };
                ws.*.edges[edge_count + 1] = Edge2 { u: tri.b, v: tri.c };
                ws.*.edges[edge_count + 2] = Edge2 { u: tri.c, v: tri.a };
                ws.*.edge_dup[edge_count] = 0;
                ws.*.edge_dup[edge_count + 1] = 0;
                ws.*.edge_dup[edge_count + 2] = 0;
                edge_count += 3;
            } else {
                ws.*.bad[t] = 0;
            }
            t += 1;
        }

        // Mark shared interior edges of the polygonal hole
        let e1: usize = 0;
        while (e1 < edge_count) {
            if (ws.*.edge_dup[e1] == 0) {
                let e2: usize = e1 + 1;
                while (e2 < edge_count) {
                    if (edgesUndirectedEqual(ws.*.edges[e1], ws.*.edges[e2])) {
                        ws.*.edge_dup[e1] = 1;
                        ws.*.edge_dup[e2] = 1;
                    }
                    e2 += 1;
                }
            }
            e1 += 1;
        }

        // Compact remaining good triangles in-place
        let kept: usize = 0;
        let tc: usize = 0;
        while (tc < tri_count) {
            if (ws.*.bad[tc] == 0) {
                ws.*.tris[kept] = ws.*.tris[tc];
                kept += 1;
            }
            tc += 1;
        }
        tri_count = kept;

        // Re-triangulate the polygonal cavity with CCW orientation
        let eb: usize = 0;
        while (eb < edge_count) {
            if (ws.*.edge_dup[eb] == 0) {
                const edge: Edge2 = ws.*.edges[eb];
                const pu: Point2 = fetchPoint(points, super_slice, edge.u);
                const pv: Point2 = fetchPoint(points, super_slice, edge.v);
                const o: f64 = orient2d(pu, pv, p);
                if (absF64(o) > DEGEN_EPS) {
                    if (tri_count >= ws.*.tris.len) {
                        return null;
                    }
                    if (o > 0.0) {
                        ws.*.tris[tri_count] = Triangle { a: edge.u, b: edge.v, c: pi };
                    } else {
                        ws.*.tris[tri_count] = Triangle { a: edge.v, b: edge.u, c: pi };
                    }
                    tri_count += 1;
                }
            }
            eb += 1;
        }

        pi += 1;
    }

    // Filter out triangles touching super-triangle vertices or near-zero collinear slivers
    const min_area: f64 = 1.0e-13 * dmax * dmax;
    let out_count: usize = 0;
    let tf: usize = 0;
    while (tf < tri_count) {
        const tri: Triangle = ws.*.tris[tf];
        if (tri.a < n && tri.b < n && tri.c < n) {
            const o: f64 = orient2d(points[tri.a], points[tri.b], points[tri.c]);
            if (o > min_area) {
                if (out_count >= out_triangles.*.len) {
                    return null;
                }
                out_triangles.*[out_count] = tri;
                out_count += 1;
            }
        }
        tf += 1;
    }

    return out_count;
}

/// Verifies that every triangle is strictly CCW, references valid distinct vertices,
/// and satisfies the Delaunay empty-circumcircle criterion within relative tolerance `eps`.
pub fn verifyDelaunayEmptyCircle(
    points: []const Point2,
    triangles: []const Triangle,
    eps: f64,
): bool {
    if (!isFiniteF64(eps) || eps < 0.0) {
        return false;
    }
    const n: usize = points.len;
    let t: usize = 0;
    while (t < triangles.len) {
        const tri: Triangle = triangles[t];
        if (tri.a >= n || tri.b >= n || tri.c >= n) {
            return false;
        }
        if (tri.a == tri.b || tri.b == tri.c || tri.a == tri.c) {
            return false;
        }
        const pa: Point2 = points[tri.a];
        const pb: Point2 = points[tri.b];
        const pc: Point2 = points[tri.c];
        if (orient2d(pa, pb, pc) <= 0.0) {
            return false;
        }
        const cc: Circumcircle = circumcircle(pa, pb, pc) ?? return false;
        const limit: f64 = cc.radiusSq - eps * (1.0 + cc.radiusSq);

        let k: usize = 0;
        while (k < n) {
            if (k != tri.a && k != tri.b && k != tri.c) {
                const pk: Point2 = points[k];
                const ddx: f64 = pk.x - cc.cx;
                const ddy: f64 = pk.y - cc.cy;
                if ((ddx * ddx + ddy * ddy) < limit) {
                    return false;
                }
            }
            k += 1;
        }
        t += 1;
    }
    return true;
}

/// Locates the index of a triangle containing point `p` (interior or boundary).
/// Returns `null` if `p` is outside all triangles or non-finite.
pub fn locateTriangle(
    points: []const Point2,
    triangles: []const Triangle,
    p: Point2,
): usize? {
    if (!isFiniteF64(p.x) || !isFiniteF64(p.y)) {
        return null;
    }
    const n: usize = points.len;
    let t: usize = 0;
    while (t < triangles.len) {
        const tri: Triangle = triangles[t];
        if (tri.a < n && tri.b < n && tri.c < n) {
            if (pointInTriangle(points[tri.a], points[tri.b], points[tri.c], p)) {
                return t;
            }
        }
        t += 1;
    }
    return null;
}

// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc 2D Simple Polygon Ear-Clipping Triangulation & Geometry Primitives)

const EPS: f64 = 1.0e-12;

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

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn cross2d(a: Point2D, b: Point2D, c: Point2D): f64 {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

fn samePoint(a: Point2D, b: Point2D): bool {
    return absF64(a.x - b.x) <= EPS && absF64(a.y - b.y) <= EPS;
}

/// Computes the signed area of a 2D polygon using the shoelace formula.
/// Positive for counter-clockwise (CCW) winding, negative for clockwise (CW) winding.
pub fn signedArea(pts: []const Point2D): f64 {
    if (pts.len < 3) {
        return 0.0;
    }
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < pts.len) {
        const j = if (i + 1 == pts.len) 0 as usize else i + 1;
        acc = acc + (pts[i].x * pts[j].y - pts[j].x * pts[i].y);
        i += 1;
    }
    return acc * 0.5;
}

/// Computes the non-negative area of a 2D polygon.
pub fn polygonArea(pts: []const Point2D): f64 {
    return absF64(signedArea(pts));
}

/// Returns `true` if `pts` has strictly positive counter-clockwise (CCW) signed area.
pub fn isCounterClockwise(pts: []const Point2D): bool {
    return signedArea(pts) > 0.0;
}

/// Returns `true` if `pts` forms a non-degenerate convex polygon (all consecutive turns have the same sign).
pub fn isConvex(pts: []const Point2D): bool {
    if (pts.len < 3) {
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
    if (absF64(cross2d(a, b, c)) <= 1.0e-15) {
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
    if (pts.len < 3) {
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

    // Ensure no other remaining vertex lies inside or on triangle (a, b, c)
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

/// Triangulates a simple 2D polygon (`pts`) via zero-allocation ear-clipping.
/// - Automatically normalizes both CCW and CW input winding to CCW output triangles.
/// - Requires `pts.len >= 3`, `out_tris.len >= pts.len - 2`, `scratch_idx.len >= pts.len`,
///   and non-zero polygon area.
/// - Returns number of triangles written (`pts.len - 2`), or `null` on invalid/degenerate input.
pub fn triangulate(pts: []const Point2D, out_tris: []TriangleIdx, scratch_idx: []usize): usize? {
    const n = pts.len;
    if (n < 3 || out_tris.len < n - 2 || scratch_idx.len < n) {
        return null;
    }
    const area = signedArea(pts);
    if (absF64(area) <= EPS) {
        return null;
    }

    // Populate working vertex ring in CCW order
    let i: usize = 0;
    if (area > 0.0) {
        while (i < n) {
            scratch_idx[i] = i;
            i += 1;
        }
    } else {
        while (i < n) {
            scratch_idx[i] = n - 1 - i;
            i += 1;
        }
    }

    let rem: usize = n;
    let tri_count: usize = 0;

    while (rem > 3) {
        let found: bool = false;
        let ear_k: usize = 0;

        // First pass: strictly convex ears
        let k: usize = 0;
        while (k < rem) {
            if (isEar(pts, scratch_idx[0..rem], rem, k, false)) {
                ear_k = k;
                found = true;
                break;
            }
            k += 1;
        }

        // Fallback pass: allow collinear boundary vertices if needed
        if (!found) {
            let k2: usize = 0;
            while (k2 < rem) {
                if (isEar(pts, scratch_idx[0..rem], rem, k2, true)) {
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
            a: scratch_idx[prev_pos],
            b: scratch_idx[ear_k],
            c: scratch_idx[next_pos],
        };
        tri_count += 1;

        // Remove ear_k from working ring
        let shift = ear_k;
        while (shift + 1 < rem) {
            scratch_idx[shift] = scratch_idx[shift + 1];
            shift += 1;
        }
        rem -= 1;
    }

    // Final remaining triangle
    out_tris[tri_count] = TriangleIdx {
        a: scratch_idx[0],
        b: scratch_idx[1],
        c: scratch_idx[2],
    };
    tri_count += 1;

    return tri_count;
}

/// Computes the sum of unsigned areas of all indexed triangles in `tris`.
/// For a valid triangulation, `triangulationTotalArea(pts, tris) == polygonArea(pts)`.
pub fn triangulationTotalArea(pts: []const Point2D, tris: []const TriangleIdx): f64 {
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < tris.len) {
        const t = tris[i];
        if (t.a < pts.len && t.b < pts.len && t.c < pts.len) {
            total = total + 0.5 * absF64(cross2d(pts[t.a], pts[t.b], pts[t.c]));
        }
        i += 1;
    }
    return total;
}

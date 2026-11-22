// Origin: Pure-CP Stdlib (Zero-Alloc 2D Bounded Voronoi Diagram via Half-Plane Clipping & Lloyd Relaxation) | Created: 2026-11-22

import std from "std";

const COINCIDENT_EPS2: f64 = 1.0e-24;
const VERTEX_DEDUP_EPS2: f64 = 1.0e-24;
const AREA_DEGEN_EPS: f64 = 1.0e-15;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn clampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/// 2D Euclidean point.
pub struct Point2 {
    x: f64;
    y: f64;
}

/// Axis-aligned 2D rectangular bounding box `[min_x, max_x] x [min_y, max_y]`.
pub struct Bounds2 {
    min_x: f64;
    min_y: f64;
    max_x: f64;
    max_y: f64;

    /// Returns `true` if all coordinates are finite and `min_x < max_x && min_y < max_y`.
    fn isValid(): bool {
        if (!isFiniteF64(_.min_x) || !isFiniteF64(_.min_y) || !isFiniteF64(_.max_x) || !isFiniteF64(_.max_y)) {
            return false;
        }
        return _.min_x < _.max_x && _.min_y < _.max_y;
    }

    /// Returns the area of the bounding box (or `0.0` if invalid).
    fn area(): f64 {
        if (!_.isValid()) {
            return 0.0;
        }
        return (_.max_x - _.min_x) * (_.max_y - _.min_y);
    }

    /// Returns `true` if `p` lies inside or on the boundary of the box.
    fn contains(p: Point2): bool {
        return p.x >= _.min_x && p.x <= _.max_x && p.y >= _.min_y && p.y <= _.max_y;
    }
}

/// Geometric summary of a clipped Voronoi cell polygon.
pub struct CellMetrics {
    vertex_count: usize;
    area: f64;
    centroid: Point2;
}

fn appendDedupPoint(out: []Point2, out_len: *usize, p: Point2): bool {
    if (out_len.* > 0) {
        const last = out[out_len.* - 1];
        const dx = p.x - last.x;
        const dy = p.y - last.y;
        if (dx * dx + dy * dy <= VERTEX_DEDUP_EPS2) {
            return true;
        }
    }
    if (out_len.* >= out.len) {
        return false;
    }
    out[out_len.*] = p;
    out_len.* += 1;
    return true;
}

/// Clips a convex polygon `poly_in` against the half-plane `a * x + b * y <= c`
/// using the Sutherland-Hodgman algorithm, writing the resulting vertices into `poly_out`.
///
/// Returns the number of output vertices (`0` if clipped away completely),
/// or `null` if `(a, b, c)` contains non-finite values or `poly_out` is too small.
pub fn clipPolygonHalfPlane(
    poly_in: []const Point2,
    a: f64,
    b: f64,
    c: f64,
    poly_out: []Point2,
): usize? {
    if (!isFiniteF64(a) || !isFiniteF64(b) || !isFiniteF64(c)) {
        return null;
    }
    const n = poly_in.len;
    if (n == 0) {
        return 0;
    }

    const norm = std.math.sqrt(a * a + b * b);
    if (norm == 0.0) {
        if (c >= 0.0) {
            if (poly_out.len < n) {
                return null;
            }
            let k: usize = 0;
            while (k < n) {
                poly_out[k] = poly_in[k];
                k += 1;
            }
            return n;
        }
        return 0;
    }

    const eps = 1.0e-12 * norm;
    let out_len: usize = 0;
    let prev = poly_in[n - 1];
    let d_prev = a * prev.x + b * prev.y - c;
    let prev_in = d_prev <= eps;

    let i: usize = 0;
    while (i < n) {
        const curr = poly_in[i];
        const d_curr = a * curr.x + b * curr.y - c;
        const curr_in = d_curr <= eps;

        if (prev_in && curr_in) {
            if (!appendDedupPoint(poly_out, &out_len, curr)) {
                return null;
            }
        } else if (prev_in && !curr_in) {
            const denom = d_prev - d_curr;
            const t = if (denom != 0.0) clampF64(d_prev / denom, 0.0, 1.0) else 0.0;
            const inter = Point2 {
                x: prev.x + t * (curr.x - prev.x),
                y: prev.y + t * (curr.y - prev.y),
            };
            if (!appendDedupPoint(poly_out, &out_len, inter)) {
                return null;
            }
        } else if (!prev_in && curr_in) {
            const denom = d_prev - d_curr;
            const t = if (denom != 0.0) clampF64(d_prev / denom, 0.0, 1.0) else 0.0;
            const inter = Point2 {
                x: prev.x + t * (curr.x - prev.x),
                y: prev.y + t * (curr.y - prev.y),
            };
            if (!appendDedupPoint(poly_out, &out_len, inter)) {
                return null;
            }
            if (!appendDedupPoint(poly_out, &out_len, curr)) {
                return null;
            }
        }

        prev = curr;
        d_prev = d_curr;
        prev_in = curr_in;
        i += 1;
    }

    // Remove wrap-around duplicate between last and first vertex
    while (out_len > 1) {
        const first = poly_out[0];
        const last = poly_out[out_len - 1];
        const dx = first.x - last.x;
        const dy = first.y - last.y;
        if (dx * dx + dy * dy <= VERTEX_DEDUP_EPS2) {
            out_len -= 1;
        } else {
            break;
        }
    }

    if (out_len < 3) {
        return 0;
    }
    return out_len;
}

/// Computes the unsigned area of a simple polygon using the Shoelace formula.
pub fn polygonArea(poly: []const Point2): f64 {
    const n = poly.len;
    if (n < 3) {
        return 0.0;
    }
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const p = poly[i];
        const q = poly[if (i + 1 < n) i + 1 else 0];
        acc = acc + (p.x * q.y - q.x * p.y);
        i += 1;
    }
    return 0.5 * absF64(acc);
}

/// Computes the area-weighted centroid of a simple polygon, or `null` if `< 3` vertices or degenerate zero area.
pub fn polygonCentroid(poly: []const Point2): Point2? {
    const n = poly.len;
    if (n < 3) {
        return null;
    }
    let area2: f64 = 0.0;
    let cx_acc: f64 = 0.0;
    let cy_acc: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const p = poly[i];
        const q = poly[if (i + 1 < n) i + 1 else 0];
        const cross = p.x * q.y - q.x * p.y;
        area2 = area2 + cross;
        cx_acc = cx_acc + (p.x + q.x) * cross;
        cy_acc = cy_acc + (p.y + q.y) * cross;
        i += 1;
    }
    if (absF64(area2) <= AREA_DEGEN_EPS) {
        return null;
    }
    const inv = 1.0 / (3.0 * area2);
    return Point2 {
        x: cx_acc * inv,
        y: cy_acc * inv,
    };
}

/// Computes the bounded 2D Voronoi cell for `sites[site_idx]` within `bounds`
/// via ping-pong half-plane clipping against all perpendicular bisectors.
///
/// Writes the resulting CCW convex polygon vertices into `out_poly[0..metrics.vertex_count]`.
/// Automatically skips coincident duplicate sites.
/// Returns `null` if `sites` is empty, `site_idx >= sites.len`, `bounds` is invalid,
/// any site contains non-finite coordinates, or `out_poly` / `scratch_poly` overflows.
pub fn computeVoronoiCell(
    sites: []const Point2,
    site_idx: usize,
    bounds: Bounds2,
    out_poly: []Point2,
    scratch_poly: []Point2,
): CellMetrics? {
    if (sites.len == 0 || site_idx >= sites.len || !bounds.isValid()) {
        return null;
    }
    if (out_poly.len < 4 || scratch_poly.len < 4) {
        return null;
    }

    let chk: usize = 0;
    while (chk < sites.len) {
        if (!isFiniteF64(sites[chk].x) || !isFiniteF64(sites[chk].y)) {
            return null;
        }
        chk += 1;
    }

    // Initialize CCW bounding rectangle in out_poly
    out_poly[0] = Point2 { x: bounds.min_x, y: bounds.min_y };
    out_poly[1] = Point2 { x: bounds.max_x, y: bounds.min_y };
    out_poly[2] = Point2 { x: bounds.max_x, y: bounds.max_y };
    out_poly[3] = Point2 { x: bounds.min_x, y: bounds.max_y };

    let cur_len: usize = 4;
    let src_is_out = true;
    const p = sites[site_idx];

    let j: usize = 0;
    while (j < sites.len && cur_len > 0) {
        if (j != site_idx) {
            const q = sites[j];
            const dx = q.x - p.x;
            const dy = q.y - p.y;
            if (dx * dx + dy * dy > COINCIDENT_EPS2) {
                const mx = 0.5 * (p.x + q.x);
                const my = 0.5 * (p.y + q.y);
                const c = dx * mx + dy * my;

                if (src_is_out) {
                    const next_len = clipPolygonHalfPlane(out_poly[0..cur_len], dx, dy, c, scratch_poly) ?? return null;
                    cur_len = next_len;
                    src_is_out = false;
                } else {
                    const next_len = clipPolygonHalfPlane(scratch_poly[0..cur_len], dx, dy, c, out_poly) ?? return null;
                    cur_len = next_len;
                    src_is_out = true;
                }
            }
        }
        j += 1;
    }

    if (!src_is_out && cur_len > 0) {
        if (out_poly.len < cur_len) {
            return null;
        }
        let k: usize = 0;
        while (k < cur_len) {
            out_poly[k] = scratch_poly[k];
            k += 1;
        }
    }

    const area = polygonArea(out_poly[0..cur_len]);
    const centroid = polygonCentroid(out_poly[0..cur_len]) ?? p;
    return CellMetrics {
        vertex_count: cur_len,
        area: area,
        centroid: centroid,
    };
}

/// Performs one iteration of Lloyd's relaxation: moves each site to the centroid of its
/// bounded Voronoi cell, writing updated coordinates to `next_sites_out[0..sites.len]`.
///
/// Returns the maximum Euclidean displacement across all sites, or `null` on invalid inputs / buffer overflow.
pub fn lloydRelaxationStep(
    sites: []const Point2,
    bounds: Bounds2,
    next_sites_out: []Point2,
    poly_buf_a: []Point2,
    poly_buf_b: []Point2,
): f64? {
    if (sites.len == 0 || !bounds.isValid() || next_sites_out.len < sites.len) {
        return null;
    }
    if (poly_buf_a.len < 4 || poly_buf_b.len < 4) {
        return null;
    }

    let max_disp: f64 = 0.0;
    let i: usize = 0;
    while (i < sites.len) {
        const metrics = computeVoronoiCell(sites, i, bounds, poly_buf_a, poly_buf_b) ?? return null;
        const clamped = Point2 {
            x: clampF64(metrics.centroid.x, bounds.min_x, bounds.max_x),
            y: clampF64(metrics.centroid.y, bounds.min_y, bounds.max_y),
        };
        next_sites_out[i] = clamped;
        const dx = clamped.x - sites[i].x;
        const dy = clamped.y - sites[i].y;
        const disp = std.math.sqrt(dx * dx + dy * dy);
        if (disp > max_disp) {
            max_disp = disp;
        }
        i += 1;
    }
    return max_disp;
}

/// Returns the index of the Euclidean nearest site to `p` (smallest index on tie),
/// or `null` if `sites` is empty or any coordinate is non-finite.
pub fn nearestSite(sites: []const Point2, p: Point2): usize? {
    if (sites.len == 0 || !isFiniteF64(p.x) || !isFiniteF64(p.y)) {
        return null;
    }
    let best_idx: usize = 0;
    let best_d2: f64 = 0.0;
    let i: usize = 0;
    while (i < sites.len) {
        const s = sites[i];
        if (!isFiniteF64(s.x) || !isFiniteF64(s.y)) {
            return null;
        }
        const dx = s.x - p.x;
        const dy = s.y - p.y;
        const d2 = dx * dx + dy * dy;
        if (i == 0 || d2 < best_d2) {
            best_d2 = d2;
            best_idx = i;
        }
        i += 1;
    }
    return best_idx;
}

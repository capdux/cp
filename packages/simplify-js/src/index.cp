// simplify-js — Zero-heap two-stage Radial Distance + Ramer-Douglas-Peucker 2D/3D polyline simplification
// Clean-room MIT implementation of `simplify-js` in pure CP.

pub const SIMPLIFY_MAX_STACK_PTS: usize = 1024;

pub struct Pt2 {
    pub x: f64,
    pub y: f64,
}

pub type Point2D = Pt2;

pub struct Point3D {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

/// Squared Euclidean distance between two 2D points.
pub fn getSqDist(p1: Pt2, p2: Pt2): f64 {
    const dx = p1.x - p2.x;
    const dy = p1.y - p2.y;
    return dx * dx + dy * dy;
}

/// Squared perpendicular distance from 2D point `p` to segment `[a, b]`.
pub fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64 {
    let x = a.x;
    let y = a.y;
    let dx = b.x - x;
    let dy = b.y - y;
    const len_sq = dx * dx + dy * dy;
    if (len_sq != 0.0) {
        let t = ((p.x - x) * dx + (p.y - y) * dy) / len_sq;
        if (t > 1.0) {
            x = b.x;
            y = b.y;
        } else if (t > 0.0) {
            x += dx * t;
            y += dy * t;
        }
    }
    dx = p.x - x;
    dy = p.y - y;
    return dx * dx + dy * dy;
}

/// Alias for `pointToSegmentDistSq` matching `simplify-js` naming.
pub fn getSqSegDist(p: Pt2, p1: Pt2, p2: Pt2): f64 {
    return pointToSegmentDistSq(p, p1, p2);
}

/// Squared Euclidean distance between two 3D points.
pub fn getSqDist3D(p1: Point3D, p2: Point3D): f64 {
    const dx = p1.x - p2.x;
    const dy = p1.y - p2.y;
    const dz = p1.z - p2.z;
    return dx * dx + dy * dy + dz * dz;
}

/// Squared perpendicular distance from 3D point `p` to segment `[a, b]`.
pub fn getSqSegDist3D(p: Point3D, a: Point3D, b: Point3D): f64 {
    let x = a.x;
    let y = a.y;
    let z = a.z;
    let dx = b.x - x;
    let dy = b.y - y;
    let dz = b.z - z;
    const len_sq = dx * dx + dy * dy + dz * dz;
    if (len_sq != 0.0) {
        let t = ((p.x - x) * dx + (p.y - y) * dy + (p.z - z) * dz) / len_sq;
        if (t > 1.0) {
            x = b.x;
            y = b.y;
            z = b.z;
        } else if (t > 0.0) {
            x += dx * t;
            y += dy * t;
            z += dz * t;
        }
    }
    dx = p.x - x;
    dy = p.y - y;
    dz = p.z - z;
    return dx * dx + dy * dy + dz * dz;
}

/// Stage 1 (2D): Radial distance pre-filtering with squared tolerance `sqTolerance`.
/// Always preserves the first and last point. Returns number of points written into `out`.
pub fn simplifyRadialDist(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize {
    const n = points.len;
    if (n == 0 || out.len == 0) { return 0; }
    let prev = points[0];
    out[0] = prev;
    let written: usize = 1;
    if (n == 1) { return written; }

    let i: usize = 1;
    while (i < n) {
        const pt = points[i];
        if (getSqDist(pt, prev) > sqTolerance) {
            if (written < out.len) {
                out[written] = pt;
                written += 1;
            }
            prev = pt;
        }
        i += 1;
    }

    const last = points[n - 1];
    if (prev.x != last.x || prev.y != last.y) {
        if (written < out.len) {
            out[written] = last;
            written += 1;
        }
    }
    return written;
}

/// Core iterative Ramer-Douglas-Peucker 2D simplification using caller-provided `keep_mask`.
pub fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize {
    let n = pts.len;
    if (n == 0) { return 0; }
    if (n <= 2) {
        let i: usize = 0;
        let lim = if (n < out.len) n else out.len;
        while (i < lim) {
            out[i] = pts[i];
            i += 1;
        }
        return lim;
    }
    if (keep_mask.len < n) { return 0; }

    let i: usize = 0;
    while (i < n) {
        keep_mask[i] = 0;
        i += 1;
    }
    keep_mask[0] = 1;
    keep_mask[n - 1] = 1;

    const eps_sq = epsilon * epsilon;
    let stack_start: [256]usize = undefined;
    let stack_end: [256]usize = undefined;
    let sp: usize = 0;
    stack_start[sp] = 0;
    stack_end[sp] = n - 1;
    sp = 1;

    while (sp > 0) {
        sp -= 1;
        const first = stack_start[sp];
        const last = stack_end[sp];
        let max_dist_sq: f64 = -1.0;
        let max_idx: usize = first;

        let k: usize = first + 1;
        while (k < last) {
            const d2 = pointToSegmentDistSq(pts[k], pts[first], pts[last]);
            if (d2 > max_dist_sq) {
                max_dist_sq = d2;
                max_idx = k;
            }
            k += 1;
        }

        if (max_dist_sq > eps_sq && max_idx > first && max_idx < last) {
            keep_mask[max_idx] = 1;
            if (sp + 2 <= 256) {
                stack_start[sp] = first;
                stack_end[sp] = max_idx;
                sp += 1;
                stack_start[sp] = max_idx;
                stack_end[sp] = last;
                sp += 1;
            }
        }
    }

    let written: usize = 0;
    let j: usize = 0;
    while (j < n && written < out.len) {
        if (keep_mask[j] != 0) {
            out[written] = pts[j];
            written += 1;
        }
        j += 1;
    }
    return written;
}

/// Stage 2 (2D): Iterative Ramer-Douglas-Peucker simplification taking squared tolerance `sqTolerance`.
/// Zero-heap using an internal stack mask (supports up to 1024 points).
pub fn simplifyDouglasPeucker(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize {
    const n = points.len;
    if (n == 0 || out.len == 0) { return 0; }
    if (n <= 2) {
        let i: usize = 0;
        const lim = if (n < out.len) n else out.len;
        while (i < lim) {
            out[i] = points[i];
            i += 1;
        }
        return lim;
    }

    const limit = if (n > SIMPLIFY_MAX_STACK_PTS) SIMPLIFY_MAX_STACK_PTS else n;
    let keep: [1024]u8 = undefined;
    let i: usize = 0;
    while (i < limit) {
        keep[i] = 0;
        i += 1;
    }
    keep[0] = 1;
    keep[limit - 1] = 1;

    let stack_start: [256]usize = undefined;
    let stack_end: [256]usize = undefined;
    let sp: usize = 0;
    stack_start[0] = 0;
    stack_end[0] = limit - 1;
    sp = 1;

    while (sp > 0) {
        sp -= 1;
        const first = stack_start[sp];
        const last = stack_end[sp];
        let max_sq: f64 = sqTolerance;
        let max_idx: usize = first;

        let k: usize = first + 1;
        while (k < last) {
            const d2 = pointToSegmentDistSq(points[k], points[first], points[last]);
            if (d2 > max_sq) {
                max_sq = d2;
                max_idx = k;
            }
            k += 1;
        }

        if (max_sq > sqTolerance && max_idx > first && max_idx < last) {
            keep[max_idx] = 1;
            if (sp + 2 <= 256) {
                stack_start[sp] = first;
                stack_end[sp] = max_idx;
                sp += 1;
                stack_start[sp] = max_idx;
                stack_end[sp] = last;
                sp += 1;
            }
        }
    }

    let written: usize = 0;
    let j: usize = 0;
    while (j < limit && written < out.len) {
        if (keep[j] != 0) {
            out[written] = points[j];
            written += 1;
        }
        j += 1;
    }
    return written;
}

/// Complete `simplify-js` 2D pipeline.
/// When `highQuality` is `false`, runs fast Radial Distance pass followed by Douglas-Peucker.
/// When `highQuality` is `true`, runs Douglas-Peucker directly on all points.
pub fn simplify(points: []const Pt2, tolerance: f64, highQuality: bool, out: []Pt2): usize {
    const n = points.len;
    if (n <= 2) {
        let i: usize = 0;
        const lim = if (n < out.len) n else out.len;
        while (i < lim) {
            out[i] = points[i];
            i += 1;
        }
        return lim;
    }
    const tol = if (tolerance > 0.0) tolerance else 0.0;
    const sq_tol = tol * tol;
    if (highQuality) {
        return simplifyDouglasPeucker(points, sq_tol, out);
    }
    let radial_buf: [1024]Pt2 = undefined;
    const rn = simplifyRadialDist(points, sq_tol, radial_buf[0..1024]);
    return simplifyDouglasPeucker(radial_buf[0..rn], sq_tol, out);
}

/// Stage 1 (3D): Radial distance pre-filtering with squared tolerance `sqTolerance`.
pub fn simplifyRadialDist3D(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize {
    const n = points.len;
    if (n == 0 || out.len == 0) { return 0; }
    let prev = points[0];
    out[0] = prev;
    let written: usize = 1;
    if (n == 1) { return written; }

    let i: usize = 1;
    while (i < n) {
        const pt = points[i];
        if (getSqDist3D(pt, prev) > sqTolerance) {
            if (written < out.len) {
                out[written] = pt;
                written += 1;
            }
            prev = pt;
        }
        i += 1;
    }

    const last = points[n - 1];
    if (prev.x != last.x || prev.y != last.y || prev.z != last.z) {
        if (written < out.len) {
            out[written] = last;
            written += 1;
        }
    }
    return written;
}

/// Stage 2 (3D): Iterative Ramer-Douglas-Peucker simplification with squared tolerance `sqTolerance`.
pub fn simplifyDouglasPeucker3D(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize {
    const n = points.len;
    if (n == 0 || out.len == 0) { return 0; }
    if (n <= 2) {
        let i: usize = 0;
        const lim = if (n < out.len) n else out.len;
        while (i < lim) {
            out[i] = points[i];
            i += 1;
        }
        return lim;
    }

    const limit = if (n > SIMPLIFY_MAX_STACK_PTS) SIMPLIFY_MAX_STACK_PTS else n;
    let keep: [1024]u8 = undefined;
    let i: usize = 0;
    while (i < limit) {
        keep[i] = 0;
        i += 1;
    }
    keep[0] = 1;
    keep[limit - 1] = 1;

    let stack_start: [256]usize = undefined;
    let stack_end: [256]usize = undefined;
    let sp: usize = 0;
    stack_start[0] = 0;
    stack_end[0] = limit - 1;
    sp = 1;

    while (sp > 0) {
        sp -= 1;
        const first = stack_start[sp];
        const last = stack_end[sp];
        let max_sq: f64 = sqTolerance;
        let max_idx: usize = first;

        let k: usize = first + 1;
        while (k < last) {
            const d2 = getSqSegDist3D(points[k], points[first], points[last]);
            if (d2 > max_sq) {
                max_sq = d2;
                max_idx = k;
            }
            k += 1;
        }

        if (max_sq > sqTolerance && max_idx > first && max_idx < last) {
            keep[max_idx] = 1;
            if (sp + 2 <= 256) {
                stack_start[sp] = first;
                stack_end[sp] = max_idx;
                sp += 1;
                stack_start[sp] = max_idx;
                stack_end[sp] = last;
                sp += 1;
            }
        }
    }

    let written: usize = 0;
    let j: usize = 0;
    while (j < limit && written < out.len) {
        if (keep[j] != 0) {
            out[written] = points[j];
            written += 1;
        }
        j += 1;
    }
    return written;
}

/// Complete `simplify-js` 3D pipeline with optional radial distance pre-pass.
pub fn simplify3D(points: []const Point3D, tolerance: f64, highQuality: bool, out: []Point3D): usize {
    const n = points.len;
    if (n <= 2) {
        let i: usize = 0;
        const lim = if (n < out.len) n else out.len;
        while (i < lim) {
            out[i] = points[i];
            i += 1;
        }
        return lim;
    }
    const tol = if (tolerance > 0.0) tolerance else 0.0;
    const sq_tol = tol * tol;
    if (highQuality) {
        return simplifyDouglasPeucker3D(points, sq_tol, out);
    }
    let radial_buf: [1024]Point3D = undefined;
    const rn = simplifyRadialDist3D(points, sq_tol, radial_buf[0..1024]);
    return simplifyDouglasPeucker3D(radial_buf[0..rn], sq_tol, out);
}

export {
    SIMPLIFY_MAX_STACK_PTS,
    Pt2,
    Point2D,
    Point3D,
    getSqDist,
    pointToSegmentDistSq,
    getSqSegDist,
    getSqDist3D,
    getSqSegDist3D,
    simplifyRadialDist,
    simplifyPolyline,
    simplifyDouglasPeucker,
    simplify,
    simplifyRadialDist3D,
    simplifyDouglasPeucker3D,
    simplify3D,
};

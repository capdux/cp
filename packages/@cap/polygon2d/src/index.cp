// @cap/polygon2d — Zero-heap 2D polygon geometry algorithms in pure CP
// MIT License — Clean-room implementation with zero external dependencies

const EPS_EDGE: f64 = 0.000000001;
const EPS_AREA: f64 = 0.000000000001;

pub struct Pt2 {
    pub x: f64,
    pub y: f64,
}

/// Absolute value of an f64.
pub fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

/// Square root via Newton-Raphson iteration (returns 0.0 for x <= 0.0).
pub fn sqrtF64(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = if (x > 1.0) x * 0.5 else 1.0;
    let i: u32 = 0;
    while (i < 48) {
        const next: f64 = 0.5 * (g + x / g);
        if (absF64(next - g) <= 0.000000000000001 * next) {
            return next;
        }
        g = next;
        i += 1;
    }
    return g;
}

/// Signed area via the Shoelace formula.
/// Positive for counter-clockwise (CCW) winding, negative for clockwise (CW), 0.0 for < 3 vertices.
pub fn signedArea(pts: []const Pt2): f64 {
    if (pts.len < 3) {
        return 0.0;
    }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < pts.len) {
        const j: usize = if (i + 1 < pts.len) i + 1 else 0;
        sum = sum + (pts[i].x * pts[j].y - pts[j].x * pts[i].y);
        i += 1;
    }
    return 0.5 * sum;
}

/// Unsigned (absolute) area of the polygon.
pub fn area(pts: []const Pt2): f64 {
    return absF64(signedArea(pts));
}

/// Return true if polygon vertices are wound counter-clockwise (signedArea > 0).
pub fn isCounterClockwise(pts: []const Pt2): bool {
    return signedArea(pts) > 0.0;
}

/// Closed perimeter length of the polygon (0.0 for < 2 vertices).
pub fn perimeter(pts: []const Pt2): f64 {
    if (pts.len < 2) {
        return 0.0;
    }
    if (pts.len == 2) {
        const dx: f64 = pts[1].x - pts[0].x;
        const dy: f64 = pts[1].y - pts[0].y;
        return sqrtF64(dx * dx + dy * dy);
    }
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < pts.len) {
        const j: usize = if (i + 1 < pts.len) i + 1 else 0;
        const dx: f64 = pts[j].x - pts[i].x;
        const dy: f64 = pts[j].y - pts[i].y;
        total = total + sqrtF64(dx * dx + dy * dy);
        i += 1;
    }
    return total;
}

/// Geometric centroid (center of mass) of the polygon.
/// Falls back to vertex mean when degenerate (< 3 vertices or near-zero area), or (0, 0) when empty.
pub fn centroid(pts: []const Pt2): Pt2 {
    if (pts.len == 0) {
        return Pt2 { x: 0.0, y: 0.0 };
    }
    if (pts.len == 1) {
        return pts[0];
    }
    const a: f64 = signedArea(pts);
    if (pts.len < 3 || absF64(a) <= EPS_AREA) {
        let sx: f64 = 0.0;
        let sy: f64 = 0.0;
        let i: usize = 0;
        while (i < pts.len) {
            sx = sx + pts[i].x;
            sy = sy + pts[i].y;
            i += 1;
        }
        const n: f64 = pts.len as f64;
        return Pt2 { x: sx / n, y: sy / n };
    }
    let cx: f64 = 0.0;
    let cy: f64 = 0.0;
    let i: usize = 0;
    while (i < pts.len) {
        const j: usize = if (i + 1 < pts.len) i + 1 else 0;
        const cross: f64 = pts[i].x * pts[j].y - pts[j].x * pts[i].y;
        cx = cx + (pts[i].x + pts[j].x) * cross;
        cy = cy + (pts[i].y + pts[j].y) * cross;
        i += 1;
    }
    const factor: f64 = 6.0 * a;
    return Pt2 { x: cx / factor, y: cy / factor };
}

fn pointOnSegment(p: Pt2, a: Pt2, b: Pt2): bool {
    const abx: f64 = b.x - a.x;
    const aby: f64 = b.y - a.y;
    const apx: f64 = p.x - a.x;
    const apy: f64 = p.y - a.y;
    const cross: f64 = apx * aby - apy * abx;
    const scale: f64 = absF64(abx) + absF64(aby) + 1.0;
    if (absF64(cross) > EPS_EDGE * scale) {
        return false;
    }
    const min_x: f64 = if (a.x < b.x) a.x else b.x;
    const max_x: f64 = if (a.x > b.x) a.x else b.x;
    const min_y: f64 = if (a.y < b.y) a.y else b.y;
    const max_y: f64 = if (a.y > b.y) a.y else b.y;
    return p.x >= min_x - EPS_EDGE &&
        p.x <= max_x + EPS_EDGE &&
        p.y >= min_y - EPS_EDGE &&
        p.y <= max_y + EPS_EDGE;
}

/// Ray-casting point-in-polygon test with boundary edge tolerance.
/// Returns true if `p` is inside or on the boundary of `pts`.
pub fn containsPoint(pts: []const Pt2, p: Pt2): bool {
    if (pts.len < 3) {
        return false;
    }
    let inside: bool = false;
    let i: usize = 0;
    while (i < pts.len) {
        const j: usize = if (i == 0) pts.len - 1 else i - 1;
        if (pointOnSegment(p, pts[j], pts[i])) {
            return true;
        }
        const yi: f64 = pts[i].y;
        const yj: f64 = pts[j].y;
        if ((yi > p.y) != (yj > p.y)) {
            const x_int: f64 = pts[i].x + (p.y - yi) * (pts[j].x - pts[i].x) / (yj - yi);
            if (p.x < x_int) {
                inside = !inside;
            }
        }
        i += 1;
    }
    return inside;
}

/// Return true if the polygon is convex (all non-zero consecutive edge cross products share the same sign).
/// Returns false for < 3 vertices or fully collinear degenerate polygons.
pub fn isConvex(pts: []const Pt2): bool {
    if (pts.len < 3) {
        return false;
    }
    let sign: i32 = 0;
    let i: usize = 0;
    while (i < pts.len) {
        const j: usize = if (i + 1 < pts.len) i + 1 else 0;
        const k: usize = if (j + 1 < pts.len) j + 1 else 0;
        const dx1: f64 = pts[j].x - pts[i].x;
        const dy1: f64 = pts[j].y - pts[i].y;
        const dx2: f64 = pts[k].x - pts[j].x;
        const dy2: f64 = pts[k].y - pts[j].y;
        const cross: f64 = dx1 * dy2 - dy1 * dx2;
        if (absF64(cross) > EPS_EDGE) {
            const cur_sign: i32 = if (cross > 0.0) 1 else -1;
            if (sign == 0) {
                sign = cur_sign;
            } else if (sign != cur_sign) {
                return false;
            }
        }
        i += 1;
    }
    return sign != 0;
}

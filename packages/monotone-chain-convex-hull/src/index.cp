// @cap/convex-hull — Zero-heap 2D Andrew's monotone chain convex hull in pure CP
// MIT License — Clean-room implementation with zero external dependencies

const EPS_EQ: f64 = 0.000000000001;
const EPS_COLLIN: f64 = 0.000000000001;

pub struct Pt2 {
    pub x: f64,
    pub y: f64,
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn sqrtF64(x: f64): f64 {
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

fn ptLess(a: Pt2, b: Pt2): bool {
    if (a.x < b.x) {
        return true;
    }
    if (a.x > b.x) {
        return false;
    }
    return a.y < b.y;
}

fn ptEqual(a: Pt2, b: Pt2): bool {
    return absF64(a.x - b.x) <= EPS_EQ && absF64(a.y - b.y) <= EPS_EQ;
}

/// 2D cross product of vectors OA and OB: (a - o) x (b - o).
/// Positive if O->A->B makes a counter-clockwise (left) turn.
pub fn crossOAB(o: Pt2, a: Pt2, b: Pt2): f64 {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

/// Sort points in-place in ascending lexicographical (x, y) order using Shell sort.
pub fn sortPointsXY(pts: []Pt2) {
    const n: usize = pts.len;
    if (n < 2) {
        return;
    }
    let gap: usize = 1;
    while (gap < n / 3) {
        gap = gap * 3 + 1;
    }
    while (gap >= 1) {
        let i: usize = gap;
        while (i < n) {
            const tmp: Pt2 = pts[i];
            let j: usize = i;
            while (j >= gap && ptLess(tmp, pts[j - gap])) {
                pts[j] = pts[j - gap];
                j -= gap;
            }
            pts[j] = tmp;
            i += 1;
        }
        gap = gap / 3;
    }
}

/// Compute the 2D convex hull via Andrew's monotone chain algorithm.
/// Sorts `work_pts` in-place, deduplicates coincident points, skips interior and collinear
/// intermediate points, and writes CCW hull vertices into `out_hull`.
/// Returns the number of hull vertices written (0 if empty or `out_hull` is too small).
pub fn convexHull(work_pts: []Pt2, out_hull: []Pt2): usize {
    if (work_pts.len == 0 || out_hull.len == 0) {
        return 0;
    }
    sortPointsXY(work_pts);

    // Deduplicate sorted points in-place
    let uniq: usize = 1;
    let r: usize = 1;
    while (r < work_pts.len) {
        if (!ptEqual(work_pts[r], work_pts[uniq - 1])) {
            work_pts[uniq] = work_pts[r];
            uniq += 1;
        }
        r += 1;
    }

    if (uniq == 1) {
        out_hull[0] = work_pts[0];
        return 1;
    }
    if (uniq == 2) {
        if (out_hull.len < 2) {
            return 0;
        }
        out_hull[0] = work_pts[0];
        out_hull[1] = work_pts[1];
        return 2;
    }

    // Build lower hull
    let k: usize = 0;
    let i: usize = 0;
    while (i < uniq) {
        while (k >= 2 && crossOAB(out_hull[k - 2], out_hull[k - 1], work_pts[i]) <= EPS_COLLIN) {
            k -= 1;
        }
        if (k >= out_hull.len) {
            return 0;
        }
        out_hull[k] = work_pts[i];
        k += 1;
        i += 1;
    }

    // Build upper hull (skip pushing work_pts[0] again at the end)
    const lower_len: usize = k + 1;
    let idx: usize = uniq - 1;
    while (idx > 0) {
        idx -= 1;
        while (k >= lower_len && crossOAB(out_hull[k - 2], out_hull[k - 1], work_pts[idx]) <= EPS_COLLIN) {
            k -= 1;
        }
        if (idx > 0) {
            if (k >= out_hull.len) {
                return 0;
            }
            out_hull[k] = work_pts[idx];
            k += 1;
        }
    }
    return k;
}

/// Unsigned area of a convex hull polygon (0.0 for < 3 vertices).
pub fn hullArea(hull: []const Pt2): f64 {
    if (hull.len < 3) {
        return 0.0;
    }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < hull.len) {
        const j: usize = if (i + 1 < hull.len) i + 1 else 0;
        sum = sum + (hull[i].x * hull[j].y - hull[j].x * hull[i].y);
        i += 1;
    }
    return absF64(0.5 * sum);
}

/// Perimeter of a convex hull polygon (0.0 for < 2 vertices; segment length for 2 vertices).
pub fn hullPerimeter(hull: []const Pt2): f64 {
    if (hull.len < 2) {
        return 0.0;
    }
    if (hull.len == 2) {
        const dx: f64 = hull[1].x - hull[0].x;
        const dy: f64 = hull[1].y - hull[0].y;
        return sqrtF64(dx * dx + dy * dy);
    }
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < hull.len) {
        const j: usize = if (i + 1 < hull.len) i + 1 else 0;
        const dx: f64 = hull[j].x - hull[i].x;
        const dy: f64 = hull[j].y - hull[i].y;
        total = total + sqrtF64(dx * dx + dy * dy);
        i += 1;
    }
    return total;
}

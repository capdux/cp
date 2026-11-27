// @cap/ramer-douglas-peucker — Zero-heap iterative Ramer-Douglas-Peucker 2D polyline simplification
// Clean-room MIT implementation in pure CP.

pub struct Pt2 {
    pub x: f64,
    pub y: f64,
}

pub fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64 {
    let dx = b.x - a.x;
    let dy = b.y - a.y;
    let len_sq = dx * dx + dy * dy;
    if (len_sq == 0.0) {
        let ex = p.x - a.x;
        let ey = p.y - a.y;
        return ex * ex + ey * ey;
    }
    let t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len_sq;
    if (t < 0.0) { t = 0.0; }
    if (t > 1.0) { t = 1.0; }
    let proj_x = a.x + t * dx;
    let proj_y = a.y + t * dy;
    let ex = p.x - proj_x;
    let ey = p.y - proj_y;
    return ex * ex + ey * ey;
}

pub fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize {
    let n = pts.len;
    if (n == 0) { return 0; }
    if (n <= 2) {
        let i: usize = 0;
        let lim = if (n < out.len) { n } else { out.len };
        while (i < lim) {
            out[i] = pts[i];
            i = i + 1;
        }
        return lim;
    }
    if (keep_mask.len < n) { return 0; }

    let i: usize = 0;
    while (i < n) {
        keep_mask[i] = 0;
        i = i + 1;
    }
    keep_mask[0] = 1;
    keep_mask[n - 1] = 1;

    let eps_sq = epsilon * epsilon;
    let stack_start: [128]usize = [0; 128];
    let stack_end: [128]usize = [0; 128];
    let sp: usize = 0;
    stack_start[sp] = 0;
    stack_end[sp] = n - 1;
    sp = 1;

    while (sp > 0) {
        sp = sp - 1;
        let first = stack_start[sp];
        let last = stack_end[sp];
        let max_dist_sq: f64 = -1.0;
        let max_idx: usize = first;

        let k: usize = first + 1;
        while (k < last) {
            let d2 = pointToSegmentDistSq(pts[k], pts[first], pts[last]);
            if (d2 > max_dist_sq) {
                max_dist_sq = d2;
                max_idx = k;
            }
            k = k + 1;
        }

        if (max_dist_sq > eps_sq && max_idx > first && max_idx < last) {
            keep_mask[max_idx] = 1;
            if (sp + 2 <= 128) {
                stack_start[sp] = first;
                stack_end[sp] = max_idx;
                sp = sp + 1;
                stack_start[sp] = max_idx;
                stack_end[sp] = last;
                sp = sp + 1;
            }
        }
    }

    let written: usize = 0;
    let j: usize = 0;
    while (j < n && written < out.len) {
        if (keep_mask[j] != 0) {
            out[written] = pts[j];
            written = written + 1;
        }
        j = j + 1;
    }
    return written;
}

// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Deterministic RANSAC Robust Estimation for 2D Line, 2D Circle, 3D Plane & 1D Linear Regression)

import std from "std";

const DEGEN_EPS: f64 = 1.0e-12;
const DEFAULT_SEED: u64 = 0x9e3779b97f4a7c15;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn splitmix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9e3779b97f4a7c15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn sampleDistinctPair(state: *u64, n: usize, out_idx0: *usize, out_idx1: *usize) {
    const idx0 = (splitmix64Next(state) as usize) % n;
    let idx1 = (splitmix64Next(state) as usize) % (n - 1);
    if (idx1 >= idx0) {
        idx1 += 1;
    }
    out_idx0.* = idx0;
    out_idx1.* = idx1;
}

fn sampleDistinctTriple(state: *u64, n: usize, out_idx0: *usize, out_idx1: *usize, out_idx2: *usize) {
    const idx0 = (splitmix64Next(state) as usize) % n;
    let idx1 = (splitmix64Next(state) as usize) % (n - 1);
    if (idx1 >= idx0) {
        idx1 += 1;
    }
    const lo = if (idx0 < idx1) idx0 else idx1;
    const hi = if (idx0 < idx1) idx1 else idx0;
    let idx2 = (splitmix64Next(state) as usize) % (n - 2);
    if (idx2 >= lo) {
        idx2 += 1;
    }
    if (idx2 >= hi) {
        idx2 += 1;
    }
    out_idx0.* = idx0;
    out_idx1.* = idx1;
    out_idx2.* = idx2;
}

/// Configuration for deterministic RANSAC robust model fitting.
pub struct RansacConfig {
    threshold: f64;
    max_iterations: usize;
    seed: u64;
    min_inliers: usize;
    refine: bool;

    /// Returns sensible default RANSAC settings for a given inlier distance `threshold`.
    static fn default(threshold: f64): RansacConfig {
        return RansacConfig {
            threshold: threshold,
            max_iterations: 200,
            seed: 42,
            min_inliers: 0,
            refine: true,
        };
    }
}

/// 2D Euclidean point.
pub struct Point2 {
    x: f64;
    y: f64;
}

/// 3D Euclidean point.
pub struct Point3 {
    x: f64;
    y: f64;
    z: f64;
}

/// Normalized 2D line model `a * x + b * y + c = 0` with `a^2 + b^2 = 1`.
pub struct Line2Model {
    a: f64;
    b: f64;
    c: f64;
    inlier_count: usize;
    rmse: f64;
    iterations: usize;
}

/// 2D circle model with center `(cx, cy)` and positive `radius`.
pub struct Circle2Model {
    cx: f64;
    cy: f64;
    radius: f64;
    inlier_count: usize;
    rmse: f64;
    iterations: usize;
}

/// Normalized 3D plane model `a * x + b * y + c * z + d = 0` with `a^2 + b^2 + c^2 = 1`.
pub struct Plane3Model {
    a: f64;
    b: f64;
    c: f64;
    d: f64;
    inlier_count: usize;
    rmse: f64;
    iterations: usize;
}

/// 1D linear regression model `y = slope * x + intercept`.
pub struct Linear1DModel {
    slope: f64;
    intercept: f64;
    inlier_count: usize;
    rmse: f64;
    iterations: usize;
}

fn validateConfig(cfg: RansacConfig): bool {
    return isFiniteF64(cfg.threshold) && cfg.threshold > 0.0 && cfg.max_iterations > 0;
}

fn canonicalizeLine(a_in: f64, b_in: f64, c_in: f64, a_out: *f64, b_out: *f64, c_out: *f64) {
    if (a_in < 0.0 || (a_in == 0.0 && b_in < 0.0)) {
        a_out.* = 0.0 - a_in;
        b_out.* = 0.0 - b_in;
        c_out.* = 0.0 - c_in;
    } else {
        a_out.* = a_in;
        b_out.* = b_in;
        c_out.* = c_in;
    }
}

fn canonicalizePlane(a_in: f64, b_in: f64, c_in: f64, d_in: f64, a_out: *f64, b_out: *f64, c_out: *f64, d_out: *f64) {
    if (a_in < 0.0 || (a_in == 0.0 && b_in < 0.0) || (a_in == 0.0 && b_in == 0.0 && c_in < 0.0)) {
        a_out.* = 0.0 - a_in;
        b_out.* = 0.0 - b_in;
        c_out.* = 0.0 - c_in;
        d_out.* = 0.0 - d_in;
    } else {
        a_out.* = a_in;
        b_out.* = b_in;
        c_out.* = c_in;
        d_out.* = d_in;
    }
}

/// Robustly fits a 2D line `a * x + b * y + c = 0` (`a^2 + b^2 = 1`) using orthogonal distance
/// RANSAC and optional Total Least Squares (2x2 covariance eigenvector) refinement over inliers.
///
/// Writes `1` for inliers and `0` for outliers into `inlier_mask_out[0..points.len]`.
/// Returns `null` if inputs are invalid, contain non-finite coordinates, buffer is undersized,
/// or no non-degenerate model meets `min_inliers`.
pub fn ransacFitLine2D(
    points: []const Point2,
    cfg: RansacConfig,
    inlier_mask_out: []u8,
): Line2Model? {
    const n = points.len;
    if (n < 2 || inlier_mask_out.len < n || !validateConfig(cfg)) {
        return null;
    }
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(points[chk].x) || !isFiniteF64(points[chk].y)) {
            return null;
        }
        chk += 1;
    }

    let rng: u64 = if (cfg.seed == 0) DEFAULT_SEED else cfg.seed;
    let found = false;
    let best_a: f64 = 0.0;
    let best_b: f64 = 0.0;
    let best_c: f64 = 0.0;
    let best_inliers: usize = 0;
    let best_sq_sum: f64 = 0.0;

    let iter: usize = 0;
    while (iter < cfg.max_iterations) {
        let idx0: usize = 0;
        let idx1: usize = 0;
        sampleDistinctPair(&rng, n, &idx0, &idx1);
        const p0 = points[idx0];
        const p1 = points[idx1];
        const dx = p1.x - p0.x;
        const dy = p1.y - p0.y;
        const len = std.math.sqrt(dx * dx + dy * dy);
        if (len > DEGEN_EPS) {
            const a = (0.0 - dy) / len;
            const b = dx / len;
            const c = 0.0 - (a * p0.x + b * p0.y);

            let cnt: usize = 0;
            let sq_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                const dist = absF64(a * points[k].x + b * points[k].y + c);
                if (dist <= cfg.threshold) {
                    cnt += 1;
                    sq_sum = sq_sum + dist * dist;
                }
                k += 1;
            }
            if (!found || cnt > best_inliers || (cnt == best_inliers && sq_sum < best_sq_sum)) {
                found = true;
                best_a = a;
                best_b = b;
                best_c = c;
                best_inliers = cnt;
                best_sq_sum = sq_sum;
            }
        }
        iter += 1;
    }

    const req_inliers = if (cfg.min_inliers > 2) cfg.min_inliers else 2;
    if (!found || best_inliers < req_inliers) {
        return null;
    }

    // Optional TLS refinement via smallest eigenvector of 2x2 inlier covariance
    if (cfg.refine && best_inliers >= 2) {
        let mx: f64 = 0.0;
        let my: f64 = 0.0;
        let m_cnt: usize = 0;
        let k: usize = 0;
        while (k < n) {
            const dist = absF64(best_a * points[k].x + best_b * points[k].y + best_c);
            if (dist <= cfg.threshold) {
                mx = mx + points[k].x;
                my = my + points[k].y;
                m_cnt += 1;
            }
            k += 1;
        }
        if (m_cnt >= 2) {
            mx = mx / (m_cnt as f64);
            my = my / (m_cnt as f64);
            let sxx: f64 = 0.0;
            let sxy: f64 = 0.0;
            let syy: f64 = 0.0;
            k = 0;
            while (k < n) {
                const dist = absF64(best_a * points[k].x + best_b * points[k].y + best_c);
                if (dist <= cfg.threshold) {
                    const ux = points[k].x - mx;
                    const uy = points[k].y - my;
                    sxx = sxx + ux * ux;
                    sxy = sxy + ux * uy;
                    syy = syy + uy * uy;
                }
                k += 1;
            }
            const trace = sxx + syy;
            const diff = sxx - syy;
            const disc = std.math.sqrt(diff * diff + 4.0 * sxy * sxy);
            const lam_min = 0.5 * (trace - disc);

            // Pick numerically larger eigenvector candidate for lam_min
            const vx1 = sxy;
            const vy1 = lam_min - sxx;
            const n1 = vx1 * vx1 + vy1 * vy1;
            const vx2 = lam_min - syy;
            const vy2 = sxy;
            const n2 = vx2 * vx2 + vy2 * vy2;

            let ra = best_a;
            let rb = best_b;
            let valid_ref = false;
            if (n1 >= n2 && n1 > 1.0e-24) {
                const inv_l = 1.0 / std.math.sqrt(n1);
                ra = vx1 * inv_l;
                rb = vy1 * inv_l;
                valid_ref = true;
            } else if (n2 > 1.0e-24) {
                const inv_l = 1.0 / std.math.sqrt(n2);
                ra = vx2 * inv_l;
                rb = vy2 * inv_l;
                valid_ref = true;
            }

            if (valid_ref) {
                const rc = 0.0 - (ra * mx + rb * my);
                let ref_cnt: usize = 0;
                let ref_sq: f64 = 0.0;
                k = 0;
                while (k < n) {
                    const d = absF64(ra * points[k].x + rb * points[k].y + rc);
                    if (d <= cfg.threshold) {
                        ref_cnt += 1;
                        ref_sq = ref_sq + d * d;
                    }
                    k += 1;
                }
                if (ref_cnt > best_inliers || (ref_cnt == best_inliers && ref_sq <= best_sq_sum)) {
                    best_a = ra;
                    best_b = rb;
                    best_c = rc;
                    best_inliers = ref_cnt;
                    best_sq_sum = ref_sq;
                }
            }
        }
    }

    let ca: f64 = 0.0;
    let cb: f64 = 0.0;
    let cc: f64 = 0.0;
    canonicalizeLine(best_a, best_b, best_c, &ca, &cb, &cc);

    let final_cnt: usize = 0;
    let final_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const dist = absF64(ca * points[i].x + cb * points[i].y + cc);
        if (dist <= cfg.threshold) {
            inlier_mask_out[i] = 1;
            final_cnt += 1;
            final_sq = final_sq + dist * dist;
        } else {
            inlier_mask_out[i] = 0;
        }
        i += 1;
    }

    const rmse = if (final_cnt > 0) std.math.sqrt(final_sq / (final_cnt as f64)) else 0.0;
    return Line2Model {
        a: ca,
        b: cb,
        c: cc,
        inlier_count: final_cnt,
        rmse: rmse,
        iterations: cfg.max_iterations,
    };
}

/// Robustly fits a 2D circle `(cx, cy, radius)` via 3-point circumcircle RANSAC
/// with optional Kåsa algebraic refinement over inliers.
pub fn ransacFitCircle2D(
    points: []const Point2,
    cfg: RansacConfig,
    inlier_mask_out: []u8,
): Circle2Model? {
    const n = points.len;
    if (n < 3 || inlier_mask_out.len < n || !validateConfig(cfg)) {
        return null;
    }
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(points[chk].x) || !isFiniteF64(points[chk].y)) {
            return null;
        }
        chk += 1;
    }

    let rng: u64 = if (cfg.seed == 0) DEFAULT_SEED else cfg.seed;
    let found = false;
    let best_cx: f64 = 0.0;
    let best_cy: f64 = 0.0;
    let best_r: f64 = 0.0;
    let best_inliers: usize = 0;
    let best_sq_sum: f64 = 0.0;

    let iter: usize = 0;
    while (iter < cfg.max_iterations) {
        let idx0: usize = 0;
        let idx1: usize = 0;
        let idx2: usize = 0;
        sampleDistinctTriple(&rng, n, &idx0, &idx1, &idx2);
        const p0 = points[idx0];
        const p1 = points[idx1];
        const p2 = points[idx2];

        const u1x = p1.x - p0.x;
        const u1y = p1.y - p0.y;
        const u2x = p2.x - p0.x;
        const u2y = p2.y - p0.y;
        const denom = 2.0 * (u1x * u2y - u1y * u2x);

        if (absF64(denom) > DEGEN_EPS) {
            const s1 = u1x * u1x + u1y * u1y;
            const s2 = u2x * u2x + u2y * u2y;
            const ox = (u2y * s1 - u1y * s2) / denom;
            const oy = (u1x * s2 - u2x * s1) / denom;
            const r = std.math.sqrt(ox * ox + oy * oy);
            if (isFiniteF64(r) && r > DEGEN_EPS) {
                const cx = p0.x + ox;
                const cy = p0.y + oy;
                let cnt: usize = 0;
                let sq_sum: f64 = 0.0;
                let k: usize = 0;
                while (k < n) {
                    const dx = points[k].x - cx;
                    const dy = points[k].y - cy;
                    const err = absF64(std.math.sqrt(dx * dx + dy * dy) - r);
                    if (err <= cfg.threshold) {
                        cnt += 1;
                        sq_sum = sq_sum + err * err;
                    }
                    k += 1;
                }
                if (!found || cnt > best_inliers || (cnt == best_inliers && sq_sum < best_sq_sum)) {
                    found = true;
                    best_cx = cx;
                    best_cy = cy;
                    best_r = r;
                    best_inliers = cnt;
                    best_sq_sum = sq_sum;
                }
            }
        }
        iter += 1;
    }

    const req_inliers = if (cfg.min_inliers > 3) cfg.min_inliers else 3;
    if (!found || best_inliers < req_inliers) {
        return null;
    }

    // Optional Kåsa algebraic refinement over inliers
    if (cfg.refine && best_inliers >= 3) {
        let mx: f64 = 0.0;
        let my: f64 = 0.0;
        let m_cnt: usize = 0;
        let k: usize = 0;
        while (k < n) {
            const dx = points[k].x - best_cx;
            const dy = points[k].y - best_cy;
            if (absF64(std.math.sqrt(dx * dx + dy * dy) - best_r) <= cfg.threshold) {
                mx = mx + points[k].x;
                my = my + points[k].y;
                m_cnt += 1;
            }
            k += 1;
        }
        if (m_cnt >= 3) {
            mx = mx / (m_cnt as f64);
            my = my / (m_cnt as f64);
            let suu: f64 = 0.0;
            let suv: f64 = 0.0;
            let svv: f64 = 0.0;
            let suuu: f64 = 0.0;
            let suvv: f64 = 0.0;
            let suuv: f64 = 0.0;
            let svvv: f64 = 0.0;
            k = 0;
            while (k < n) {
                const dx = points[k].x - best_cx;
                const dy = points[k].y - best_cy;
                if (absF64(std.math.sqrt(dx * dx + dy * dy) - best_r) <= cfg.threshold) {
                    const u = points[k].x - mx;
                    const v = points[k].y - my;
                    const u_sq = u * u;
                    const v_sq = v * v;
                    suu = suu + u_sq;
                    suv = suv + u * v;
                    svv = svv + v_sq;
                    suuu = suuu + u_sq * u;
                    suvv = suvv + u * v_sq;
                    suuv = suuv + u_sq * v;
                    svvv = svvv + v_sq * v;
                }
                k += 1;
            }
            const det = suu * svv - suv * suv;
            if (absF64(det) > DEGEN_EPS) {
                const rhs1 = 0.5 * (suuu + suvv);
                const rhs2 = 0.5 * (svvv + suuv);
                const cu = (svv * rhs1 - suv * rhs2) / det;
                const cv = (suu * rhs2 - suv * rhs1) / det;
                const r_sq = cu * cu + cv * cv + (suu + svv) / (m_cnt as f64);
                if (isFiniteF64(r_sq) && r_sq > 0.0) {
                    const rcx = mx + cu;
                    const rcy = my + cv;
                    const rr = std.math.sqrt(r_sq);
                    let ref_cnt: usize = 0;
                    let ref_sq: f64 = 0.0;
                    k = 0;
                    while (k < n) {
                        const dx = points[k].x - rcx;
                        const dy = points[k].y - rcy;
                        const err = absF64(std.math.sqrt(dx * dx + dy * dy) - rr);
                        if (err <= cfg.threshold) {
                            ref_cnt += 1;
                            ref_sq = ref_sq + err * err;
                        }
                        k += 1;
                    }
                    if (ref_cnt > best_inliers || (ref_cnt == best_inliers && ref_sq <= best_sq_sum)) {
                        best_cx = rcx;
                        best_cy = rcy;
                        best_r = rr;
                        best_inliers = ref_cnt;
                        best_sq_sum = ref_sq;
                    }
                }
            }
        }
    }

    let final_cnt: usize = 0;
    let final_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const dx = points[i].x - best_cx;
        const dy = points[i].y - best_cy;
        const err = absF64(std.math.sqrt(dx * dx + dy * dy) - best_r);
        if (err <= cfg.threshold) {
            inlier_mask_out[i] = 1;
            final_cnt += 1;
            final_sq = final_sq + err * err;
        } else {
            inlier_mask_out[i] = 0;
        }
        i += 1;
    }

    const rmse = if (final_cnt > 0) std.math.sqrt(final_sq / (final_cnt as f64)) else 0.0;
    return Circle2Model {
        cx: best_cx,
        cy: best_cy,
        radius: best_r,
        inlier_count: final_cnt,
        rmse: rmse,
        iterations: cfg.max_iterations,
    };
}

fn smallestEigenvec3x3(c_in: []const f64, out_x: *f64, out_y: *f64, out_z: *f64): bool {
    let a: [9]f64 = std.mem.zeroes([9]f64);
    let ci: usize = 0;
    while (ci < 9) {
        a[ci] = c_in[ci];
        ci += 1;
    }
    let v: [9]f64 = [
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    ];

    let sweep: usize = 0;
    while (sweep < 25) {
        let max_off: f64 = 0.0;
        let p: usize = 0;
        while (p < 3) {
            let q = p + 1;
            while (q < 3) {
                const apq = a[p * 3 + q];
                const aapq = absF64(apq);
                if (aapq > max_off) {
                    max_off = aapq;
                }
                if (aapq > 1.0e-15) {
                    const app = a[p * 3 + p];
                    const aqq = a[q * 3 + q];
                    const tau = (aqq - app) / (2.0 * apq);
                    const t = if (tau >= 0.0)
                        1.0 / (tau + std.math.sqrt(1.0 + tau * tau))
                    else
                        -1.0 / ((0.0 - tau) + std.math.sqrt(1.0 + tau * tau));
                    const cs = 1.0 / std.math.sqrt(1.0 + t * t);
                    const sn = t * cs;

                    a[p * 3 + p] = app - t * apq;
                    a[q * 3 + q] = aqq + t * apq;
                    a[p * 3 + q] = 0.0;
                    a[q * 3 + p] = 0.0;

                    let r: usize = 0;
                    while (r < 3) {
                        if (r != p && r != q) {
                            const arp = a[r * 3 + p];
                            const arq = a[r * 3 + q];
                            const nrp = cs * arp - sn * arq;
                            const nrq = sn * arp + cs * arq;
                            a[r * 3 + p] = nrp;
                            a[p * 3 + r] = nrp;
                            a[r * 3 + q] = nrq;
                            a[q * 3 + r] = nrq;
                        }
                        r += 1;
                    }

                    r = 0;
                    while (r < 3) {
                        const vrp = v[r * 3 + p];
                        const vrq = v[r * 3 + q];
                        v[r * 3 + p] = cs * vrp - sn * vrq;
                        v[r * 3 + q] = sn * vrp + cs * vrq;
                        r += 1;
                    }
                }
                q += 1;
            }
            p += 1;
        }
        if (max_off <= 1.0e-14) {
            break;
        }
        sweep += 1;
    }

    let min_col: usize = 0;
    if (a[4] < a[0]) {
        min_col = 1;
    }
    if (a[8] < a[min_col * 3 + min_col]) {
        min_col = 2;
    }

    const vx = v[0 * 3 + min_col];
    const vy = v[1 * 3 + min_col];
    const vz = v[2 * 3 + min_col];
    const norm = std.math.sqrt(vx * vx + vy * vy + vz * vz);
    if (!isFiniteF64(norm) || norm <= DEGEN_EPS) {
        return false;
    }
    out_x.* = vx / norm;
    out_y.* = vy / norm;
    out_z.* = vz / norm;
    return true;
}

/// Robustly fits a 3D plane `a * x + b * y + c * z + d = 0` (`a^2 + b^2 + c^2 = 1`) via
/// 3-point RANSAC and optional 3x3 covariance smallest-eigenvector refinement over inliers.
pub fn ransacFitPlane3D(
    points: []const Point3,
    cfg: RansacConfig,
    inlier_mask_out: []u8,
): Plane3Model? {
    const n = points.len;
    if (n < 3 || inlier_mask_out.len < n || !validateConfig(cfg)) {
        return null;
    }
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(points[chk].x) || !isFiniteF64(points[chk].y) || !isFiniteF64(points[chk].z)) {
            return null;
        }
        chk += 1;
    }

    let rng: u64 = if (cfg.seed == 0) DEFAULT_SEED else cfg.seed;
    let found = false;
    let best_a: f64 = 0.0;
    let best_b: f64 = 0.0;
    let best_c: f64 = 0.0;
    let best_d: f64 = 0.0;
    let best_inliers: usize = 0;
    let best_sq_sum: f64 = 0.0;

    let iter: usize = 0;
    while (iter < cfg.max_iterations) {
        let idx0: usize = 0;
        let idx1: usize = 0;
        let idx2: usize = 0;
        sampleDistinctTriple(&rng, n, &idx0, &idx1, &idx2);
        const p0 = points[idx0];
        const p1 = points[idx1];
        const p2 = points[idx2];

        const e1x = p1.x - p0.x;
        const e1y = p1.y - p0.y;
        const e1z = p1.z - p0.z;
        const e2x = p2.x - p0.x;
        const e2y = p2.y - p0.y;
        const e2z = p2.z - p0.z;

        const nx = e1y * e2z - e1z * e2y;
        const ny = e1z * e2x - e1x * e2z;
        const nz = e1x * e2y - e1y * e2x;
        const norm = std.math.sqrt(nx * nx + ny * ny + nz * nz);

        if (norm > DEGEN_EPS) {
            const a = nx / norm;
            const b = ny / norm;
            const c = nz / norm;
            const d = 0.0 - (a * p0.x + b * p0.y + c * p0.z);

            let cnt: usize = 0;
            let sq_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                const dist = absF64(a * points[k].x + b * points[k].y + c * points[k].z + d);
                if (dist <= cfg.threshold) {
                    cnt += 1;
                    sq_sum = sq_sum + dist * dist;
                }
                k += 1;
            }
            if (!found || cnt > best_inliers || (cnt == best_inliers && sq_sum < best_sq_sum)) {
                found = true;
                best_a = a;
                best_b = b;
                best_c = c;
                best_d = d;
                best_inliers = cnt;
                best_sq_sum = sq_sum;
            }
        }
        iter += 1;
    }

    const req_inliers = if (cfg.min_inliers > 3) cfg.min_inliers else 3;
    if (!found || best_inliers < req_inliers) {
        return null;
    }

    // Optional 3x3 covariance smallest-eigenvector refinement
    if (cfg.refine && best_inliers >= 3) {
        let mx: f64 = 0.0;
        let my: f64 = 0.0;
        let mz: f64 = 0.0;
        let m_cnt: usize = 0;
        let k: usize = 0;
        while (k < n) {
            const dist = absF64(best_a * points[k].x + best_b * points[k].y + best_c * points[k].z + best_d);
            if (dist <= cfg.threshold) {
                mx = mx + points[k].x;
                my = my + points[k].y;
                mz = mz + points[k].z;
                m_cnt += 1;
            }
            k += 1;
        }
        if (m_cnt >= 3) {
            const inv_m = 1.0 / (m_cnt as f64);
            mx = mx * inv_m;
            my = my * inv_m;
            mz = mz * inv_m;
            let cov: [9]f64 = std.mem.zeroes([9]f64);
            k = 0;
            while (k < n) {
                const dist = absF64(best_a * points[k].x + best_b * points[k].y + best_c * points[k].z + best_d);
                if (dist <= cfg.threshold) {
                    const ux = points[k].x - mx;
                    const uy = points[k].y - my;
                    const uz = points[k].z - mz;
                    cov[0] = cov[0] + ux * ux;
                    cov[1] = cov[1] + ux * uy;
                    cov[2] = cov[2] + ux * uz;
                    cov[4] = cov[4] + uy * uy;
                    cov[5] = cov[5] + uy * uz;
                    cov[8] = cov[8] + uz * uz;
                }
                k += 1;
            }
            cov[3] = cov[1];
            cov[6] = cov[2];
            cov[7] = cov[5];

            let ra: f64 = 0.0;
            let rb: f64 = 0.0;
            let rc: f64 = 0.0;
            if (smallestEigenvec3x3(cov[0..], &ra, &rb, &rc)) {
                const rd = 0.0 - (ra * mx + rb * my + rc * mz);
                let ref_cnt: usize = 0;
                let ref_sq: f64 = 0.0;
                k = 0;
                while (k < n) {
                    const dist = absF64(ra * points[k].x + rb * points[k].y + rc * points[k].z + rd);
                    if (dist <= cfg.threshold) {
                        ref_cnt += 1;
                        ref_sq = ref_sq + dist * dist;
                    }
                    k += 1;
                }
                if (ref_cnt > best_inliers || (ref_cnt == best_inliers && ref_sq <= best_sq_sum)) {
                    best_a = ra;
                    best_b = rb;
                    best_c = rc;
                    best_d = rd;
                    best_inliers = ref_cnt;
                    best_sq_sum = ref_sq;
                }
            }
        }
    }

    let ca: f64 = 0.0;
    let cb: f64 = 0.0;
    let cc: f64 = 0.0;
    let cd: f64 = 0.0;
    canonicalizePlane(best_a, best_b, best_c, best_d, &ca, &cb, &cc, &cd);

    let final_cnt: usize = 0;
    let final_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const dist = absF64(ca * points[i].x + cb * points[i].y + cc * points[i].z + cd);
        if (dist <= cfg.threshold) {
            inlier_mask_out[i] = 1;
            final_cnt += 1;
            final_sq = final_sq + dist * dist;
        } else {
            inlier_mask_out[i] = 0;
        }
        i += 1;
    }

    const rmse = if (final_cnt > 0) std.math.sqrt(final_sq / (final_cnt as f64)) else 0.0;
    return Plane3Model {
        a: ca,
        b: cb,
        c: cc,
        d: cd,
        inlier_count: final_cnt,
        rmse: rmse,
        iterations: cfg.max_iterations,
    };
}

/// Robustly fits a 1D linear model `y = slope * x + intercept` via RANSAC
/// with optional Ordinary Least Squares (OLS) refinement over inliers.
pub fn ransacFitLinear1D(
    xs: []const f64,
    ys: []const f64,
    cfg: RansacConfig,
    inlier_mask_out: []u8,
): Linear1DModel? {
    const n = xs.len;
    if (n < 2 || ys.len != n || inlier_mask_out.len < n || !validateConfig(cfg)) {
        return null;
    }
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(xs[chk]) || !isFiniteF64(ys[chk])) {
            return null;
        }
        chk += 1;
    }

    let rng: u64 = if (cfg.seed == 0) DEFAULT_SEED else cfg.seed;
    let found = false;
    let best_slope: f64 = 0.0;
    let best_intercept: f64 = 0.0;
    let best_inliers: usize = 0;
    let best_sq_sum: f64 = 0.0;

    let iter: usize = 0;
    while (iter < cfg.max_iterations) {
        let idx0: usize = 0;
        let idx1: usize = 0;
        sampleDistinctPair(&rng, n, &idx0, &idx1);
        const dx = xs[idx1] - xs[idx0];
        if (absF64(dx) > DEGEN_EPS) {
            const slope = (ys[idx1] - ys[idx0]) / dx;
            const intercept = ys[idx0] - slope * xs[idx0];

            let cnt: usize = 0;
            let sq_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                const err = absF64(ys[k] - (slope * xs[k] + intercept));
                if (err <= cfg.threshold) {
                    cnt += 1;
                    sq_sum = sq_sum + err * err;
                }
                k += 1;
            }
            if (!found || cnt > best_inliers || (cnt == best_inliers && sq_sum < best_sq_sum)) {
                found = true;
                best_slope = slope;
                best_intercept = intercept;
                best_inliers = cnt;
                best_sq_sum = sq_sum;
            }
        }
        iter += 1;
    }

    const req_inliers = if (cfg.min_inliers > 2) cfg.min_inliers else 2;
    if (!found || best_inliers < req_inliers) {
        return null;
    }

    // Optional OLS refinement over inliers
    if (cfg.refine && best_inliers >= 2) {
        let mx: f64 = 0.0;
        let my: f64 = 0.0;
        let m_cnt: usize = 0;
        let k: usize = 0;
        while (k < n) {
            const err = absF64(ys[k] - (best_slope * xs[k] + best_intercept));
            if (err <= cfg.threshold) {
                mx = mx + xs[k];
                my = my + ys[k];
                m_cnt += 1;
            }
            k += 1;
        }
        if (m_cnt >= 2) {
            mx = mx / (m_cnt as f64);
            my = my / (m_cnt as f64);
            let sxx: f64 = 0.0;
            let sxy: f64 = 0.0;
            k = 0;
            while (k < n) {
                const err = absF64(ys[k] - (best_slope * xs[k] + best_intercept));
                if (err <= cfg.threshold) {
                    const ux = xs[k] - mx;
                    const uy = ys[k] - my;
                    sxx = sxx + ux * ux;
                    sxy = sxy + ux * uy;
                }
                k += 1;
            }
            if (sxx > 1.0e-15) {
                const r_slope = sxy / sxx;
                const r_intercept = my - r_slope * mx;
                let ref_cnt: usize = 0;
                let ref_sq: f64 = 0.0;
                k = 0;
                while (k < n) {
                    const err = absF64(ys[k] - (r_slope * xs[k] + r_intercept));
                    if (err <= cfg.threshold) {
                        ref_cnt += 1;
                        ref_sq = ref_sq + err * err;
                    }
                    k += 1;
                }
                if (ref_cnt > best_inliers || (ref_cnt == best_inliers && ref_sq <= best_sq_sum)) {
                    best_slope = r_slope;
                    best_intercept = r_intercept;
                    best_inliers = ref_cnt;
                    best_sq_sum = ref_sq;
                }
            }
        }
    }

    let final_cnt: usize = 0;
    let final_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const err = absF64(ys[i] - (best_slope * xs[i] + best_intercept));
        if (err <= cfg.threshold) {
            inlier_mask_out[i] = 1;
            final_cnt += 1;
            final_sq = final_sq + err * err;
        } else {
            inlier_mask_out[i] = 0;
        }
        i += 1;
    }

    const rmse = if (final_cnt > 0) std.math.sqrt(final_sq / (final_cnt as f64)) else 0.0;
    return Linear1DModel {
        slope: best_slope,
        intercept: best_intercept,
        inlier_count: final_cnt,
        rmse: rmse,
        iterations: cfg.max_iterations,
    };
}

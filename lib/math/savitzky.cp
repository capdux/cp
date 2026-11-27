// Origin: Pure-CP Stdlib (Zero-Alloc Savitzky-Golay Polynomial Smoothing/Differentiation & Cleveland LOESS) | Created: 2026-11-22

const PIVOT_EPS: f64 = 1.0e-14;

/// Boundary extension mode for `savgolFilter`.
pub enum BoundaryMode {
    Mirror,
    Nearest,
    ConstantZero,
}

/// Caller-provided zero-allocation workspace for `savgolCoeffs` (`p = poly_order`, `m = p + 1`).
/// Required lengths: `ata >= m * m`, `rhs >= m`, `sol >= m`, `piv >= m`.
pub struct SavgolWorkspace {
    ata: []f64,
    rhs: []f64,
    sol: []f64,
    piv: []usize,
}

/// Hyperparameters for Cleveland (1979) 1D LOESS / LOWESS locally weighted regression.
pub struct LoessConfig {
    bandwidth: f64,
    robust_iters: usize,
    degree: usize,

    static fn default(): LoessConfig {
        return {
            bandwidth: 0.3,
            robust_iters: 2,
            degree: 1,
        };
    }
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isAllFinite(xs: []const f64): bool {
    let i: usize = 0;
    while (i < xs.len) {
        if (!isFinite(xs[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn solveLinearSystem(
    a: []f64,
    rhs: []const f64,
    sol: []f64,
    piv: []usize,
    n: usize,
): bool {
    let i_init: usize = 0;
    while (i_init < n) {
        piv[i_init] = i_init;
        i_init += 1;
    }

    let col: usize = 0;
    while (col < n) {
        let best_row = col;
        let best_abs = absF64(a[piv[col] * n + col]);
        let r_scan = col + 1;
        while (r_scan < n) {
            const v_abs = absF64(a[piv[r_scan] * n + col]);
            if (v_abs > best_abs) {
                best_abs = v_abs;
                best_row = r_scan;
            }
            r_scan += 1;
        }
        if (!isFinite(best_abs) || best_abs < PIVOT_EPS) {
            return false;
        }
        if (best_row != col) {
            const tmp = piv[col];
            piv[col] = piv[best_row];
            piv[best_row] = tmp;
        }
        const piv_row = piv[col];
        const diag = a[piv_row * n + col];
        let row = col + 1;
        while (row < n) {
            const r_idx = piv[row];
            const factor = a[r_idx * n + col] / diag;
            a[r_idx * n + col] = factor;
            let j = col + 1;
            while (j < n) {
                a[r_idx * n + j] -= factor * a[piv_row * n + j];
                j += 1;
            }
            row += 1;
        }
        col += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const r_idx = piv[i];
        let sum = rhs[r_idx];
        let j: usize = 0;
        while (j < i) {
            sum -= a[r_idx * n + j] * sol[j];
            j += 1;
        }
        sol[i] = sum;
        i += 1;
    }

    let back = n;
    while (back > 0) {
        back -= 1;
        const r_idx = piv[back];
        let sum = sol[back];
        let j = back + 1;
        while (j < n) {
            sum -= a[r_idx * n + j] * sol[j];
            j += 1;
        }
        const diag = a[r_idx * n + back];
        if (!isFinite(diag) || absF64(diag) < PIVOT_EPS) {
            return false;
        }
        const val = sum / diag;
        if (!isFinite(val)) {
            return false;
        }
        sol[back] = val;
    }
    return true;
}

/// Designs zero-phase Savitzky-Golay FIR weights for smoothing (`deriv_order == 0`) or differentiation (`deriv_order >= 1`).
/// Requires odd `window_len >= 3`, `poly_order < window_len`, `deriv_order <= poly_order`, finite `delta > 0`,
/// `coeffs_out.*.len >= window_len`, and workspace slices sized for `m = poly_order + 1`.
pub fn savgolCoeffs(
    window_len: usize,
    poly_order: usize,
    deriv_order: usize,
    delta: f64,
    coeffs_out: *[]f64,
    ws: *SavgolWorkspace,
): bool {
    if (window_len < 3 || (window_len % 2) == 0) {
        return false;
    }
    if (poly_order >= window_len || deriv_order > poly_order) {
        return false;
    }
    if (!isFinite(delta) || delta <= 0.0) {
        return false;
    }
    if (coeffs_out.*.len < window_len) {
        return false;
    }
    const m = poly_order + 1;
    if (ws.*.ata.len / m < m || ws.*.rhs.len < m || ws.*.sol.len < m || ws.*.piv.len < m) {
        return false;
    }

    const half = (window_len - 1) / 2;
    const half_f = half as f64;

    // Zero normal matrix A^T A (m x m) and build Gram sums
    let z: usize = 0;
    while (z < m * m) {
        ws.*.ata[z] = 0.0;
        z += 1;
    }

    let k: usize = 0;
    while (k < window_len) {
        const t = (k as f64) - half_f;
        let row_pow: f64 = 1.0;
        let r: usize = 0;
        while (r < m) {
            let col_pow: f64 = row_pow;
            let c: usize = r;
            while (c < m) {
                ws.*.ata[r * m + c] += col_pow;
                col_pow *= t;
                c += 1;
            }
            row_pow *= t;
            r += 1;
        }
        k += 1;
    }

    // Symmetrize lower triangle
    let r_sym: usize = 0;
    while (r_sym < m) {
        let c_sym: usize = r_sym + 1;
        while (c_sym < m) {
            ws.*.ata[c_sym * m + r_sym] = ws.*.ata[r_sym * m + c_sym];
            c_sym += 1;
        }
        r_sym += 1;
    }

    // Build RHS: (deriv_order! / delta^deriv_order) * e_{deriv_order}
    let scale: f64 = 1.0;
    let d_idx: usize = 1;
    while (d_idx <= deriv_order) {
        scale = scale * (d_idx as f64) / delta;
        d_idx += 1;
    }
    if (!isFinite(scale)) {
        return false;
    }

    let j_rhs: usize = 0;
    while (j_rhs < m) {
        ws.*.rhs[j_rhs] = if (j_rhs == deriv_order) scale else 0.0;
        j_rhs += 1;
    }

    if (!solveLinearSystem(ws.*.ata, ws.*.rhs[0..m], ws.*.sol, ws.*.piv, m)) {
        return false;
    }

    // Evaluate polynomial weights at each sample offset t = k - half
    let idx: usize = 0;
    while (idx < window_len) {
        const t = (idx as f64) - half_f;
        let acc: f64 = 0.0;
        let t_pow: f64 = 1.0;
        let p: usize = 0;
        while (p < m) {
            acc += ws.*.sol[p] * t_pow;
            t_pow *= t;
            p += 1;
        }
        if (!isFinite(acc)) {
            return false;
        }
        coeffs_out.*[idx] = acc;
        idx += 1;
    }
    return true;
}

fn sampleWithBoundary(signal: []const f64, idx: isize, mode: BoundaryMode): f64 {
    const n = signal.len as isize;
    if (idx >= 0 && idx < n) {
        return signal[idx as usize];
    }
    match (mode) {
        BoundaryMode.ConstantZero => return 0.0,
        BoundaryMode.Nearest => {
            if (idx < 0) {
                return signal[0];
            }
            return signal[(n - 1) as usize];
        },
        BoundaryMode.Mirror => {
            if (n <= 1) {
                return signal[0];
            }
            let pos = idx;
            const last = n - 1;
            while (pos < 0 || pos > last) {
                if (pos < 0) {
                    pos = 0 - pos;
                }
                if (pos > last) {
                    pos = 2 * last - pos;
                }
            }
            return signal[pos as usize];
        },
    }
}

/// Applies a Savitzky-Golay kernel `coeffs` (odd length `>= 3`) to `signal`, writing to `out`.
pub fn savgolFilter(
    signal: []const f64,
    coeffs: []const f64,
    mode: BoundaryMode,
    out: *[]f64,
): bool {
    const n = signal.len;
    const w = coeffs.len;
    if (n == 0 || w < 3 || (w % 2) == 0 || out.*.len < n) {
        return false;
    }
    if (!isAllFinite(signal) || !isAllFinite(coeffs)) {
        return false;
    }

    const half = ((w - 1) / 2) as isize;
    let i: usize = 0;
    while (i < n) {
        const center = i as isize;
        let acc: f64 = 0.0;
        let k: usize = 0;
        while (k < w) {
            const offset = (k as isize) - half;
            const s = sampleWithBoundary(signal, center + offset, mode);
            acc += coeffs[k] * s;
            k += 1;
        }
        out.*[i] = acc;
        i += 1;
    }
    return true;
}

// In-place Hoare-style quickselect returning the k-th smallest element (0-indexed) in `buf[0..n]`.
fn quickSelectF64(buf: []f64, n: usize, k_target: usize): f64 {
    let left: usize = 0;
    let right: usize = n - 1;
    while (left < right) {
        const mid = left + (right - left) / 2;
        const pivot = buf[mid];
        // Move pivot to right
        buf[mid] = buf[right];
        buf[right] = pivot;

        let store = left;
        let i = left;
        while (i < right) {
            if (buf[i] < pivot) {
                const tmp = buf[store];
                buf[store] = buf[i];
                buf[i] = tmp;
                store += 1;
            }
            i += 1;
        }
        buf[right] = buf[store];
        buf[store] = pivot;

        if (store == k_target) {
            return buf[store];
        } else if (k_target < store) {
            if (store == 0) {
                break;
            }
            right = store - 1;
        } else {
            left = store + 1;
        }
    }
    return buf[k_target];
}

fn tricube(u: f64): f64 {
    if (u >= 1.0) {
        return 0.0;
    }
    const t = 1.0 - u * u * u;
    return t * t * t;
}

fn fitLocalPoint(
    xs: []const f64,
    ys: []const f64,
    rob_w: []const f64,
    x0: f64,
    h_scale: f64,
    degree: usize,
): f64 {
    const n = xs.len;
    if (h_scale <= 1.0e-15) {
        // Coincident neighbors: return robust weighted mean at x0
        let w_sum: f64 = 0.0;
        let wy_sum: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            if (absF64(xs[j] - x0) <= 1.0e-15) {
                const w = rob_w[j];
                w_sum += w;
                wy_sum += w * ys[j];
            }
            j += 1;
        }
        return if (w_sum > 0.0) wy_sum / w_sum else 0.0;
    }

    let s0: f64 = 0.0;
    let s1: f64 = 0.0;
    let s2: f64 = 0.0;
    let s3: f64 = 0.0;
    let s4: f64 = 0.0;
    let t0: f64 = 0.0;
    let t1: f64 = 0.0;
    let t2: f64 = 0.0;

    let j: usize = 0;
    while (j < n) {
        const dx = xs[j] - x0;
        const dist = absF64(dx);
        if (dist < h_scale) {
            const w = rob_w[j] * tricube(dist / h_scale);
            if (w > 0.0) {
                const u = dx / h_scale;
                const u_sq = u * u;
                const yj = ys[j];
                s0 += w;
                s1 += w * u;
                s2 += w * u_sq;
                t0 += w * yj;
                t1 += w * u * yj;
                if (degree == 2) {
                    s3 += w * u_sq * u;
                    s4 += w * u_sq * u_sq;
                    t2 += w * u_sq * yj;
                }
            }
        }
        j += 1;
    }

    if (s0 <= 1.0e-15) {
        return 0.0;
    }

    if (degree == 2) {
        let a_mat: [9]f64 = [
            s0, s1, s2,
            s1, s2, s3,
            s2, s3, s4,
        ];
        const rhs_vec: [3]f64 = [t0, t1, t2];
        let sol_vec: [3]f64 = [0.0; 3];
        let piv_vec: [3]usize = [0; 3];
        if (solveLinearSystem(a_mat[0..], rhs_vec[0..], sol_vec[0..], piv_vec[0..], 3)) {
            return sol_vec[0];
        }
    }

    // Linear fit (degree == 1, or graceful fallback from degenerate quadratic)
    const det = s0 * s2 - s1 * s1;
    if (absF64(det) > 1.0e-14 * s0 * s2 + 1.0e-30) {
        return (s2 * t0 - s1 * t1) / det;
    }
    return t0 / s0;
}

/// Zero-allocation Cleveland (1979) 1D LOESS / LOWESS locally weighted regression with tricube kernel
/// and optional bisquare robustness iterations for outlier rejection.
/// Supports arbitrary (including non-uniform or unsorted) `(xs, ys)` samples.
pub fn loessSmooth1D(
    xs: []const f64,
    ys: []const f64,
    cfg: LoessConfig,
    y_out: *[]f64,
    rob_w_s: *[]f64,
    dist_s: *[]f64,
): bool {
    const n = xs.len;
    if (n < 2 || ys.len != n) {
        return false;
    }
    if (cfg.degree != 1 && cfg.degree != 2) {
        return false;
    }
    if (n < cfg.degree + 1) {
        return false;
    }
    if (!isFinite(cfg.bandwidth) || cfg.bandwidth <= 0.0 || cfg.bandwidth > 1.0) {
        return false;
    }
    if (y_out.*.len < n || rob_w_s.*.len < n || dist_s.*.len < n) {
        return false;
    }
    if (!isAllFinite(xs) || !isAllFinite(ys)) {
        return false;
    }

    const raw_k = @ceil(cfg.bandwidth * (n as f64)) as usize;
    const min_k = cfg.degree + 1;
    let k_neigh = if (raw_k < min_k) min_k else raw_k;
    if (k_neigh > n) {
        k_neigh = n;
    }

    // Initialize robustness weights to 1.0
    let w_init: usize = 0;
    while (w_init < n) {
        rob_w_s.*[w_init] = 1.0;
        w_init += 1;
    }

    let iter: usize = 0;
    while (iter <= cfg.robust_iters) {
        let i: usize = 0;
        while (i < n) {
            const x0 = xs[i];
            let j: usize = 0;
            while (j < n) {
                dist_s.*[j] = absF64(xs[j] - x0);
                j += 1;
            }
            const h_k = quickSelectF64(dist_s.*, n, k_neigh - 1);
            // Slight expansion so the k-th neighbor carries positive tricube weight
            const h_scale = h_k * (1.0 + 1.0e-6);
            y_out.*[i] = fitLocalPoint(xs, ys, rob_w_s.*, x0, h_scale, cfg.degree);
            i += 1;
        }

        if (iter == cfg.robust_iters) {
            break;
        }

        // Update bisquare robustness weights from median absolute residual
        let r_i: usize = 0;
        while (r_i < n) {
            dist_s.*[r_i] = absF64(ys[r_i] - y_out.*[r_i]);
            r_i += 1;
        }
        const med = quickSelectF64(dist_s.*, n, n / 2);
        const s6 = 6.0 * med;
        if (s6 <= 1.0e-15) {
            break;
        }

        let u_i: usize = 0;
        while (u_i < n) {
            const r_ratio = absF64(ys[u_i] - y_out.*[u_i]) / s6;
            if (r_ratio < 1.0) {
                const t = 1.0 - r_ratio * r_ratio;
                rob_w_s.*[u_i] = t * t;
            } else {
                rob_w_s.*[u_i] = 0.0;
            }
            u_i += 1;
        }

        iter += 1;
    }
    return true;
}

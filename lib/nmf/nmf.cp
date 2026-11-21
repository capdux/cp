// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Non-negative Matrix Factorization via Lee-Seung Multiplicative Updates)

/// Objective divergence / loss minimized by NMF.
pub enum NmfLoss {
    Frobenius,
    KlDivergence,
}

/// Hyperparameters for deterministic zero-allocation NMF.
pub struct NmfConfig {
    loss: NmfLoss,
    max_iter: usize,
    tol: f64,
    eps: f64,
    seed: u64,

    fn default(): NmfConfig {
        return {
            loss: NmfLoss.Frobenius,
            max_iter: 1000,
            tol: 1.0e-7,
            eps: 1.0e-12,
            seed: 0x9E3779B97F4A7C15,
        };
    }
}

/// Convergence and reconstruction summary returned by `nmfFit` and `nmfTransform`.
pub struct NmfResult {
    converged: bool,
    iterations: usize,
    loss: f64,
    frobenius_error: f64,
}

/// Caller-provided zero-allocation scratch workspace for Lee-Seung multiplicative updates.
/// Required lengths:
/// - `num_buf`: `>= max(m * r, r * n)` (or `>= m_new * r` for `nmfTransform`)
/// - `den_buf`: `>= max(m * r, r * n)` (or `>= m_new * r` for `nmfTransform`)
/// - `wh_buf`:  `>= m * n`
/// - `rr_buf`:  `>= r * r`
pub struct NmfWorkspace {
    num_buf: []f64,
    den_buf: []f64,
    wh_buf: []f64,
    rr_buf: []f64,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn splitMix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn randUniformPos(state: *u64): f64 {
    const bits = splitMix64Next(state) >> 11;
    // Strictly in (0, 1] so initial factors are strictly positive
    return ((bits as f64) + 1.0) * (1.0 / 9007199254740992.0);
}

fn validateConfig(cfg: NmfConfig): bool {
    if (cfg.max_iter == 0) {
        return false;
    }
    if (!isFinite(cfg.tol) || cfg.tol < 0.0) {
        return false;
    }
    if (!isFinite(cfg.eps) || cfg.eps <= 0.0) {
        return false;
    }
    return true;
}

fn checkNonNegativeMatrix(mat: []const f64, len: usize): bool {
    if (mat.len < len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        const v = mat[i];
        if (!isFinite(v) || v < 0.0) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn checkFiniteMatrix(mat: []const f64, len: usize): bool {
    if (mat.len < len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (!isFinite(mat[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn computeWH(w: []const f64, h: []const f64, m: usize, n: usize, r: usize, wh: []f64) {
    let i: usize = 0;
    while (i < m) {
        const w_row = i * r;
        const wh_row = i * n;
        let j: usize = 0;
        while (j < n) {
            let acc: f64 = 0.0;
            let k: usize = 0;
            while (k < r) {
                acc += w[w_row + k] * h[k * n + j];
                k += 1;
            }
            wh[wh_row + j] = acc;
            j += 1;
        }
        i += 1;
    }
}

fn frobeniusSqFromWH(v: []const f64, wh: []const f64, mn: usize): f64 {
    let sq: f64 = 0.0;
    let idx: usize = 0;
    while (idx < mn) {
        const d = v[idx] - wh[idx];
        sq += d * d;
        idx += 1;
    }
    return sq;
}

fn klFromWH(v: []const f64, wh: []const f64, mn: usize, eps: f64): f64 {
    let kl: f64 = 0.0;
    let idx: usize = 0;
    while (idx < mn) {
        const vij = v[idx];
        const whij = wh[idx];
        if (vij > 0.0) {
            const denom = if (whij > eps) whij else eps;
            kl += vij * @log(vij / denom) - vij + whij;
        } else {
            kl += whij;
        }
        idx += 1;
    }
    return if (kl < 0.0) 0.0 else kl;
}

// Computes H * H^T (r x r) into rr_buf
fn computeHHt(h: []const f64, r: usize, n: usize, rr: []f64) {
    let a: usize = 0;
    while (a < r) {
        const ha = a * n;
        let b: usize = a;
        while (b < r) {
            const hb = b * n;
            let acc: f64 = 0.0;
            let j: usize = 0;
            while (j < n) {
                acc += h[ha + j] * h[hb + j];
                j += 1;
            }
            rr[a * r + b] = acc;
            rr[b * r + a] = acc;
            b += 1;
        }
        a += 1;
    }
}

// Computes W^T * W (r x r) into rr_buf
fn computeWtW(w: []const f64, m: usize, r: usize, rr: []f64) {
    let a: usize = 0;
    while (a < r) {
        let b: usize = a;
        while (b < r) {
            let acc: f64 = 0.0;
            let i: usize = 0;
            while (i < m) {
                const row = i * r;
                acc += w[row + a] * w[row + b];
                i += 1;
            }
            rr[a * r + b] = acc;
            rr[b * r + a] = acc;
            b += 1;
        }
        a += 1;
    }
}

fn updateHFrobenius(
    v: []const f64,
    w: []const f64,
    h: *[]f64,
    m: usize,
    n: usize,
    r: usize,
    eps: f64,
    ws: *NmfWorkspace,
) {
    // num = W^T * V (r x n)
    let k: usize = 0;
    while (k < r) {
        const h_row = k * n;
        let j: usize = 0;
        while (j < n) {
            let acc: f64 = 0.0;
            let i: usize = 0;
            while (i < m) {
                acc += w[i * r + k] * v[i * n + j];
                i += 1;
            }
            ws.*.num_buf[h_row + j] = acc;
            j += 1;
        }
        k += 1;
    }

    // rr = W^T * W (r x r)
    computeWtW(w, m, r, ws.*.rr_buf);

    // den = (W^T * W) * H (r x n)
    let k2: usize = 0;
    while (k2 < r) {
        const h_row = k2 * n;
        const rr_row = k2 * r;
        let j: usize = 0;
        while (j < n) {
            let acc: f64 = 0.0;
            let p: usize = 0;
            while (p < r) {
                acc += ws.*.rr_buf[rr_row + p] * h.*[p * n + j];
                p += 1;
            }
            ws.*.den_buf[h_row + j] = acc;
            j += 1;
        }
        k2 += 1;
    }

    const rn = r * n;
    let idx: usize = 0;
    while (idx < rn) {
        const num = ws.*.num_buf[idx];
        const den = ws.*.den_buf[idx];
        const safe_den = if (den > eps) den else eps;
        const updated = h.*[idx] * (num / safe_den);
        h.*[idx] = if (updated > 0.0) updated else 0.0;
        idx += 1;
    }
}

fn updateWFrobenius(
    v: []const f64,
    w: *[]f64,
    h: []const f64,
    m: usize,
    n: usize,
    r: usize,
    eps: f64,
    recompute_hht: bool,
    ws: *NmfWorkspace,
) {
    // num = V * H^T (m x r)
    let i: usize = 0;
    while (i < m) {
        const v_row = i * n;
        const w_row = i * r;
        let k: usize = 0;
        while (k < r) {
            const h_row = k * n;
            let acc: f64 = 0.0;
            let j: usize = 0;
            while (j < n) {
                acc += v[v_row + j] * h[h_row + j];
                j += 1;
            }
            ws.*.num_buf[w_row + k] = acc;
            k += 1;
        }
        i += 1;
    }

    if (recompute_hht) {
        computeHHt(h, r, n, ws.*.rr_buf);
    }

    // den = W * (H * H^T) (m x r)
    let row_i: usize = 0;
    while (row_i < m) {
        const w_row = row_i * r;
        let k: usize = 0;
        while (k < r) {
            let acc: f64 = 0.0;
            let p: usize = 0;
            while (p < r) {
                acc += w.*[w_row + p] * ws.*.rr_buf[p * r + k];
                p += 1;
            }
            ws.*.den_buf[w_row + k] = acc;
            k += 1;
        }
        row_i += 1;
    }

    const mr = m * r;
    let idx: usize = 0;
    while (idx < mr) {
        const num = ws.*.num_buf[idx];
        const den = ws.*.den_buf[idx];
        const safe_den = if (den > eps) den else eps;
        const updated = w.*[idx] * (num / safe_den);
        w.*[idx] = if (updated > 0.0) updated else 0.0;
        idx += 1;
    }
}

fn updateHKl(
    v: []const f64,
    w: []const f64,
    h: *[]f64,
    m: usize,
    n: usize,
    r: usize,
    eps: f64,
    ws: *NmfWorkspace,
) {
    // wh_buf <- V / max(W * H, eps)
    computeWH(w, h.*, m, n, r, ws.*.wh_buf);
    const mn = m * n;
    let q_idx: usize = 0;
    while (q_idx < mn) {
        const wh_v = ws.*.wh_buf[q_idx];
        const safe_wh = if (wh_v > eps) wh_v else eps;
        ws.*.wh_buf[q_idx] = v[q_idx] / safe_wh;
        q_idx += 1;
    }

    // For each component k: col_sum = sum_i W[i, k], num[k, j] = sum_i W[i, k] * Q[i, j]
    let k: usize = 0;
    while (k < r) {
        let col_sum: f64 = 0.0;
        let i_sum: usize = 0;
        while (i_sum < m) {
            col_sum += w[i_sum * r + k];
            i_sum += 1;
        }
        const safe_den = if (col_sum > eps) col_sum else eps;
        const h_row = k * n;
        let j: usize = 0;
        while (j < n) {
            let num: f64 = 0.0;
            let i: usize = 0;
            while (i < m) {
                num += w[i * r + k] * ws.*.wh_buf[i * n + j];
                i += 1;
            }
            const updated = h.*[h_row + j] * (num / safe_den);
            h.*[h_row + j] = if (updated > 0.0) updated else 0.0;
            j += 1;
        }
        k += 1;
    }
}

fn updateWKl(
    v: []const f64,
    w: *[]f64,
    h: []const f64,
    m: usize,
    n: usize,
    r: usize,
    eps: f64,
    ws: *NmfWorkspace,
) {
    // wh_buf <- V / max(W * H, eps)
    computeWH(w.*, h, m, n, r, ws.*.wh_buf);
    const mn = m * n;
    let q_idx: usize = 0;
    while (q_idx < mn) {
        const wh_v = ws.*.wh_buf[q_idx];
        const safe_wh = if (wh_v > eps) wh_v else eps;
        ws.*.wh_buf[q_idx] = v[q_idx] / safe_wh;
        q_idx += 1;
    }

    // Store row sums of H in den_buf[0..r]
    let k_s: usize = 0;
    while (k_s < r) {
        const h_row = k_s * n;
        let row_sum: f64 = 0.0;
        let j_s: usize = 0;
        while (j_s < n) {
            row_sum += h[h_row + j_s];
            j_s += 1;
        }
        ws.*.den_buf[k_s] = if (row_sum > eps) row_sum else eps;
        k_s += 1;
    }

    let i: usize = 0;
    while (i < m) {
        const q_row = i * n;
        const w_row = i * r;
        let k: usize = 0;
        while (k < r) {
            const h_row = k * n;
            let num: f64 = 0.0;
            let j: usize = 0;
            while (j < n) {
                num += ws.*.wh_buf[q_row + j] * h[h_row + j];
                j += 1;
            }
            const updated = w.*[w_row + k] * (num / ws.*.den_buf[k]);
            w.*[w_row + k] = if (updated > 0.0) updated else 0.0;
            k += 1;
        }
        i += 1;
    }
}

/// Zero-allocation Non-negative Matrix Factorization `V ≈ W * H` using Lee & Seung (2001) multiplicative updates.
/// Validates that all entries of `v` (`m x n`, row-major) are finite and `>= 0.0`.
/// Writes non-negative factors to `w_out` (`m x r`) and `h_out` (`r x n`).
pub fn nmfFit(
    v: []const f64,
    m: usize,
    n: usize,
    r: usize,
    cfg: NmfConfig,
    w_out: *[]f64,
    h_out: *[]f64,
    ws: *NmfWorkspace,
): NmfResult? {
    if (m == 0 || n == 0 || r == 0) {
        return null;
    }
    if (!validateConfig(cfg)) {
        return null;
    }
    if (v.len / m < n || w_out.*.len / m < r || h_out.*.len / r < n) {
        return null;
    }
    const mn = m * n;
    const mr = m * r;
    const rn = r * n;
    const max_mr_rn = if (mr > rn) mr else rn;
    if (ws.*.num_buf.len < max_mr_rn || ws.*.den_buf.len < max_mr_rn) {
        return null;
    }
    if (ws.*.wh_buf.len < mn || ws.*.rr_buf.len / r < r) {
        return null;
    }
    if (!checkNonNegativeMatrix(v, mn)) {
        return null;
    }

    // Compute mean(V) to scale SplitMix64 initialization
    let v_sum: f64 = 0.0;
    let vi: usize = 0;
    while (vi < mn) {
        v_sum += v[vi];
        vi += 1;
    }
    if (v_sum == 0.0) {
        let wi: usize = 0;
        while (wi < mr) {
            w_out.*[wi] = 0.0;
            wi += 1;
        }
        let hi: usize = 0;
        while (hi < rn) {
            h_out.*[hi] = 0.0;
            hi += 1;
        }
        return {
            converged: true,
            iterations: 0,
            loss: 0.0,
            frobenius_error: 0.0,
        };
    }

    const scale = 2.0 * @sqrt((v_sum / (mn as f64)) / (r as f64));
    let rng: u64 = cfg.seed;
    let wi: usize = 0;
    while (wi < mr) {
        w_out.*[wi] = scale * randUniformPos(*rng);
        wi += 1;
    }
    let hi: usize = 0;
    while (hi < rn) {
        h_out.*[hi] = scale * randUniformPos(*rng);
        hi += 1;
    }

    computeWH(w_out.*, h_out.*, m, n, r, ws.*.wh_buf);
    let prev_loss: f64 = match (cfg.loss) {
        NmfLoss.Frobenius => 0.5 * frobeniusSqFromWH(v, ws.*.wh_buf, mn),
        NmfLoss.KlDivergence => klFromWH(v, ws.*.wh_buf, mn, cfg.eps),
    };

    let cur_loss = prev_loss;
    let frob_sq: f64 = frobeniusSqFromWH(v, ws.*.wh_buf, mn);
    let iters: usize = 0;
    let converged = false;

    while (iters < cfg.max_iter) {
        match (cfg.loss) {
            NmfLoss.Frobenius => {
                updateHFrobenius(v, w_out.*, h_out, m, n, r, cfg.eps, ws);
                updateWFrobenius(v, w_out, h_out.*, m, n, r, cfg.eps, true, ws);
                computeWH(w_out.*, h_out.*, m, n, r, ws.*.wh_buf);
                frob_sq = frobeniusSqFromWH(v, ws.*.wh_buf, mn);
                cur_loss = 0.5 * frob_sq;
            },
            NmfLoss.KlDivergence => {
                updateHKl(v, w_out.*, h_out, m, n, r, cfg.eps, ws);
                updateWKl(v, w_out, h_out.*, m, n, r, cfg.eps, ws);
                computeWH(w_out.*, h_out.*, m, n, r, ws.*.wh_buf);
                cur_loss = klFromWH(v, ws.*.wh_buf, mn, cfg.eps);
            },
        }
        iters += 1;

        const denom_scale = if (prev_loss > 1.0) prev_loss else 1.0;
        if (absF64(prev_loss - cur_loss) <= cfg.tol * denom_scale) {
            converged = true;
            break;
        }
        prev_loss = cur_loss;
    }

    if (cfg.loss == NmfLoss.KlDivergence) {
        frob_sq = frobeniusSqFromWH(v, ws.*.wh_buf, mn);
    }

    return {
        converged: converged,
        iterations: iters,
        loss: cur_loss,
        frobenius_error: @sqrt(frob_sq),
    };
}

/// Projects new non-negative samples `v_new` (`m_new x n`) onto a fixed non-negative basis `h_fixed` (`r x n`).
/// Updates only `w_out` (`m_new x r`) via Lee-Seung multiplicative updates.
pub fn nmfTransform(
    v_new: []const f64,
    m_new: usize,
    n: usize,
    r: usize,
    h_fixed: []const f64,
    cfg: NmfConfig,
    w_out: *[]f64,
    ws: *NmfWorkspace,
): NmfResult? {
    if (m_new == 0 || n == 0 || r == 0) {
        return null;
    }
    if (!validateConfig(cfg)) {
        return null;
    }
    if (v_new.len / m_new < n || h_fixed.len / r < n || w_out.*.len / m_new < r) {
        return null;
    }
    const mn = m_new * n;
    const mr = m_new * r;
    const rn = r * n;
    if (ws.*.num_buf.len < mr || ws.*.den_buf.len < mr) {
        return null;
    }
    if (ws.*.wh_buf.len < mn || ws.*.rr_buf.len / r < r) {
        return null;
    }
    if (!checkNonNegativeMatrix(v_new, mn) || !checkNonNegativeMatrix(h_fixed, rn)) {
        return null;
    }

    let v_sum: f64 = 0.0;
    let vi: usize = 0;
    while (vi < mn) {
        v_sum += v_new[vi];
        vi += 1;
    }
    if (v_sum == 0.0) {
        let wi: usize = 0;
        while (wi < mr) {
            w_out.*[wi] = 0.0;
            wi += 1;
        }
        return {
            converged: true,
            iterations: 0,
            loss: 0.0,
            frobenius_error: 0.0,
        };
    }

    let h_sum: f64 = 0.0;
    let hi: usize = 0;
    while (hi < rn) {
        h_sum += h_fixed[hi];
        hi += 1;
    }
    const h_mean = h_sum / (rn as f64);
    const v_mean = v_sum / (mn as f64);
    const scale = if (h_mean > cfg.eps) (2.0 * v_mean / ((r as f64) * h_mean)) else 1.0;

    let rng: u64 = cfg.seed;
    let wi: usize = 0;
    while (wi < mr) {
        w_out.*[wi] = scale * randUniformPos(*rng);
        wi += 1;
    }

    // Precompute H * H^T once for Frobenius updates
    if (cfg.loss == NmfLoss.Frobenius) {
        computeHHt(h_fixed, r, n, ws.*.rr_buf);
    }

    computeWH(w_out.*, h_fixed, m_new, n, r, ws.*.wh_buf);
    let prev_loss: f64 = match (cfg.loss) {
        NmfLoss.Frobenius => 0.5 * frobeniusSqFromWH(v_new, ws.*.wh_buf, mn),
        NmfLoss.KlDivergence => klFromWH(v_new, ws.*.wh_buf, mn, cfg.eps),
    };

    let cur_loss = prev_loss;
    let frob_sq: f64 = frobeniusSqFromWH(v_new, ws.*.wh_buf, mn);
    let iters: usize = 0;
    let converged = false;

    while (iters < cfg.max_iter) {
        match (cfg.loss) {
            NmfLoss.Frobenius => {
                updateWFrobenius(v_new, w_out, h_fixed, m_new, n, r, cfg.eps, false, ws);
                computeWH(w_out.*, h_fixed, m_new, n, r, ws.*.wh_buf);
                frob_sq = frobeniusSqFromWH(v_new, ws.*.wh_buf, mn);
                cur_loss = 0.5 * frob_sq;
            },
            NmfLoss.KlDivergence => {
                updateWKl(v_new, w_out, h_fixed, m_new, n, r, cfg.eps, ws);
                computeWH(w_out.*, h_fixed, m_new, n, r, ws.*.wh_buf);
                cur_loss = klFromWH(v_new, ws.*.wh_buf, mn, cfg.eps);
            },
        }
        iters += 1;

        const denom_scale = if (prev_loss > 1.0) prev_loss else 1.0;
        if (absF64(prev_loss - cur_loss) <= cfg.tol * denom_scale) {
            converged = true;
            break;
        }
        prev_loss = cur_loss;
    }

    if (cfg.loss == NmfLoss.KlDivergence) {
        frob_sq = frobeniusSqFromWH(v_new, ws.*.wh_buf, mn);
    }

    return {
        converged: converged,
        iterations: iters,
        loss: cur_loss,
        frobenius_error: @sqrt(frob_sq),
    };
}

/// Computes `v_hat_out = w * h` (`m x n`, row-major). Returns `false` on invalid dimensions, short buffers, or non-finite values.
pub fn nmfReconstruct(
    w: []const f64,
    h: []const f64,
    m: usize,
    n: usize,
    r: usize,
    v_hat_out: *[]f64,
): bool {
    if (m == 0 || n == 0 || r == 0) {
        return false;
    }
    if (w.len / m < r || h.len / r < n || v_hat_out.*.len / m < n) {
        return false;
    }
    if (!checkFiniteMatrix(w, m * r) || !checkFiniteMatrix(h, r * n)) {
        return false;
    }
    computeWH(w, h, m, n, r, v_hat_out.*);
    return true;
}

/// Computes Frobenius reconstruction error `||V - W * H||_F` without scratch allocation.
/// Returns `NaN` on invalid dimensions, short buffers, or non-finite inputs.
pub fn nmfFrobeniusError(
    v: []const f64,
    w: []const f64,
    h: []const f64,
    m: usize,
    n: usize,
    r: usize,
): f64 {
    if (m == 0 || n == 0 || r == 0) {
        return std.math.nan(f64);
    }
    if (v.len / m < n || w.len / m < r || h.len / r < n) {
        return std.math.nan(f64);
    }
    if (!checkFiniteMatrix(v, m * n) || !checkFiniteMatrix(w, m * r) || !checkFiniteMatrix(h, r * n)) {
        return std.math.nan(f64);
    }
    let sq: f64 = 0.0;
    let i: usize = 0;
    while (i < m) {
        const w_row = i * r;
        const v_row = i * n;
        let j: usize = 0;
        while (j < n) {
            let wh_ij: f64 = 0.0;
            let k: usize = 0;
            while (k < r) {
                wh_ij += w[w_row + k] * h[k * n + j];
                k += 1;
            }
            const d = v[v_row + j] - wh_ij;
            sq += d * d;
            j += 1;
        }
        i += 1;
    }
    return @sqrt(sq);
}

/// Computes generalized KL divergence `sum(V * ln(V / max(WH, eps)) - V + WH)` without scratch allocation.
/// Returns `NaN` on invalid dimensions, non-positive `eps`, short buffers, negative entries, or non-finite inputs.
pub fn nmfKlDivergence(
    v: []const f64,
    w: []const f64,
    h: []const f64,
    m: usize,
    n: usize,
    r: usize,
    eps: f64,
): f64 {
    if (m == 0 || n == 0 || r == 0 || !isFinite(eps) || eps <= 0.0) {
        return std.math.nan(f64);
    }
    if (v.len / m < n || w.len / m < r || h.len / r < n) {
        return std.math.nan(f64);
    }
    if (!checkNonNegativeMatrix(v, m * n) || !checkNonNegativeMatrix(w, m * r) || !checkNonNegativeMatrix(h, r * n)) {
        return std.math.nan(f64);
    }
    let kl: f64 = 0.0;
    let i: usize = 0;
    while (i < m) {
        const w_row = i * r;
        const v_row = i * n;
        let j: usize = 0;
        while (j < n) {
            let wh_ij: f64 = 0.0;
            let k: usize = 0;
            while (k < r) {
                wh_ij += w[w_row + k] * h[k * n + j];
                k += 1;
            }
            const vij = v[v_row + j];
            if (vij > 0.0) {
                const denom = if (wh_ij > eps) wh_ij else eps;
                kl += vij * @log(vij / denom) - vij + wh_ij;
            } else {
                kl += wh_ij;
            }
            j += 1;
        }
        i += 1;
    }
    return if (kl < 0.0) 0.0 else kl;
}

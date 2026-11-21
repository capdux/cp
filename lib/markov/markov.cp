// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Discrete-Time Markov Chain DTMC Analysis, Absorbing Chains, Entropy Rate & Deterministic Sampling)

import std from "std";

const DEFAULT_SEED: u64 = 0x9e3779b97f4a7c15;
const STOCHASTIC_TOL: f64 = 1.0e-9;
const LU_PIVOT_TOL: f64 = 1.0e-12;
const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn checkRectLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn splitmix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9e3779b97f4a7c15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn matMulSquare(a: []const f64, b: []const f64, out: []f64, n: usize) {
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                sum = sum + a[i * n + k] * b[k * n + j];
                k += 1;
            }
            out[i * n + j] = sum;
            j += 1;
        }
        i += 1;
    }
}

/// Validates that row-major `n x n` matrix `p` is a valid right-stochastic transition matrix:
/// every entry is finite and `>= 0`, and every row sums to `1.0` within `tol`.
pub fn isStochasticMatrix(p: []const f64, n: usize, tol: f64): bool {
    if (!checkRectLen(p.len, n, n) || !isFiniteF64(tol) || tol < 0.0) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        let row_sum: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            const v = p[i * n + j];
            if (!isFiniteF64(v) || v < 0.0) {
                return false;
            }
            row_sum = row_sum + v;
            j += 1;
        }
        if (!isFiniteF64(row_sum) || absF64(row_sum - 1.0) > tol) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Advances state distribution row vector by one step: `next_out = dist * P`.
///
/// Returns `false` if dimensions/buffers are invalid or inputs contain non-finite values.
pub fn stepDistribution(
    dist: []const f64,
    p: []const f64,
    n: usize,
    next_out: []f64,
): bool {
    if (!checkRectLen(p.len, n, n) || dist.len < n || next_out.len < n) {
        return false;
    }
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(dist[chk])) {
            return false;
        }
        chk += 1;
    }
    let j: usize = 0;
    while (j < n) {
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < n) {
            const pij = p[i * n + j];
            if (!isFiniteF64(pij)) {
                return false;
            }
            sum = sum + dist[i] * pij;
            i += 1;
        }
        next_out[j] = sum;
        j += 1;
    }
    return true;
}

/// Computes the `steps`-step Chapman-Kolmogorov transition matrix `P^steps` via binary exponentiation.
///
/// Requires `p` to be a valid stochastic matrix and `out_mat`, `base_s`, `tmp_s` to each have length `>= n * n`.
pub fn transitionPower(
    p: []const f64,
    n: usize,
    steps: u64,
    out_mat: []f64,
    base_s: []f64,
    tmp_s: []f64,
): bool {
    if (!isStochasticMatrix(p, n, STOCHASTIC_TOL)) {
        return false;
    }
    if (!checkRectLen(out_mat.len, n, n) || !checkRectLen(base_s.len, n, n) || !checkRectLen(tmp_s.len, n, n)) {
        return false;
    }

    // Initialize out_mat = I_n, base_s = P
    let r: usize = 0;
    while (r < n) {
        let c: usize = 0;
        while (c < n) {
            const idx = r * n + c;
            out_mat[idx] = if (r == c) 1.0 else 0.0;
            base_s[idx] = p[idx];
            c += 1;
        }
        r += 1;
    }

    const total = n * n;
    let exp = steps;
    while (exp > 0) {
        if ((exp & 1) == 1) {
            matMulSquare(out_mat[0..total], base_s[0..total], tmp_s[0..total], n);
            let k: usize = 0;
            while (k < total) {
                out_mat[k] = tmp_s[k];
                k += 1;
            }
        }
        exp = exp >> 1;
        if (exp > 0) {
            matMulSquare(base_s[0..total], base_s[0..total], tmp_s[0..total], n);
            let k: usize = 0;
            while (k < total) {
                base_s[k] = tmp_s[k];
                k += 1;
            }
        }
    }

    return true;
}

/// Convergence report for `stationaryPowerIter`.
pub struct StationaryResult {
    converged: bool;
    iterations: usize;
    residual_l1: f64;
}

/// Computes the stationary distribution `pi` satisfying `pi * P = pi` and `sum(pi) = 1`
/// via power iteration from a uniform initial distribution.
///
/// Returns `null` if `p` is not stochastic, `tol <= 0`, or output/scratch slices have length `< n`.
pub fn stationaryPowerIter(
    p: []const f64,
    n: usize,
    pi_out: []f64,
    tmp_s: []f64,
    max_iter: usize,
    tol: f64,
): StationaryResult? {
    if (!isStochasticMatrix(p, n, STOCHASTIC_TOL)) {
        return null;
    }
    if (pi_out.len < n || tmp_s.len < n || !isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }

    const inv_n = 1.0 / (n as f64);
    let i: usize = 0;
    while (i < n) {
        pi_out[i] = inv_n;
        i += 1;
    }

    if (max_iter == 0) {
        return StationaryResult {
            converged: false,
            iterations: 0,
            residual_l1: 0.0,
        };
    }

    let last_diff: f64 = 0.0;
    let iter: usize = 0;
    while (iter < max_iter) {
        iter += 1;

        let sum_next: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            let acc: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                acc = acc + pi_out[k] * p[k * n + j];
                k += 1;
            }
            tmp_s[j] = acc;
            sum_next = sum_next + acc;
            j += 1;
        }

        const inv_sum = if (sum_next > 0.0) 1.0 / sum_next else 1.0;
        let diff: f64 = 0.0;
        j = 0;
        while (j < n) {
            const val = tmp_s[j] * inv_sum;
            diff = diff + absF64(val - pi_out[j]);
            pi_out[j] = val;
            j += 1;
        }

        last_diff = diff;
        if (diff <= tol) {
            return StationaryResult {
                converged: true,
                iterations: iter,
                residual_l1: diff,
            };
        }
    }

    return StationaryResult {
        converged: false,
        iterations: max_iter,
        residual_l1: last_diff,
    };
}

/// Computes the fundamental matrix `N = (I - Q)^-1` of an absorbing Markov chain
/// with `t x t` transient-to-transient submatrix `q_mat`, using in-place partial-pivoting LU inversion.
///
/// Returns `false` if `t == 0`, buffers are undersized, `q_mat` has invalid entries, or `(I - Q)` is singular.
pub fn absorbingFundamental(
    q_mat: []const f64,
    t: usize,
    fundamental_out: []f64,
    lu_scratch: []f64,
    piv_scratch: []usize,
    rhs_s: []f64,
    col_s: []f64,
): bool {
    if (!checkRectLen(q_mat.len, t, t) || !checkRectLen(fundamental_out.len, t, t) || !checkRectLen(lu_scratch.len, t, t)) {
        return false;
    }
    if (piv_scratch.len < t || rhs_s.len < t || col_s.len < t) {
        return false;
    }

    // Build M = I - Q in lu_scratch while validating Q
    let r: usize = 0;
    while (r < t) {
        let row_sum: f64 = 0.0;
        let c: usize = 0;
        while (c < t) {
            const qv = q_mat[r * t + c];
            if (!isFiniteF64(qv) || qv < 0.0) {
                return false;
            }
            row_sum = row_sum + qv;
            lu_scratch[r * t + c] = (if (r == c) 1.0 else 0.0) - qv;
            c += 1;
        }
        if (row_sum > 1.0 + STOCHASTIC_TOL) {
            return false;
        }
        piv_scratch[r] = r;
        r += 1;
    }

    // Partial-pivoting Doolittle LU factorization of (I - Q)
    let k: usize = 0;
    while (k < t) {
        let piv_row = k;
        let max_abs = absF64(lu_scratch[k * t + k]);
        let row = k + 1;
        while (row < t) {
            const cand = absF64(lu_scratch[row * t + k]);
            if (cand > max_abs) {
                max_abs = cand;
                piv_row = row;
            }
            row += 1;
        }
        if (!isFiniteF64(max_abs) || max_abs <= LU_PIVOT_TOL) {
            return false;
        }
        if (piv_row != k) {
            let c: usize = 0;
            while (c < t) {
                const tmp = lu_scratch[k * t + c];
                lu_scratch[k * t + c] = lu_scratch[piv_row * t + c];
                lu_scratch[piv_row * t + c] = tmp;
                c += 1;
            }
            const tp = piv_scratch[k];
            piv_scratch[k] = piv_scratch[piv_row];
            piv_scratch[piv_row] = tp;
        }

        const pivot = lu_scratch[k * t + k];
        row = k + 1;
        while (row < t) {
            const factor = lu_scratch[row * t + k] / pivot;
            lu_scratch[row * t + k] = factor;
            let c = k + 1;
            while (c < t) {
                lu_scratch[row * t + c] = lu_scratch[row * t + c] - factor * lu_scratch[k * t + c];
                c += 1;
            }
            row += 1;
        }
        k += 1;
    }

    // Invert column-by-column: solve (I - Q) * col_s = e_col
    let col: usize = 0;
    while (col < t) {
        let i: usize = 0;
        while (i < t) {
            rhs_s[i] = if (i == col) 1.0 else 0.0;
            i += 1;
        }
        i = 0;
        while (i < t) {
            col_s[i] = rhs_s[piv_scratch[i]];
            i += 1;
        }
        i = 0;
        while (i < t) {
            let sum = col_s[i];
            let j: usize = 0;
            while (j < i) {
                sum = sum - lu_scratch[i * t + j] * col_s[j];
                j += 1;
            }
            col_s[i] = sum;
            i += 1;
        }
        let rev = t;
        while (rev > 0) {
            rev -= 1;
            let sum = col_s[rev];
            let j = rev + 1;
            while (j < t) {
                sum = sum - lu_scratch[rev * t + j] * col_s[j];
                j += 1;
            }
            const diag = lu_scratch[rev * t + rev];
            if (!isFiniteF64(diag) || absF64(diag) <= LU_PIVOT_TOL) {
                return false;
            }
            col_s[rev] = sum / diag;
        }
        i = 0;
        while (i < t) {
            fundamental_out[i * t + col] = col_s[i];
            i += 1;
        }
        col += 1;
    }

    return true;
}

/// Computes expected number of steps before absorption from each transient state: `steps = N * 1`.
pub fn expectedAbsorptionSteps(
    fundamental: []const f64,
    t: usize,
    steps_out: []f64,
): bool {
    if (!checkRectLen(fundamental.len, t, t) || steps_out.len < t) {
        return false;
    }
    let i: usize = 0;
    while (i < t) {
        let sum: f64 = 0.0;
        let j: usize = 0;
        while (j < t) {
            const v = fundamental[i * t + j];
            if (!isFiniteF64(v)) {
                return false;
            }
            sum = sum + v;
            j += 1;
        }
        steps_out[i] = sum;
        i += 1;
    }
    return true;
}

/// Computes the `t x r` absorption probability matrix `B = N * R`, where `B[i, k]` is the
/// probability of being absorbed in absorbing state `k` starting from transient state `i`.
pub fn absorptionProbabilities(
    fundamental: []const f64,
    r_mat: []const f64,
    t: usize,
    r: usize,
    probs_out: []f64,
): bool {
    if (!checkRectLen(fundamental.len, t, t) || !checkRectLen(r_mat.len, t, r) || !checkRectLen(probs_out.len, t, r)) {
        return false;
    }
    let i: usize = 0;
    while (i < t) {
        let k: usize = 0;
        while (k < r) {
            let sum: f64 = 0.0;
            let j: usize = 0;
            while (j < t) {
                const nv = fundamental[i * t + j];
                const rv = r_mat[j * r + k];
                if (!isFiniteF64(nv) || !isFiniteF64(rv)) {
                    return false;
                }
                sum = sum + nv * rv;
                j += 1;
            }
            probs_out[i * r + k] = sum;
            k += 1;
        }
        i += 1;
    }
    return true;
}

/// Deterministically samples a state trajectory of length `path_out.len` starting at `start_state`
/// using SplitMix64 row-CDF sampling.
///
/// Returns `false` if `p` is not stochastic or `start_state >= n`.
pub fn sampleTrajectory(
    p: []const f64,
    n: usize,
    start_state: usize,
    seed: u64,
    path_out: []usize,
): bool {
    if (!isStochasticMatrix(p, n, STOCHASTIC_TOL) || start_state >= n) {
        return false;
    }
    if (path_out.len == 0) {
        return true;
    }

    let rng: u64 = if (seed == 0) DEFAULT_SEED else seed;
    let cur = start_state;
    path_out[0] = cur;

    let step: usize = 1;
    while (step < path_out.len) {
        const u = ((splitmix64Next(&rng) >> 11) as f64) * INV_2_POW_53;
        let cdf: f64 = 0.0;
        let chosen = n - 1;
        let last_pos = n - 1;
        let j: usize = 0;
        while (j < n) {
            const prob = p[cur * n + j];
            if (prob > 0.0) {
                last_pos = j;
            }
            cdf = cdf + prob;
            if (u < cdf) {
                chosen = j;
                break;
            }
            j += 1;
        }
        if (j == n) {
            chosen = last_pos;
        }
        cur = chosen;
        path_out[step] = cur;
        step += 1;
    }

    return true;
}

/// Computes the Shannon entropy rate (in bits per step) of a stationary Markov chain:
/// `H = -sum_i pi[i] * sum_j P[i, j] * log2(P[i, j])`.
///
/// Returns `null` if `p` is not stochastic or `pi` is not a valid probability distribution.
pub fn entropyRate(p: []const f64, pi: []const f64, n: usize): f64? {
    if (!isStochasticMatrix(p, n, STOCHASTIC_TOL) || pi.len < n) {
        return null;
    }

    let pi_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const w = pi[i];
        if (!isFiniteF64(w) || w < 0.0) {
            return null;
        }
        pi_sum = pi_sum + w;
        i += 1;
    }
    if (absF64(pi_sum - 1.0) > 1.0e-6) {
        return null;
    }

    let h: f64 = 0.0;
    i = 0;
    while (i < n) {
        const w = pi[i];
        if (w > 0.0) {
            let row_h: f64 = 0.0;
            let j: usize = 0;
            while (j < n) {
                const pij = p[i * n + j];
                if (pij > 0.0) {
                    row_h = row_h - pij * std.math.log2(pij);
                }
                j += 1;
            }
            h = h + w * row_h;
        }
        i += 1;
    }
    return if (h < 0.0) 0.0 else h;
}

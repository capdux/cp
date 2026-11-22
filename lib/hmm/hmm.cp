// Origin: Pure-CP Stdlib (Zero-Alloc Discrete HMM: Log-Space Viterbi, Rabiner Scaled Forward-Backward & Baum-Welch EM) | Created: 2026-11-22

import std from "std";

const LN2: f64 = 0.69314718055994530942;
const PROB_SUM_TOL: f64 = 1.0e-6;

/// Discrete Hidden Markov Model `lambda = (pi, A, B)`.
/// - `init_prob`: length `>= num_states`
/// - `trans_prob`: row-major `num_states x num_states`
/// - `emit_prob`: row-major `num_states x num_obs`
pub struct HmmModel {
    num_states: usize,
    num_obs: usize,
    init_prob: []const f64,
    trans_prob: []const f64,
    emit_prob: []const f64,

    /// Validates non-empty dimensions, sufficient slice lengths, non-negative finite probabilities,
    /// and row-wise normalization (`sum == 1.0` within `1e-6`).
    fn validate(): bool {
        const n: usize = _.num_states;
        const m: usize = _.num_obs;
        if (n == 0 || m == 0) {
            return false;
        }
        if (_.init_prob.len < n || !checkMatrixLen(_.trans_prob.len, n, n) || !checkMatrixLen(_.emit_prob.len, n, m)) {
            return false;
        }

        // Check initial state distribution
        let pi_sum: f64 = 0.0;
        let i: usize = 0;
        while (i < n) {
            const v: f64 = _.init_prob[i];
            if (!isFiniteF64(v) || v < 0.0) {
                return false;
            }
            pi_sum += v;
            i += 1;
        }
        if (absF64(pi_sum - 1.0) > PROB_SUM_TOL) {
            return false;
        }

        // Check transition matrix rows
        let r: usize = 0;
        while (r < n) {
            let row_sum: f64 = 0.0;
            let c: usize = 0;
            while (c < n) {
                const v: f64 = _.trans_prob[r * n + c];
                if (!isFiniteF64(v) || v < 0.0) {
                    return false;
                }
                row_sum += v;
                c += 1;
            }
            if (absF64(row_sum - 1.0) > PROB_SUM_TOL) {
                return false;
            }
            r += 1;
        }

        // Check emission matrix rows
        let re: usize = 0;
        while (re < n) {
            let row_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < m) {
                const v: f64 = _.emit_prob[re * m + k];
                if (!isFiniteF64(v) || v < 0.0) {
                    return false;
                }
                row_sum += v;
                k += 1;
            }
            if (absF64(row_sum - 1.0) > PROB_SUM_TOL) {
                return false;
            }
            re += 1;
        }

        return true;
    }
}

/// Result returned by `viterbiDecode`.
pub struct ViterbiResult {
    log_prob: f64,
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn checkMatrixLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

/// Pure-CP double-precision natural logarithm for `x > 0` (~1e-16 accuracy).
fn lnF64(x: f64): f64 {
    let m: f64 = x;
    let k: i32 = 0;
    while (m >= 1.5) {
        m *= 0.5;
        k += 1;
    }
    while (m < 0.75) {
        m *= 2.0;
        k -= 1;
    }
    const t: f64 = (m - 1.0) / (m + 1.0);
    const t2: f64 = t * t;
    let term: f64 = t;
    let sum: f64 = 0.0;
    let denom: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 12) {
        sum += term / denom;
        term *= t2;
        denom += 2.0;
        iter += 1;
    }
    return 2.0 * sum + (k as f64) * LN2;
}

fn safeLog(p: f64): f64 {
    if (p > 0.0) {
        return lnF64(p);
    }
    return 0.0 - std.math.inf(f64);
}

fn checkObsValid(obs: []const usize, num_obs: usize): bool {
    if (obs.len == 0) {
        return false;
    }
    let t: usize = 0;
    while (t < obs.len) {
        if (obs[t] >= num_obs) {
            return false;
        }
        t += 1;
    }
    return true;
}

/// Log-space Viterbi decoding to find the most likely hidden state sequence `path_out[0..T]`.
/// Required lengths: `path_out >= T`, `dp_prev_s >= N`, `dp_curr_s >= N`, `backptr_s >= T * N`.
/// Returns `null` on invalid model/observations, insufficient buffers, or zero-probability sequence.
pub fn viterbiDecode(
    model: HmmModel,
    obs: []const usize,
    path_out: *[]usize,
    dp_prev_s: *[]f64,
    dp_curr_s: *[]f64,
    backptr_s: *[]usize,
): ViterbiResult? {
    if (!model.validate() || !checkObsValid(obs, model.num_obs)) {
        return null;
    }
    const n: usize = model.num_states;
    const m: usize = model.num_obs;
    const t_len: usize = obs.len;
    if (path_out.*.len < t_len || dp_prev_s.*.len < n || dp_curr_s.*.len < n || !checkMatrixLen(backptr_s.*.len, t_len, n)) {
        return null;
    }

    const neg_inf: f64 = 0.0 - std.math.inf(f64);

    // t = 0
    const o0: usize = obs[0];
    let st0: usize = 0;
    while (st0 < n) {
        const lp_init: f64 = safeLog(model.init_prob[st0]);
        const lp_emit: f64 = safeLog(model.emit_prob[st0 * m + o0]);
        dp_prev_s.*[st0] = if (lp_init > neg_inf && lp_emit > neg_inf) (lp_init + lp_emit) else neg_inf;
        backptr_s.*[st0] = 0;
        st0 += 1;
    }

    // t = 1 .. T - 1
    let t: usize = 1;
    while (t < t_len) {
        const ot: usize = obs[t];
        let j: usize = 0;
        while (j < n) {
            let best_val: f64 = neg_inf;
            let best_prev: usize = 0;
            let i: usize = 0;
            while (i < n) {
                const prev_v: f64 = dp_prev_s.*[i];
                if (prev_v > neg_inf) {
                    const lp_tr: f64 = safeLog(model.trans_prob[i * n + j]);
                    if (lp_tr > neg_inf) {
                        const cand: f64 = prev_v + lp_tr;
                        if (cand > best_val) {
                            best_val = cand;
                            best_prev = i;
                        }
                    }
                }
                i += 1;
            }
            const lp_em: f64 = safeLog(model.emit_prob[j * m + ot]);
            dp_curr_s.*[j] = if (best_val > neg_inf && lp_em > neg_inf) (best_val + lp_em) else neg_inf;
            backptr_s.*[t * n + j] = best_prev;
            j += 1;
        }

        let cp: usize = 0;
        while (cp < n) {
            dp_prev_s.*[cp] = dp_curr_s.*[cp];
            cp += 1;
        }
        t += 1;
    }

    // Termination
    let best_prob: f64 = neg_inf;
    let best_state: usize = 0;
    let k: usize = 0;
    while (k < n) {
        if (dp_prev_s.*[k] > best_prob) {
            best_prob = dp_prev_s.*[k];
            best_state = k;
        }
        k += 1;
    }
    if (best_prob <= neg_inf || !isFiniteF64(best_prob)) {
        return null;
    }

    // Backtrack
    path_out.*[t_len - 1] = best_state;
    let step: usize = t_len - 1;
    while (step > 0) {
        const cur_st: usize = path_out.*[step];
        path_out.*[step - 1] = backptr_s.*[step * n + cur_st];
        step -= 1;
    }

    return ViterbiResult { log_prob: best_prob };
}

/// Rabiner scaled Forward algorithm.
/// Writes normalized forward variables `alpha_out[t * N + i]` (row-sum == 1) and
/// scaling factors `scale_out[t] = 1 / sum_i alpha_tilde[t, i]`.
/// Returns exact log-likelihood `ln P(O | lambda)`, or `null` if invalid or impossible sequence.
pub fn forwardScaled(
    model: HmmModel,
    obs: []const usize,
    alpha_out: *[]f64,
    scale_out: *[]f64,
): f64? {
    if (!model.validate() || !checkObsValid(obs, model.num_obs)) {
        return null;
    }
    const n: usize = model.num_states;
    const m: usize = model.num_obs;
    const t_len: usize = obs.len;
    if (!checkMatrixLen(alpha_out.*.len, t_len, n) || scale_out.*.len < t_len) {
        return null;
    }

    // t = 0
    const o0: usize = obs[0];
    let sum0: f64 = 0.0;
    let st0: usize = 0;
    while (st0 < n) {
        const v: f64 = model.init_prob[st0] * model.emit_prob[st0 * m + o0];
        alpha_out.*[st0] = v;
        sum0 += v;
        st0 += 1;
    }
    if (sum0 <= 0.0 || !isFiniteF64(sum0)) {
        return null;
    }
    const c0: f64 = 1.0 / sum0;
    scale_out.*[0] = c0;
    let i0n: usize = 0;
    while (i0n < n) {
        alpha_out.*[i0n] *= c0;
        i0n += 1;
    }

    let log_lik: f64 = lnF64(sum0);

    // t = 1 .. T - 1
    let t: usize = 1;
    while (t < t_len) {
        const ot: usize = obs[t];
        const prev_base: usize = (t - 1) * n;
        const curr_base: usize = t * n;
        let sum_t: f64 = 0.0;

        let j: usize = 0;
        while (j < n) {
            let acc: f64 = 0.0;
            let i: usize = 0;
            while (i < n) {
                acc += alpha_out.*[prev_base + i] * model.trans_prob[i * n + j];
                i += 1;
            }
            const val: f64 = acc * model.emit_prob[j * m + ot];
            alpha_out.*[curr_base + j] = val;
            sum_t += val;
            j += 1;
        }

        if (sum_t <= 0.0 || !isFiniteF64(sum_t)) {
            return null;
        }
        const ct: f64 = 1.0 / sum_t;
        scale_out.*[t] = ct;
        let jn: usize = 0;
        while (jn < n) {
            alpha_out.*[curr_base + jn] *= ct;
            jn += 1;
        }
        log_lik += lnF64(sum_t);
        t += 1;
    }

    return log_lik;
}

/// Rabiner scaled Backward algorithm paired with `scale` produced by `forwardScaled`.
/// Writes scaled backward variables into `beta_out[0..T * N]`.
pub fn backwardScaled(
    model: HmmModel,
    obs: []const usize,
    scale: []const f64,
    beta_out: *[]f64,
): bool {
    if (!model.validate() || !checkObsValid(obs, model.num_obs)) {
        return false;
    }
    const n: usize = model.num_states;
    const m: usize = model.num_obs;
    const t_len: usize = obs.len;
    if (scale.len < t_len || !checkMatrixLen(beta_out.*.len, t_len, n)) {
        return false;
    }

    // t = T - 1
    const last_base: usize = (t_len - 1) * n;
    const c_last: f64 = scale[t_len - 1];
    if (!isFiniteF64(c_last) || c_last <= 0.0) {
        return false;
    }
    let il: usize = 0;
    while (il < n) {
        beta_out.*[last_base + il] = c_last;
        il += 1;
    }

    // t = T - 2 .. 0
    if (t_len >= 2) {
        let step: usize = t_len - 1;
        while (step > 0) {
            const t: usize = step - 1;
            const ct: f64 = scale[t];
            if (!isFiniteF64(ct) || ct <= 0.0) {
                return false;
            }
            const o_next: usize = obs[t + 1];
            const curr_base: usize = t * n;
            const next_base: usize = (t + 1) * n;

            let i: usize = 0;
            while (i < n) {
                let acc: f64 = 0.0;
                let j: usize = 0;
                while (j < n) {
                    acc += model.trans_prob[i * n + j]
                        * model.emit_prob[j * m + o_next]
                        * beta_out.*[next_base + j];
                    j += 1;
                }
                beta_out.*[curr_base + i] = acc * ct;
                i += 1;
            }
            step -= 1;
        }
    }

    return true;
}

/// Computes normalized posterior state probabilities `gamma_t(i) = P(S_t = i | O, lambda)`
/// from scaled `alpha` and `beta`. Every time-step row sums to `1.0`.
pub fn posteriorStates(
    alpha: []const f64,
    beta: []const f64,
    t_len: usize,
    num_states: usize,
    gamma_out: *[]f64,
): bool {
    if (t_len == 0 || num_states == 0) {
        return false;
    }
    if (!checkMatrixLen(alpha.len, t_len, num_states)
        || !checkMatrixLen(beta.len, t_len, num_states)
        || !checkMatrixLen(gamma_out.*.len, t_len, num_states)) {
        return false;
    }

    let t: usize = 0;
    while (t < t_len) {
        const base: usize = t * num_states;
        let denom: f64 = 0.0;
        let i: usize = 0;
        while (i < num_states) {
            const raw: f64 = alpha[base + i] * beta[base + i];
            if (!isFiniteF64(raw) || raw < 0.0) {
                return false;
            }
            gamma_out.*[base + i] = raw;
            denom += raw;
            i += 1;
        }
        if (denom <= 0.0 || !isFiniteF64(denom)) {
            return false;
        }
        const inv_d: f64 = 1.0 / denom;
        let inorm: usize = 0;
        while (inorm < num_states) {
            gamma_out.*[base + inorm] *= inv_d;
            inorm += 1;
        }
        t += 1;
    }

    return true;
}

/// Performs one Baum-Welch EM re-estimation step, writing updated parameters into
/// `next_init`, `next_trans`, and `next_emit`.
/// Returns the pre-update log-likelihood `ln P(O | lambda)`, or `null` on invalid inputs.
pub fn baumWelchStep(
    model: HmmModel,
    obs: []const usize,
    next_init: *[]f64,
    next_trans: *[]f64,
    next_emit: *[]f64,
    alpha_s: *[]f64,
    beta_s: *[]f64,
    scale_s: *[]f64,
): f64? {
    if (!model.validate() || !checkObsValid(obs, model.num_obs)) {
        return null;
    }
    const n: usize = model.num_states;
    const m: usize = model.num_obs;
    const t_len: usize = obs.len;
    if (next_init.*.len < n || !checkMatrixLen(next_trans.*.len, n, n) || !checkMatrixLen(next_emit.*.len, n, m)) {
        return null;
    }
    if (!checkMatrixLen(alpha_s.*.len, t_len, n) || !checkMatrixLen(beta_s.*.len, t_len, n) || scale_s.*.len < t_len) {
        return null;
    }

    const log_lik: f64 = forwardScaled(model, obs, alpha_s, scale_s) ?? return null;
    if (!backwardScaled(model, obs, scale_s.*, beta_s)) {
        return null;
    }

    // Zero accumulators
    let zi: usize = 0;
    while (zi < n) {
        next_init.*[zi] = 0.0;
        zi += 1;
    }
    const nn: usize = n * n;
    let zt: usize = 0;
    while (zt < nn) {
        next_trans.*[zt] = 0.0;
        zt += 1;
    }
    const nm: usize = n * m;
    let ze: usize = 0;
    while (ze < nm) {
        next_emit.*[ze] = 0.0;
        ze += 1;
    }

    // Accumulate expected initial, emission, and transition counts
    let t: usize = 0;
    while (t < t_len) {
        const base: usize = t * n;
        const ot: usize = obs[t];

        let g_denom: f64 = 0.0;
        let i_d: usize = 0;
        while (i_d < n) {
            g_denom += alpha_s.*[base + i_d] * beta_s.*[base + i_d];
            i_d += 1;
        }
        if (g_denom <= 0.0 || !isFiniteF64(g_denom)) {
            return null;
        }
        const inv_g: f64 = 1.0 / g_denom;

        let i_g: usize = 0;
        while (i_g < n) {
            const gamma_ti: f64 = (alpha_s.*[base + i_g] * beta_s.*[base + i_g]) * inv_g;
            if (t == 0) {
                next_init.*[i_g] = gamma_ti;
            }
            next_emit.*[i_g * m + ot] += gamma_ti;
            i_g += 1;
        }

        if (t + 1 < t_len) {
            const next_base: usize = (t + 1) * n;
            const o_next: usize = obs[t + 1];

            let xi_denom: f64 = 0.0;
            let i_x: usize = 0;
            while (i_x < n) {
                const a_ti: f64 = alpha_s.*[base + i_x];
                let j_x: usize = 0;
                while (j_x < n) {
                    xi_denom += a_ti
                        * model.trans_prob[i_x * n + j_x]
                        * model.emit_prob[j_x * m + o_next]
                        * beta_s.*[next_base + j_x];
                    j_x += 1;
                }
                i_x += 1;
            }

            if (xi_denom > 0.0 && isFiniteF64(xi_denom)) {
                const inv_xi: f64 = 1.0 / xi_denom;
                let i_a: usize = 0;
                while (i_a < n) {
                    const a_ti: f64 = alpha_s.*[base + i_a];
                    let j_a: usize = 0;
                    while (j_a < n) {
                        const xi_ij: f64 = (a_ti
                            * model.trans_prob[i_a * n + j_a]
                            * model.emit_prob[j_a * m + o_next]
                            * beta_s.*[next_base + j_a]) * inv_xi;
                        next_trans.*[i_a * n + j_a] += xi_ij;
                        j_a += 1;
                    }
                    i_a += 1;
                }
            }
        }
        t += 1;
    }

    // Normalize transition rows (fallback to previous row if state had zero expected transitions)
    let rt: usize = 0;
    while (rt < n) {
        let row_sum: f64 = 0.0;
        let ct: usize = 0;
        while (ct < n) {
            row_sum += next_trans.*[rt * n + ct];
            ct += 1;
        }
        if (row_sum > 0.0 && isFiniteF64(row_sum)) {
            const inv_r: f64 = 1.0 / row_sum;
            let cn: usize = 0;
            while (cn < n) {
                next_trans.*[rt * n + cn] *= inv_r;
                cn += 1;
            }
        } else {
            let cf: usize = 0;
            while (cf < n) {
                next_trans.*[rt * n + cf] = model.trans_prob[rt * n + cf];
                cf += 1;
            }
        }
        rt += 1;
    }

    // Normalize emission rows (fallback to previous row if state had zero expected occupancy)
    let re: usize = 0;
    while (re < n) {
        let row_sum: f64 = 0.0;
        let ce: usize = 0;
        while (ce < m) {
            row_sum += next_emit.*[re * m + ce];
            ce += 1;
        }
        if (row_sum > 0.0 && isFiniteF64(row_sum)) {
            const inv_r: f64 = 1.0 / row_sum;
            let cn: usize = 0;
            while (cn < m) {
                next_emit.*[re * m + cn] *= inv_r;
                cn += 1;
            }
        } else {
            let cf: usize = 0;
            while (cf < m) {
                next_emit.*[re * m + cf] = model.emit_prob[re * m + cf];
                cf += 1;
            }
        }
        re += 1;
    }

    return log_lik;
}

// Origin: Pure-CP Stdlib (AI Inference Sparse MoE Top-K Router, Load-Balance Loss & Dispatch) | Created: 2026-11-22
// Zero-heap-allocation Sparse Mixture-of-Experts (MoE) inference primitives modeled after
// Mixtral, Switch Transformer, Qwen3-MoE, and DeepSeek-V3:
//   - Top-K expert gating with Softmax or Sigmoid scoring + optional auxiliary-loss-free expert bias
//   - Capacity-constrained batch token-to-expert CSR bucketing with drop accounting
//   - Switch/Mixtral load-balancing auxiliary loss (L_aux = E * sum(f_e * P_e))
//   - Weighted multi-expert output combination

const std = @import("std");

/// Gating nonlinearity used by `moeRouteToken`.
/// - `Softmax`: Mixtral / Switch / Qwen3 style probability normalization over all experts.
/// - `Sigmoid`: DeepSeek-V3 style independent sigmoid affinity scoring per expert.
pub enum MoeGateKind {
    Softmax,
    Sigmoid,
}

/// Single routed expert assignment `(expert_id, weight)` for a token.
pub struct MoeRouteEntry {
    expert_id: usize = 0;
    weight: f64 = 0.0;
}

/// Summary statistics returned by `moeDispatchBatch`.
pub struct MoeDispatchStats {
    total_assignments: usize = 0;
    dispatched: usize = 0;
    dropped: usize = 0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn sigmoidF64(x: f64): f64 {
    if (x >= 0.0) {
        return 1.0 / (1.0 + std.math.exp(0.0 - x));
    }
    const e = std.math.exp(x);
    return e / (1.0 + e);
}

/// Zero-heap-allocation Top-K expert gating router for a single token.
///
/// Computes gating weights via `Softmax` or `Sigmoid` over `router_logits`.
/// When `expert_bias` is non-empty (length `>= router_logits.len`), selection uses
/// `gate_weight[e] + expert_bias[e]` (DeepSeek-V3 auxiliary-loss-free load balancing),
/// while emitted `MoeRouteEntry.weight` values retain the unbiased gating weights.
/// Ties in selection score are deterministically broken by ascending `expert_id`.
/// When `renorm_topk == true`, the selected Top-K weights are renormalized to sum to `1.0`.
///
/// Returns `false` on empty logits, `top_k == 0`, `top_k > n_experts`, undersized buffers,
/// or NaN/Inf inputs.
pub fn moeRouteToken(
    router_logits: []const f64,
    expert_bias: []const f64,
    top_k: usize,
    kind: MoeGateKind,
    renorm_topk: bool,
    out_routes: *[]MoeRouteEntry,
): bool {
    const n_experts = router_logits.len;
    if (n_experts == 0 || top_k == 0 || top_k > n_experts) return false;
    if (out_routes.len < top_k) return false;

    const use_bias = expert_bias.len > 0;
    if (use_bias && expert_bias.len < n_experts) return false;

    let max_logit: f64 = 0.0 - std.math.inf(f64);
    let i: usize = 0;
    while (i < n_experts) : (i += 1) {
        const lg = router_logits[i];
        if (!isFiniteF64(lg)) return false;
        if (use_bias && !isFiniteF64(expert_bias[i])) return false;
        if (lg > max_logit) max_logit = lg;
    }

    let softmax_sum: f64 = 0.0;
    if (kind == MoeGateKind.Softmax) {
        let j: usize = 0;
        while (j < n_experts) : (j += 1) {
            softmax_sum += std.math.exp(router_logits[j] - max_logit);
        }
        if (!(softmax_sum > 0.0) || !isFiniteF64(softmax_sum)) return false;
    }

    // In-place O(E * K) insertion into out_routes[0..top_k], descending by selection score,
    // breaking ties by ascending expert_id.
    let filled: usize = 0;
    let e: usize = 0;
    while (e < n_experts) : (e += 1) {
        const gate_w = if (kind == MoeGateKind.Softmax)
            std.math.exp(router_logits[e] - max_logit) / softmax_sum
        else
            sigmoidF64(router_logits[e]);

        const sel_score = if (use_bias) gate_w + expert_bias[e] else gate_w;
        if (!isFiniteF64(gate_w) || !isFiniteF64(sel_score)) return false;

        // Since experts are visited in strictly increasing order `e`, any tie with an
        // already-inserted entry keeps the smaller `expert_id` ahead; only strictly
        // greater `sel_score` displaces earlier entries.
        let pos: usize = filled;
        while (pos > 0) {
            const prev_entry = out_routes[pos - 1];
            const prev_sel = if (use_bias)
                prev_entry.weight + expert_bias[prev_entry.expert_id]
            else
                prev_entry.weight;
            if (sel_score > prev_sel) {
                pos -= 1;
            } else {
                break;
            }
        }

        if (pos < top_k) {
            const new_filled = if (filled < top_k) filled + 1 else top_k;
            let shift: usize = new_filled - 1;
            while (shift > pos) : (shift -= 1) {
                out_routes[shift] = out_routes[shift - 1];
            }
            out_routes[pos] = MoeRouteEntry{
                expert_id: e,
                weight: gate_w,
            };
            filled = new_filled;
        }
    }

    if (renorm_topk) {
        let w_sum: f64 = 0.0;
        let k: usize = 0;
        while (k < top_k) : (k += 1) {
            w_sum += out_routes[k].weight;
        }
        if (w_sum > 0.0 && isFiniteF64(w_sum)) {
            let k2: usize = 0;
            while (k2 < top_k) : (k2 += 1) {
                out_routes[k2].weight /= w_sum;
            }
        } else {
            const uniform = 1.0 / (top_k as f64);
            let k2: usize = 0;
            while (k2 < top_k) : (k2 += 1) {
                out_routes[k2].weight = uniform;
            }
        }
    }

    return true;
}

/// Zero-heap-allocation capacity-constrained CSR batch token-to-expert dispatcher.
///
/// Groups `[n_tokens, top_k]` routing entries in `token_routes` by `expert_id` into CSR
/// buckets `[expert_offsets_out[e], expert_offsets_out[e + 1])`. At most `expert_capacity`
/// tokens are accepted per expert in arrival order; overflow assignments are counted in
/// `MoeDispatchStats.dropped`.
///
/// Caller-provided buffers:
/// - `expert_offsets_out`: length `>= n_experts + 1`
/// - `counts_scratch`: length `>= n_experts`
/// - `expert_tokens_out` & `expert_weights_out`: length `>= dispatched` (up to `n_tokens * top_k`)
///
/// Returns `null` on invalid dimensions, out-of-range `expert_id`, non-finite weights,
/// or undersized buffers.
pub fn moeDispatchBatch(
    token_routes: []const MoeRouteEntry,
    n_tokens: usize,
    top_k: usize,
    n_experts: usize,
    expert_capacity: usize,
    expert_offsets_out: *[]usize,
    expert_tokens_out: *[]usize,
    expert_weights_out: *[]f64,
    counts_scratch: *[]usize,
): MoeDispatchStats? {
    if (n_experts == 0 || top_k == 0 || top_k > n_experts) return null;
    if (n_experts == std.math.maxInt(usize)) return null;
    if (expert_offsets_out.len < n_experts + 1) return null;
    if (counts_scratch.len < n_experts) return null;

    if (n_tokens == 0) {
        let e0: usize = 0;
        while (e0 <= n_experts) : (e0 += 1) {
            expert_offsets_out[e0] = 0;
        }
        return MoeDispatchStats{
            total_assignments: 0,
            dispatched: 0,
            dropped: 0,
        };
    }

    if (n_tokens > std.math.maxInt(usize) / top_k) return null;
    const total_assign = n_tokens * top_k;
    if (token_routes.len < total_assign) return null;

    let e_zero: usize = 0;
    while (e_zero < n_experts) : (e_zero += 1) {
        counts_scratch[e_zero] = 0;
    }

    // Pass 1: validate entries and count capacity-capped assignments per expert.
    let dispatched: usize = 0;
    let dropped: usize = 0;
    let idx: usize = 0;
    while (idx < total_assign) : (idx += 1) {
        const entry = token_routes[idx];
        if (entry.expert_id >= n_experts) return null;
        if (!isFiniteF64(entry.weight)) return null;

        if (counts_scratch[entry.expert_id] < expert_capacity) {
            counts_scratch[entry.expert_id] += 1;
            dispatched += 1;
        } else {
            dropped += 1;
        }
    }

    if (expert_tokens_out.len < dispatched || expert_weights_out.len < dispatched) return null;

    // Prefix sum for CSR offsets; reuse counts_scratch as per-expert write cursors.
    expert_offsets_out[0] = 0;
    let e_off: usize = 0;
    while (e_off < n_experts) : (e_off += 1) {
        const start = expert_offsets_out[e_off];
        expert_offsets_out[e_off + 1] = start + counts_scratch[e_off];
        counts_scratch[e_off] = start;
    }

    // Pass 2: scatter accepted (token_id, weight) pairs into CSR buckets.
    let t: usize = 0;
    while (t < n_tokens) : (t += 1) {
        let k: usize = 0;
        while (k < top_k) : (k += 1) {
            const entry = token_routes[t * top_k + k];
            const eid = entry.expert_id;
            const write_pos = counts_scratch[eid];
            if (write_pos < expert_offsets_out[eid + 1]) {
                expert_tokens_out[write_pos] = t;
                expert_weights_out[write_pos] = entry.weight;
                counts_scratch[eid] = write_pos + 1;
            }
        }
    }

    return MoeDispatchStats{
        total_assignments: total_assign,
        dispatched: dispatched,
        dropped: dropped,
    };
}

/// Computes the Switch Transformer / Mixtral load-balancing auxiliary loss:
/// `L_aux = E * sum_{e=0}^{E-1} (f_e * P_e)`
/// where:
/// - `f_e` is the fraction of routed `(token, slot)` assignments dispatched to expert `e`,
/// - `P_e` is the mean router probability assigned to expert `e` across all tokens.
///
/// Under perfectly balanced routing (`f_e = P_e = 1/E`), `L_aux == 1.0`.
/// Under total collapse onto a single expert (`f_0 = P_0 = 1.0`), `L_aux == E`.
///
/// Returns `null` on invalid dimensions, out-of-range expert indices, undersized buffers,
/// or NaN/Inf probabilities.
pub fn moeLoadBalanceLoss(
    router_probs_flat: []const f64,
    chosen_experts_flat: []const usize,
    n_tokens: usize,
    n_experts: usize,
    top_k: usize,
    frac_scratch: *[]f64,
    prob_scratch: *[]f64,
): f64? {
    if (n_tokens == 0 || n_experts == 0 || top_k == 0 || top_k > n_experts) return null;
    if (n_tokens > std.math.maxInt(usize) / n_experts) return null;
    if (n_tokens > std.math.maxInt(usize) / top_k) return null;

    const prob_elems = n_tokens * n_experts;
    const chosen_elems = n_tokens * top_k;
    if (router_probs_flat.len < prob_elems || chosen_experts_flat.len < chosen_elems) return null;
    if (frac_scratch.len < n_experts || prob_scratch.len < n_experts) return null;

    let e0: usize = 0;
    while (e0 < n_experts) : (e0 += 1) {
        frac_scratch[e0] = 0.0;
        prob_scratch[e0] = 0.0;
    }

    let t: usize = 0;
    while (t < n_tokens) : (t += 1) {
        const p_base = t * n_experts;
        let e: usize = 0;
        while (e < n_experts) : (e += 1) {
            const p = router_probs_flat[p_base + e];
            if (!isFiniteF64(p)) return null;
            prob_scratch[e] += p;
        }

        const c_base = t * top_k;
        let k: usize = 0;
        while (k < top_k) : (k += 1) {
            const eid = chosen_experts_flat[c_base + k];
            if (eid >= n_experts) return null;
            frac_scratch[eid] += 1.0;
        }
    }

    const inv_assign = 1.0 / (chosen_elems as f64);
    const inv_tokens = 1.0 / (n_tokens as f64);

    let dot: f64 = 0.0;
    let e1: usize = 0;
    while (e1 < n_experts) : (e1 += 1) {
        const f_e = frac_scratch[e1] * inv_assign;
        const p_e = prob_scratch[e1] * inv_tokens;
        frac_scratch[e1] = f_e;
        prob_scratch[e1] = p_e;
        dot += f_e * p_e;
    }

    const loss = (n_experts as f64) * dot;
    if (!isFiniteF64(loss)) return null;
    return loss;
}

/// Weighted combination of Top-K expert outputs for a single token:
/// `out_vec[d] = sum_{k=0}^{routes.len - 1} routes[k].weight * expert_outs_flat[k * hidden_dim + d]`.
///
/// Returns `false` on empty `routes`, `hidden_dim == 0`, undersized buffers, or NaN/Inf inputs.
pub fn moeCombineOutputs(
    expert_outs_flat: []const f64,
    routes: []const MoeRouteEntry,
    hidden_dim: usize,
    out_vec: *[]f64,
): bool {
    const k_len = routes.len;
    if (k_len == 0 || hidden_dim == 0) return false;
    if (k_len > std.math.maxInt(usize) / hidden_dim) return false;

    const needed = k_len * hidden_dim;
    if (expert_outs_flat.len < needed || out_vec.len < hidden_dim) return false;

    let r0: usize = 0;
    while (r0 < k_len) : (r0 += 1) {
        if (!isFiniteF64(routes[r0].weight)) return false;
    }
    let idx: usize = 0;
    while (idx < needed) : (idx += 1) {
        if (!isFiniteF64(expert_outs_flat[idx])) return false;
    }

    let d0: usize = 0;
    while (d0 < hidden_dim) : (d0 += 1) {
        out_vec[d0] = 0.0;
    }

    let r: usize = 0;
    while (r < k_len) : (r += 1) {
        const w = routes[r].weight;
        const base = r * hidden_dim;
        let d: usize = 0;
        while (d < hidden_dim) : (d += 1) {
            out_vec[d] += w * expert_outs_flat[base + d];
        }
    }

    return true;
}

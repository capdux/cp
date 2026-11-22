// Origin: Pure-CP Stdlib (AI Multi-Agent Self-Consistency, Weighted Consensus & Quorum Arbiter) | Created: 2026-11-22
// Pure CP zero-allocation multi-agent consensus and voting arbiter providing:
// - Wang et al. (2023) Self-Consistency majority & confidence-weighted voting (`resolveConsensus`)
// - Quorum threshold enforcement, tie detection, and Shannon disagreement entropy (in bits)
// - Weighted and equal-weight Borda count ranking aggregation (`resolveBordaRanking`)
// - Defensive guards against NaN/Inf, invalid confidence/weights/quorum, duplicate ranks & small buffers

/// A single agent's vote for a candidate answer.
pub struct AgentVote {
    answer_hash: u64 = 0,
    answer_index: usize = 0,
    confidence: f64 = 1.0,
    agent_weight: f64 = 1.0,
}

/// Outcome status of multi-agent consensus resolution.
pub enum ConsensusStatus {
    Reached,
    NoQuorum,
    TieUnresolved,
    Empty,
}

/// Detailed result of `resolveConsensus`.
pub struct ConsensusResult {
    status: ConsensusStatus = ConsensusStatus.Empty,
    winner_hash: u64 = 0,
    winner_index: usize = 0,
    winner_weight: f64 = 0.0,
    total_weight: f64 = 0.0,
    agreement_ratio: f64 = 0.0,
    disagreement_entropy: f64 = 0.0,
    unique_answers: usize = 0,
}

/// Result of Borda count ranking aggregation.
pub struct BordaResult {
    winner_candidate: usize = 0,
    winner_points: f64 = 0.0,
    is_tie: bool = false,
}

// ============================================================================
// Internal Numeric Helpers (Pure CP, Zero External Dependencies)
// ============================================================================

fn isFiniteF64(x: f64): bool {
    return (x - x) == 0.0;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

/// Pure-CP IEEE-754 full-precision `log2(x)` for `x > 0.0`.
/// Reduces `x` into `[1.0, 2.0)` and evaluates `ln(m)` via the rapidly converging
/// `atanh` series `2 * sum_{k=0}^{12} y^(2k+1) / (2k+1)` where `y = (m-1)/(m+1)`.
fn log2F64(x: f64): f64 {
    if (x <= 0.0 || x == 1.0) {
        return 0.0;
    }

    let m = x;
    let exp_part: f64 = 0.0;
    while (m >= 2.0) {
        m *= 0.5;
        exp_part += 1.0;
    }
    while (m < 1.0) {
        m *= 2.0;
        exp_part -= 1.0;
    }

    if (m == 1.0) {
        return exp_part;
    }

    const y = (m - 1.0) / (m + 1.0);
    const y2 = y * y;
    let term = y;
    let sum = y;
    let k: usize = 1;
    while (k <= 12) : (k += 1) {
        term *= y2;
        sum += term / ((2 * k + 1) as f64);
    }

    const ln2: f64 = 0.693147180559945309417232121458176568;
    return exp_part + (2.0 * sum) / ln2;
}

// ============================================================================
// Public Multi-Agent Consensus & Borda Ranking APIs
// ============================================================================

/// Aggregates multi-agent votes by `answer_hash` using either equal/agent weights or
/// `agent_weight * confidence` (`use_confidence_weight == true`), computes the winning
/// agreement ratio and Shannon disagreement entropy (in bits), and enforces `quorum_ratio`
/// and strict unique-winner tie detection.
/// Returns `null` on invalid `quorum_ratio` (not in `[0, 1]`), invalid vote fields
/// (`NaN`/Inf, `confidence` outside `[0, 1]`, `agent_weight <= 0`), or insufficient scratch buffers.
pub fn resolveConsensus(
    votes: []const AgentVote,
    quorum_ratio: f64,
    use_confidence_weight: bool,
    unique_hashes_s: *[]u64,
    unique_weights_s: *[]f64,
    unique_first_idx_s: *[]usize,
): ConsensusResult? {
    if (!isFiniteF64(quorum_ratio) || quorum_ratio < 0.0 || quorum_ratio > 1.0) {
        return null;
    }

    if (votes.len == 0) {
        return ConsensusResult{
            status: ConsensusStatus.Empty,
            winner_hash: 0,
            winner_index: 0,
            winner_weight: 0.0,
            total_weight: 0.0,
            agreement_ratio: 0.0,
            disagreement_entropy: 0.0,
            unique_answers: 0,
        };
    }

    let unique_count: usize = 0;
    let total_weight: f64 = 0.0;

    let i: usize = 0;
    while (i < votes.len) : (i += 1) {
        const v = votes[i];
        if (!isFiniteF64(v.confidence) || v.confidence < 0.0 || v.confidence > 1.0) {
            return null;
        }
        if (!isFiniteF64(v.agent_weight) || v.agent_weight <= 0.0) {
            return null;
        }

        const w: f64 = if (use_confidence_weight) v.agent_weight * v.confidence else v.agent_weight;

        let found: bool = false;
        let u: usize = 0;
        while (u < unique_count) : (u += 1) {
            if (unique_hashes_s[u] == v.answer_hash) {
                unique_weights_s[u] += w;
                found = true;
                break;
            }
        }

        if (!found) {
            if (unique_count >= unique_hashes_s.len ||
                unique_count >= unique_weights_s.len ||
                unique_count >= unique_first_idx_s.len) {
                return null;
            }
            unique_hashes_s[unique_count] = v.answer_hash;
            unique_weights_s[unique_count] = w;
            unique_first_idx_s[unique_count] = v.answer_index;
            unique_count += 1;
        }

        total_weight += w;
    }

    if (total_weight <= 0.0) {
        return ConsensusResult{
            status: ConsensusStatus.NoQuorum,
            winner_hash: unique_hashes_s[0],
            winner_index: unique_first_idx_s[0],
            winner_weight: 0.0,
            total_weight: 0.0,
            agreement_ratio: 0.0,
            disagreement_entropy: 0.0,
            unique_answers: unique_count,
        };
    }

    // Locate maximum weight bucket
    let best_u: usize = 0;
    let max_w: f64 = unique_weights_s[0];
    let u: usize = 1;
    while (u < unique_count) : (u += 1) {
        if (unique_weights_s[u] > max_w) {
            max_w = unique_weights_s[u];
            best_u = u;
        }
    }

    // Count top-tied buckets within floating-point tolerance
    const tie_tol: f64 = 1.0e-12 * (1.0 + max_w);
    let top_count: usize = 0;
    u = 0;
    while (u < unique_count) : (u += 1) {
        if (absF64(unique_weights_s[u] - max_w) <= tie_tol) {
            top_count += 1;
        }
    }

    // Compute Shannon disagreement entropy in bits: -sum(p * log2(p))
    let entropy: f64 = 0.0;
    if (unique_count > 1) {
        u = 0;
        while (u < unique_count) : (u += 1) {
            const p = unique_weights_s[u] / total_weight;
            if (p > 0.0) {
                entropy -= p * log2F64(p);
            }
        }
        if (entropy < 0.0) {
            entropy = 0.0;
        }
    }

    const agreement_ratio = max_w / total_weight;
    let status = ConsensusStatus.Reached;
    if (top_count > 1) {
        status = ConsensusStatus.TieUnresolved;
    } else if (agreement_ratio + 1.0e-12 < quorum_ratio) {
        status = ConsensusStatus.NoQuorum;
    }

    return ConsensusResult{
        status: status,
        winner_hash: unique_hashes_s[best_u],
        winner_index: unique_first_idx_s[best_u],
        winner_weight: max_w,
        total_weight: total_weight,
        agreement_ratio: agreement_ratio,
        disagreement_entropy: entropy,
        unique_answers: unique_count,
    };
}

/// Aggregates multi-agent candidate rankings via weighted or equal-weight Borda count.
/// `rankings_flat` is a row-major `[n_voters][n_candidates]` permutation table where
/// rank `0` receives `(n_candidates - 1) * weight` points down to `0` for last place.
/// Pass an empty `voter_weights` slice for equal weights (`1.0` per voter).
/// Writes per-candidate totals into `points_out[0..n_candidates]` and returns `BordaResult`,
/// or `null` on invalid dimensions, out-of-range/duplicate candidate IDs, invalid weights,
/// or insufficient `points_out` capacity.
pub fn resolveBordaRanking(
    rankings_flat: []const usize,
    n_voters: usize,
    n_candidates: usize,
    voter_weights: []const f64,
    points_out: *[]f64,
): BordaResult? {
    if (n_voters == 0 || n_candidates == 0) {
        return null;
    }
    if (rankings_flat.len != n_voters * n_candidates) {
        return null;
    }
    if (points_out.len < n_candidates) {
        return null;
    }
    const weighted = voter_weights.len > 0;
    if (weighted && voter_weights.len != n_voters) {
        return null;
    }

    let c: usize = 0;
    while (c < n_candidates) : (c += 1) {
        points_out[c] = 0.0;
    }

    let v: usize = 0;
    while (v < n_voters) : (v += 1) {
        let w: f64 = 1.0;
        if (weighted) {
            w = voter_weights[v];
            if (!isFiniteF64(w) || w <= 0.0) {
                return null;
            }
        }

        const row_base = v * n_candidates;
        let r: usize = 0;
        while (r < n_candidates) : (r += 1) {
            const cand = rankings_flat[row_base + r];
            if (cand >= n_candidates) {
                return null;
            }
            // Reject duplicate candidate within the same voter's ballot
            let prev: usize = 0;
            while (prev < r) : (prev += 1) {
                if (rankings_flat[row_base + prev] == cand) {
                    return null;
                }
            }
            const pts = ((n_candidates - 1 - r) as f64) * w;
            points_out[cand] += pts;
        }
    }

    let best_c: usize = 0;
    let best_pts: f64 = points_out[0];
    let c2: usize = 1;
    while (c2 < n_candidates) : (c2 += 1) {
        if (points_out[c2] > best_pts) {
            best_pts = points_out[c2];
            best_c = c2;
        }
    }

    const tie_tol: f64 = 1.0e-12 * (1.0 + best_pts);
    let top_count: usize = 0;
    let c3: usize = 0;
    while (c3 < n_candidates) : (c3 += 1) {
        if (absF64(points_out[c3] - best_pts) <= tie_tol) {
            top_count += 1;
        }
    }

    return BordaResult{
        winner_candidate: best_c,
        winner_points: best_pts,
        is_tie: top_count > 1,
    };
}

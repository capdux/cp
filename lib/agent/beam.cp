// Origin: Pure-CP Stdlib (AI Inference Beam Search: GNMT Length Penalty & No-Repeat N-Gram) | Created: 2026-11-22

import std from "std";

fn isNaN(x: f64): bool {
    return x != x;
}

/// Wu et al. (2016) Google NMT length penalty: `((5 + len) / 6)^alpha`.
pub fn gnmtLengthPenalty(len: usize, alpha: f64): f64 {
    if (alpha == 0.0 || isNaN(alpha)) {
        return 1.0;
    }
    const base = (5.0 + (len as f64)) / 6.0;
    return std.math.pow(f64, base, alpha);
}

/// Computes length-normalized beam score `log_prob_sum / gnmtLengthPenalty(len, alpha)`.
pub fn normalizedBeamScore(log_prob_sum: f64, len: usize, alpha: f64): f64 {
    const lp = gnmtLengthPenalty(len, alpha);
    if (lp <= 0.0 || isNaN(lp)) {
        return log_prob_sum;
    }
    return log_prob_sum / lp;
}

/// Zero-allocation check whether appending `candidate_token` to `history` would form an `n`-gram
/// that already appeared earlier in `history`. Returns `false` if `n == 0`.
pub fn wouldRepeatNgram(history: []const u32, candidate_token: u32, n: usize): bool {
    if (n == 0) {
        return false;
    }
    if (n == 1) {
        let i: usize = 0;
        while (i < history.len) {
            if (history[i] == candidate_token) {
                return true;
            }
            i += 1;
        }
        return false;
    }
    if (history.len < n) {
        return false;
    }

    const prefix_len = n - 1;
    const suffix_start = history.len - prefix_len;
    const max_start = history.len - n;

    let start: usize = 0;
    while (start <= max_start) {
        if (history[start + prefix_len] == candidate_token) {
            let matched: bool = true;
            let k: usize = 0;
            while (k < prefix_len) {
                if (history[start + k] != history[suffix_start + k]) {
                    matched = false;
                    break;
                }
                k += 1;
            }
            if (matched) {
                return true;
            }
        }
        start += 1;
    }
    return false;
}

/// Configuration for `BeamSearch`.
pub struct BeamConfig {
    beam_width: usize,
    max_len: usize,
    eos_token: u32,
    length_penalty_alpha: f64,
    no_repeat_ngram: usize,
}

/// Hypothesis metadata for an active or finished beam.
pub struct BeamHyp {
    score_sum: f64,
    norm_score: f64,
    len: usize,
    finished: bool,
}

/// Expansion candidate stored in caller-provided `cand_scratch` during `stepLogProbs`.
pub struct BeamCandidate {
    parent_beam: usize,
    token: u32,
    score_sum: f64,
    norm_score: f64,
}

fn candidateBetter(a: BeamCandidate, b: BeamCandidate): bool {
    if (a.norm_score != b.norm_score) {
        return a.norm_score > b.norm_score;
    }
    if (a.score_sum != b.score_sum) {
        return a.score_sum > b.score_sum;
    }
    if (a.parent_beam != b.parent_beam) {
        return a.parent_beam < b.parent_beam;
    }
    return a.token < b.token;
}

fn insertCandidateSorted(cands: *[]BeamCandidate, count: *usize, cap: usize, cand: BeamCandidate) {
    if (cap == 0) {
        return;
    }
    if (count.* == cap && !candidateBetter(cand, cands.*[cap - 1])) {
        return;
    }

    let pos: usize = count.*;
    if (pos == cap) {
        pos = cap - 1;
    } else {
        count.* += 1;
    }

    while (pos > 0 && candidateBetter(cand, cands.*[pos - 1])) {
        cands.*[pos] = cands.*[pos - 1];
        pos -= 1;
    }
    cands.*[pos] = cand;
}

/// Zero-allocation double-buffered Beam Search decoder with GNMT length penalty,
/// no-repeat N-gram blocking, and EOS early stopping.
pub struct BeamSearch {
    cfg: BeamConfig,
    tokens_a: []u32,
    tokens_b: []u32,
    hyps_a: []BeamHyp,
    hyps_b: []BeamHyp,
    finished_tokens: []u32,
    finished_hyps: []BeamHyp,
    use_b: bool,
    active_count: usize,
    finished_count: usize,
    step_index: usize,

    /// Initializes `BeamSearch` over caller-provided ping-pong and finished hypothesis buffers.
    /// Returns `null` if `beam_width == 0`, `max_len == 0`, `length_penalty_alpha` is `NaN`, or any buffer is undersized.
    static fn init(
        cfg: BeamConfig,
        tokens_a: *[]u32,
        tokens_b: *[]u32,
        hyps_a: *[]BeamHyp,
        hyps_b: *[]BeamHyp,
        finished_tokens: *[]u32,
        finished_hyps: *[]BeamHyp,
    ): BeamSearch? {
        if (cfg.beam_width == 0 || cfg.max_len == 0 || isNaN(cfg.length_penalty_alpha)) {
            return null;
        }
        const tok_need = cfg.beam_width * cfg.max_len;
        if (
            tokens_a.*.len < tok_need ||
            tokens_b.*.len < tok_need ||
            hyps_a.*.len < cfg.beam_width ||
            hyps_b.*.len < cfg.beam_width ||
            finished_tokens.*.len < tok_need ||
            finished_hyps.*.len < cfg.beam_width
        ) {
            return null;
        }

        hyps_a.*[0] = BeamHyp {
            score_sum: 0.0,
            norm_score: 0.0,
            len: 0,
            finished: false,
        };

        return BeamSearch {
            cfg: cfg,
            tokens_a: tokens_a.*,
            tokens_b: tokens_b.*,
            hyps_a: hyps_a.*,
            hyps_b: hyps_b.*,
            finished_tokens: finished_tokens.*,
            finished_hyps: finished_hyps.*,
            use_b: false,
            active_count: 1,
            finished_count: 0,
            step_index: 0,
        };
    }

    /// Returns the number of currently active (unfinished) beams.
    fn activeCount(): usize {
        return _.active_count;
    }

    /// Returns the number of completed (EOS-terminated) beams archived so far.
    fn finishedCount(): usize {
        return _.finished_count;
    }

    /// Returns the token slice generated so far for active beam `beam_idx` (empty if out of range).
    fn activeBeamTokens(beam_idx: usize): []const u32 {
        if (beam_idx >= _.active_count) {
            return _.tokens_a[0..0];
        }
        const stride = _.cfg.max_len;
        const start = beam_idx * stride;
        if (_.use_b) {
            const len = _.hyps_b[beam_idx].len;
            return _.tokens_b[start..(start + len)];
        }
        const len = _.hyps_a[beam_idx].len;
        return _.tokens_a[start..(start + len)];
    }

    fn archiveFinished(*_, parent_toks: []const u32, cand: BeamCandidate) {
        const stride = _.cfg.max_len;
        let slot: usize = 0;

        if (_.finished_count < _.cfg.beam_width) {
            slot = _.finished_count;
            _.finished_count += 1;
        } else {
            // Replace worst finished beam only if candidate is strictly better
            let worst: usize = 0;
            let i: usize = 1;
            while (i < _.finished_count) {
                if (_.finished_hyps[i].norm_score < _.finished_hyps[worst].norm_score) {
                    worst = i;
                }
                i += 1;
            }
            if (cand.norm_score <= _.finished_hyps[worst].norm_score) {
                return;
            }
            slot = worst;
        }

        const dst_off = slot * stride;
        let k: usize = 0;
        while (k < parent_toks.len) {
            _.finished_tokens[dst_off + k] = parent_toks[k];
            k += 1;
        }
        _.finished_tokens[dst_off + parent_toks.len] = cand.token;
        _.finished_hyps[slot] = BeamHyp {
            score_sum: cand.score_sum,
            norm_score: cand.norm_score,
            len: parent_toks.len + 1,
            finished: true,
        };
    }

    /// Advances beam search by one decoding step using flattened `beam_log_probs` (`activeCount() * vocab_size`).
    /// Returns `true` when decoding is complete (all beams finished, `max_len` reached, or early-stopped).
    fn stepLogProbs(
        *_,
        beam_log_probs: []const f64,
        vocab_size: usize,
        cand_scratch: *[]BeamCandidate,
    ): bool {
        if (_.active_count == 0 || _.step_index >= _.cfg.max_len) {
            return true;
        }
        if (vocab_size == 0 || beam_log_probs.len != _.active_count * vocab_size || cand_scratch.*.len == 0) {
            return true;
        }

        const want_cap = _.cfg.beam_width * 2;
        const cand_cap = if (cand_scratch.*.len < want_cap) cand_scratch.*.len else want_cap;
        let cand_count: usize = 0;

        const stride = _.cfg.max_len;
        let b: usize = 0;
        while (b < _.active_count) {
            const parent_hyp = if (_.use_b) _.hyps_b[b] else _.hyps_a[b];
            const parent_toks = _.activeBeamTokens(b);
            const row_off = b * vocab_size;

            let v: usize = 0;
            while (v < vocab_size) {
                const lp = beam_log_probs[row_off + v];
                if (!isNaN(lp)) {
                    const tok = v as u32;
                    let blocked: bool = false;
                    if (_.cfg.no_repeat_ngram > 0 && tok != _.cfg.eos_token) {
                        blocked = wouldRepeatNgram(parent_toks, tok, _.cfg.no_repeat_ngram);
                    }
                    if (!blocked) {
                        const new_sum = parent_hyp.score_sum + lp;
                        const new_len = parent_hyp.len + 1;
                        const new_norm = normalizedBeamScore(new_sum, new_len, _.cfg.length_penalty_alpha);
                        insertCandidateSorted(cand_scratch, &cand_count, cand_cap, BeamCandidate {
                            parent_beam: b,
                            token: tok,
                            score_sum: new_sum,
                            norm_score: new_norm,
                        });
                    }
                }
                v += 1;
            }
            b += 1;
        }

        let next_active: usize = 0;
        let c: usize = 0;
        while (c < cand_count) {
            const cand = cand_scratch.*[c];
            const parent_toks = _.activeBeamTokens(cand.parent_beam);
            if (cand.token == _.cfg.eos_token) {
                _.archiveFinished(parent_toks, cand);
            } else if (next_active < _.cfg.beam_width) {
                const dst_off = next_active * stride;
                if (_.use_b) {
                    let k: usize = 0;
                    while (k < parent_toks.len) {
                        _.tokens_a[dst_off + k] = parent_toks[k];
                        k += 1;
                    }
                    _.tokens_a[dst_off + parent_toks.len] = cand.token;
                    _.hyps_a[next_active] = BeamHyp {
                        score_sum: cand.score_sum,
                        norm_score: cand.norm_score,
                        len: parent_toks.len + 1,
                        finished: false,
                    };
                } else {
                    let k: usize = 0;
                    while (k < parent_toks.len) {
                        _.tokens_b[dst_off + k] = parent_toks[k];
                        k += 1;
                    }
                    _.tokens_b[dst_off + parent_toks.len] = cand.token;
                    _.hyps_b[next_active] = BeamHyp {
                        score_sum: cand.score_sum,
                        norm_score: cand.norm_score,
                        len: parent_toks.len + 1,
                        finished: false,
                    };
                }
                next_active += 1;
            }
            c += 1;
        }

        _.use_b = !_.use_b;
        _.active_count = next_active;
        _.step_index += 1;

        return _.active_count == 0 ||
            _.step_index >= _.cfg.max_len ||
            _.finished_count >= _.cfg.beam_width;
    }

    /// Copies the highest-scoring hypothesis tokens into `out_tokens` (preferring `finished` beams,
    /// falling back to the best active beam if none finished) and returns its `BeamHyp`.
    fn bestSequence(out_tokens: *[]u32): BeamHyp? {
        const stride = _.cfg.max_len;

        if (_.finished_count > 0) {
            let best: usize = 0;
            let i: usize = 1;
            while (i < _.finished_count) {
                if (_.finished_hyps[i].norm_score > _.finished_hyps[best].norm_score) {
                    best = i;
                }
                i += 1;
            }
            const hyp = _.finished_hyps[best];
            if (out_tokens.*.len < hyp.len) {
                return null;
            }
            const src_off = best * stride;
            let k: usize = 0;
            while (k < hyp.len) {
                out_tokens.*[k] = _.finished_tokens[src_off + k];
                k += 1;
            }
            return hyp;
        }

        if (_.active_count == 0) {
            return null;
        }

        let best: usize = 0;
        let i: usize = 1;
        while (i < _.active_count) {
            const hi = if (_.use_b) _.hyps_b[i] else _.hyps_a[i];
            const hb = if (_.use_b) _.hyps_b[best] else _.hyps_a[best];
            if (hi.norm_score > hb.norm_score) {
                best = i;
            }
            i += 1;
        }

        const hyp = if (_.use_b) _.hyps_b[best] else _.hyps_a[best];
        if (out_tokens.*.len < hyp.len) {
            return null;
        }
        const toks = _.activeBeamTokens(best);
        let k: usize = 0;
        while (k < hyp.len) {
            out_tokens.*[k] = toks[k];
            k += 1;
        }
        return hyp;
    }
}

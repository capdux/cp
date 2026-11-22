// Origin: Pure-CP Stdlib (AI Inference Zero-Alloc Beam Search & Diverse Beam Decoding Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation Beam Search & Diverse Beam Search (Vijayakumar et al. 2016) decoder:
//   - Fixed-capacity stack state for beam_width 1..16, max_seq_len 1..128, and up to 8 diversity groups
//   - Google NMT length normalization ((5 + len) / 6)^alpha & exact no_repeat_ngram_size blocking
//   - Inter-group Hamming diversity penalty for multi-group Diverse Beam Decoding
//   - Step-wise expansion from pre-filtered Top-K log-prob candidates or raw dense vocab logits
//   - Automatic EOS hypothesis archiving, early_stopping convergence & top-N hypothesis extraction

const std = @import("std");

// ============================================================================
// 1. Constants & Internal Numeric / Ordering Helpers
// ============================================================================

/// Maximum supported beam width (zero dynamic heap allocation).
pub const BEAM_MAX_WIDTH: usize = 16;

/// Maximum supported token sequence length per hypothesis.
pub const BEAM_MAX_SEQ_LEN: usize = 128;

/// Maximum supported Diverse Beam Search groups.
pub const BEAM_MAX_GROUPS: usize = 8;

/// Maximum completed hypotheses retained in the finished pool.
pub const BEAM_MAX_FINISHED: usize = 32;

/// Internal top-K cap per active beam during dense-logits expansion or group selection.
const BEAM_GROUP_CAND_CAP: usize = 32;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Internal candidate expansion record used during a single decoding step.
struct BeamExpandCand {
    parent_idx: usize = 0;
    token_id: u32 = 0;
    raw_log_prob: f32 = 0.0;
    rank_score: f32 = 0.0;
    valid: bool = false;
}

/// Deterministic descending ordering for expansion candidates:
/// `(rank_score desc, raw_log_prob desc, token_id asc, parent_idx asc)`.
fn expandCandBetter(a: BeamExpandCand, b: BeamExpandCand): bool {
    if (a.rank_score > b.rank_score) return true;
    if (a.rank_score < b.rank_score) return false;
    if (a.raw_log_prob > b.raw_log_prob) return true;
    if (a.raw_log_prob < b.raw_log_prob) return false;
    if (a.token_id < b.token_id) return true;
    if (a.token_id > b.token_id) return false;
    return a.parent_idx < b.parent_idx;
}

/// Deterministic descending ordering for hypotheses:
/// `(normalized_score desc, log_prob desc, len asc, group_id asc)`.
fn hypBetter(a: BeamHypothesis, b: BeamHypothesis): bool {
    if (a.normalized_score > b.normalized_score) return true;
    if (a.normalized_score < b.normalized_score) return false;
    if (a.log_prob > b.log_prob) return true;
    if (a.log_prob < b.log_prob) return false;
    if (a.len < b.len) return true;
    if (a.len > b.len) return false;
    return a.group_id < b.group_id;
}

/// Inserts an expansion candidate into a descending sorted fixed buffer of capacity `cap`.
fn insertExpandCandSorted(
    buf: *[]BeamExpandCand,
    count: *usize,
    cap: usize,
    cand: BeamExpandCand,
) {
    if (cap == 0) return;
    let cur_n: usize = count.*;
    if (cur_n >= cap && !expandCandBetter(cand, buf[cap - 1])) {
        return;
    }

    let pos: usize = if (cur_n < cap) cur_n else (cap - 1);
    while (pos > 0 && expandCandBetter(cand, buf[pos - 1])) {
        buf[pos] = buf[pos - 1];
        pos -= 1;
    }
    buf[pos] = cand;
    if (cur_n < cap) {
        count.* = cur_n + 1;
    }
}

/// Inserts a `BeamCandidate` into a descending sorted top-K buffer by `(log_prob desc, token_id asc)`.
fn insertTopKCand(
    buf: *[]BeamCandidate,
    count: *usize,
    cap: usize,
    cand: BeamCandidate,
) {
    if (cap == 0) return;
    let cur_n: usize = count.*;
    if (cur_n >= cap) {
        const tail = buf[cap - 1];
        if (cand.log_prob < tail.log_prob || (cand.log_prob == tail.log_prob && cand.token_id >= tail.token_id)) {
            return;
        }
    }

    let pos: usize = if (cur_n < cap) cur_n else (cap - 1);
    while (pos > 0) {
        const prev = buf[pos - 1];
        const better: bool = (cand.log_prob > prev.log_prob) ||
            (cand.log_prob == prev.log_prob && cand.token_id < prev.token_id);
        if (!better) break;
        buf[pos] = prev;
        pos -= 1;
    }
    buf[pos] = cand;
    if (cur_n < cap) {
        count.* = cur_n + 1;
    }
}

// ============================================================================
// 2. Public Scoring & N-gram Blocking Utilities
// ============================================================================

/// Computes Google NMT length penalty: `((5 + seq_len) / 6)^alpha`.
/// Returns `1.0` when `alpha == 0.0` or `alpha` is non-finite.
pub fn beamLengthPenalty(seq_len: usize, alpha: f32): f32 {
    if (!isFiniteF32(alpha) || alpha == 0.0) return 1.0;
    const base: f64 = (5.0 + (seq_len as f64)) / 6.0;
    const p: f64 = std.math.pow(f64, base, alpha as f64);
    if (!std.math.isFinite(p) || p <= 0.0) return 1.0;
    return p as f32;
}

/// Computes length-normalized score `log_prob / beamLengthPenalty(seq_len, alpha)`.
pub fn beamNormalizedScore(log_prob: f32, seq_len: usize, alpha: f32): f32 {
    const lp: f32 = beamLengthPenalty(seq_len, alpha);
    return log_prob / lp;
}

/// Returns `true` if appending `next_token` to `tokens` would create a repeated
/// contiguous N-gram of length `ngram_size` that already appears earlier in `tokens`.
/// - `ngram_size == 0`: repetition blocking disabled (returns `false`).
/// - `ngram_size == 1`: returns `true` iff `next_token` already exists anywhere in `tokens`.
/// - `ngram_size >= 2`: checks whether `[tokens[len-(N-1)..len], next_token]` already occurred.
pub fn beamHasRepeatedNgram(
    tokens: []const u32,
    next_token: u32,
    ngram_size: usize,
): bool {
    if (ngram_size == 0) return false;
    const n_len: usize = tokens.len;

    if (ngram_size == 1) {
        let i: usize = 0;
        while (i < n_len) : (i += 1) {
            if (tokens[i] == next_token) return true;
        }
        return false;
    }

    if (n_len < ngram_size) return false;
    const prefix_len: usize = ngram_size - 1;
    const tail_start: usize = n_len - prefix_len;
    const max_start: usize = n_len - ngram_size;

    let start: usize = 0;
    while (start <= max_start) : (start += 1) {
        if (tokens[start + prefix_len] != next_token) continue;
        let matched: bool = true;
        let k: usize = 0;
        while (k < prefix_len) : (k += 1) {
            if (tokens[start + k] != tokens[tail_start + k]) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }

    return false;
}

// ============================================================================
// 3. Public Data Models
// ============================================================================

/// Pre-filtered candidate token with log-probability (`<= 0.0` typically) for a beam step.
pub struct BeamCandidate {
    token_id: u32 = 0;
    log_prob: f32 = 0.0;
}

/// Configuration for Beam Search and Diverse Beam Search decoding.
pub struct BeamSearchConfig {
    beam_width: usize = 4;
    max_seq_len: usize = 64;
    num_groups: usize = 1;
    diversity_penalty: f32 = 0.0;
    length_penalty_alpha: f32 = 0.6;
    no_repeat_ngram_size: usize = 0;
    early_stopping: bool = true;
    eos_token_id: u32 = 2;
    min_len: usize = 0;

    /// Returns production defaults: `beam_width=4`, `max_seq_len=64`, `num_groups=1`,
    /// `length_penalty_alpha=0.6`, `early_stopping=true`, `eos_token_id=2`.
    static fn defaults(): BeamSearchConfig {
        return BeamSearchConfig{
            beam_width: 4,
            max_seq_len: 64,
            num_groups: 1,
            diversity_penalty: 0.0,
            length_penalty_alpha: 0.6,
            no_repeat_ngram_size: 0,
            early_stopping: true,
            eos_token_id: 2,
            min_len: 0,
        };
    }

    /// Validates all structural, grouping, and numeric invariants.
    fn isValid(): bool {
        if (_.beam_width == 0 || _.beam_width > BEAM_MAX_WIDTH) return false;
        if (_.max_seq_len == 0 || _.max_seq_len > BEAM_MAX_SEQ_LEN) return false;
        if (_.num_groups == 0 || _.num_groups > BEAM_MAX_GROUPS) return false;
        if (_.num_groups > _.beam_width) return false;
        if (_.beam_width % _.num_groups != 0) return false;
        if (!isFiniteF32(_.diversity_penalty) || _.diversity_penalty < 0.0) return false;
        if (!isFiniteF32(_.length_penalty_alpha) || _.length_penalty_alpha < 0.0) return false;
        if (_.no_repeat_ngram_size > BEAM_MAX_SEQ_LEN) return false;
        if (_.min_len > _.max_seq_len) return false;
        return true;
    }
}

/// Fixed-capacity token sequence hypothesis maintained by `BeamSearchDecoder`.
pub struct BeamHypothesis {
    tokens: [128]u32 = [0; 128];
    len: usize = 0;
    log_prob: f32 = 0.0;
    normalized_score: f32 = 0.0;
    group_id: u16 = 0;
    finished: bool = false;

    /// Returns an empty zero-initialized hypothesis.
    static fn empty(): BeamHypothesis {
        return BeamHypothesis{};
    }

    /// Returns a read-only slice of the valid generated/prompt tokens `tokens[0..len]`.
    fn tokenSlice(): []const u32 {
        return _.tokens[0.._.len];
    }
}

/// Summary returned after executing a single decoding step.
pub struct BeamStepOutcome {
    step_index: usize = 0;
    active_beams: usize = 0;
    newly_finished: usize = 0;
    total_finished: usize = 0;
    blocked_ngrams: usize = 0;
    stopped: bool = false;
}

/// Diagnostic telemetry snapshot for a `BeamSearchDecoder` session.
pub struct BeamSearchStats {
    steps: usize = 0;
    active_beams: usize = 0;
    finished_count: usize = 0;
    blocked_ngram_count: usize = 0;
    diversity_penalties_applied: usize = 0;
    early_stopped: bool = false;
    reached_max_len: bool = false;
}

// ============================================================================
// 4. Zero-Alloc Beam Search & Diverse Beam Search Decoder
// ============================================================================

/// Zero-heap-allocation Beam Search & Diverse Beam Search decoding engine.
pub struct BeamSearchDecoder {
    cfg: BeamSearchConfig = BeamSearchConfig{},
    beams: [16]BeamHypothesis = [BeamHypothesis{}; 16],
    active_count: usize = 0,
    finished: [32]BeamHypothesis = [BeamHypothesis{}; 32],
    finished_count: usize = 0,
    step_count: usize = 0,
    blocked_ngram_count: usize = 0,
    diversity_penalties_applied: usize = 0,
    early_stopped: bool = false,
    reached_max_len: bool = false;

    /// Initializes a decoder from `cfg` and optional `prompt_tokens`.
    /// Returns `null` if `cfg` is invalid or `prompt_tokens.len > cfg.max_seq_len`.
    static fn init(cfg: BeamSearchConfig, prompt_tokens: []const u32): BeamSearchDecoder? {
        if (!cfg.isValid()) return null;
        if (prompt_tokens.len > cfg.max_seq_len) return null;

        let decoder = BeamSearchDecoder{
            cfg: cfg,
        };
        if (!decoder.reset(prompt_tokens)) return null;
        return decoder;
    }

    /// Resets decoder state to a fresh prompt while retaining `cfg`.
    /// Initializes one seed beam per diversity group (`active_count = cfg.num_groups`).
    fn reset(*_, prompt_tokens: []const u32): bool {
        if (!_.cfg.isValid()) return false;
        if (prompt_tokens.len > _.cfg.max_seq_len) return false;

        _.active_count = 0;
        _.finished_count = 0;
        _.step_count = 0;
        _.blocked_ngram_count = 0;
        _.diversity_penalties_applied = 0;
        _.early_stopped = false;
        _.reached_max_len = (prompt_tokens.len >= _.cfg.max_seq_len);

        let g: usize = 0;
        while (g < _.cfg.num_groups) : (g += 1) {
            let hyp = BeamHypothesis{};
            let i: usize = 0;
            while (i < prompt_tokens.len) : (i += 1) {
                hyp.tokens[i] = prompt_tokens[i];
            }
            hyp.len = prompt_tokens.len;
            hyp.log_prob = 0.0;
            hyp.normalized_score = 0.0;
            hyp.group_id = g as u16;
            hyp.finished = false;
            _.beams[g] = hyp;
        }
        _.active_count = _.cfg.num_groups;
        return true;
    }

    /// Returns the number of currently active beams.
    fn activeCount(): usize {
        return _.active_count;
    }

    /// Returns a copy of active beam `idx`, or `null` if out of range.
    fn activeBeam(idx: usize): BeamHypothesis? {
        if (idx >= _.active_count) return null;
        return _.beams[idx];
    }

    /// Returns `true` when decoding has terminated via early stopping, max length, or beam exhaustion.
    fn isDone(): bool {
        return _.early_stopped || _.reached_max_len || _.active_count == 0;
    }

    /// Returns a telemetry snapshot of decoding progress and counters.
    fn stats(): BeamSearchStats {
        return BeamSearchStats{
            steps: _.step_count,
            active_beams: _.active_count,
            finished_count: _.finished_count,
            blocked_ngram_count: _.blocked_ngram_count,
            diversity_penalties_applied: _.diversity_penalties_applied,
            early_stopped: _.early_stopped,
            reached_max_len: _.reached_max_len,
        };
    }

    /// Internal helper: inserts a completed hypothesis into `finished` sorted by `hypBetter`.
    fn archiveFinished(*_, hyp: BeamHypothesis) {
        let cur_n: usize = _.finished_count;
        if (cur_n >= BEAM_MAX_FINISHED && !hypBetter(hyp, _.finished[BEAM_MAX_FINISHED - 1])) {
            return;
        }
        let pos: usize = if (cur_n < BEAM_MAX_FINISHED) cur_n else (BEAM_MAX_FINISHED - 1);
        while (pos > 0 && hypBetter(hyp, _.finished[pos - 1])) {
            _.finished[pos] = _.finished[pos - 1];
            pos -= 1;
        }
        _.finished[pos] = hyp;
        if (cur_n < BEAM_MAX_FINISHED) {
            _.finished_count = cur_n + 1;
        }
    }

    /// Advances beam search by one token using per-beam pre-filtered candidates.
    ///
    /// `beam_candidates` is a row-major slice of length `>= activeCount() * candidates_per_beam`,
    /// where row `b` supplies candidate `(token_id, log_prob)` pairs for active beam `b`.
    /// Candidates matching `eos_token_id` are archived into the finished hypothesis pool.
    ///
    /// Returns `null` if `candidates_per_beam == 0` or `beam_candidates` is too short.
    fn step(
        *_,
        beam_candidates: []const BeamCandidate,
        candidates_per_beam: usize,
        eos_token_id: u32,
    ): BeamStepOutcome? {
        if (candidates_per_beam == 0) return null;
        if (_.active_count > 0) {
            if (_.active_count > std.math.maxInt(usize) / candidates_per_beam) return null;
            if (beam_candidates.len < _.active_count * candidates_per_beam) return null;
        }

        if (_.isDone()) {
            return BeamStepOutcome{
                step_index: _.step_count,
                active_beams: _.active_count,
                newly_finished: 0,
                total_finished: _.finished_count,
                blocked_ngrams: 0,
                stopped: true,
            };
        }

        const num_groups: usize = _.cfg.num_groups;
        const group_size: usize = _.cfg.beam_width / num_groups;
        const alpha: f32 = _.cfg.length_penalty_alpha;
        const div_pen: f32 = _.cfg.diversity_penalty;
        const ngram_sz: usize = _.cfg.no_repeat_ngram_size;

        let next_beams: [16]BeamHypothesis = [BeamHypothesis{}; 16];
        let next_active_count: usize = 0;

        // Tokens selected by earlier groups (0..g-1) at this step for Hamming diversity penalty.
        let prev_group_tokens: [16]u32 = [0; 16];
        let prev_group_token_count: usize = 0;

        let step_newly_finished: usize = 0;
        let step_blocked_ngrams: usize = 0;

        let g: usize = 0;
        while (g < num_groups) : (g += 1) {
            const cand_cap: usize = if (group_size * 2 <= BEAM_GROUP_CAND_CAP) (group_size * 2) else BEAM_GROUP_CAND_CAP;
            let group_cands: [32]BeamExpandCand = [BeamExpandCand{}; 32];
            let group_cands_s: []BeamExpandCand = group_cands[0..cand_cap];
            let group_cand_count: usize = 0;

            let b: usize = 0;
            while (b < _.active_count) : (b += 1) {
                if ((_.beams[b].group_id as usize) != g) continue;
                const parent_len: usize = _.beams[b].len;
                if (parent_len >= _.cfg.max_seq_len) continue;

                const row_base: usize = b * candidates_per_beam;
                let c: usize = 0;
                while (c < candidates_per_beam) : (c += 1) {
                    const cand = beam_candidates[row_base + c];
                    if (!isFiniteF32(cand.log_prob)) continue;

                    const is_eos: bool = (cand.token_id == eos_token_id);
                    if (is_eos && (parent_len + 1) < _.cfg.min_len) {
                        continue;
                    }
                    if (!is_eos && ngram_sz > 0) {
                        if (beamHasRepeatedNgram(_.beams[b].tokens[0..parent_len], cand.token_id, ngram_sz)) {
                            step_blocked_ngrams += 1;
                            continue;
                        }
                    }

                    let hamming_hits: usize = 0;
                    if (g > 0 && div_pen > 0.0) {
                        let p: usize = 0;
                        while (p < prev_group_token_count) : (p += 1) {
                            if (prev_group_tokens[p] == cand.token_id) {
                                hamming_hits += 1;
                            }
                        }
                        if (hamming_hits > 0) {
                            _.diversity_penalties_applied += 1;
                        }
                    }

                    const raw_lp: f32 = _.beams[b].log_prob + cand.log_prob;
                    const rank_sc: f32 = raw_lp - div_pen * (hamming_hits as f32);
                    const rec = BeamExpandCand{
                        parent_idx: b,
                        token_id: cand.token_id,
                        raw_log_prob: raw_lp,
                        rank_score: rank_sc,
                        valid: true,
                    };
                    insertExpandCandSorted(*group_cands_s, *group_cand_count, cand_cap, rec);
                }
            }

            // Select up to `group_size` active continuations and archive EOS candidates ranked in top `group_size`.
            let group_selected_active: usize = 0;
            let k: usize = 0;
            while (k < group_cand_count && group_selected_active < group_size) : (k += 1) {
                const chosen = group_cands[k];
                if (!chosen.valid) continue;

                let child = _.beams[chosen.parent_idx];
                const new_len: usize = child.len + 1;
                child.tokens[child.len] = chosen.token_id;
                child.len = new_len;
                child.log_prob = chosen.raw_log_prob;
                child.normalized_score = beamNormalizedScore(chosen.raw_log_prob, new_len, alpha);
                child.group_id = g as u16;

                if (chosen.token_id == eos_token_id) {
                    if (k < group_size) {
                        child.finished = true;
                        _.archiveFinished(child);
                        step_newly_finished += 1;
                    }
                    continue;
                }

                child.finished = false;
                if (next_active_count < BEAM_MAX_WIDTH) {
                    next_beams[next_active_count] = child;
                    next_active_count += 1;
                    group_selected_active += 1;
                    if (prev_group_token_count < BEAM_MAX_WIDTH) {
                        prev_group_tokens[prev_group_token_count] = chosen.token_id;
                        prev_group_token_count += 1;
                    }
                }
            }
        }

        // Commit next active beams.
        let i: usize = 0;
        while (i < next_active_count) : (i += 1) {
            _.beams[i] = next_beams[i];
        }
        _.active_count = next_active_count;
        _.step_count += 1;
        _.blocked_ngram_count += step_blocked_ngrams;

        // Check max sequence length termination across all active beams.
        if (_.active_count > 0) {
            let all_at_max: bool = true;
            let m: usize = 0;
            while (m < _.active_count) : (m += 1) {
                if (_.beams[m].len < _.cfg.max_seq_len) {
                    all_at_max = false;
                    break;
                }
            }
            if (all_at_max) {
                _.reached_max_len = true;
            }
        }

        // Early stopping & optimistic upper-bound convergence check.
        if (_.active_count == 0) {
            _.early_stopped = true;
        } else if (_.cfg.early_stopping && _.finished_count >= _.cfg.beam_width) {
            _.early_stopped = true;
        } else if (!_.cfg.early_stopping && _.finished_count >= _.cfg.beam_width) {
            // Non-early-stopping heuristic bound: stop only if the beam_width-th finished hypothesis
            // strictly beats the optimistic upper bound of the best active beam at max_seq_len.
            let best_active_lp: f32 = _.beams[0].log_prob;
            let a_idx: usize = 1;
            while (a_idx < _.active_count) : (a_idx += 1) {
                if (_.beams[a_idx].log_prob > best_active_lp) {
                    best_active_lp = _.beams[a_idx].log_prob;
                }
            }
            const max_pen: f32 = beamLengthPenalty(_.cfg.max_seq_len, alpha);
            const optimistic_bound: f32 = if (best_active_lp <= 0.0) (best_active_lp / max_pen) else best_active_lp;
            const worst_top_finished: f32 = _.finished[_.cfg.beam_width - 1].normalized_score;
            if (worst_top_finished >= optimistic_bound) {
                _.early_stopped = true;
            }
        }

        return BeamStepOutcome{
            step_index: _.step_count,
            active_beams: _.active_count,
            newly_finished: step_newly_finished,
            total_finished: _.finished_count,
            blocked_ngrams: step_blocked_ngrams,
            stopped: _.isDone(),
        };
    }

    /// Convenience step entry taking raw dense logits `[activeCount(), vocab_size]` (row-major).
    /// Applies numerically stable Log-Softmax per active beam, extracts Top-K candidates on stack
    /// (zero heap allocation), and delegates to `step`.
    fn stepFromLogits(
        *_,
        beam_logits: []const f32,
        vocab_size: usize,
        eos_token_id: u32,
    ): BeamStepOutcome? {
        if (vocab_size == 0) return null;
        if (_.active_count > 0) {
            if (_.active_count > std.math.maxInt(usize) / vocab_size) return null;
            if (beam_logits.len < _.active_count * vocab_size) return null;
        }
        if (_.isDone()) {
            return BeamStepOutcome{
                step_index: _.step_count,
                active_beams: _.active_count,
                newly_finished: 0,
                total_finished: _.finished_count,
                blocked_ngrams: 0,
                stopped: true,
            };
        }

        const top_k: usize = if (vocab_size < BEAM_GROUP_CAND_CAP) vocab_size else BEAM_GROUP_CAND_CAP;
        // Max stack buffer: 16 beams * 32 candidates = 512 BeamCandidate (~4 KB).
        let cands: [512]BeamCandidate = [BeamCandidate{}; 512];

        let b: usize = 0;
        while (b < _.active_count) : (b += 1) {
            const row_start: usize = b * vocab_size;
            let max_logit: f64 = 0.0 - std.math.inf(f64);
            let has_finite: bool = false;
            let v: usize = 0;
            while (v < vocab_size) : (v += 1) {
                const x: f32 = beam_logits[row_start + v];
                if (!isFiniteF32(x)) continue;
                const xf: f64 = x as f64;
                if (!has_finite || xf > max_logit) {
                    max_logit = xf;
                    has_finite = true;
                }
            }

            let out_row: []BeamCandidate = cands[b * top_k .. (b + 1) * top_k];
            let k_count: usize = 0;

            if (!has_finite) {
                let z: usize = 0;
                while (z < top_k) : (z += 1) {
                    out_row[z] = BeamCandidate{
                        token_id: 0,
                        log_prob: std.math.nan(f32),
                    };
                }
                continue;
            }

            let sum_exp: f64 = 0.0;
            v = 0;
            while (v < vocab_size) : (v += 1) {
                const x: f32 = beam_logits[row_start + v];
                if (!isFiniteF32(x)) continue;
                sum_exp += std.math.exp((x as f64) - max_logit);
            }
            const lse: f64 = max_logit + std.math.log(f64, std.math.e, sum_exp);

            v = 0;
            while (v < vocab_size) : (v += 1) {
                const x: f32 = beam_logits[row_start + v];
                if (!isFiniteF32(x)) continue;
                const lp: f32 = ((x as f64) - lse) as f32;
                insertTopKCand(*out_row, *k_count, top_k, BeamCandidate{
                    token_id: v as u32,
                    log_prob: lp,
                });
            }
            while (k_count < top_k) : (k_count += 1) {
                out_row[k_count] = BeamCandidate{
                    token_id: 0,
                    log_prob: std.math.nan(f32),
                };
            }
        }

        return _.step(cands[0 .. _.active_count * top_k], top_k, eos_token_id);
    }

    /// Returns the highest-scoring hypothesis (prefers completed hypotheses; falls back to
    /// the best active beam if none have emitted EOS yet). Returns `null` if empty.
    fn bestHypothesis(): BeamHypothesis? {
        if (_.finished_count > 0) {
            return _.finished[0];
        }
        if (_.active_count == 0) return null;

        let best_idx: usize = 0;
        let i: usize = 1;
        while (i < _.active_count) : (i += 1) {
            if (hypBetter(_.beams[i], _.beams[best_idx])) {
                best_idx = i;
            }
        }
        return _.beams[best_idx];
    }

    /// Copies up to `out_hyps.len` top-ranked hypotheses into `out_hyps`.
    /// Prioritizes completed hypotheses (sorted by `normalized_score` descending),
    /// then fills any remaining slots with active beams sorted by `normalized_score` descending.
    /// Returns the number of hypotheses written.
    fn topHypotheses(out_hyps: *[]BeamHypothesis): usize {
        if (out_hyps.len == 0) return 0;

        let written: usize = 0;
        let i: usize = 0;
        while (i < _.finished_count && written < out_hyps.len) : (i += 1) {
            out_hyps[written] = _.finished[i];
            written += 1;
        }

        if (written < out_hyps.len && _.active_count > 0) {
            let sorted_active: [16]BeamHypothesis = [BeamHypothesis{}; 16];
            let k: usize = 0;
            while (k < _.active_count) : (k += 1) {
                const hyp = _.beams[k];
                let pos: usize = k;
                while (pos > 0 && hypBetter(hyp, sorted_active[pos - 1])) {
                    sorted_active[pos] = sorted_active[pos - 1];
                    pos -= 1;
                }
                sorted_active[pos] = hyp;
            }
            let a: usize = 0;
            while (a < _.active_count && written < out_hyps.len) : (a += 1) {
                out_hyps[written] = sorted_active[a];
                written += 1;
            }
        }

        return written;
    }

    /// Copies up to `out_hyps.len` completed (EOS-terminated) hypotheses sorted by
    /// `normalized_score` descending. Returns the number of hypotheses written.
    fn finishedHypotheses(out_hyps: *[]BeamHypothesis): usize {
        if (out_hyps.len == 0) return 0;
        const n: usize = if (_.finished_count < out_hyps.len) _.finished_count else out_hyps.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out_hyps[i] = _.finished[i];
        }
        return n;
    }
}

// Origin: Pure-CP Stdlib (LLM Prompt Boundary Token Healing & Byte-Prefix Logits Masking Engine) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation Prompt Boundary Token Healing engine (Guidance / llama.cpp style):
//   - Compact byte-pool vocabulary table (HealVocabTable) with zero-copy slice lookup
//   - Prompt suffix backtracking planner (single-token & multi-token boundary healing)
//   - Byte-prefix candidate bitset builder (HealCandidateMask) with hardware @popCount
//   - Strict superset matching (eliminates BPE boundary artifacts like "http:" -> "http://")
//   - Multi-step prefix healing state machine (TokenHealingSession) with automatic mask release
//   - In-place -Inf logits masking (applyHealingLogitsMaskInPlace) with full boundary guards

const std = @import("std");

// ============================================================================
// Constants
// ============================================================================

/// Maximum number of vocabulary tokens supported by `HealVocabTable` & `HealCandidateMask`.
pub const HEAL_MAX_VOCAB_TOKENS: usize = 512;

/// Number of 64-bit bitset words covering `HEAL_MAX_VOCAB_TOKENS` (`512 / 64 = 8`).
pub const HEAL_MASK_WORDS: usize = 8;

/// Fixed byte pool capacity (in bytes) inside `HealVocabTable`.
pub const HEAL_MAX_BYTE_POOL: usize = 8192;

/// Maximum byte length of a prompt boundary prefix extracted for token healing.
pub const HEAL_MAX_PREFIX_BYTES: usize = 64;

/// Maximum number of trailing prompt tokens that may be backtracked in a single heal plan.
pub const HEAL_MAX_BACKTRACK_TOKENS: usize = 8;

// ============================================================================
// Internal Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn bytesStartsWith(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (haystack[i] != prefix[i]) return false;
    }
    return true;
}

fn tokenMatchesPrefix(tok_bytes: []const u8, prefix: []const u8, mode: HealMatchMode): bool {
    if (tok_bytes.len == 0 || prefix.len == 0) return false;
    if (tok_bytes.len >= prefix.len) {
        return bytesStartsWith(tok_bytes, prefix);
    }
    if (mode == HealMatchMode.MultiStepPrefix) {
        return bytesStartsWith(prefix, tok_bytes);
    }
    return false;
}

fn tokenIsStrictExtension(tok_bytes: []const u8, prefix: []const u8): bool {
    if (prefix.len == 0 || tok_bytes.len <= prefix.len) return false;
    return bytesStartsWith(tok_bytes, prefix);
}

// ============================================================================
// 1. Enums & Configuration
// ============================================================================

/// Boundary matching strategy for candidate token filtering during token healing.
pub enum HealMatchMode: u8 {
    /// Strict superset: candidate token bytes must start with the full `heal_prefix`.
    /// Eliminates BPE split artifacts (e.g. `"http:"` -> `"http://"`).
    StrictSuperset = 0,
    /// Multi-step prefix: allows both full supersets and non-empty strict prefixes of `heal_prefix`.
    /// Enables multi-step healing when a single token only covers part of `heal_prefix`.
    MultiStepPrefix = 1,
}

/// Lifecycle state of `TokenHealingSession`.
pub enum HealSessionState: u8 {
    /// No healing was needed (degraded safely or empty prompt).
    Inactive = 0,
    /// Actively constraining logits until the remaining `heal_prefix` is fully covered.
    Active = 1,
    /// Boundary prefix has been fully covered; logits masking is released.
    Completed = 2,
    /// An incompatible or unknown token was fed while active.
    Failed = 3,
}

/// Configuration for prompt boundary token healing planning.
pub struct TokenHealConfig {
    /// Maximum number of trailing prompt tokens to backtrack (`1..=HEAL_MAX_BACKTRACK_TOKENS`).
    max_backtrack_tokens: usize = 1;
    /// Matching strategy for candidate bitset construction & session advancement.
    mode: HealMatchMode = HealMatchMode.StrictSuperset;
    /// When `true`, only trigger healing if at least one candidate is strictly longer than `heal_prefix`.
    require_strict_extension: bool = false;

    /// Default single-token strict-superset healing configuration.
    static fn defaults(): TokenHealConfig {
        return TokenHealConfig{
            max_backtrack_tokens: 1,
            mode: HealMatchMode.StrictSuperset,
            require_strict_extension: false,
        };
    }

    /// Multi-step healing configuration with configurable trailing token backtrack depth.
    static fn multiStep(max_backtrack: usize): TokenHealConfig {
        return TokenHealConfig{
            max_backtrack_tokens: max_backtrack,
            mode: HealMatchMode.MultiStepPrefix,
            require_strict_extension: false,
        };
    }
}

// ============================================================================
// 2. HealVocabTable (Compact Zero-Alloc Vocabulary Byte Pool)
// ============================================================================

/// Zero-heap-allocation compact vocabulary byte pool mapping `token_id -> []const u8`.
pub struct HealVocabTable {
    bytes_pool: [HEAL_MAX_BYTE_POOL]u8 = undefined;
    pool_used: usize = 0;
    offsets: [HEAL_MAX_VOCAB_TOKENS]u32 = undefined;
    lengths: [HEAL_MAX_VOCAB_TOKENS]u16 = undefined;
    present: [HEAL_MAX_VOCAB_TOKENS]bool = [false; HEAL_MAX_VOCAB_TOKENS];
    vocab_size: usize = 0;

    /// Creates an empty vocabulary byte pool.
    static fn init(): HealVocabTable {
        return HealVocabTable{};
    }

    /// Appends a new token at `token_id = vocab_size` and returns the assigned ID.
    /// Returns `null` if the token table or byte pool is full or `token_bytes.len > 65535`.
    fn addToken(*_, token_bytes: []const u8): u32? {
        if (_.vocab_size >= HEAL_MAX_VOCAB_TOKENS) return null;
        const id: u32 = _.vocab_size as u32;
        if (!_.setToken(id, token_bytes)) return null;
        return id;
    }

    /// Registers or replaces `token_id` with `token_bytes` in the compact byte pool.
    /// Returns `false` if `token_id >= HEAL_MAX_VOCAB_TOKENS`, `token_bytes.len > 65535`,
    /// or the remaining byte pool capacity is insufficient.
    fn setToken(*_, token_id: u32, token_bytes: []const u8): bool {
        const idx: usize = token_id as usize;
        if (idx >= HEAL_MAX_VOCAB_TOKENS) return false;
        if (token_bytes.len > 65535) return false;
        if (_.pool_used + token_bytes.len > HEAL_MAX_BYTE_POOL) return false;

        const off: usize = _.pool_used;
        if (token_bytes.len > 0) {
            @memcpy(_.bytes_pool[off..off + token_bytes.len], token_bytes);
        }
        _.pool_used += token_bytes.len;
        _.offsets[idx] = off as u32;
        _.lengths[idx] = token_bytes.len as u16;
        _.present[idx] = true;
        if (idx + 1 > _.vocab_size) {
            _.vocab_size = idx + 1;
        }
        return true;
    }

    /// Zero-copy lookup of raw bytes for `token_id`. Returns `null` if unknown or out of bounds.
    fn tokenBytes(token_id: u32): ([]const u8)? {
        const idx: usize = token_id as usize;
        if (idx >= _.vocab_size || idx >= HEAL_MAX_VOCAB_TOKENS) return null;
        if (!_.present[idx]) return null;
        const off: usize = _.offsets[idx] as usize;
        const len: usize = _.lengths[idx] as usize;
        if (off + len > _.pool_used) return null;
        return _.bytes_pool[off .. off + len];
    }

    /// Returns `true` iff `token_id` is registered in this vocabulary table.
    fn isValidToken(token_id: u32): bool {
        const idx: usize = token_id as usize;
        if (idx >= _.vocab_size || idx >= HEAL_MAX_VOCAB_TOKENS) return false;
        return _.present[idx];
    }

    /// Returns the logical vocabulary size (highest registered `token_id + 1`).
    fn vocabSize(): usize {
        return _.vocab_size;
    }

    /// Returns the total bytes currently occupied in the internal byte pool.
    fn poolUsed(): usize {
        return _.pool_used;
    }
}

// ============================================================================
// 3. HealCandidateMask (Fixed-Capacity Bitset with Hardware @popCount)
// ============================================================================

/// Zero-heap-allocation candidate token bitset over `0..vocab_size` backed by `[8]u64`.
pub struct HealCandidateMask {
    words: [HEAL_MASK_WORDS]u64 = [0; HEAL_MASK_WORDS];
    vocab_size: usize = 0;

    /// Creates an all-zero (all-forbidden) candidate mask for `vocab_size` tokens.
    static fn empty(vocab_size: usize): HealCandidateMask {
        const clamped: usize = if (vocab_size > HEAL_MAX_VOCAB_TOKENS) HEAL_MAX_VOCAB_TOKENS else vocab_size;
        return HealCandidateMask{
            words: [0; HEAL_MASK_WORDS],
            vocab_size: clamped,
        };
    }

    /// Creates an all-allowed candidate mask over `0..vocab_size` (trailing bits strictly cleared).
    static fn allowAll(vocab_size: usize): HealCandidateMask {
        let m = HealCandidateMask.empty(vocab_size);
        let i: usize = 0;
        while (i < m.vocab_size) : (i += 1) {
            * = m.allowToken(i);
        }
        return m;
    }

    /// Clears all candidate bits.
    fn clearAll(*_) {
        let w: usize = 0;
        while (w < HEAL_MASK_WORDS) : (w += 1) {
            _.words[w] = 0;
        }
    }

    /// Marks `token_id` as allowed. Returns `false` if `token_id >= vocab_size`.
    fn allowToken(*_, token_id: usize): bool {
        if (token_id >= _.vocab_size) return false;
        const bit: u6 = @intCast(token_id & 63);
        _.words[token_id >> 6] = _.words[token_id >> 6] | ((1 as u64) << bit);
        return true;
    }

    /// Marks `token_id` as forbidden. Returns `false` if `token_id >= vocab_size`.
    fn forbidToken(*_, token_id: usize): bool {
        if (token_id >= _.vocab_size) return false;
        const bit: u6 = @intCast(token_id & 63);
        _.words[token_id >> 6] = _.words[token_id >> 6] & ~((1 as u64) << bit);
        return true;
    }

    /// Returns `true` iff `token_id < vocab_size` and its bit is set.
    fn isAllowed(token_id: usize): bool {
        if (token_id >= _.vocab_size) return false;
        const bit: u6 = @intCast(token_id & 63);
        return (_.words[token_id >> 6] & ((1 as u64) << bit)) != 0;
    }

    /// Counts the exact number of allowed candidate tokens using hardware `@popCount`.
    fn allowedCount(): usize {
        let total: usize = 0;
        let w: usize = 0;
        while (w < HEAL_MASK_WORDS) : (w += 1) {
            total += @popCount(_.words[w]) as usize;
        }
        return total;
    }

    /// Returns the smallest allowed `token_id`, or `null` if no token is allowed.
    fn firstAllowedToken(): u32? {
        let w: usize = 0;
        while (w < HEAL_MASK_WORDS) : (w += 1) {
            const word: u64 = _.words[w];
            if (word != 0) {
                let b: usize = 0;
                while (b < 64) : (b += 1) {
                    const shift: u6 = @intCast(b);
                    if ((word & ((1 as u64) << shift)) != 0) {
                        const tid: usize = (w << 6) + b;
                        if (tid < _.vocab_size) return tid as u32;
                    }
                }
            }
        }
        return null;
    }
}

/// Builds a `HealCandidateMask` for all registered tokens in `vocab` matching `prefix` under `mode`.
pub fn buildHealingCandidateMask(
    vocab: HealVocabTable,
    prefix: []const u8,
    mode: HealMatchMode,
): HealCandidateMask {
    let mask = HealCandidateMask.empty(vocab.vocab_size);
    if (prefix.len == 0 || vocab.vocab_size == 0) return mask;

    let id: usize = 0;
    while (id < vocab.vocab_size) : (id += 1) {
        if (tok_bytes := vocab.tokenBytes(id as u32)) {
            if (tokenMatchesPrefix(tok_bytes, prefix, mode)) {
                * = mask.allowToken(id);
            }
        }
    }
    return mask;
}

/// Applies `mask` in-place to `logits`, setting every forbidden (or non-finite / out-of-vocab)
/// entry to `-inf`.
///
/// Guards:
/// - Returns `null` without mutating `logits` if `logits.len == 0`, `mask.vocab_size == 0`,
///   or `logits.len < mask.vocab_size`.
/// - Returns the count of finite allowed tokens retained (`0` if all candidates were masked/non-finite).
pub fn applyHealingLogitsMaskInPlace(
    logits: *[]f32,
    mask: HealCandidateMask,
): usize? {
    if (logits.len == 0 || mask.vocab_size == 0) return null;
    if (logits.len < mask.vocab_size) return null;

    const neg_inf: f32 = (0.0 as f32) - std.math.inf(f32);
    let kept: usize = 0;
    let i: usize = 0;
    while (i < logits.len) : (i += 1) {
        if (i < mask.vocab_size && mask.isAllowed(i) && isFiniteF32(logits[i])) {
            kept += 1;
        } else {
            logits[i] = neg_inf;
        }
    }
    return kept;
}

// ============================================================================
// 4. TokenHealPlan & Prompt Suffix Backtracking Planner
// ============================================================================

/// Result of analyzing a prompt suffix for boundary token healing.
pub struct TokenHealPlan {
    /// `true` when healing is unnecessary or safely degraded (empty prompt, unknown token, no match).
    no_heal_needed: bool = true;
    /// Prefix token length to feed into the model after backtracking trailing boundary tokens.
    trimmed_prompt_len: usize = 0;
    /// Number of trailing tokens removed from the prompt (`prompt_tokens.len - trimmed_prompt_len`).
    backtrack_count: usize = 0;
    /// Concatenated raw byte prefix of the backtracked boundary tokens.
    heal_prefix: [HEAL_MAX_PREFIX_BYTES]u8 = undefined;
    /// Valid byte length inside `heal_prefix`.
    heal_prefix_len: usize = 0;
    /// Number of matching candidate tokens in `vocab` for `heal_prefix`.
    candidate_count: usize = 0;
    /// Number of candidate tokens strictly longer than `heal_prefix_len` that start with `heal_prefix`.
    strict_extension_count: usize = 0;

    /// Returns a zero-copy slice of the extracted boundary byte prefix.
    fn prefixSlice(): []const u8 {
        return _.heal_prefix[0.._.heal_prefix_len];
    }
}

/// Plans prompt boundary token healing using the default single-token strict-superset strategy.
/// Safely degrades to `no_heal_needed = true` on empty prompt, unknown token, or zero matches.
pub fn planTokenHealing(
    prompt_tokens: []const u32,
    vocab: HealVocabTable,
): TokenHealPlan {
    return planTokenHealingWithConfig(prompt_tokens, vocab, TokenHealConfig.defaults());
}

/// Plans prompt boundary token healing with configurable backtrack depth and matching strategy.
///
/// Evaluates backtracking `k = 1..min(prompt_tokens.len, max_backtrack_tokens, HEAL_MAX_BACKTRACK_TOKENS)`
/// trailing tokens and selects the deepest valid backtrack whose concatenated byte prefix yields
/// eligible candidates (and strictly longer extensions when `cfg.require_strict_extension` is set).
/// If no backtrack depth produces eligible candidates, degrades safely to `no_heal_needed = true`.
pub fn planTokenHealingWithConfig(
    prompt_tokens: []const u32,
    vocab: HealVocabTable,
    cfg: TokenHealConfig,
): TokenHealPlan {
    let best = TokenHealPlan{
        no_heal_needed: true,
        trimmed_prompt_len: prompt_tokens.len,
        backtrack_count: 0,
        heal_prefix: undefined,
        heal_prefix_len: 0,
        candidate_count: 0,
        strict_extension_count: 0,
    };

    if (prompt_tokens.len == 0 || vocab.vocab_size == 0 || cfg.max_backtrack_tokens == 0) {
        return best;
    }

    let max_k: usize = prompt_tokens.len;
    if (cfg.max_backtrack_tokens < max_k) max_k = cfg.max_backtrack_tokens;
    if (HEAL_MAX_BACKTRACK_TOKENS < max_k) max_k = HEAL_MAX_BACKTRACK_TOKENS;

    let k: usize = 1;
    while (k <= max_k) : (k += 1) {
        const start_idx: usize = prompt_tokens.len - k;
        let cand_buf: [HEAL_MAX_PREFIX_BYTES]u8 = undefined;
        let cand_len: usize = 0;
        let valid_seq: bool = true;

        let t: usize = start_idx;
        while (t < prompt_tokens.len) : (t += 1) {
            if (tb := vocab.tokenBytes(prompt_tokens[t])) {
                if (tb.len == 0 || cand_len + tb.len > HEAL_MAX_PREFIX_BYTES) {
                    valid_seq = false;
                    break;
                }
                @memcpy(cand_buf[cand_len..cand_len + tb.len], tb);
                cand_len += tb.len;
            } else {
                valid_seq = false;
                break;
            }
        }

        if (!valid_seq || cand_len == 0) {
            // An unknown/empty/overflowing token prevents deeper backtracking.
            break;
        }

        const cand_slice: []const u8 = cand_buf[0..cand_len];
        const mask = buildHealingCandidateMask(vocab, cand_slice, cfg.mode);
        const cand_cnt: usize = mask.allowedCount();

        let ext_cnt: usize = 0;
        let vid: usize = 0;
        while (vid < vocab.vocab_size) : (vid += 1) {
            if (tb := vocab.tokenBytes(vid as u32)) {
                if (tokenIsStrictExtension(tb, cand_slice)) {
                    ext_cnt += 1;
                }
            }
        }

        const eligible: bool = if (cfg.require_strict_extension) (ext_cnt > 0) else (cand_cnt > 0);
        if (eligible) {
            best.no_heal_needed = false;
            best.trimmed_prompt_len = start_idx;
            best.backtrack_count = k;
            best.heal_prefix_len = cand_len;
            best.candidate_count = cand_cnt;
            best.strict_extension_count = ext_cnt;
            @memcpy(best.heal_prefix[0..cand_len], cand_buf[0..cand_len]);
        }
    }

    return best;
}

// ============================================================================
// 5. TokenHealingSession (Multi-Step Continuous Healing State Machine)
// ============================================================================

/// Outcome returned by `TokenHealingSession.advance(...)` after sampling a token.
pub struct HealStepOutcome {
    /// `true` if `emitted_token` is compatible with the remaining boundary prefix.
    accepted: bool = false;
    /// `true` once the entire `heal_prefix` has been covered and logits masking is released.
    completed: bool = false;
    /// Number of bytes from `remaining_prefix` consumed by this step.
    consumed_prefix_bytes: usize = 0;
    /// Number of prefix bytes still remaining to be covered after this step.
    remaining_prefix_len: usize = 0;
    /// Number of newly generated bytes extending past `remaining_prefix` on the completing step.
    extra_healed_bytes: usize = 0;
}

/// Stateful multi-step token healing session.
///
/// Tracks progressive byte consumption across one or more generated tokens until the full
/// `heal_prefix` has been covered, then automatically transitions to `Completed` and releases
/// logits masking for subsequent unconstrained decoding steps.
pub struct TokenHealingSession {
    plan: TokenHealPlan = TokenHealPlan{},
    mode: HealMatchMode = HealMatchMode.StrictSuperset,
    state: HealSessionState = HealSessionState.Inactive,
    consumed_bytes: usize = 0,
    steps_taken: usize = 0,

    /// Plans healing for `prompt_tokens` and initializes a `TokenHealingSession`.
    static fn init(
        prompt_tokens: []const u32,
        vocab: HealVocabTable,
        cfg: TokenHealConfig,
    ): TokenHealingSession {
        const plan = planTokenHealingWithConfig(prompt_tokens, vocab, cfg);
        return TokenHealingSession.fromPlan(plan, cfg.mode);
    }

    /// Constructs a `TokenHealingSession` directly from a precomputed `TokenHealPlan`.
    static fn fromPlan(plan: TokenHealPlan, mode: HealMatchMode): TokenHealingSession {
        const init_state: HealSessionState = if (plan.no_heal_needed || plan.heal_prefix_len == 0)
            HealSessionState.Inactive
        else
            HealSessionState.Active;
        return TokenHealingSession{
            plan: plan,
            mode: mode,
            state: init_state,
            consumed_bytes: 0,
            steps_taken: 0,
        };
    }

    /// Returns `true` while boundary healing constraints are actively enforced.
    fn isActive(): bool {
        return _.state == HealSessionState.Active;
    }

    /// Returns `true` once healing has completed and masking is released.
    fn isCompleted(): bool {
        return _.state == HealSessionState.Completed;
    }

    /// Returns the prompt length to prefill before starting healed generation.
    fn trimmedPromptLen(): usize {
        return _.plan.trimmed_prompt_len;
    }

    /// Returns the remaining uncovered boundary byte prefix (`""` once inactive/completed).
    fn remainingPrefix(): []const u8 {
        if (!_.isActive()) return _.plan.heal_prefix[0..0];
        if (_.consumed_bytes >= _.plan.heal_prefix_len) return _.plan.heal_prefix[0..0];
        return _.plan.heal_prefix[_.consumed_bytes.._.plan.heal_prefix_len];
    }

    /// Builds the candidate token bitset for the current decoding step.
    /// When the session is inactive or completed, returns an all-allowed mask (unconstrained).
    fn buildStepMask(vocab: HealVocabTable): HealCandidateMask {
        if (!_.isActive()) {
            return HealCandidateMask.allowAll(vocab.vocab_size);
        }
        return buildHealingCandidateMask(vocab, _.remainingPrefix(), _.mode);
    }

    /// Applies the current healing mask in-place to `logits`.
    ///
    /// - When inactive/completed (mask released): leaves `logits` untouched and returns `logits.len`
    ///   (or `null` if `logits.len < vocab.vocab_size`).
    /// - When active: masks non-matching logits to `-inf` and returns the kept candidate count.
    ///   If zero candidates match the remaining prefix, safely leaves `logits` untouched and returns `0`.
    fn applyStepLogitsMaskInPlace(vocab: HealVocabTable, logits: *[]f32): usize? {
        if (logits.len == 0) return null;
        if (vocab.vocab_size > 0 && logits.len < vocab.vocab_size) return null;

        if (!_.isActive()) {
            return logits.len;
        }

        const mask = buildHealingCandidateMask(vocab, _.remainingPrefix(), _.mode);
        if (mask.allowedCount() == 0) {
            return 0;
        }
        return applyHealingLogitsMaskInPlace(logits, mask);
    }

    /// Advances the healing state machine by observing `emitted_token`.
    ///
    /// - If `emitted_token` covers the entire `remainingPrefix()` (exact or superset),
    ///   marks the session `Completed` and releases masking.
    /// - In `MultiStepPrefix` mode, if `emitted_token` is a strict prefix of `remainingPrefix()`,
    ///   advances `consumed_bytes` and keeps the session `Active` for the next step.
    /// - Otherwise marks the session `Failed` and returns `accepted = false`.
    fn advance(*_, emitted_token: u32, vocab: HealVocabTable): HealStepOutcome {
        if (!_.isActive()) {
            return HealStepOutcome{
                accepted: true,
                completed: true,
                consumed_prefix_bytes: 0,
                remaining_prefix_len: 0,
                extra_healed_bytes: 0,
            };
        }

        const rem_start: usize = _.consumed_bytes;
        const rem_end: usize = _.plan.heal_prefix_len;
        const rem_len: usize = rem_end - rem_start;
        const rem_slice: []const u8 = _.plan.heal_prefix[rem_start..rem_end];

        if (tok_bytes := vocab.tokenBytes(emitted_token)) {
            if (tok_bytes.len == 0) {
                _.state = HealSessionState.Failed;
                return HealStepOutcome{
                    accepted: false,
                    completed: false,
                    consumed_prefix_bytes: 0,
                    remaining_prefix_len: rem_len,
                    extra_healed_bytes: 0,
                };
            }

            // Full coverage (exact or strict superset): completes healing and releases mask.
            if (tok_bytes.len >= rem_len && bytesStartsWith(tok_bytes, rem_slice)) {
                _.consumed_bytes = rem_end;
                _.steps_taken += 1;
                _.state = HealSessionState.Completed;
                return HealStepOutcome{
                    accepted: true,
                    completed: true,
                    consumed_prefix_bytes: rem_len,
                    remaining_prefix_len: 0,
                    extra_healed_bytes: tok_bytes.len - rem_len,
                };
            }

            // Partial prefix coverage: allowed only in MultiStepPrefix mode.
            if (_.mode == HealMatchMode.MultiStepPrefix &&
                tok_bytes.len < rem_len &&
                bytesStartsWith(rem_slice, tok_bytes))
            {
                _.consumed_bytes += tok_bytes.len;
                _.steps_taken += 1;
                return HealStepOutcome{
                    accepted: true,
                    completed: false,
                    consumed_prefix_bytes: tok_bytes.len,
                    remaining_prefix_len: rem_end - _.consumed_bytes,
                    extra_healed_bytes: 0,
                };
            }
        }

        _.state = HealSessionState.Failed;
        return HealStepOutcome{
            accepted: false,
            completed: false,
            consumed_prefix_bytes: 0,
            remaining_prefix_len: rem_len,
            extra_healed_bytes: 0,
        };
    }
}

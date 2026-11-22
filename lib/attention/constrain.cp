// Origin: Pure-CP Stdlib (AI Structured Constrained Decoding: Vocab Bitset Mask & DFA Logit Filter) | Created: 2026-11-22
// Pure CP zero-heap-allocation structured constrained decoding primitives (xgrammar / Outlines style):
// vocab bitset masking with hardware @popCount & tail-bit hygiene, -inf logit masking, DFA state
// guided vocabulary filtering, and zero-alloc temperature scaling + deterministic Top-K masking.

const std = @import("std");

const TOP_K_HEAP_CAP: usize = 512;

// ============================================================================
// Internal Numeric, Bitset & Top-K Min-Heap Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn vocabWordsForSize(vocab_size: usize): usize {
    if (vocab_size == 0) return 0;
    return ((vocab_size - 1) >> 6) + 1;
}

/// Strict total order on `(logit_val desc, token_id asc)`: returns `true` if `a` ranks ahead of `b`.
fn logitBetter(a_val: f32, a_id: usize, b_val: f32, b_id: usize): bool {
    if (a_val > b_val) return true;
    if (a_val < b_val) return false;
    return a_id < b_id;
}

/// Min-heap sift-up (root = worst kept element, i.e. smallest `val` / largest `id`).
fn topKHeapSiftUp(vals: *[]f32, ids: *[]usize, idx_in: usize) {
    let idx: usize = idx_in;
    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (logitBetter(vals[parent], ids[parent], vals[idx], ids[idx])) {
            const tv: f32 = vals[parent];
            const ti: usize = ids[parent];
            vals[parent] = vals[idx];
            ids[parent] = ids[idx];
            vals[idx] = tv;
            ids[idx] = ti;
            idx = parent;
        } else {
            break;
        }
    }
}

/// Min-heap sift-down (root = worst kept element).
fn topKHeapSiftDown(vals: *[]f32, ids: *[]usize, count: usize) {
    let root: usize = 0;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= count) break;
        let worst: usize = root;
        if (logitBetter(vals[worst], ids[worst], vals[left], ids[left])) {
            worst = left;
        }
        const right: usize = left + 1;
        if (right < count && logitBetter(vals[worst], ids[worst], vals[right], ids[right])) {
            worst = right;
        }
        if (worst == root) break;
        const tv: f32 = vals[root];
        const ti: usize = ids[root];
        vals[root] = vals[worst];
        ids[root] = ids[worst];
        vals[worst] = tv;
        ids[worst] = ti;
        root = worst;
    }
}

// ============================================================================
// 1. VocabBitset (Caller-Owned Zero-Alloc Vocabulary Mask)
// ============================================================================

/// Zero-heap-allocation bitset over a vocabulary of `vocab_size` tokens backed by caller-owned `u64` words.
/// Always keeps trailing unused bits above `vocab_size - 1` cleared so `allowedCount()` is exact.
pub struct VocabBitset {
    words: []u64;
    vocab_size: usize = 0;

    /// Returns the minimum number of `u64` words required to cover `vocab_size` tokens.
    static fn wordsNeeded(vocab_size: usize): usize {
        return vocabWordsForSize(vocab_size);
    }

    /// Initializes a cleared `VocabBitset` over caller-owned `words` for `vocab_size` tokens.
    /// Returns `null` if `vocab_size == 0` or `words.len < wordsNeeded(vocab_size)`.
    static fn init(words: *[]u64, vocab_size: usize): VocabBitset? {
        if (vocab_size == 0) return null;
        const n_words: usize = vocabWordsForSize(vocab_size);
        if (words.len < n_words) return null;

        let w: usize = 0;
        while (w < n_words) : (w += 1) {
            words[w] = 0;
        }
        return VocabBitset{
            words: words[0..n_words],
            vocab_size: vocab_size,
        };
    }

    /// Clears unused high bits beyond `_.vocab_size - 1` in the final word.
    fn clearTailBits(*_) {
        const rem: usize = _.vocab_size & 63;
        if (rem != 0) {
            const shift: u6 = @intCast(rem);
            const mask: u64 = ((1 as u64) << shift) - (1 as u64);
            const last_idx: usize = _.words.len - 1;
            _.words[last_idx] = _.words[last_idx] & mask;
        }
    }

    /// Marks all `vocab_size` tokens as allowed (with tail bits strictly cleared).
    fn allowAll(*_) {
        const all_ones: u64 = std.math.maxInt(u64);
        let w: usize = 0;
        while (w < _.words.len) : (w += 1) {
            _.words[w] = all_ones;
        }
        _.clearTailBits();
    }

    /// Marks all tokens as forbidden.
    fn clearAll(*_) {
        let w: usize = 0;
        while (w < _.words.len) : (w += 1) {
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

    /// Returns the exact count of allowed tokens using hardware `@popCount`.
    fn allowedCount(): usize {
        let total: usize = 0;
        let w: usize = 0;
        while (w < _.words.len) : (w += 1) {
            total += @popCount(_.words[w]) as usize;
        }
        return total;
    }

    /// Intersects this bitset in-place with `other_words` (`self &= other`), clearing tail bits.
    /// Returns `false` if `other_words.len < _.words.len`.
    fn intersectWith(*_, other_words: []const u64): bool {
        if (other_words.len < _.words.len) return false;
        let w: usize = 0;
        while (w < _.words.len) : (w += 1) {
            _.words[w] = _.words[w] & other_words[w];
        }
        _.clearTailBits();
        return true;
    }
}

// ============================================================================
// 2. Logit Masking & Zero-Alloc Temperature + Top-K Filtering
// ============================================================================

/// Applies a vocabulary bitset mask in-place to `logits`: every token whose bit in `mask_words`
/// is `0` (or whose logit is `NaN`) is set to `-inf`.
/// Returns the number of finite allowed tokens retained (`0` when all tokens are forbidden so
/// the caller can safely trigger a fallback), or `null` on empty `logits` or undersized `mask_words`.
pub fn applyVocabMaskInPlace(
    logits: *[]f32,
    mask_words: []const u64,
): usize? {
    if (logits.len == 0) return null;
    const words_needed: usize = vocabWordsForSize(logits.len);
    if (mask_words.len < words_needed) return null;

    const neg_inf: f32 = (0.0 as f32) - std.math.inf(f32);
    let kept: usize = 0;
    let i: usize = 0;
    while (i < logits.len) : (i += 1) {
        const bit: u6 = @intCast(i & 63);
        const allowed: bool = (mask_words[i >> 6] & ((1 as u64) << bit)) != 0;
        if (allowed && isFiniteF32(logits[i])) {
            kept += 1;
        } else {
            logits[i] = neg_inf;
        }
    }
    return kept;
}

/// Applies temperature scaling (`logit /= temperature`) and retains only the top `top_k` finite
/// logits (tie-broken by smaller token index), masking all remaining logits to `-inf`.
///
/// - `top_k == 0` disables Top-K truncation (applies temperature scaling only).
/// - Returns the number of retained finite logits (`0` if all inputs were already `-inf`/`NaN`),
///   or `null` on empty `logits` or invalid `temperature <= 0.0` / non-finite.
pub fn applyTemperatureAndTopKMaskInPlace(
    logits: *[]f32,
    temperature: f32,
    top_k: usize,
): usize? {
    if (logits.len == 0) return null;
    if (!isFiniteF32(temperature) || temperature <= 0.0) return null;

    const neg_inf: f32 = (0.0 as f32) - std.math.inf(f32);
    const inv_temp: f32 = 1.0 / temperature;

    // Step 1: Temperature scale finite logits and count active candidates.
    let active_count: usize = 0;
    let i: usize = 0;
    while (i < logits.len) : (i += 1) {
        const v: f32 = logits[i];
        if (isFiniteF32(v)) {
            const scaled: f32 = v * inv_temp;
            if (isFiniteF32(scaled)) {
                logits[i] = scaled;
                active_count += 1;
            } else {
                logits[i] = neg_inf;
            }
        } else {
            logits[i] = neg_inf;
        }
    }

    if (active_count == 0 || top_k == 0 || top_k >= active_count) {
        return active_count;
    }

    // Step 2: Zero-alloc Top-K min-heap to find the exact K-th cutoff `(cutoff_val, cutoff_id)`.
    if (top_k > TOP_K_HEAP_CAP) {
        return null;
    }

    let heap_vals: [TOP_K_HEAP_CAP]f32 = [0.0; TOP_K_HEAP_CAP];
    let heap_ids: [TOP_K_HEAP_CAP]usize = [0; TOP_K_HEAP_CAP];
    let vals_s: []f32 = heap_vals[0..top_k];
    let ids_s: []usize = heap_ids[0..top_k];
    let count: usize = 0;

    let j: usize = 0;
    while (j < logits.len) : (j += 1) {
        const v: f32 = logits[j];
        if (!isFiniteF32(v)) continue;
        if (count < top_k) {
            vals_s[count] = v;
            ids_s[count] = j;
            topKHeapSiftUp(vals_s, ids_s, count);
            count += 1;
        } else if (logitBetter(v, j, vals_s[0], ids_s[0])) {
            vals_s[0] = v;
            ids_s[0] = j;
            topKHeapSiftDown(vals_s, ids_s, count);
        }
    }

    const cutoff_val: f32 = vals_s[0];
    const cutoff_id: usize = ids_s[0];

    // Step 3: Mask all tokens strictly worse than `(cutoff_val, cutoff_id)` to `-inf`.
    let kept: usize = 0;
    let m: usize = 0;
    while (m < logits.len) : (m += 1) {
        const v: f32 = logits[m];
        if (isFiniteF32(v) && (v > cutoff_val || (v == cutoff_val && m <= cutoff_id))) {
            kept += 1;
        } else {
            logits[m] = neg_inf;
        }
    }

    return kept;
}

// ============================================================================
// 3. ConstrainedDfa (Deterministic Finite Automaton State-Guided Masking)
// ============================================================================

/// Directed state transition edge in a constrained decoding DFA.
pub struct DfaEdge {
    from_state: u16 = 0;
    to_state: u16 = 0;
    token_id: u32 = 0;
}

/// Zero-heap-allocation DFA over a fixed transition table and accepting-state bitset
/// for zero-retry structured generation (e.g., JSON grammar / tool-call schemas).
pub struct ConstrainedDfa {
    edges: []const DfaEdge;
    accepting_bits: []const u64;
    num_states: u16 = 0;
    vocab_size: usize = 0;

    /// Initializes a `ConstrainedDfa`, validating all state indices and token IDs upfront.
    /// Returns `null` if `num_states == 0`, `vocab_size == 0`, `accepting_bits` is undersized,
    /// or any edge references an out-of-range state or token ID.
    static fn init(
        edges: []const DfaEdge,
        accepting_bits: []const u64,
        num_states: u16,
        vocab_size: usize,
    ): ConstrainedDfa? {
        if (num_states == 0 || vocab_size == 0) return null;
        if (accepting_bits.len < vocabWordsForSize(num_states as usize)) return null;

        let i: usize = 0;
        while (i < edges.len) : (i += 1) {
            const e = edges[i];
            if (e.from_state >= num_states || e.to_state >= num_states) return null;
            if ((e.token_id as usize) >= vocab_size) return null;
        }

        return ConstrainedDfa{
            edges: edges,
            accepting_bits: accepting_bits,
            num_states: num_states,
            vocab_size: vocab_size,
        };
    }

    /// Populates `out_mask_words` with the bitmask of all tokens allowed to transition out of `state`.
    /// Clears `out_mask_words` first and returns the count of unique allowed tokens (`0` on invalid state
    /// or undersized `out_mask_words`).
    fn buildAllowedMask(state: u16, out_mask_words: *[]u64): usize {
        const words_needed: usize = vocabWordsForSize(_.vocab_size);
        if (out_mask_words.len < words_needed) return 0;

        let w: usize = 0;
        while (w < words_needed) : (w += 1) {
            out_mask_words[w] = 0;
        }
        if (state >= _.num_states) return 0;

        let count: usize = 0;
        let i: usize = 0;
        while (i < _.edges.len) : (i += 1) {
            const e = _.edges[i];
            if (e.from_state == state) {
                const tid: usize = e.token_id as usize;
                const word_idx: usize = tid >> 6;
                const bit: u6 = @intCast(tid & 63);
                const mask: u64 = (1 as u64) << bit;
                if ((out_mask_words[word_idx] & mask) == 0) {
                    out_mask_words[word_idx] = out_mask_words[word_idx] | mask;
                    count += 1;
                }
            }
        }
        return count;
    }

    /// Advances the DFA from `state` on `token_id`. Returns the next state or `null` if invalid.
    fn step(state: u16, token_id: u32): u16? {
        if (state >= _.num_states || (token_id as usize) >= _.vocab_size) return null;
        let i: usize = 0;
        while (i < _.edges.len) : (i += 1) {
            const e = _.edges[i];
            if (e.from_state == state && e.token_id == token_id) {
                return e.to_state;
            }
        }
        return null;
    }

    /// Returns `true` iff `state < num_states` and its accepting bit is set.
    fn isAccepting(state: u16): bool {
        if (state >= _.num_states) return false;
        const s: usize = state as usize;
        const bit: u6 = @intCast(s & 63);
        return (_.accepting_bits[s >> 6] & ((1 as u64) << bit)) != 0;
    }
}

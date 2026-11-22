// Origin: Pure-CP Stdlib (AI Dynamic Few-Shot Exemplar Selector: Jaccard, Class Balance & Budget) | Created: 2026-11-22
// Pure CP zero-allocation dynamic Few-Shot exemplar selector with:
// - Case-insensitive bag-of-words Jaccard similarity (64-bit FNV-1a token hashing + sorted set merge)
// - Relevance-ranked greedy selection under token budget (`max_tokens`), class balance (`max_per_label`),
//   and pairwise similarity deduplication (`dedup_threshold`)
// - Zero-allocation Few-Shot prompt block renderer (`renderFewShotBlock`)

/// A candidate Few-Shot exemplar in the retrieval pool.
pub struct FewShotExample {
    id: u32 = 0,
    input: string = "",
    output: string = "",
    label: i32 = 0,
    est_tokens: u32 = 0,
}

/// A selected exemplar index (into `pool`) paired with its query relevance score.
pub struct FewShotSelection {
    example_index: usize = 0,
    score: f64 = 0.0,
}

/// Selection constraints for `selectFewShotExamples`.
pub struct FewShotConfig {
    max_examples: usize = 5,
    max_tokens: u32 = 1024,
    max_per_label: usize = 0,
    dedup_threshold: f64 = 0.9,

    pub static fn default(): FewShotConfig {
        return FewShotConfig{
            max_examples: 5,
            max_tokens: 1024,
            max_per_label: 0,
            dedup_threshold: 0.9,
        };
    }
}

// ============================================================================
// Internal Token Hashing, Sorting & Set Jaccard Helpers
// ============================================================================

fn isWordByte(b: u8): bool {
    if (b >= 'a' && b <= 'z') return true;
    if (b >= 'A' && b <= 'Z') return true;
    if (b >= '0' && b <= '9') return true;
    if (b >= 0x80) return true;
    return false;
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn sortU64Asc(buf: *[]u64, len: usize) {
    if (len < 2) {
        return;
    }
    let i: usize = 1;
    while (i < len) : (i += 1) {
        const key = buf[i];
        let j: usize = i;
        while (j > 0 && buf[j - 1] > key) {
            buf[j] = buf[j - 1];
            j -= 1;
        }
        buf[j] = key;
    }
}

fn dedupSortedU64(buf: *[]u64, len: usize): usize {
    if (len <= 1) {
        return len;
    }
    let write_i: usize = 1;
    let read_i: usize = 1;
    while (read_i < len) : (read_i += 1) {
        if (buf[read_i] != buf[write_i - 1]) {
            buf[write_i] = buf[read_i];
            write_i += 1;
        }
    }
    return write_i;
}

/// Tokenizes `text` into lowercase FNV-1a 64-bit word hashes stored sorted & deduplicated
/// in `out_hashes`. Returns the number of unique token hashes written.
fn extractSortedUniqueHashes(text: string, out_hashes: *[]u64): usize {
    if (text.len == 0 || out_hashes.len == 0) {
        return 0;
    }

    let count: usize = 0;
    let pos: usize = 0;
    while (pos < text.len && count < out_hashes.len) {
        while (pos < text.len && !isWordByte(text[pos])) {
            pos += 1;
        }
        if (pos >= text.len) {
            break;
        }
        // FNV-1a 64-bit hash of the normalized word (0xcbf29ce484222325)
        let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
        while (pos < text.len && isWordByte(text[pos])) : (pos += 1) {
            const b = toLowerAscii(text[pos]);
            h = (h ^ (b as u64)) *% 1099511628211;
        }
        out_hashes[count] = h;
        count += 1;
    }

    sortU64Asc(out_hashes, count);
    return dedupSortedU64(out_hashes, count);
}

fn sortedSetJaccard(a: []const u64, b: []const u64): f64 {
    if (a.len == 0 || b.len == 0) {
        return 0.0;
    }
    let ia: usize = 0;
    let ib: usize = 0;
    let inter: usize = 0;

    while (ia < a.len && ib < b.len) {
        if (a[ia] == b[ib]) {
            inter += 1;
            ia += 1;
            ib += 1;
        } else if (a[ia] < b[ib]) {
            ia += 1;
        } else {
            ib += 1;
        }
    }

    const union_cnt = a.len + b.len - inter;
    if (union_cnt == 0) {
        return 0.0;
    }
    return (inter as f64) / (union_cnt as f64);
}

fn appendSlice(out_buf: *[]u8, pos: usize, data: string): usize? {
    if (pos + data.len > out_buf.len) {
        return null;
    }
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        out_buf[pos + i] = data[i];
    }
    return pos + data.len;
}

// ============================================================================
// Public Dynamic Few-Shot Selection & Rendering APIs
// ============================================================================

/// Computes case-insensitive ASCII bag-of-words Jaccard similarity between `a` and `b`
/// in `[0.0, 1.0]` using caller-provided scratch hash buffers `hashes_a_s` and `hashes_b_s`.
pub fn tokenJaccardSimilarity(
    a: string,
    b: string,
    hashes_a_s: *[]u64,
    hashes_b_s: *[]u64,
): f64 {
    const len_a = extractSortedUniqueHashes(a, hashes_a_s);
    const len_b = extractSortedUniqueHashes(b, hashes_b_s);
    return sortedSetJaccard(hashes_a_s[0..len_a], hashes_b_s[0..len_b]);
}

/// Dynamically selects up to `cfg.max_examples` exemplars from `pool` ranked by
/// descending `tokenJaccardSimilarity(query, example.input)`, greedily enforcing:
/// - total `est_tokens <= cfg.max_tokens`
/// - per-class cap `cfg.max_per_label` (when `> 0`)
/// - pairwise deduplication skipping candidates with similarity `> cfg.dedup_threshold`
///   to any already-selected exemplar.
/// Returns the number of selected exemplars written to `out_selected`, or `null` on
/// NaN `dedup_threshold` or insufficient scratch/output buffer capacity.
pub fn selectFewShotExamples(
    query: string,
    pool: []const FewShotExample,
    cfg: FewShotConfig,
    out_selected: *[]FewShotSelection,
    scores_s: *[]f64,
    order_s: *[]usize,
    hashes_a_s: *[]u64,
    hashes_b_s: *[]u64,
): usize? {
    // NaN guard on dedup_threshold
    if (cfg.dedup_threshold != cfg.dedup_threshold) {
        return null;
    }
    if (pool.len == 0 || cfg.max_examples == 0 || cfg.max_tokens == 0) {
        return 0;
    }
    if (scores_s.len < pool.len || order_s.len < pool.len) {
        return null;
    }

    // 1. Score every candidate against `query`
    const q_len = extractSortedUniqueHashes(query, hashes_a_s);
    let i: usize = 0;
    while (i < pool.len) : (i += 1) {
        const p_len = extractSortedUniqueHashes(pool[i].input, hashes_b_s);
        scores_s[i] = sortedSetJaccard(hashes_a_s[0..q_len], hashes_b_s[0..p_len]);
        order_s[i] = i;
    }

    // 2. Stable descending sort of `order_s[0..pool.len]` by score (tie-break: lower original index)
    let si: usize = 1;
    while (si < pool.len) : (si += 1) {
        const key_idx = order_s[si];
        const key_score = scores_s[key_idx];
        let sj: usize = si;
        while (sj > 0) {
            const prev_idx = order_s[sj - 1];
            const prev_score = scores_s[prev_idx];
            if (prev_score < key_score || (prev_score == key_score && prev_idx > key_idx)) {
                order_s[sj] = prev_idx;
                sj -= 1;
            } else {
                break;
            }
        }
        order_s[sj] = key_idx;
    }

    // 3. Greedy constrained selection
    let selected_count: usize = 0;
    let used_tokens: u64 = 0;
    const max_tokens_u64: u64 = cfg.max_tokens as u64;

    let k: usize = 0;
    while (k < pool.len && selected_count < cfg.max_examples) : (k += 1) {
        const cand_idx = order_s[k];
        const cand = pool[cand_idx];

        // Token budget constraint
        if (used_tokens + (cand.est_tokens as u64) > max_tokens_u64) {
            continue;
        }

        // Per-label class balance constraint
        if (cfg.max_per_label > 0) {
            let label_cnt: usize = 0;
            let s: usize = 0;
            while (s < selected_count) : (s += 1) {
                if (pool[out_selected[s].example_index].label == cand.label) {
                    label_cnt += 1;
                }
            }
            if (label_cnt >= cfg.max_per_label) {
                continue;
            }
        }

        // Pairwise deduplication diversity constraint
        let is_dup: bool = false;
        if (selected_count > 0 && cfg.dedup_threshold < 1.0) {
            const cand_len = extractSortedUniqueHashes(cand.input, hashes_a_s);
            let s: usize = 0;
            while (s < selected_count) : (s += 1) {
                const sel_idx = out_selected[s].example_index;
                const sel_len = extractSortedUniqueHashes(pool[sel_idx].input, hashes_b_s);
                const sim = sortedSetJaccard(hashes_a_s[0..cand_len], hashes_b_s[0..sel_len]);
                if (sim > cfg.dedup_threshold) {
                    is_dup = true;
                    break;
                }
            }
        }
        if (is_dup) {
            continue;
        }

        if (selected_count >= out_selected.len) {
            return null;
        }
        out_selected[selected_count] = FewShotSelection{
            example_index: cand_idx,
            score: scores_s[cand_idx],
        };
        selected_count += 1;
        used_tokens += cand.est_tokens as u64;
    }

    return selected_count;
}

/// Renders the selected Few-Shot exemplars into `out_buf` as:
/// `<input_prefix><input><output_prefix><output>` joined by `separator`.
/// Returns total bytes written, or `null` on out-of-bounds index or insufficient `out_buf`.
pub fn renderFewShotBlock(
    pool: []const FewShotExample,
    selected: []const FewShotSelection,
    input_prefix: string,
    output_prefix: string,
    separator: string,
    out_buf: *[]u8,
): usize? {
    if (selected.len == 0) {
        return 0;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < selected.len) : (i += 1) {
        const idx = selected[i].example_index;
        if (idx >= pool.len) {
            return null;
        }
        if (i > 0) {
            if (p := appendSlice(out_buf, pos, separator)) {
                pos = p;
            } else {
                return null;
            }
        }
        if (p := appendSlice(out_buf, pos, input_prefix)) {
            pos = p;
        } else {
            return null;
        }
        if (p := appendSlice(out_buf, pos, pool[idx].input)) {
            pos = p;
        } else {
            return null;
        }
        if (p := appendSlice(out_buf, pos, output_prefix)) {
            pos = p;
        } else {
            return null;
        }
        if (p := appendSlice(out_buf, pos, pool[idx].output)) {
            pos = p;
        } else {
            return null;
        }
    }

    return pos;
}

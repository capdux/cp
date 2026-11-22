// Origin: Pure-CP Stdlib (AI Byte-Level BPE Tokenizer: Merge-Rank Encode, Decode & Special Tokens) | Created: 2026-11-22
// Pure CP zero-allocation byte-level Byte-Pair Encoding (BPE) tokenizer with:
// - 0..255 base byte vocabulary + ranked BPE merge rules + atomic special control tokens
// - Greedy lowest-rank iterative in-place segment encoding
// - Longest-match atomic special token splitting (e.g. <|endoftext|>, <|im_start|>)
// - Non-recursive iterative stack decoding with cycle, unknown-ID, and overflow guards
// - Exact token counting without requiring an output token buffer

/// A single BPE merge rule combining `(left, right) -> merged_id` with priority `rank`.
/// Lower `rank` values have higher merge priority (rank 0 merges before rank 1).
pub struct BpeMerge {
    left: u32 = 0,
    right: u32 = 0,
    merged_id: u32 = 0,
    rank: u32 = 0,
}

/// An atomic special control token (e.g. `<|endoftext|>`, `<|im_start|>`) mapped to a fixed ID.
pub struct SpecialToken {
    text: string = "",
    id: u32 = 0,
}

/// Read-only view of a byte-level BPE vocabulary (base bytes 0..255 + merges + special tokens).
pub struct BpeVocab {
    merges: []const BpeMerge,
    specials: []const SpecialToken,

    pub static fn init(merges: []const BpeMerge, specials: []const SpecialToken): BpeVocab {
        return BpeVocab{
            merges: merges,
            specials: specials,
        };
    }
}

// ============================================================================
// Internal Lookup & Matching Helpers
// ============================================================================

fn findBestMergeRule(merges: []const BpeMerge, left: u32, right: u32): usize? {
    let best_idx: usize = 0;
    let best_rank: u32 = 0;
    let found: bool = false;

    let i: usize = 0;
    while (i < merges.len) : (i += 1) {
        if (merges[i].left == left && merges[i].right == right) {
            if (!found || merges[i].rank < best_rank) {
                found = true;
                best_idx = i;
                best_rank = merges[i].rank;
            }
        }
    }

    if (found) {
        return best_idx;
    }
    return null;
}

fn findMergeById(merges: []const BpeMerge, id: u32): usize? {
    let i: usize = 0;
    while (i < merges.len) : (i += 1) {
        if (merges[i].merged_id == id) {
            return i;
        }
    }
    return null;
}

fn findSpecialById(specials: []const SpecialToken, id: u32): usize? {
    let i: usize = 0;
    while (i < specials.len) : (i += 1) {
        if (specials[i].id == id) {
            return i;
        }
    }
    return null;
}

fn startsWithAt(text: string, pos: usize, needle: string): bool {
    if (needle.len == 0 || pos + needle.len > text.len) {
        return false;
    }
    let j: usize = 0;
    while (j < needle.len) : (j += 1) {
        if (text[pos + j] != needle[j]) {
            return false;
        }
    }
    return true;
}

/// Finds the longest matching non-empty special token starting at `text[pos..]`.
/// On ties, the earlier entry in `specials` wins.
fn matchLongestSpecialAt(specials: []const SpecialToken, text: string, pos: usize): usize? {
    let best_idx: usize = 0;
    let best_len: usize = 0;
    let found: bool = false;

    let i: usize = 0;
    while (i < specials.len) : (i += 1) {
        const sp_text = specials[i].text;
        if (sp_text.len > 0 && sp_text.len > best_len) {
            if (startsWithAt(text, pos, sp_text)) {
                found = true;
                best_idx = i;
                best_len = sp_text.len;
            }
        }
    }

    if (found) {
        return best_idx;
    }
    return null;
}

// ============================================================================
// Public BPE Encoding, Decoding & Counting APIs
// ============================================================================

/// Encodes a single raw byte segment in-place using greedy lowest-rank BPE merges.
/// Initializes `work_tokens[0..bytes.len]` with base byte IDs `0..255` and iteratively
/// merges all non-overlapping occurrences of the lowest-rank adjacent pair until no
/// merge rule applies.
/// Returns the final token count written into `work_tokens[0..n]`, or `null` if
/// `work_tokens.len < bytes.len`.
pub fn bpeEncodeSegment(vocab: BpeVocab, bytes: []const u8, work_tokens: *[]u32): usize? {
    if (bytes.len == 0) {
        return 0;
    }
    if (bytes.len > work_tokens.len) {
        return null;
    }

    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        work_tokens[i] = bytes[i] as u32;
    }

    let cur_len: usize = bytes.len;
    if (cur_len < 2 || vocab.merges.len == 0) {
        return cur_len;
    }

    while (cur_len >= 2) {
        let best_rule_idx: usize = 0;
        let best_rank: u32 = 0;
        let found: bool = false;

        let p: usize = 0;
        while (p + 1 < cur_len) : (p += 1) {
            if (rule_idx := findBestMergeRule(vocab.merges, work_tokens[p], work_tokens[p + 1])) {
                const r = vocab.merges[rule_idx].rank;
                if (!found || r < best_rank) {
                    found = true;
                    best_rank = r;
                    best_rule_idx = rule_idx;
                }
            }
        }

        if (!found) {
            break;
        }

        const target_left = vocab.merges[best_rule_idx].left;
        const target_right = vocab.merges[best_rule_idx].right;
        const target_merged = vocab.merges[best_rule_idx].merged_id;

        let read_i: usize = 0;
        let write_i: usize = 0;
        while (read_i < cur_len) {
            if (read_i + 1 < cur_len &&
                work_tokens[read_i] == target_left &&
                work_tokens[read_i + 1] == target_right) {
                work_tokens[write_i] = target_merged;
                write_i += 1;
                read_i += 2;
            } else {
                work_tokens[write_i] = work_tokens[read_i];
                write_i += 1;
                read_i += 1;
            }
        }

        // Strict monotonic shrink guard
        if (write_i >= cur_len) {
            break;
        }
        cur_len = write_i;
    }

    return cur_len;
}

/// Encodes `text` into `out_tokens`, atomically preserving longest-match special tokens
/// from `vocab.specials` while BPE-encoding intervening segments via `seg_work`.
/// Returns total token count written to `out_tokens`, or `null` if either `seg_work`
/// or `out_tokens` is too small.
pub fn bpeEncodeWithSpecials(
    vocab: BpeVocab,
    text: string,
    out_tokens: *[]u32,
    seg_work: *[]u32,
): usize? {
    if (text.len == 0) {
        return 0;
    }

    let out_len: usize = 0;
    let seg_start: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        if (sp_idx := matchLongestSpecialAt(vocab.specials, text, pos)) {
            if (pos > seg_start) {
                if (seg_n := bpeEncodeSegment(vocab, text[seg_start..pos], seg_work)) {
                    if (out_len + seg_n > out_tokens.len) {
                        return null;
                    }
                    let k: usize = 0;
                    while (k < seg_n) : (k += 1) {
                        out_tokens[out_len + k] = seg_work[k];
                    }
                    out_len += seg_n;
                } else {
                    return null;
                }
            }

            if (out_len + 1 > out_tokens.len) {
                return null;
            }
            out_tokens[out_len] = vocab.specials[sp_idx].id;
            out_len += 1;

            pos += vocab.specials[sp_idx].text.len;
            seg_start = pos;
        } else {
            pos += 1;
        }
    }

    if (seg_start < text.len) {
        if (seg_n := bpeEncodeSegment(vocab, text[seg_start..text.len], seg_work)) {
            if (out_len + seg_n > out_tokens.len) {
                return null;
            }
            let k: usize = 0;
            while (k < seg_n) : (k += 1) {
                out_tokens[out_len + k] = seg_work[k];
            }
            out_len += seg_n;
        } else {
            return null;
        }
    }

    return out_len;
}

/// Decodes a sequence of BPE token IDs back into raw bytes using an explicit iterative stack
/// (zero recursion, zero heap allocation).
/// Handles base byte tokens (`0..255`), merged token trees (`vocab.merges`), and special
/// tokens (`vocab.specials`).
/// Returns total decoded byte count written to `out_bytes`, or `null` on:
/// - unknown token ID
/// - cyclic merge rule (direct or indirect cycle guard)
/// - insufficient `expand_stack` or `out_bytes` capacity
pub fn bpeDecode(
    vocab: BpeVocab,
    tokens: []const u32,
    out_bytes: *[]u8,
    expand_stack: *[]u32,
): usize? {
    if (tokens.len == 0) {
        return 0;
    }

    let out_len: usize = 0;
    // Upper bound on merge expansions per top-level token to defend against cyclic merge rules.
    const max_expansions: usize = out_bytes.len * 4 + vocab.merges.len + 64;

    let t: usize = 0;
    while (t < tokens.len) : (t += 1) {
        const root_tok = tokens[t];

        // Fast path: special token or base byte token needs no stack expansion.
        if (sp_idx := findSpecialById(vocab.specials, root_tok)) {
            const sp_text = vocab.specials[sp_idx].text;
            if (out_len + sp_text.len > out_bytes.len) {
                return null;
            }
            let j: usize = 0;
            while (j < sp_text.len) : (j += 1) {
                out_bytes[out_len + j] = sp_text[j];
            }
            out_len += sp_text.len;
            continue;
        }

        if (root_tok < 256) {
            if (out_len + 1 > out_bytes.len) {
                return null;
            }
            out_bytes[out_len] = root_tok as u8;
            out_len += 1;
            continue;
        }

        // Merged token: iterative LIFO stack expansion (push right first, then left).
        if (expand_stack.len < 2) {
            return null;
        }
        expand_stack[0] = root_tok;
        let sp: usize = 1;
        let expansions: usize = 0;

        while (sp > 0) {
            sp -= 1;
            const tok = expand_stack[sp];

            if (sp_idx := findSpecialById(vocab.specials, tok)) {
                const sp_text = vocab.specials[sp_idx].text;
                if (out_len + sp_text.len > out_bytes.len) {
                    return null;
                }
                let j: usize = 0;
                while (j < sp_text.len) : (j += 1) {
                    out_bytes[out_len + j] = sp_text[j];
                }
                out_len += sp_text.len;
                continue;
            }

            if (tok < 256) {
                if (out_len + 1 > out_bytes.len) {
                    return null;
                }
                out_bytes[out_len] = tok as u8;
                out_len += 1;
                continue;
            }

            if (rule_idx := findMergeById(vocab.merges, tok)) {
                const rule = vocab.merges[rule_idx];
                // Direct self-cycle guard
                if (rule.left == tok || rule.right == tok) {
                    return null;
                }
                expansions += 1;
                // Indirect cycle / pathological DAG expansion guard
                if (expansions > max_expansions) {
                    return null;
                }
                if (sp + 2 > expand_stack.len) {
                    return null;
                }
                expand_stack[sp] = rule.right;
                expand_stack[sp + 1] = rule.left;
                sp += 2;
            } else {
                // Unknown token ID
                return null;
            }
        }
    }

    return out_len;
}

/// Computes the exact BPE token count for `text` (including atomic special tokens)
/// without needing an output token buffer.
/// Returns `null` if `seg_work` is too small for any non-special segment.
pub fn bpeCountTokens(vocab: BpeVocab, text: string, seg_work: *[]u32): usize? {
    if (text.len == 0) {
        return 0;
    }

    let total: usize = 0;
    let seg_start: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        if (sp_idx := matchLongestSpecialAt(vocab.specials, text, pos)) {
            if (pos > seg_start) {
                if (seg_n := bpeEncodeSegment(vocab, text[seg_start..pos], seg_work)) {
                    total += seg_n;
                } else {
                    return null;
                }
            }
            total += 1;
            pos += vocab.specials[sp_idx].text.len;
            seg_start = pos;
        } else {
            pos += 1;
        }
    }

    if (seg_start < text.len) {
        if (seg_n := bpeEncodeSegment(vocab, text[seg_start..text.len], seg_work)) {
            total += seg_n;
        } else {
            return null;
        }
    }

    return total;
}

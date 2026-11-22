// Origin: Pure-CP Stdlib (AI SentencePiece Unigram Tokenizer: Viterbi Optimal Segmentation) | Created: 2026-11-22
// Pure CP zero-allocation SentencePiece Unigram Language Model tokenizer with:
// - Exact O(N * |V|) Viterbi dynamic programming maximizing total log-probability
// - UTF-8 codepoint-safe unknown character fallback (`unk_id`, `unk_penalty`)
// - SentencePiece metaspace (`▁`, U+2581 = 0xE2 0x96 0x81) normalization & lossless decoding
// - Defensive guards against NaN/Inf log-probabilities, unknown IDs, and buffer exhaustion

/// A single vocabulary piece in a SentencePiece Unigram language model.
pub struct UnigramPiece {
    piece: string = "",
    log_prob: f64 = 0.0,
    id: u32 = 0,
}

/// Read-only view of a SentencePiece Unigram vocabulary.
pub struct UnigramVocab {
    pieces: []const UnigramPiece,
    unk_id: u32 = 0,
    unk_penalty: f64 = -10.0,

    pub static fn init(pieces: []const UnigramPiece, unk_id: u32, unk_penalty: f64): UnigramVocab {
        return UnigramVocab{
            pieces: pieces,
            unk_id: unk_id,
            unk_penalty: unk_penalty,
        };
    }
}

/// Result summary of Viterbi optimal Unigram segmentation.
pub struct UnigramResult {
    token_count: usize = 0,
    total_log_prob: f64 = 0.0,
    unk_count: usize = 0,
}

// ============================================================================
// Internal UTF-8, Float & Matching Helpers
// ============================================================================

fn isFiniteF64(x: f64): bool {
    return (x - x) == 0.0;
}

fn isAsciiWhitespace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}

/// Returns the byte length (1..4) of the UTF-8 codepoint starting at `text[pos]`.
/// Falls back to 1 on invalid or truncated byte sequences so progress is guaranteed.
fn utf8StepLen(text: string, pos: usize): usize {
    if (pos >= text.len) {
        return 0;
    }
    const b0 = text[pos];
    if (b0 < 0x80) {
        return 1;
    }
    let expected: usize = 1;
    if ((b0 & 0xE0) == 0xC0) {
        expected = 2;
    } else if ((b0 & 0xF0) == 0xE0) {
        expected = 3;
    } else if ((b0 & 0xF8) == 0xF0) {
        expected = 4;
    } else {
        return 1;
    }

    if (pos + expected > text.len) {
        return 1;
    }
    let k: usize = 1;
    while (k < expected) : (k += 1) {
        if ((text[pos + k] & 0xC0) != 0x80) {
            return 1;
        }
    }
    return expected;
}

fn startsWithAt(text: string, pos: usize, needle: string): bool {
    if (needle.len == 0 || pos + needle.len > text.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) : (i += 1) {
        if (text[pos + i] != needle[i]) {
            return false;
        }
    }
    return true;
}

fn findPieceById(pieces: []const UnigramPiece, id: u32): usize? {
    let i: usize = 0;
    while (i < pieces.len) : (i += 1) {
        if (pieces[i].id == id) {
            return i;
        }
    }
    return null;
}

// ============================================================================
// Public SentencePiece Unigram APIs
// ============================================================================

/// Normalizes `text` for SentencePiece Unigram encoding: replaces ASCII whitespace bytes
/// with the metaspace marker `▁` (`0xE2 0x96 0x81`) and optionally prepends a leading `▁`
/// for non-empty inputs (`add_dummy_prefix`).
/// Returns total bytes written into `out_buf`, or `null` if `out_buf` is too small.
pub fn normalizeSentencePiece(text: string, add_dummy_prefix: bool, out_buf: *[]u8): usize? {
    if (text.len == 0) {
        return 0;
    }

    let out_len: usize = 0;
    if (add_dummy_prefix) {
        if (out_len + 3 > out_buf.len) {
            return null;
        }
        out_buf[out_len] = 0xE2;
        out_buf[out_len + 1] = 0x96;
        out_buf[out_len + 2] = 0x81;
        out_len += 3;
    }

    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const b = text[i];
        if (isAsciiWhitespace(b)) {
            if (out_len + 3 > out_buf.len) {
                return null;
            }
            out_buf[out_len] = 0xE2;
            out_buf[out_len + 1] = 0x96;
            out_buf[out_len + 2] = 0x81;
            out_len += 3;
        } else {
            if (out_len + 1 > out_buf.len) {
                return null;
            }
            out_buf[out_len] = b;
            out_len += 1;
        }
    }

    return out_len;
}

/// Encodes `text` using zero-allocation Viterbi dynamic programming to find the globally
/// maximum log-likelihood subword segmentation under `vocab`.
/// Caller provides scratch DP buffers of length at least `text.len + 1`:
/// `dp_score_s`, `dp_prev_s`, and `dp_id_s`, plus `out_ids` for the final token sequence.
/// Unmatched UTF-8 codepoints cleanly fall back to a single `vocab.unk_id` with
/// `vocab.unk_penalty` without ever splitting multibyte UTF-8 characters.
/// Returns `UnigramResult`, or `null` on NaN/Inf probabilities or insufficient buffers.
pub fn unigramEncodeViterbi(
    vocab: UnigramVocab,
    text: string,
    out_ids: *[]u32,
    dp_score_s: *[]f64,
    dp_prev_s: *[]usize,
    dp_id_s: *[]u32,
): UnigramResult? {
    // Validate all probabilities are finite (guard against NaN / Inf)
    if (!isFiniteF64(vocab.unk_penalty)) {
        return null;
    }
    let v: usize = 0;
    while (v < vocab.pieces.len) : (v += 1) {
        if (!isFiniteF64(vocab.pieces[v].log_prob)) {
            return null;
        }
    }

    const n = text.len;
    if (n == 0) {
        return UnigramResult{
            token_count: 0,
            total_log_prob: 0.0,
            unk_count: 0,
        };
    }

    if (dp_score_s.len <= n || dp_prev_s.len <= n || dp_id_s.len <= n) {
        return null;
    }

    const unreached: usize = n + 1;
    dp_score_s[0] = 0.0;
    dp_prev_s[0] = 0;
    dp_id_s[0] = 0;

    let init_i: usize = 1;
    while (init_i <= n) : (init_i += 1) {
        dp_score_s[init_i] = 0.0;
        dp_prev_s[init_i] = unreached;
        dp_id_s[init_i] = 0;
    }

    // Forward Viterbi pass
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (dp_prev_s[i] == unreached) {
            continue;
        }
        const base_score = dp_score_s[i];

        // Try all matching vocabulary pieces starting at byte offset `i`
        let p_idx: usize = 0;
        while (p_idx < vocab.pieces.len) : (p_idx += 1) {
            const p = vocab.pieces[p_idx];
            if (p.piece.len == 0) {
                continue;
            }
            if (i + p.piece.len <= n && startsWithAt(text, i, p.piece)) {
                const next_pos = i + p.piece.len;
                const cand_score = base_score + p.log_prob;
                if (dp_prev_s[next_pos] == unreached || cand_score > dp_score_s[next_pos]) {
                    dp_score_s[next_pos] = cand_score;
                    dp_prev_s[next_pos] = i;
                    dp_id_s[next_pos] = p.id;
                }
            }
        }

        // Single UTF-8 codepoint fallback to `unk_id` (preserves multibyte boundaries)
        const step = utf8StepLen(text, i);
        if (step > 0 && i + step <= n) {
            const unk_next = i + step;
            const unk_score = base_score + vocab.unk_penalty;
            if (dp_prev_s[unk_next] == unreached || unk_score > dp_score_s[unk_next]) {
                dp_score_s[unk_next] = unk_score;
                dp_prev_s[unk_next] = i;
                dp_id_s[unk_next] = vocab.unk_id;
            }
        }
    }

    if (dp_prev_s[n] == unreached) {
        return null;
    }

    // Pass 1 backtrack: count optimal path length
    let count: usize = 0;
    let cur: usize = n;
    while (cur > 0) {
        const prev = dp_prev_s[cur];
        if (prev >= cur) {
            return null;
        }
        count += 1;
        cur = prev;
    }

    if (count > out_ids.len) {
        return null;
    }

    // Pass 2 backtrack: populate `out_ids` in forward order
    let unk_count: usize = 0;
    let write_idx: usize = count;
    cur = n;
    while (cur > 0) {
        write_idx -= 1;
        const tok_id = dp_id_s[cur];
        out_ids[write_idx] = tok_id;
        if (tok_id == vocab.unk_id) {
            unk_count += 1;
        }
        cur = dp_prev_s[cur];
    }

    return UnigramResult{
        token_count: count,
        total_log_prob: dp_score_s[n],
        unk_count: unk_count,
    };
}

/// Decodes a sequence of Unigram token IDs back into raw UTF-8 bytes, replacing SentencePiece
/// metaspace markers `▁` (`0xE2 0x96 0x81`) with ASCII space `' '` and optionally stripping
/// a single leading dummy space (`strip_leading_space`).
/// Returns total decoded byte count written to `out_bytes`, or `null` on unknown token ID
/// or insufficient `out_bytes` capacity.
pub fn unigramDecode(
    vocab: UnigramVocab,
    ids: []const u32,
    strip_leading_space: bool,
    out_bytes: *[]u8,
): usize? {
    if (ids.len == 0) {
        return 0;
    }

    let out_len: usize = 0;
    let at_start: bool = true;

    let i: usize = 0;
    while (i < ids.len) : (i += 1) {
        if (p_idx := findPieceById(vocab.pieces, ids[i])) {
            const s = vocab.pieces[p_idx].piece;
            let j: usize = 0;
            while (j < s.len) {
                if (j + 2 < s.len && s[j] == 0xE2 && s[j + 1] == 0x96 && s[j + 2] == 0x81) {
                    j += 3;
                    if (at_start && strip_leading_space) {
                        at_start = false;
                        continue;
                    }
                    at_start = false;
                    if (out_len + 1 > out_bytes.len) {
                        return null;
                    }
                    out_bytes[out_len] = ' ';
                    out_len += 1;
                } else {
                    at_start = false;
                    if (out_len + 1 > out_bytes.len) {
                        return null;
                    }
                    out_bytes[out_len] = s[j];
                    out_len += 1;
                    j += 1;
                }
            }
        } else {
            return null;
        }
    }

    return out_len;
}

// Origin: Pure-CP Stdlib (AI Embedding WordPiece Subword Tokenizer & Encoder Input Packer) | Created: 2026-11-22
// Pure CP zero-allocation WordPiece subword tokenizer and BERT/Transformer encoder input
// packer with greedy longest-subword matching (`##` continuation prefix), basic whitespace
// & ASCII punctuation pre-tokenization, `[UNK]` fallback for OOV/overlong words, and
// single/pair sequence packing (`[CLS] A [SEP] B [SEP]`, `attention_mask`, `token_type_ids`,
// `longest_first` truncation, and `[PAD]` padding).

/// Configuration for WordPiece tokenization and encoder input packing.
pub struct WordPieceConfig {
    unk_id: u32 = 100,
    cls_id: u32 = 101,
    sep_id: u32 = 102,
    pad_id: u32 = 0,
    lowercase: bool = true,
    max_word_chars: usize = 100,

    pub static fn default(): WordPieceConfig {
        return WordPieceConfig{
            unk_id: 100,
            cls_id: 101,
            sep_id: 102,
            pad_id: 0,
            lowercase: true,
            max_word_chars: 100,
        };
    }
}

/// Read-only view over a WordPiece vocabulary slice where array index equals token ID.
pub struct WordPieceVocab {
    tokens: []const string,

    pub static fn init(tokens: []const string): WordPieceVocab {
        return WordPieceVocab{
            tokens: tokens,
        };
    }

    /// Looks up `piece` in the vocabulary and returns its token ID (index), or `null` if absent.
    pub fn lookup(piece: string): u32? {
        let i: usize = 0;
        while (i < _.tokens.len) : (i += 1) {
            if (strEquals(_.tokens[i], piece)) {
                return i as u32;
            }
        }
        return null;
    }
}

/// Summary statistics returned by `wordPieceEncodePair`.
pub struct EncodedSequenceStats {
    total_len: usize = 0,
    real_tokens: usize = 0,
    truncated: bool = false,
}

// ============================================================================
// Internal UTF-8, ASCII & WordPiece Helpers
// ============================================================================

fn strEquals(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn isAsciiWhitespace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\n' || b == '\r' || b == 0x0B || b == 0x0C;
}

fn isAsciiPunctuation(b: u8): bool {
    if (b >= 33 && b <= 47) return true;
    if (b >= 58 && b <= 64) return true;
    if (b >= 91 && b <= 96) return true;
    if (b >= 123 && b <= 126) return true;
    return false;
}

fn asciiToLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn utf8CharCount(s: string): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if ((s[i] & 0xC0) != 0x80) {
            count += 1;
        }
    }
    return count;
}

/// Steps backward from `end` to the start of the previous UTF-8 codepoint boundary.
fn prevUtf8Boundary(s: string, end: usize): usize {
    if (end == 0) {
        return 0;
    }
    let p: usize = end - 1;
    while (p > 0 && (s[p] & 0xC0) == 0x80) {
        p -= 1;
    }
    return p;
}

/// Builds the candidate subword string in `work_buf` (with optional `"##"` continuation
/// prefix and ASCII lowercasing) and returns its byte length, or `null` if `work_buf`
/// is too small.
fn buildCandidatePiece(
    word: string,
    start: usize,
    end: usize,
    lowercase: bool,
    work_buf: *[]u8,
): usize? {
    const slice_len = end - start;
    const prefix_len: usize = if (start > 0) 2 else 0;
    const total_len = prefix_len + slice_len;
    if (total_len > work_buf.len) {
        return null;
    }
    if (start > 0) {
        work_buf[0] = '#';
        work_buf[1] = '#';
    }
    let i: usize = 0;
    while (i < slice_len) : (i += 1) {
        const b = word[start + i];
        work_buf[prefix_len + i] = if (lowercase) asciiToLower(b) else b;
    }
    return total_len;
}

/// Counts how many WordPiece tokens a single pre-tokenized word produces (1 on OOV/overlong).
/// Returns `null` only if `work_buf` is too small.
fn countWordPieceWord(
    vocab: WordPieceVocab,
    word: string,
    cfg: WordPieceConfig,
    work_buf: *[]u8,
): usize? {
    if (word.len == 0) {
        return 0;
    }
    const chars = utf8CharCount(word);
    if (chars > cfg.max_word_chars) {
        return 1;
    }

    let start: usize = 0;
    let count: usize = 0;
    let is_bad: bool = false;

    while (start < word.len) {
        let end: usize = word.len;
        let found: bool = false;

        while (start < end) {
            if (cand_len := buildCandidatePiece(word, start, end, cfg.lowercase, work_buf)) {
                if (vocab.lookup(work_buf[0..cand_len]) != null) {
                    found = true;
                    break;
                }
            } else {
                return null;
            }
            end = prevUtf8Boundary(word, end);
        }

        if (!found) {
            is_bad = true;
            break;
        }
        count += 1;
        start = end;
    }

    if (is_bad) {
        return 1;
    }
    return count;
}

/// Counts total WordPiece tokens for `text`, stopping early once `total >= cap_stop`.
fn countWordPieceText(
    vocab: WordPieceVocab,
    text: string,
    cfg: WordPieceConfig,
    cap_stop: usize,
    work_buf: *[]u8,
): usize? {
    if (text.len == 0) {
        return 0;
    }

    let total: usize = 0;
    let pos: usize = 0;

    while (pos < text.len && total < cap_stop) {
        if (isAsciiWhitespace(text[pos])) {
            pos += 1;
            continue;
        }
        if (isAsciiPunctuation(text[pos])) {
            if (n := countWordPieceWord(vocab, text[pos..pos + 1], cfg, work_buf)) {
                total += n;
            } else {
                return null;
            }
            pos += 1;
            continue;
        }

        const word_start = pos;
        while (pos < text.len && !isAsciiWhitespace(text[pos]) && !isAsciiPunctuation(text[pos])) {
            pos += 1;
        }
        if (n := countWordPieceWord(vocab, text[word_start..pos], cfg, work_buf)) {
            total += n;
        } else {
            return null;
        }
    }

    return total;
}

/// Encodes a single word writing at most `max_emit` tokens into `out_ids[0..max_emit]`.
/// Still validates full word decomposition so OOV words properly collapse to a single `unk_id`.
fn encodeWordCapped(
    vocab: WordPieceVocab,
    word: string,
    cfg: WordPieceConfig,
    out_ids: *[]u32,
    max_emit: usize,
    work_buf: *[]u8,
): usize? {
    if (word.len == 0 || max_emit == 0) {
        return 0;
    }
    if (max_emit > out_ids.len) {
        return null;
    }

    const chars = utf8CharCount(word);
    if (chars > cfg.max_word_chars) {
        out_ids[0] = cfg.unk_id;
        return 1;
    }

    let start: usize = 0;
    let emitted: usize = 0;
    let is_bad: bool = false;

    while (start < word.len) {
        let end: usize = word.len;
        let matched_id: u32 = 0;
        let found: bool = false;

        while (start < end) {
            if (cand_len := buildCandidatePiece(word, start, end, cfg.lowercase, work_buf)) {
                if (id := vocab.lookup(work_buf[0..cand_len])) {
                    matched_id = id;
                    found = true;
                    break;
                }
            } else {
                return null;
            }
            end = prevUtf8Boundary(word, end);
        }

        if (!found) {
            is_bad = true;
            break;
        }

        if (emitted < max_emit) {
            out_ids[emitted] = matched_id;
            emitted += 1;
        }
        start = end;
    }

    if (is_bad) {
        out_ids[0] = cfg.unk_id;
        return 1;
    }
    return emitted;
}

/// Encodes `text` writing at most `max_emit` tokens into `out_ids[0..max_emit]`.
fn encodeTextCapped(
    vocab: WordPieceVocab,
    text: string,
    cfg: WordPieceConfig,
    out_ids: *[]u32,
    max_emit: usize,
    work_buf: *[]u8,
): usize? {
    if (text.len == 0 || max_emit == 0) {
        return 0;
    }
    if (max_emit > out_ids.len) {
        return null;
    }

    let written: usize = 0;
    let pos: usize = 0;

    while (pos < text.len && written < max_emit) {
        if (isAsciiWhitespace(text[pos])) {
            pos += 1;
            continue;
        }
        const rem = max_emit - written;
        let sub_out: []u32 = out_ids[written..max_emit];

        if (isAsciiPunctuation(text[pos])) {
            if (n := encodeWordCapped(vocab, text[pos..pos + 1], cfg, sub_out, rem, work_buf)) {
                written += n;
            } else {
                return null;
            }
            pos += 1;
            continue;
        }

        const word_start = pos;
        while (pos < text.len && !isAsciiWhitespace(text[pos]) && !isAsciiPunctuation(text[pos])) {
            pos += 1;
        }
        if (n := encodeWordCapped(vocab, text[word_start..pos], cfg, sub_out, rem, work_buf)) {
            written += n;
        } else {
            return null;
        }
    }

    return written;
}

// ============================================================================
// Public WordPiece Tokenization & Encoder Packing APIs
// ============================================================================

/// Encodes a single pre-tokenized word using greedy longest-subword WordPiece matching.
/// Continuation subwords automatically prepend `"##"` when querying `vocab`.
/// Words longer than `cfg.max_word_chars` or containing any unmatchable segment fall back
/// to a single `cfg.unk_id`.
/// Returns token count written into `out_ids`, or `null` if `out_ids` or `work_buf` is too small.
pub fn wordPieceEncodeWord(
    vocab: WordPieceVocab,
    word: string,
    cfg: WordPieceConfig,
    out_ids: *[]u32,
    work_buf: *[]u8,
): usize? {
    if (word.len == 0) {
        return 0;
    }

    const chars = utf8CharCount(word);
    if (chars > cfg.max_word_chars) {
        if (out_ids.len < 1) {
            return null;
        }
        out_ids[0] = cfg.unk_id;
        return 1;
    }

    let start: usize = 0;
    let emitted: usize = 0;
    let is_bad: bool = false;

    while (start < word.len) {
        let end: usize = word.len;
        let matched_id: u32 = 0;
        let found: bool = false;

        while (start < end) {
            if (cand_len := buildCandidatePiece(word, start, end, cfg.lowercase, work_buf)) {
                if (id := vocab.lookup(work_buf[0..cand_len])) {
                    matched_id = id;
                    found = true;
                    break;
                }
            } else {
                return null;
            }
            end = prevUtf8Boundary(word, end);
        }

        if (!found) {
            is_bad = true;
            break;
        }

        if (emitted >= out_ids.len) {
            return null;
        }
        out_ids[emitted] = matched_id;
        emitted += 1;
        start = end;
    }

    if (is_bad) {
        if (out_ids.len < 1) {
            return null;
        }
        out_ids[0] = cfg.unk_id;
        return 1;
    }

    return emitted;
}

/// Pre-tokenizes `text` by whitespace and ASCII punctuation (splitting each punctuation byte
/// into its own token) and applies `wordPieceEncodeWord` to every token.
/// Returns total token count written into `out_ids`, or `null` if `out_ids` or `work_buf` is too small.
pub fn wordPieceEncodeText(
    vocab: WordPieceVocab,
    text: string,
    cfg: WordPieceConfig,
    out_ids: *[]u32,
    work_buf: *[]u8,
): usize? {
    if (text.len == 0) {
        return 0;
    }

    let total: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        if (isAsciiWhitespace(text[pos])) {
            pos += 1;
            continue;
        }

        let sub_out: []u32 = out_ids[total..];
        if (isAsciiPunctuation(text[pos])) {
            if (n := wordPieceEncodeWord(vocab, text[pos..pos + 1], cfg, sub_out, work_buf)) {
                total += n;
            } else {
                return null;
            }
            pos += 1;
            continue;
        }

        const word_start = pos;
        while (pos < text.len && !isAsciiWhitespace(text[pos]) && !isAsciiPunctuation(text[pos])) {
            pos += 1;
        }
        if (n := wordPieceEncodeWord(vocab, text[word_start..pos], cfg, sub_out, work_buf)) {
            total += n;
        } else {
            return null;
        }
    }

    return total;
}

/// Packs single-sentence (`text_b.len == 0` -> `[CLS] A [SEP]`) or sentence-pair
/// (`text_b.len > 0` -> `[CLS] A [SEP] B [SEP]`) inputs for BERT / embedding / cross-encoder
/// models with `attention_mask`, `token_type_ids`, `longest_first` truncation respecting
/// `max_seq_len` (always preserving `[CLS]` and `[SEP]`), and optional `[PAD]` padding.
/// Returns `EncodedSequenceStats`, or `null` on invalid `max_seq_len` or insufficient buffers.
pub fn wordPieceEncodePair(
    vocab: WordPieceVocab,
    text_a: string,
    text_b: string,
    cfg: WordPieceConfig,
    max_seq_len: usize,
    pad_to_max: bool,
    input_ids_out: *[]u32,
    attention_mask_out: *[]u8,
    token_type_ids_out: *[]u8,
    work_buf: *[]u8,
): EncodedSequenceStats? {
    const has_b = text_b.len > 0;
    const special_count: usize = if (has_b) 3 else 2;
    if (max_seq_len < special_count) {
        return null;
    }

    const content_budget = max_seq_len - special_count;
    const cap_stop = content_budget + 1;

    let cnt_a: usize = 0;
    if (ca := countWordPieceText(vocab, text_a, cfg, cap_stop, work_buf)) {
        cnt_a = ca;
    } else {
        return null;
    }

    let cnt_b: usize = 0;
    if (has_b) {
        if (cb := countWordPieceText(vocab, text_b, cfg, cap_stop, work_buf)) {
            cnt_b = cb;
        } else {
            return null;
        }
    }

    let keep_a = cnt_a;
    let keep_b = cnt_b;
    let truncated: bool = false;
    while (keep_a + keep_b > content_budget) {
        truncated = true;
        if (keep_a >= keep_b && keep_a > 0) {
            keep_a -= 1;
        } else if (keep_b > 0) {
            keep_b -= 1;
        } else {
            break;
        }
    }

    const real_tokens = special_count + keep_a + keep_b;
    const total_len: usize = if (pad_to_max) max_seq_len else real_tokens;

    if (input_ids_out.len < total_len ||
        attention_mask_out.len < total_len ||
        token_type_ids_out.len < total_len) {
        return null;
    }

    // [CLS]
    let idx: usize = 0;
    input_ids_out[idx] = cfg.cls_id;
    attention_mask_out[idx] = 1;
    token_type_ids_out[idx] = 0;
    idx += 1;

    // Segment A
    if (keep_a > 0) {
        let slice_a: []u32 = input_ids_out[idx..idx + keep_a];
        if (wa := encodeTextCapped(vocab, text_a, cfg, slice_a, keep_a, work_buf)) {
            let k: usize = 0;
            while (k < wa) : (k += 1) {
                attention_mask_out[idx + k] = 1;
                token_type_ids_out[idx + k] = 0;
            }
            idx += wa;
        } else {
            return null;
        }
    }

    // First [SEP]
    input_ids_out[idx] = cfg.sep_id;
    attention_mask_out[idx] = 1;
    token_type_ids_out[idx] = 0;
    idx += 1;

    // Optional Segment B + Second [SEP]
    if (has_b) {
        if (keep_b > 0) {
            let slice_b: []u32 = input_ids_out[idx..idx + keep_b];
            if (wb := encodeTextCapped(vocab, text_b, cfg, slice_b, keep_b, work_buf)) {
                let k: usize = 0;
                while (k < wb) : (k += 1) {
                    attention_mask_out[idx + k] = 1;
                    token_type_ids_out[idx + k] = 1;
                }
                idx += wb;
            } else {
                return null;
            }
        }

        input_ids_out[idx] = cfg.sep_id;
        attention_mask_out[idx] = 1;
        token_type_ids_out[idx] = 1;
        idx += 1;
    }

    // Optional [PAD] tail
    let p: usize = idx;
    while (p < total_len) : (p += 1) {
        input_ids_out[p] = cfg.pad_id;
        attention_mask_out[p] = 0;
        token_type_ids_out[p] = 0;
    }

    return EncodedSequenceStats{
        total_len: total_len,
        real_tokens: idx,
        truncated: truncated,
    };
}

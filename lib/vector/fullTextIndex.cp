// Origin: Pure-CP Stdlib (Unbounded Zero-Heap Multi-Field BM25F Full-Text Search & CRC32 Snapshot Engine) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, caller-owned external-slice full-text search engine:
//   - Multilingual tokenizer: ASCII case folding, Unicode whitespace/punctuation splitting,
//     CJK unigram + 2-gram segmentation, English stopword filtering, and Porter-lite stemming
//   - Unbounded external-slice inverted index (`FullTextIndex`) storing `(doc_slot, field_id, tf, positions_slice)`
//     with soft-deletion tombstones (`markDeleted`) and exact per-field length tracking
//   - Multi-field Robertson/Zaragoza BM25F relevance scoring (`k1`, per-field `weight` and `b`)
//   - Full query engine: recursive-descent boolean parser (`AND`/`OR`/`NOT`/parentheses/`+`/`-`),
//     positional phrase & slop proximity (`"quick brown fox"~2`), prefix (`comp*`), and
//     Damerau-Levenshtein fuzzy correction (`term~1|2`)
//   - Dense-window snippet extractor (`extractBestSnippet`) with merged `<mark>...</mark>` highlighting
//   - IEEE CRC32-verified binary snapshot (`serializeSnapshot` / `deserializeSnapshot`)

const std = @import("std");
import { Simd } from "../simd/index.cp";

// ============================================================================
// 1. Constants, Error Set, Configuration & Data Models
// ============================================================================

/// Little-endian magic tag `"FTDX"` (`0x58445446`) for binary snapshot headers.
pub const FT_SNAPSHOT_MAGIC: u32 = 0x58445446;

/// Binary snapshot wire format version.
pub const FT_SNAPSHOT_VERSION: u32 = 1;

/// Sentinel index for empty hash table slot or end of intrusive posting list.
pub const FT_INVALID_IDX: u32 = 0xFFFF_FFFF;

const FNV1A_64_OFFSET: u64 = 0xcbf29ce484222325;
const FNV1A_64_PRIME: u64 = 0x100000001b3;
const FT_MAX_TOKEN_BYTES: usize = 64;
const FT_MAX_PHRASE_TERMS: usize = 32;
const FT_SNAPSHOT_HEADER_BYTES: usize = 64;
const FT_SNAPSHOT_CRC_BYTES: usize = 4;

/// Error set returned by full-text indexing, query execution, snippet extraction, and snapshots.
pub err FullTextError {
    InvalidConfig,
    BufferTooSmall,
    CapacityFull,
    DocNotFound,
    InvalidField,
    InvalidQuery,
    InvalidSnapshot,
    ChecksumMismatch,
}

/// Per-field BM25F boost weight (`weight >= 0`) and length normalization (`b in [0.0, 1.0]`).
pub struct FullTextFieldSpec {
    weight: f64 = 1.0;
    b: f64 = 0.75;

    /// Returns `true` iff `weight` is finite non-negative and `b` is in `[0.0, 1.0]`.
    fn isValid(): bool {
        if (std.math.isNan(weight) || std.math.isInf(weight) || weight < 0.0) return false;
        if (std.math.isNan(b) || std.math.isInf(b) || b < 0.0 || b > 1.0) return false;
        return true;
    }
}

/// Configuration for `FullTextIndex`.
pub struct FullTextConfig {
    k1: f64 = 1.2;
    field_count: usize = 2;
    enable_stemming: bool = true;
    enable_stopwords: bool = true;
    enable_cjk_bigrams: bool = true;

    /// Returns `true` iff `k1 > 0` and `field_count >= 1`.
    fn isValid(): bool {
        if (std.math.isNan(k1) || std.math.isInf(k1) || k1 <= 0.0) return false;
        if (field_count == 0) return false;
        return true;
    }
}

/// Per-document metadata stored in caller's `docs: []FtDocMeta` slice.
pub struct FtDocMeta {
    doc_id: u64 = 0;
    deleted: bool = false;
    field_len_offset: u32 = 0;
    total_tokens: u32 = 0;
}

/// Dictionary term entry stored in caller's `terms: []FtTermEntry` slice.
pub struct FtTermEntry {
    hash: u64 = 0;
    text_offset: u32 = 0;
    text_len: u16 = 0;
    df: u32 = 0;
    head_posting: u32 = FT_INVALID_IDX;
    tail_posting: u32 = FT_INVALID_IDX;
}

/// Inverted posting entry recording `(doc_slot, field_id, tf, positions_slice)`.
pub struct FtPosting {
    doc_slot: u32 = 0;
    field_id: u16 = 0;
    tf: u32 = 0;
    pos_offset: u32 = 0;
    pos_len: u32 = 0;
    next_posting: u32 = FT_INVALID_IDX;
}

/// Ranked full-text search hit returned by query APIs.
pub struct FtHit {
    doc_id: u64 = 0;
    doc_slot: u32 = 0;
    score: f64 = 0.0;
    matched_terms: u32 = 0;
}

/// Operational statistics snapshot for `FullTextIndex`.
pub struct FtStats {
    doc_count: usize = 0;
    active_doc_count: usize = 0;
    deleted_doc_count: usize = 0;
    term_count: usize = 0;
    posting_count: usize = 0;
    position_count: usize = 0;
    term_bytes_used: usize = 0;
    field_count: usize = 0;
}

/// Token span produced by `tokenizeText` with original byte offsets and normalized text.
pub struct FtTokenSpan {
    byte_start: usize = 0;
    byte_end: usize = 0;
    position: u32 = 0;
    is_bigram: bool = false;
    norm_len: u8 = 0;
    norm_buf: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];

    /// Returns the normalized UTF-8 token slice.
    fn text(): []const u8 {
        return norm_buf[0..@as(usize, norm_len)];
    }
}

/// Caller-owned external storage slices backing `FullTextIndex` (zero heap allocation).
pub struct FullTextBuffers {
    field_specs: []FullTextFieldSpec = [];
    field_len_sums: []u64 = [];
    docs: []FtDocMeta = [];
    doc_field_lens: []u32 = [];
    terms: []FtTermEntry = [];
    term_hash_table: []u32 = [];
    term_bytes: []u8 = [];
    postings: []FtPosting = [];
    positions: []u32 = [];
}

/// Caller-owned external scratch workspace used for document ingestion and query evaluation.
pub struct FullTextScratch {
    term_ids: []u32 = [];
    token_positions: []u32 = [];
    scores: []f64 = [];
    doc_mask: []u8 = [];
    doc_mask_aux: []u8 = [];
    matched_terms_cnt: []u32 = [];
}

/// Returns the minimum `doc_field_lens` length required for `max_docs` and `field_count`.
pub fn ftDocFieldLensMin(max_docs: usize, field_count: usize): usize {
    return max_docs * field_count;
}

/// Returns the recommended open-addressing hash table length (`2 * max_terms`, min 16).
pub fn ftTermHashTableMin(max_terms: usize): usize {
    const twice: usize = max_terms * 2;
    return if (twice < 16) 16 else twice;
}

// ============================================================================
// 2. Binary Little-Endian Encoding, FNV-1a Hash & IEEE CRC32 Helpers
// ============================================================================

fn fnv1a64(bytes: []const u8): u64 {
    let h: u64 = FNV1A_64_OFFSET;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h ^= @as(u64, bytes[i]);
        h *%= FNV1A_64_PRIME;
    }
    return h;
}

fn crc32Ieee(bytes: []const u8): u32 {
    let crc: u32 = 0xFFFF_FFFF;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        crc ^= @as(u32, bytes[i]);
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

fn writeU16Le(dst: *[]u8, off: usize, v: u16) {
    dst[off + 0] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
}

fn writeU32Le(dst: *[]u8, off: usize, v: u32) {
    dst[off + 0] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    dst[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    dst[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

fn writeU64Le(dst: *[]u8, off: usize, v: u64) {
    writeU32Le(dst, off, @as(u32, @intCast(v & 0xFFFF_FFFF)));
    writeU32Le(dst, off + 4, @as(u32, @intCast((v >> 32) & 0xFFFF_FFFF)));
}

fn writeF64Le(dst: *[]u8, off: usize, v: f64) {
    const bits: u64 = @bitCast(v);
    writeU64Le(dst, off, bits);
}

fn readU16Le(src: []const u8, off: usize): u16 {
    return @as(u16, src[off + 0]) | (@as(u16, src[off + 1]) << 8);
}

fn readU32Le(src: []const u8, off: usize): u32 {
    return @as(u32, src[off + 0]) |
        (@as(u32, src[off + 1]) << 8) |
        (@as(u32, src[off + 2]) << 16) |
        (@as(u32, src[off + 3]) << 24);
}

fn readU64Le(src: []const u8, off: usize): u64 {
    const lo: u64 = @as(u64, readU32Le(src, off));
    const hi: u64 = @as(u64, readU32Le(src, off + 4));
    return lo | (hi << 32);
}

fn readF64Le(src: []const u8, off: usize): f64 {
    const bits: u64 = readU64Le(src, off);
    return @bitCast(bits);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn bytesStartsWith(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (haystack[i] != prefix[i]) return false;
    }
    return true;
}

// ============================================================================
// 3. UTF-8 Decoder, Unicode Classification, Stopwords & Porter-Lite Stemmer
// ============================================================================

struct Utf8Decoded {
    cp: u32 = 0;
    len: usize = 1;
}

/// Decodes a single UTF-8 codepoint starting at `bytes[off]`. Invalid sequences yield `U+FFFD` with `len = 1`.
fn decodeUtf8At(bytes: []const u8, off: usize): Utf8Decoded {
    if (off >= bytes.len) return Utf8Decoded{ cp: 0, len: 0 };
    const b0: u8 = bytes[off];
    if (b0 < 0x80) {
        return Utf8Decoded{ cp: @as(u32, b0), len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && off + 1 < bytes.len) {
        const b1: u8 = bytes[off + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp: u32 = (@as(u32, b0 & 0x1F) << 6) | @as(u32, b1 & 0x3F);
            if (cp >= 0x80) return Utf8Decoded{ cp: cp, len: 2 };
        }
    } else if ((b0 & 0xF0) == 0xE0 && off + 2 < bytes.len) {
        const b1: u8 = bytes[off + 1];
        const b2: u8 = bytes[off + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp: u32 = (@as(u32, b0 & 0x0F) << 12) |
                (@as(u32, b1 & 0x3F) << 6) |
                @as(u32, b2 & 0x3F);
            if (cp >= 0x800) return Utf8Decoded{ cp: cp, len: 3 };
        }
    } else if ((b0 & 0xF8) == 0xF0 && off + 3 < bytes.len) {
        const b1: u8 = bytes[off + 1];
        const b2: u8 = bytes[off + 2];
        const b3: u8 = bytes[off + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp: u32 = (@as(u32, b0 & 0x07) << 18) |
                (@as(u32, b1 & 0x3F) << 12) |
                (@as(u32, b2 & 0x3F) << 6) |
                @as(u32, b3 & 0x3F);
            if (cp >= 0x10000 && cp <= 0x10FFFF) return Utf8Decoded{ cp: cp, len: 4 };
        }
    }
    return Utf8Decoded{ cp: 0xFFFD, len: 1 };
}

fn foldAsciiLower(b: u8): u8 {
    if (b >= 65 && b <= 90) return b + 32;
    return b;
}

/// Returns `true` iff `cp` is a CJK ideograph, Kana, or Hangul syllable.
pub fn isCjkCodepoint(cp: u32): bool {
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;
    if (cp >= 0x3040 && cp <= 0x309F) return true;
    if (cp >= 0x30A0 && cp <= 0x30FF) return true;
    if (cp >= 0xAC00 && cp <= 0xD7AF) return true;
    return false;
}

fn isUnicodeSeparatorOrPunct(cp: u32): bool {
    if (cp <= 0x20 || cp == 0x7F) return true;
    if (cp < 0x80) {
        const b: u8 = @as(u8, @intCast(cp));
        if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9')) {
            return false;
        }
        return true;
    }
    if (cp == 0x00A0 || cp == 0x3000) return true;
    if (cp >= 0x2000 && cp <= 0x206F) return true;
    if (cp >= 0x3001 && cp <= 0x303F) return true;
    if (cp >= 0xFF00 && cp <= 0xFF0F) return true;
    if (cp >= 0xFF1A && cp <= 0xFF20) return true;
    if (cp >= 0xFF3B && cp <= 0xFF40) return true;
    if (cp >= 0xFF5B && cp <= 0xFF65) return true;
    return false;
}

/// Returns `true` iff lowercase ASCII `word` is a standard English stopword.
pub fn isEnglishStopword(word: []const u8): bool {
    if (word.len == 0 || word.len > 5) return false;
    if (word.len == 1) {
        return word[0] == 'a' || word[0] == 'i';
    }
    if (word.len == 2) {
        if (bytesEqual(word, "an") || bytesEqual(word, "as") || bytesEqual(word, "at")) return true;
        if (bytesEqual(word, "be") || bytesEqual(word, "by") || bytesEqual(word, "do")) return true;
        if (bytesEqual(word, "if") || bytesEqual(word, "in") || bytesEqual(word, "is")) return true;
        if (bytesEqual(word, "it") || bytesEqual(word, "no") || bytesEqual(word, "of")) return true;
        if (bytesEqual(word, "on") || bytesEqual(word, "or") || bytesEqual(word, "so")) return true;
        if (bytesEqual(word, "to") || bytesEqual(word, "up") || bytesEqual(word, "we")) return true;
        return false;
    }
    if (word.len == 3) {
        if (bytesEqual(word, "and") || bytesEqual(word, "are") || bytesEqual(word, "but")) return true;
        if (bytesEqual(word, "for") || bytesEqual(word, "had") || bytesEqual(word, "has")) return true;
        if (bytesEqual(word, "her") || bytesEqual(word, "his") || bytesEqual(word, "its")) return true;
        if (bytesEqual(word, "not") || bytesEqual(word, "our") || bytesEqual(word, "out")) return true;
        if (bytesEqual(word, "she") || bytesEqual(word, "the") || bytesEqual(word, "was")) return true;
        if (bytesEqual(word, "who") || bytesEqual(word, "you")) return true;
        return false;
    }
    if (word.len == 4) {
        if (bytesEqual(word, "from") || bytesEqual(word, "have") || bytesEqual(word, "into")) return true;
        if (bytesEqual(word, "such") || bytesEqual(word, "that") || bytesEqual(word, "then")) return true;
        if (bytesEqual(word, "they") || bytesEqual(word, "this") || bytesEqual(word, "were")) return true;
        if (bytesEqual(word, "what") || bytesEqual(word, "when") || bytesEqual(word, "will")) return true;
        if (bytesEqual(word, "with")) return true;
        return false;
    }
    if (bytesEqual(word, "their") || bytesEqual(word, "there") || bytesEqual(word, "these") ||
        bytesEqual(word, "those") || bytesEqual(word, "where") || bytesEqual(word, "which"))
    {
        return true;
    }
    return false;
}

fn isAsciiAlphaOnly(word: []const u8): bool {
    if (word.len == 0) return false;
    let i: usize = 0;
    while (i < word.len) : (i += 1) {
        if (word[i] < 'a' || word[i] > 'z') return false;
    }
    return true;
}

fn isVowelAt(word: []const u8, idx: usize): bool {
    const c: u8 = word[idx];
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') return true;
    if (c == 'y' && idx > 0) {
        const prev: u8 = word[idx - 1];
        if (prev != 'a' && prev != 'e' && prev != 'i' && prev != 'o' && prev != 'u') {
            return true;
        }
    }
    return false;
}

fn containsVowel(word: []const u8, end_idx: usize): bool {
    let i: usize = 0;
    while (i < end_idx) : (i += 1) {
        if (isVowelAt(word, i)) return true;
    }
    return false;
}

/// Computes the Porter consonant-vowel measure `m` for `word[0..end_idx]`.
fn porterMeasure(word: []const u8, end_idx: usize): usize {
    let m: usize = 0;
    let i: usize = 0;
    while (i < end_idx && !isVowelAt(word, i)) : (i += 1) {}
    while (i < end_idx) {
        while (i < end_idx && isVowelAt(word, i)) : (i += 1) {}
        if (i >= end_idx) break;
        while (i < end_idx && !isVowelAt(word, i)) : (i += 1) {}
        m += 1;
    }
    return m;
}

fn endsWithDoubleConsonant(word: []const u8, len: usize): bool {
    if (len < 2) return false;
    if (word[len - 1] != word[len - 2]) return false;
    return !isVowelAt(word, len - 1);
}

fn endsWithCvc(word: []const u8, len: usize): bool {
    if (len < 3) return false;
    const c: u8 = word[len - 1];
    if (c == 'w' || c == 'x' || c == 'y') return false;
    return !isVowelAt(word, len - 1) && isVowelAt(word, len - 2) && !isVowelAt(word, len - 3);
}

fn endsWithBytes(word: []const u8, len: usize, suffix: []const u8): bool {
    if (suffix.len > len) return false;
    const start: usize = len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (word[start + i] != suffix[i]) return false;
    }
    return true;
}

fn replaceSuffix(buf: *[]u8, len: usize, suffix_len: usize, repl: []const u8): usize {
    const base: usize = len - suffix_len;
    let i: usize = 0;
    while (i < repl.len && base + i < buf.len) : (i += 1) {
        buf[base + i] = repl[i];
    }
    return base + i;
}

/// Applies Porter-lite English stemming in place on a lowercase ASCII buffer and returns the new length.
pub fn porterLiteStemInPlace(buf: *[]u8, in_len: usize): usize {
    let len: usize = in_len;
    if (len <= 2 || len > buf.len) return len;
    if (!isAsciiAlphaOnly(buf[0..len])) return len;

    // Step 1a: plurals
    if (endsWithBytes(buf[0..len], len, "sses")) {
        len -= 2;
    } else if (endsWithBytes(buf[0..len], len, "ies")) {
        len -= 2;
    } else if (endsWithBytes(buf[0..len], len, "ss")) {
        // keep "ss"
    } else if (len > 3 && buf[len - 1] == 's') {
        len -= 1;
    }

    // Step 1b: -eed, -ed, -ing
    let step1b_extra: bool = false;
    if (endsWithBytes(buf[0..len], len, "eed")) {
        if (porterMeasure(buf[0..len], len - 3) > 0) {
            len -= 1;
        }
    } else if (endsWithBytes(buf[0..len], len, "ed") && containsVowel(buf[0..len], len - 2)) {
        len -= 2;
        step1b_extra = true;
    } else if (endsWithBytes(buf[0..len], len, "ing") && containsVowel(buf[0..len], len - 3)) {
        len -= 3;
        step1b_extra = true;
    }

    if (step1b_extra) {
        if (endsWithBytes(buf[0..len], len, "at") ||
            endsWithBytes(buf[0..len], len, "bl") ||
            endsWithBytes(buf[0..len], len, "iz"))
        {
            if (len < buf.len) {
                buf[len] = 'e';
                len += 1;
            }
        } else if (endsWithDoubleConsonant(buf[0..len], len)) {
            const last: u8 = buf[len - 1];
            if (last != 'l' && last != 's' && last != 'z') {
                len -= 1;
            }
        } else if (porterMeasure(buf[0..len], len) == 1 && endsWithCvc(buf[0..len], len)) {
            if (len < buf.len) {
                buf[len] = 'e';
                len += 1;
            }
        }
    }

    // Step 1c: terminal y -> i
    if (len > 2 && buf[len - 1] == 'y' && containsVowel(buf[0..len], len - 1)) {
        buf[len - 1] = 'i';
    }

    // Step 2: common derivational suffixes (when stem measure > 0)
    if (endsWithBytes(buf[0..len], len, "ational") && porterMeasure(buf[0..len], len - 7) > 0) {
        len = replaceSuffix(buf, len, 7, "ate");
    } else if (endsWithBytes(buf[0..len], len, "tional") && porterMeasure(buf[0..len], len - 6) > 0) {
        len = replaceSuffix(buf, len, 6, "tion");
    } else if (endsWithBytes(buf[0..len], len, "fulness") && porterMeasure(buf[0..len], len - 7) > 0) {
        len = replaceSuffix(buf, len, 7, "ful");
    } else if (endsWithBytes(buf[0..len], len, "ousness") && porterMeasure(buf[0..len], len - 7) > 0) {
        len = replaceSuffix(buf, len, 7, "ous");
    } else if (endsWithBytes(buf[0..len], len, "iveness") && porterMeasure(buf[0..len], len - 7) > 0) {
        len = replaceSuffix(buf, len, 7, "ive");
    } else if (endsWithBytes(buf[0..len], len, "ization") && porterMeasure(buf[0..len], len - 7) > 0) {
        len = replaceSuffix(buf, len, 7, "ize");
    } else if (endsWithBytes(buf[0..len], len, "ation") && porterMeasure(buf[0..len], len - 5) > 0) {
        len = replaceSuffix(buf, len, 5, "ate");
    } else if (endsWithBytes(buf[0..len], len, "ator") && porterMeasure(buf[0..len], len - 4) > 0) {
        len = replaceSuffix(buf, len, 4, "ate");
    } else if (endsWithBytes(buf[0..len], len, "izer") && porterMeasure(buf[0..len], len - 4) > 0) {
        len = replaceSuffix(buf, len, 4, "ize");
    }

    // Step 3/4: common noun/adjective endings (when stem measure > 1)
    if (endsWithBytes(buf[0..len], len, "ment") && porterMeasure(buf[0..len], len - 4) > 1) {
        len -= 4;
    } else if (endsWithBytes(buf[0..len], len, "ness") && porterMeasure(buf[0..len], len - 4) > 1) {
        len -= 4;
    } else if (endsWithBytes(buf[0..len], len, "able") && porterMeasure(buf[0..len], len - 4) > 1) {
        len -= 4;
    } else if (endsWithBytes(buf[0..len], len, "ible") && porterMeasure(buf[0..len], len - 4) > 1) {
        len -= 4;
    } else if (len > 4 && buf[len - 1] == 'e') {
        const m: usize = porterMeasure(buf[0..len], len - 1);
        if (m > 1 || (m == 1 && !endsWithCvc(buf[0..len], len - 1))) {
            len -= 1;
        }
    }

    return len;
}

/// Normalizes a single ASCII/UTF-8 query term into `out_buf` using SIMD `toLowerAscii`,
/// returning normalized byte length.
pub fn normalizeSingleTerm(
    raw: []const u8,
    enable_stemming: bool,
    out_buf: *[]u8,
): usize {
    let n: usize = Simd.toLowerAscii(raw, out_buf.*);
    if (enable_stemming && n > 2) {
        n = porterLiteStemInPlace(out_buf, n);
    }
    return n;
}

/// Tokenizes `text` into caller-provided `out_spans` slice with SIMD ASCII fast-path
/// (`Simd.firstNonAscii` + `Simd.toLowerAscii` + `Simd.skipWhitespace`), Unicode punctuation
/// splitting, optional CJK unigram + 2-gram segmentation, stopword filtering, and Porter-lite stemming.
/// Returns the number of tokens written to `out_spans`.
pub fn tokenizeText(
    text: []const u8,
    enable_stemming: bool,
    enable_stopwords: bool,
    enable_cjk_bigrams: bool,
    out_spans: *[]FtTokenSpan,
): usize {
    const pure_ascii: bool = Simd.firstNonAscii(text) == null;
    let out_count: usize = 0;
    let pos: u32 = 0;
    let i: usize = 0;
    let prev_cjk_start: usize = 0;
    let prev_cjk_len: usize = 0;
    let prev_cjk_pos: u32 = 0;
    let has_prev_cjk: bool = false;

    while (i < text.len && out_count < out_spans.len) {
        const ws_adv: usize = Simd.skipWhitespace(text[i..text.len]);
        if (ws_adv > 0) {
            has_prev_cjk = false;
            i += ws_adv;
            if (i >= text.len) break;
        }

        if (pure_ascii) {
            const b0: u8 = text[i];
            if (!((b0 >= 'a' && b0 <= 'z') || (b0 >= 'A' && b0 <= 'Z') || (b0 >= '0' && b0 <= '9'))) {
                i += 1;
                continue;
            }
            const word_start: usize = i;
            while (i < text.len) : (i += 1) {
                const c: u8 = text[i];
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                    break;
                }
            }
            const word_end: usize = i;
            let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
            let norm_slice: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
            let norm_len: usize = Simd.toLowerAscii(text[word_start..word_end], norm_slice);
            if (norm_len == 0) continue;
            if (enable_stopwords && isEnglishStopword(norm_arr[0..norm_len])) {
                pos += 1;
                continue;
            }
            if (enable_stemming && norm_len > 2) {
                norm_len = porterLiteStemInPlace(*norm_slice, norm_len);
            }
            if (norm_len > 0 && out_count < out_spans.len) {
                let sp: FtTokenSpan = FtTokenSpan{
                    byte_start: word_start,
                    byte_end: word_end,
                    position: pos,
                    is_bigram: false,
                    norm_len: @as(u8, @intCast(norm_len)),
                };
                let k: usize = 0;
                while (k < norm_len) : (k += 1) {
                    sp.norm_buf[k] = norm_arr[k];
                }
                out_spans[out_count] = sp;
                out_count += 1;
            }
            pos += 1;
            continue;
        }

        const u8dec: Utf8Decoded = decodeUtf8At(text, i);
        if (u8dec.len == 0) break;

        if (isUnicodeSeparatorOrPunct(u8dec.cp)) {
            has_prev_cjk = false;
            i += u8dec.len;
            continue;
        }

        if (isCjkCodepoint(u8dec.cp)) {
            const cur_start: usize = i;
            const cur_len: usize = u8dec.len;
            const cur_pos: u32 = pos;

            // Emit unigram
            if (out_count < out_spans.len) {
                let sp: FtTokenSpan = FtTokenSpan{
                    byte_start: cur_start,
                    byte_end: cur_start + cur_len,
                    position: cur_pos,
                    is_bigram: false,
                    norm_len: @as(u8, @intCast(cur_len)),
                };
                let k: usize = 0;
                while (k < cur_len) : (k += 1) {
                    sp.norm_buf[k] = text[cur_start + k];
                }
                out_spans[out_count] = sp;
                out_count += 1;
            }

            // Emit 2-gram with previous adjacent CJK char
            if (enable_cjk_bigrams && has_prev_cjk && out_count < out_spans.len) {
                const bi_len: usize = prev_cjk_len + cur_len;
                if (bi_len <= FT_MAX_TOKEN_BYTES) {
                    let sp_bi: FtTokenSpan = FtTokenSpan{
                        byte_start: prev_cjk_start,
                        byte_end: cur_start + cur_len,
                        position: prev_cjk_pos,
                        is_bigram: true,
                        norm_len: @as(u8, @intCast(bi_len)),
                    };
                    let k: usize = 0;
                    while (k < prev_cjk_len) : (k += 1) {
                        sp_bi.norm_buf[k] = text[prev_cjk_start + k];
                    }
                    let m: usize = 0;
                    while (m < cur_len) : (m += 1) {
                        sp_bi.norm_buf[prev_cjk_len + m] = text[cur_start + m];
                    }
                    out_spans[out_count] = sp_bi;
                    out_count += 1;
                }
            }

            prev_cjk_start = cur_start;
            prev_cjk_len = cur_len;
            prev_cjk_pos = cur_pos;
            has_prev_cjk = true;
            pos += 1;
            i += cur_len;
            continue;
        }

        // Non-CJK word run (ASCII letters/digits or extended Latin/Unicode word chars)
        has_prev_cjk = false;
        const word_start: usize = i;
        let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
        let norm_len: usize = 0;

        while (i < text.len) {
            const wdec: Utf8Decoded = decodeUtf8At(text, i);
            if (wdec.len == 0) break;
            if (isUnicodeSeparatorOrPunct(wdec.cp) || isCjkCodepoint(wdec.cp)) break;
            if (wdec.len == 1) {
                if (norm_len < FT_MAX_TOKEN_BYTES) {
                    norm_arr[norm_len] = foldAsciiLower(text[i]);
                    norm_len += 1;
                }
            } else {
                if (norm_len + wdec.len <= FT_MAX_TOKEN_BYTES) {
                    let k: usize = 0;
                    while (k < wdec.len) : (k += 1) {
                        norm_arr[norm_len + k] = text[i + k];
                    }
                    norm_len += wdec.len;
                }
            }
            i += wdec.len;
        }
        const word_end: usize = i;

        if (norm_len == 0) continue;

        if (enable_stopwords && isEnglishStopword(norm_arr[0..norm_len])) {
            pos += 1;
            continue;
        }

        if (enable_stemming && norm_len > 2) {
            let norm_slice: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
            norm_len = porterLiteStemInPlace(*norm_slice, norm_len);
        }

        if (norm_len > 0 && out_count < out_spans.len) {
            let sp: FtTokenSpan = FtTokenSpan{
                byte_start: word_start,
                byte_end: word_end,
                position: pos,
                is_bigram: false,
                norm_len: @as(u8, @intCast(norm_len)),
            };
            let k: usize = 0;
            while (k < norm_len) : (k += 1) {
                sp.norm_buf[k] = norm_arr[k];
            }
            out_spans[out_count] = sp;
            out_count += 1;
        }
        pos += 1;
    }

    return out_count;
}

// ============================================================================
// 4. Damerau-Levenshtein Bounded Edit Distance & BM25F Pure Math Functions
// ============================================================================

/// Computes the Optimal String Alignment (Damerau-Levenshtein with adjacent transpositions)
/// edit distance between `a` and `b`, returning `max_edits + 1` early if exceeded.
/// Operates on UTF-8 codepoints (up to 64 codepoints per term) with zero heap allocation.
pub fn damerauLevenshteinBounded(a: []const u8, b: []const u8, max_edits: usize): usize {
    if (bytesEqual(a, b)) return 0;
    if (max_edits == 0) return 1;

    let ca: [FT_MAX_TOKEN_BYTES]u32 = [0; FT_MAX_TOKEN_BYTES];
    let cb: [FT_MAX_TOKEN_BYTES]u32 = [0; FT_MAX_TOKEN_BYTES];
    let na: usize = 0;
    let nb: usize = 0;

    let ia: usize = 0;
    while (ia < a.len && na < FT_MAX_TOKEN_BYTES) {
        const d: Utf8Decoded = decodeUtf8At(a, ia);
        if (d.len == 0) break;
        ca[na] = d.cp;
        na += 1;
        ia += d.len;
    }
    let ib: usize = 0;
    while (ib < b.len && nb < FT_MAX_TOKEN_BYTES) {
        const d: Utf8Decoded = decodeUtf8At(b, ib);
        if (d.len == 0) break;
        cb[nb] = d.cp;
        nb += 1;
        ib += d.len;
    }

    const diff: usize = if (na > nb) na - nb else nb - na;
    if (diff > max_edits) return max_edits + 1;
    if (na == 0) return nb;
    if (nb == 0) return na;

    // 3-row rolling DP for Damerau-Levenshtein (handles adjacent transpositions)
    let row_prev2: [FT_MAX_TOKEN_BYTES + 1]usize = [0; FT_MAX_TOKEN_BYTES + 1];
    let row_prev1: [FT_MAX_TOKEN_BYTES + 1]usize = [0; FT_MAX_TOKEN_BYTES + 1];
    let row_curr: [FT_MAX_TOKEN_BYTES + 1]usize = [0; FT_MAX_TOKEN_BYTES + 1];

    let j: usize = 0;
    while (j <= nb) : (j += 1) {
        row_prev1[j] = j;
    }

    let i: usize = 1;
    while (i <= na) : (i += 1) {
        row_curr[0] = i;
        let row_min: usize = i;
        j = 1;
        while (j <= nb) : (j += 1) {
            const cost: usize = if (ca[i - 1] == cb[j - 1]) 0 else 1;
            let best: usize = row_prev1[j] + 1; // deletion
            const ins: usize = row_curr[j - 1] + 1; // insertion
            const sub: usize = row_prev1[j - 1] + cost; // substitution
            if (ins < best) best = ins;
            if (sub < best) best = sub;
            if (i > 1 && j > 1 && ca[i - 1] == cb[j - 2] && ca[i - 2] == cb[j - 1]) {
                const trans: usize = row_prev2[j - 2] + 1;
                if (trans < best) best = trans;
            }
            row_curr[j] = best;
            if (best < row_min) row_min = best;
        }
        if (row_min > max_edits) {
            return max_edits + 1;
        }
        j = 0;
        while (j <= nb) : (j += 1) {
            row_prev2[j] = row_prev1[j];
            row_prev1[j] = row_curr[j];
        }
    }

    return row_prev1[nb];
}

/// Computes Lucene / Robertson smoothed non-negative IDF: `ln(1 + (N - df + 0.5) / (df + 0.5))`.
pub fn computeBm25fIdf(active_docs: usize, df: usize): f64 {
    if (active_docs == 0 || df == 0) return 0.0;
    const n_f: f64 = @as(f64, @floatFromInt(active_docs));
    const df_f: f64 = @as(f64, @floatFromInt(df));
    const ratio: f64 = (n_f - df_f + 0.5) / (df_f + 0.5);
    if (ratio <= 0.0) return 0.0;
    return std.math.log(f64, std.math.e, 1.0 + ratio);
}

/// Computes single-term BM25F score from `idf`, aggregated `pseudo_tf`, and saturation `k1`.
pub fn computeBm25fTermScore(idf: f64, pseudo_tf: f64, k1: f64): f64 {
    if (idf <= 0.0 || pseudo_tf <= 0.0) return 0.0;
    return idf * (pseudo_tf * (k1 + 1.0)) / (pseudo_tf + k1);
}

// ============================================================================
// 5. FullTextIndex — Unbounded External-Slice Inverted Index & Query Engine
// ============================================================================

/// Zero-heap-allocation, caller-slice-backed multi-field BM25F full-text search engine.
pub struct FullTextIndex {
    cfg: FullTextConfig = FullTextConfig{},
    bufs: FullTextBuffers = FullTextBuffers{},
    doc_count: usize = 0;
    active_doc_count: usize = 0;
    term_count: usize = 0;
    posting_count: usize = 0;
    position_count: usize = 0;
    term_bytes_used: usize = 0;

    /// Initializes a `FullTextIndex` over caller-provided external storage slices.
    static fn init(cfg: FullTextConfig, bufs: FullTextBuffers): FullTextIndex ! FullTextError {
        if (!cfg.isValid()) return FullTextError.InvalidConfig;
        if (bufs.field_specs.len < cfg.field_count) return FullTextError.BufferTooSmall;
        if (bufs.field_len_sums.len < cfg.field_count) return FullTextError.BufferTooSmall;
        if (bufs.docs.len == 0 || bufs.terms.len == 0 || bufs.term_hash_table.len < 2) {
            return FullTextError.BufferTooSmall;
        }
        if (bufs.doc_field_lens.len < bufs.docs.len * cfg.field_count) {
            return FullTextError.BufferTooSmall;
        }
        if (bufs.term_bytes.len == 0 || bufs.postings.len == 0 || bufs.positions.len == 0) {
            return FullTextError.BufferTooSmall;
        }

        let has_positive_weight: bool = false;
        let f: usize = 0;
        while (f < cfg.field_count) : (f += 1) {
            if (!bufs.field_specs[f].isValid()) return FullTextError.InvalidConfig;
            if (bufs.field_specs[f].weight > 0.0) has_positive_weight = true;
        }
        if (!has_positive_weight) return FullTextError.InvalidConfig;

        let idx: FullTextIndex = FullTextIndex{
            cfg: cfg,
            bufs: bufs,
        };
        idx.reset();
        return idx;
    }

    /// Resets all document, dictionary, posting, and length counters to empty state.
    fn reset(*_) {
        _.doc_count = 0;
        _.active_doc_count = 0;
        _.term_count = 0;
        _.posting_count = 0;
        _.position_count = 0;
        _.term_bytes_used = 0;

        let f: usize = 0;
        while (f < _.cfg.field_count) : (f += 1) {
            _.bufs.field_len_sums[f] = 0;
        }
        let i: usize = 0;
        while (i < _.bufs.term_hash_table.len) : (i += 1) {
            _.bufs.term_hash_table[i] = FT_INVALID_IDX;
        }
    }

    /// Returns operational telemetry for this index.
    fn stats(): FtStats {
        return FtStats{
            doc_count: doc_count,
            active_doc_count: active_doc_count,
            deleted_doc_count: doc_count - active_doc_count,
            term_count: term_count,
            posting_count: posting_count,
            position_count: position_count,
            term_bytes_used: term_bytes_used,
            field_count: cfg.field_count,
        };
    }

    /// Returns the average token length of `field_id` across active documents.
    fn avgFieldLen(field_id: usize): f64 {
        if (field_id >= cfg.field_count || active_doc_count == 0) return 0.0;
        return @as(f64, @floatFromInt(bufs.field_len_sums[field_id])) /
            @as(f64, @floatFromInt(active_doc_count));
    }

    /// Returns the dictionary UTF-8 slice for `term_idx`.
    fn termText(term_idx: usize): []const u8 {
        const entry: FtTermEntry = bufs.terms[term_idx];
        const start: usize = @as(usize, entry.text_offset);
        const end: usize = start + @as(usize, entry.text_len);
        return bufs.term_bytes[start..end];
    }

    /// Looks up `term` in the open-addressing dictionary hash table, returning `term_idx` if present.
    fn findTerm(term: []const u8): u32? {
        if (term.len == 0) return null;
        const cap: usize = bufs.term_hash_table.len;
        const h: u64 = fnv1a64(term);
        let slot: usize = @as(usize, @intCast(h % @as(u64, @intCast(cap))));
        let probes: usize = 0;
        while (probes < cap) : (probes += 1) {
            const tidx: u32 = bufs.term_hash_table[slot];
            if (tidx == FT_INVALID_IDX) return null;
            const entry: FtTermEntry = bufs.terms[@as(usize, tidx)];
            if (entry.hash == h && @as(usize, entry.text_len) == term.len) {
                const start: usize = @as(usize, entry.text_offset);
                if (bytesEqual(bufs.term_bytes[start..start + term.len], term)) {
                    return tidx;
                }
            }
            slot += 1;
            if (slot == cap) slot = 0;
        }
        return null;
    }

    /// Inserts or retrieves `term` in the dictionary, returning its `term_idx`.
    fn internTerm(*_, term: []const u8): u32 ! FullTextError {
        if (existing := _.findTerm(term)) {
            return existing;
        }
        if (_.term_count >= _.bufs.terms.len) return FullTextError.CapacityFull;
        if (_.term_bytes_used + term.len > _.bufs.term_bytes.len) return FullTextError.CapacityFull;

        const cap: usize = _.bufs.term_hash_table.len;
        const h: u64 = fnv1a64(term);
        let slot: usize = @as(usize, @intCast(h % @as(u64, @intCast(cap))));
        let probes: usize = 0;
        let target_slot: usize = cap;
        while (probes < cap) : (probes += 1) {
            if (_.bufs.term_hash_table[slot] == FT_INVALID_IDX) {
                target_slot = slot;
                break;
            }
            slot += 1;
            if (slot == cap) slot = 0;
        }
        if (target_slot == cap) return FullTextError.CapacityFull;

        const tidx: u32 = @as(u32, @intCast(_.term_count));
        const off: u32 = @as(u32, @intCast(_.term_bytes_used));
        let k: usize = 0;
        while (k < term.len) : (k += 1) {
            _.bufs.term_bytes[_.term_bytes_used + k] = term[k];
        }
        _.term_bytes_used += term.len;

        _.bufs.terms[_.term_count] = FtTermEntry{
            hash: h,
            text_offset: off,
            text_len: @as(u16, @intCast(term.len)),
            df: 0,
            head_posting: FT_INVALID_IDX,
            tail_posting: FT_INVALID_IDX,
        };
        _.bufs.term_hash_table[target_slot] = tidx;
        _.term_count += 1;
        return tidx;
    }

    /// Rebuilds the open-addressing hash table from `bufs.terms[0..term_count]`.
    fn rebuildHashTable(*_): void ! FullTextError {
        const cap: usize = _.bufs.term_hash_table.len;
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            _.bufs.term_hash_table[i] = FT_INVALID_IDX;
        }
        let t: usize = 0;
        while (t < _.term_count) : (t += 1) {
            const h: u64 = _.bufs.terms[t].hash;
            let slot: usize = @as(usize, @intCast(h % @as(u64, @intCast(cap))));
            let probes: usize = 0;
            let placed: bool = false;
            while (probes < cap) : (probes += 1) {
                if (_.bufs.term_hash_table[slot] == FT_INVALID_IDX) {
                    _.bufs.term_hash_table[slot] = @as(u32, @intCast(t));
                    placed = true;
                    break;
                }
                slot += 1;
                if (slot == cap) slot = 0;
            }
            if (!placed) return FullTextError.BufferTooSmall;
        }
    }

    /// Checks whether `term_idx` already has a posting for `doc_slot` in any field.
    fn termHasDoc(term_idx: usize, doc_slot: u32): bool {
        let p_idx: u32 = bufs.terms[term_idx].head_posting;
        while (p_idx != FT_INVALID_IDX) {
            const p: FtPosting = bufs.postings[@as(usize, p_idx)];
            if (p.doc_slot == doc_slot) return true;
            p_idx = p.next_posting;
        }
        return false;
    }

    /// Allocates a new document slot for `doc_id` and returns its internal `doc_slot`.
    fn beginDocument(*_, doc_id: u64): u32 ! FullTextError {
        if (_.doc_count >= _.bufs.docs.len) return FullTextError.CapacityFull;
        const slot: u32 = @as(u32, @intCast(_.doc_count));
        const fl_off: u32 = @as(u32, @intCast(_.doc_count * _.cfg.field_count));
        let f: usize = 0;
        while (f < _.cfg.field_count) : (f += 1) {
            _.bufs.doc_field_lens[@as(usize, fl_off) + f] = 0;
        }
        _.bufs.docs[_.doc_count] = FtDocMeta{
            doc_id: doc_id,
            deleted: false,
            field_len_offset: fl_off,
            total_tokens: 0,
        };
        _.doc_count += 1;
        _.active_doc_count += 1;
        return slot;
    }

    /// Indexes text into `field_id` of an already-started `doc_slot` using caller-owned `scratch`.
    /// Streams tokens directly into `scratch.term_ids` and `scratch.token_positions` without heap allocation.
    fn addDocumentField(
        *_,
        doc_slot: u32,
        field_id: usize,
        text: []const u8,
        scratch: *FullTextScratch,
    ): usize ! FullTextError {
        if (@as(usize, doc_slot) >= _.doc_count) return FullTextError.DocNotFound;
        if (field_id >= _.cfg.field_count) return FullTextError.InvalidField;
        if (_.bufs.docs[@as(usize, doc_slot)].deleted) return FullTextError.DocNotFound;
        if (scratch.term_ids.len == 0 || scratch.token_positions.len < scratch.term_ids.len) {
            return FullTextError.BufferTooSmall;
        }

        // Stream tokens directly into scratch.term_ids / scratch.token_positions (with SIMD ASCII fast-path)
        const pure_ascii: bool = Simd.firstNonAscii(text) == null;
        let tok_count: usize = 0;
        let logical_tokens: u32 = 0;
        let pos: u32 = 0;
        let i: usize = 0;
        let prev_cjk_start: usize = 0;
        let prev_cjk_len: usize = 0;
        let prev_cjk_pos: u32 = 0;
        let has_prev_cjk: bool = false;

        while (i < text.len) {
            const ws_adv: usize = Simd.skipWhitespace(text[i..text.len]);
            if (ws_adv > 0) {
                has_prev_cjk = false;
                i += ws_adv;
                if (i >= text.len) break;
            }

            if (pure_ascii) {
                const b0: u8 = text[i];
                if (!((b0 >= 'a' && b0 <= 'z') || (b0 >= 'A' && b0 <= 'Z') || (b0 >= '0' && b0 <= '9'))) {
                    i += 1;
                    continue;
                }
                const word_start: usize = i;
                while (i < text.len) : (i += 1) {
                    const c: u8 = text[i];
                    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                        break;
                    }
                }
                const word_end: usize = i;
                let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
                let norm_slice: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
                let norm_len: usize = Simd.toLowerAscii(text[word_start..word_end], norm_slice);
                if (norm_len == 0) continue;
                if (_.cfg.enable_stopwords && isEnglishStopword(norm_arr[0..norm_len])) {
                    pos += 1;
                    continue;
                }
                if (_.cfg.enable_stemming && norm_len > 2) {
                    norm_len = porterLiteStemInPlace(*norm_slice, norm_len);
                }
                if (norm_len > 0) {
                    if (tok_count >= scratch.term_ids.len) return FullTextError.BufferTooSmall;
                    const tid: u32 = try _.internTerm(norm_arr[0..norm_len]);
                    scratch.term_ids[tok_count] = tid;
                    scratch.token_positions[tok_count] = pos;
                    tok_count += 1;
                    logical_tokens += 1;
                }
                pos += 1;
                continue;
            }

            const u8dec: Utf8Decoded = decodeUtf8At(text, i);
            if (u8dec.len == 0) break;

            if (isUnicodeSeparatorOrPunct(u8dec.cp)) {
                has_prev_cjk = false;
                i += u8dec.len;
                continue;
            }

            if (isCjkCodepoint(u8dec.cp)) {
                const cur_start: usize = i;
                const cur_len: usize = u8dec.len;
                const cur_pos: u32 = pos;

                if (tok_count >= scratch.term_ids.len) return FullTextError.BufferTooSmall;
                const tid_uni: u32 = try _.internTerm(text[cur_start..cur_start + cur_len]);
                scratch.term_ids[tok_count] = tid_uni;
                scratch.token_positions[tok_count] = cur_pos;
                tok_count += 1;
                logical_tokens += 1;

                if (_.cfg.enable_cjk_bigrams && has_prev_cjk) {
                    const bi_len: usize = prev_cjk_len + cur_len;
                    if (bi_len <= FT_MAX_TOKEN_BYTES) {
                        if (tok_count >= scratch.term_ids.len) return FullTextError.BufferTooSmall;
                        let bi_buf: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
                        let k: usize = 0;
                        while (k < prev_cjk_len) : (k += 1) {
                            bi_buf[k] = text[prev_cjk_start + k];
                        }
                        let m: usize = 0;
                        while (m < cur_len) : (m += 1) {
                            bi_buf[prev_cjk_len + m] = text[cur_start + m];
                        }
                        const tid_bi: u32 = try _.internTerm(bi_buf[0..bi_len]);
                        scratch.term_ids[tok_count] = tid_bi;
                        scratch.token_positions[tok_count] = prev_cjk_pos;
                        tok_count += 1;
                    }
                }

                prev_cjk_start = cur_start;
                prev_cjk_len = cur_len;
                prev_cjk_pos = cur_pos;
                has_prev_cjk = true;
                pos += 1;
                i += cur_len;
                continue;
            }

            has_prev_cjk = false;
            let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
            let norm_len: usize = 0;
            while (i < text.len) {
                const wdec: Utf8Decoded = decodeUtf8At(text, i);
                if (wdec.len == 0) break;
                if (isUnicodeSeparatorOrPunct(wdec.cp) || isCjkCodepoint(wdec.cp)) break;
                if (wdec.len == 1) {
                    if (norm_len < FT_MAX_TOKEN_BYTES) {
                        norm_arr[norm_len] = foldAsciiLower(text[i]);
                        norm_len += 1;
                    }
                } else {
                    if (norm_len + wdec.len <= FT_MAX_TOKEN_BYTES) {
                        let k: usize = 0;
                        while (k < wdec.len) : (k += 1) {
                            norm_arr[norm_len + k] = text[i + k];
                        }
                        norm_len += wdec.len;
                    }
                }
                i += wdec.len;
            }

            if (norm_len == 0) continue;
            if (_.cfg.enable_stopwords && isEnglishStopword(norm_arr[0..norm_len])) {
                pos += 1;
                continue;
            }
            if (_.cfg.enable_stemming && norm_len > 2) {
                let norm_slice: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
                norm_len = porterLiteStemInPlace(*norm_slice, norm_len);
            }
            if (norm_len > 0) {
                if (tok_count >= scratch.term_ids.len) return FullTextError.BufferTooSmall;
                const tid: u32 = try _.internTerm(norm_arr[0..norm_len]);
                scratch.term_ids[tok_count] = tid;
                scratch.token_positions[tok_count] = pos;
                tok_count += 1;
                logical_tokens += 1;
            }
            pos += 1;
        }

        // Update field length stats
        const fl_idx: usize = @as(usize, _.bufs.docs[@as(usize, doc_slot)].field_len_offset) + field_id;
        _.bufs.doc_field_lens[fl_idx] += logical_tokens;
        _.bufs.field_len_sums[field_id] += @as(u64, logical_tokens);
        _.bufs.docs[@as(usize, doc_slot)].total_tokens += logical_tokens;

        if (tok_count == 0) return 0;

        // Sort (term_id, position) pairs in place by (term_id ASC, position ASC) using heapsort
        sortTermPosPairs(*scratch.term_ids, *scratch.token_positions, tok_count);

        // Group contiguous runs of identical term_id and emit one FtPosting per unique term
        let run_start: usize = 0;
        while (run_start < tok_count) {
            const tid: u32 = scratch.term_ids[run_start];
            let run_end: usize = run_start + 1;
            while (run_end < tok_count && scratch.term_ids[run_end] == tid) : (run_end += 1) {}
            const run_len: usize = run_end - run_start;

            if (_.posting_count >= _.bufs.postings.len) return FullTextError.CapacityFull;
            if (_.position_count + run_len > _.bufs.positions.len) return FullTextError.CapacityFull;

            const pos_off: u32 = @as(u32, @intCast(_.position_count));
            let k: usize = 0;
            while (k < run_len) : (k += 1) {
                _.bufs.positions[_.position_count + k] = scratch.token_positions[run_start + k];
            }
            _.position_count += run_len;

            const tidx_us: usize = @as(usize, tid);
            const already_in_doc: bool = _.termHasDoc(tidx_us, doc_slot);
            if (!already_in_doc) {
                _.bufs.terms[tidx_us].df += 1;
            }

            const p_idx: u32 = @as(u32, @intCast(_.posting_count));
            _.bufs.postings[_.posting_count] = FtPosting{
                doc_slot: doc_slot,
                field_id: @as(u16, @intCast(field_id)),
                tf: @as(u32, @intCast(run_len)),
                pos_offset: pos_off,
                pos_len: @as(u32, @intCast(run_len)),
                next_posting: FT_INVALID_IDX,
            };
            _.posting_count += 1;

            if (_.bufs.terms[tidx_us].head_posting == FT_INVALID_IDX) {
                _.bufs.terms[tidx_us].head_posting = p_idx;
                _.bufs.terms[tidx_us].tail_posting = p_idx;
            } else {
                const prev_tail: usize = @as(usize, _.bufs.terms[tidx_us].tail_posting);
                _.bufs.postings[prev_tail].next_posting = p_idx;
                _.bufs.terms[tidx_us].tail_posting = p_idx;
            }

            run_start = run_end;
        }

        return @as(usize, logical_tokens);
    }

    /// Convenience helper to index a complete multi-field document in a single call.
    fn addDocument(
        *_,
        doc_id: u64,
        fields: []const []const u8,
        scratch: *FullTextScratch,
    ): u32 ! FullTextError {
        if (fields.len > _.cfg.field_count) return FullTextError.InvalidField;
        const slot: u32 = try _.beginDocument(doc_id);
        let f: usize = 0;
        while (f < fields.len) : (f += 1) {
            * = try _.addDocumentField(slot, f, fields[f], scratch);
        }
        return slot;
    }

    /// Soft-deletes the document with `doc_id` using a tombstone, updating active doc count,
    /// per-field length sums, and exact dictionary document frequencies (`df`).
    fn markDeleted(*_, doc_id: u64): bool {
        let found_slot: u32 = FT_INVALID_IDX;
        let i: usize = 0;
        while (i < _.doc_count) : (i += 1) {
            if (!_.bufs.docs[i].deleted && _.bufs.docs[i].doc_id == doc_id) {
                found_slot = @as(u32, @intCast(i));
                break;
            }
        }
        if (found_slot == FT_INVALID_IDX) return false;

        const d_us: usize = @as(usize, found_slot);
        _.bufs.docs[d_us].deleted = true;
        if (_.active_doc_count > 0) _.active_doc_count -= 1;

        const fl_off: usize = @as(usize, _.bufs.docs[d_us].field_len_offset);
        let f: usize = 0;
        while (f < _.cfg.field_count) : (f += 1) {
            const fl: u64 = @as(u64, _.bufs.doc_field_lens[fl_off + f]);
            if (_.bufs.field_len_sums[f] >= fl) {
                _.bufs.field_len_sums[f] -= fl;
            } else {
                _.bufs.field_len_sums[f] = 0;
            }
        }

        // Decrement df for every term that appeared in this document
        let t: usize = 0;
        while (t < _.term_count) : (t += 1) {
            if (_.bufs.terms[t].df > 0 && _.termHasDoc(t, found_slot)) {
                _.bufs.terms[t].df -= 1;
            }
        }
        return true;
    }

    /// Returns `true` iff `doc_id` exists in the index and is marked deleted.
    fn isDeleted(doc_id: u64): bool {
        let i: usize = 0;
        while (i < doc_count) : (i += 1) {
            if (bufs.docs[i].doc_id == doc_id) {
                return bufs.docs[i].deleted;
            }
        }
        return false;
    }

    /// Accumulates BM25F score for `term_idx` (scaled by `boost`) into `scratch.scores`
    /// and sets `doc_mask[doc_slot] = 1` for every matching active document.
    fn scoreTermInto(
        term_idx: usize,
        boost: f64,
        out_scores: *[]f64,
        out_mask: *[]u8,
        out_matched: *[]u32,
    ) {
        if (term_idx >= term_count || active_doc_count == 0) return;
        const entry: FtTermEntry = bufs.terms[term_idx];
        if (entry.df == 0 || entry.head_posting == FT_INVALID_IDX) return;

        const idf: f64 = computeBm25fIdf(active_doc_count, @as(usize, entry.df));
        if (idf <= 0.0) return;

        // Postings are appended in non-decreasing doc_slot order.
        // Group consecutive postings with the same doc_slot to aggregate multi-field pseudo_tf.
        let p_idx: u32 = entry.head_posting;
        while (p_idx != FT_INVALID_IDX) {
            const first_p: FtPosting = bufs.postings[@as(usize, p_idx)];
            const d_slot: u32 = first_p.doc_slot;
            const d_us: usize = @as(usize, d_slot);

            let pseudo_tf: f64 = 0.0;
            let cur_idx: u32 = p_idx;
            while (cur_idx != FT_INVALID_IDX) {
                const cp: FtPosting = bufs.postings[@as(usize, cur_idx)];
                if (cp.doc_slot != d_slot) break;
                if (!bufs.docs[d_us].deleted) {
                    const fid: usize = @as(usize, cp.field_id);
                    const fspec: FullTextFieldSpec = bufs.field_specs[fid];
                    if (fspec.weight > 0.0 && cp.tf > 0) {
                        const fl_off: usize = @as(usize, bufs.docs[d_us].field_len_offset);
                        const dl: f64 = @as(f64, @floatFromInt(bufs.doc_field_lens[fl_off + fid]));
                        const avgdl: f64 = avgFieldLen(fid);
                        const norm: f64 = if (avgdl > 0.0)
                            (1.0 - fspec.b) + fspec.b * (dl / avgdl)
                        else
                            1.0;
                        const safe_norm: f64 = if (norm > 1.0e-12) norm else 1.0e-12;
                        pseudo_tf += fspec.weight * (@as(f64, @floatFromInt(cp.tf)) / safe_norm);
                    }
                }
                cur_idx = cp.next_posting;
            }

            if (!bufs.docs[d_us].deleted && pseudo_tf > 0.0) {
                out_mask[d_us] = 1;
                if (boost > 0.0) {
                    const s: f64 = computeBm25fTermScore(idf, pseudo_tf, cfg.k1) * boost;
                    out_scores[d_us] += s;
                    out_matched[d_us] += 1;
                }
            }

            p_idx = cur_idx;
        }
    }

    /// Finds the posting for `(term_idx, doc_slot, field_id)` if present.
    fn findPostingForDocField(term_idx: usize, doc_slot: u32, field_id: u16): u32? {
        let p_idx: u32 = bufs.terms[term_idx].head_posting;
        while (p_idx != FT_INVALID_IDX) {
            const p: FtPosting = bufs.postings[@as(usize, p_idx)];
            if (p.doc_slot == doc_slot && p.field_id == field_id) return p_idx;
            if (p.doc_slot > doc_slot) return null;
            p_idx = p.next_posting;
        }
        return null;
    }

    /// Verifies whether `phrase_tids[0..phrase_len]` occurs in `(doc_slot, field_id)` within `slop`.
    /// Uses positional sliding-window verification:
    ///   - `slop == 0`: exact consecutive positions `p, p+1, ..., p+(m-1)`
    ///   - `slop > 0`: ordered proximity `(p_{m-1} - p_0) - (m - 1) <= slop` or
    ///     adjacent transposition window within `slop` moves
    fn matchPhraseInDocField(
        phrase_tids: []const u32,
        doc_slot: u32,
        field_id: u16,
        slop: u32,
    ): bool {
        if (phrase_tids.len == 0) return false;
        if (phrase_tids.len == 1) {
            return findPostingForDocField(@as(usize, phrase_tids[0]), doc_slot, field_id) != null;
        }

        // Gather posting indices for all terms in the phrase for this (doc_slot, field_id)
        let p_indices: [FT_MAX_PHRASE_TERMS]u32 = [FT_INVALID_IDX; FT_MAX_PHRASE_TERMS];
        const m: usize = if (phrase_tids.len > FT_MAX_PHRASE_TERMS) FT_MAX_PHRASE_TERMS else phrase_tids.len;
        let i: usize = 0;
        while (i < m) : (i += 1) {
            if (pi := findPostingForDocField(@as(usize, phrase_tids[i]), doc_slot, field_id)) {
                p_indices[i] = pi;
            } else {
                return false;
            }
        }

        // Exact phrase (slop == 0): binary search p + i for every starting position p in term 0
        const p0: FtPosting = bufs.postings[@as(usize, p_indices[0])];
        const p0_off: usize = @as(usize, p0.pos_offset);
        const p0_len: usize = @as(usize, p0.pos_len);
        if (slop == 0) {
            let k: usize = 0;
            while (k < p0_len) : (k += 1) {
                const start_pos: u32 = bufs.positions[p0_off + k];
                let ok: bool = true;
                let j: usize = 1;
                while (j < m) : (j += 1) {
                    const pj: FtPosting = bufs.postings[@as(usize, p_indices[j])];
                    const target: u32 = start_pos + @as(u32, @intCast(j));
                    if (!posSliceContains(bufs.positions[@as(usize, pj.pos_offset)..@as(usize, pj.pos_offset + pj.pos_len)], target)) {
                        ok = false;
                        break;
                    }
                }
                if (ok) return true;
            }
            return false;
        }

        // Slop > 0: ordered greedy closest-next verification from every anchor in term 0,
        // plus unordered bounding-span check for transpositions (each inversion costs +2 slop).
        let k: usize = 0;
        while (k < p0_len) : (k += 1) {
            const first_pos: u32 = bufs.positions[p0_off + k];
            let prev_pos: u32 = first_pos;
            let ordered_ok: bool = true;
            let j: usize = 1;
            while (j < m) : (j += 1) {
                const pj: FtPosting = bufs.postings[@as(usize, p_indices[j])];
                const slice_j: []const u32 = bufs.positions[@as(usize, pj.pos_offset)..@as(usize, pj.pos_offset + pj.pos_len)];
                if (next_p := posSliceLowerBound(slice_j, prev_pos + 1)) {
                    prev_pos = next_p;
                } else {
                    ordered_ok = false;
                    break;
                }
            }
            if (ordered_ok) {
                const span: u32 = prev_pos - first_pos;
                const min_span: u32 = @as(u32, @intCast(m - 1));
                if (span >= min_span && (span - min_span) <= slop) {
                    return true;
                }
            }

            // Unordered proximity window around first_pos (supports swapped terms when slop >= 2)
            if (slop >= 2) {
                let min_p: u32 = first_pos;
                let max_p: u32 = first_pos;
                let chosen: [FT_MAX_PHRASE_TERMS]u32 = [0; FT_MAX_PHRASE_TERMS];
                chosen[0] = first_pos;
                let win_ok: bool = true;
                j = 1;
                while (j < m) : (j += 1) {
                    const pj: FtPosting = bufs.postings[@as(usize, p_indices[j])];
                    const slice_j: []const u32 = bufs.positions[@as(usize, pj.pos_offset)..@as(usize, pj.pos_offset + pj.pos_len)];
                    if (cp := posSliceClosest(slice_j, first_pos)) {
                        chosen[j] = cp;
                        if (cp < min_p) min_p = cp;
                        if (cp > max_p) max_p = cp;
                    } else {
                        win_ok = false;
                        break;
                    }
                }
                if (win_ok) {
                    let inversions: u32 = 0;
                    let a: usize = 0;
                    while (a < m) : (a += 1) {
                        let b: usize = a + 1;
                        while (b < m) : (b += 1) {
                            if (chosen[a] > chosen[b]) inversions += 1;
                        }
                    }
                    const span_u: u32 = max_p - min_p;
                    const min_span: u32 = @as(u32, @intCast(m - 1));
                    const gap_cost: u32 = if (span_u > min_span) span_u - min_span else 0;
                    if (gap_cost + inversions * 2 <= slop) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /// Evaluates a phrase query with `slop` across all fields, populating `out_mask` and `out_scores`.
    fn evalPhraseInto(
        phrase_text: []const u8,
        slop: u32,
        score_scale: f64,
        out_scores: *[]f64,
        out_mask: *[]u8,
        out_matched: *[]u32,
    ) {
        let spans: [FT_MAX_PHRASE_TERMS]FtTokenSpan = [FtTokenSpan{}; FT_MAX_PHRASE_TERMS];
        let span_slice: []FtTokenSpan = spans[0..FT_MAX_PHRASE_TERMS];
        const raw_cnt: usize = tokenizeText(
            phrase_text,
            cfg.enable_stemming,
            cfg.enable_stopwords,
            false,
            *span_slice,
        );
        if (raw_cnt == 0) return;

        let tids: [FT_MAX_PHRASE_TERMS]u32 = [0; FT_MAX_PHRASE_TERMS];
        let i: usize = 0;
        while (i < raw_cnt) : (i += 1) {
            if (tid := findTerm(spans[i].text())) {
                tids[i] = tid;
            } else {
                return; // Missing any phrase term => zero matches
            }
        }

        // Iterate candidate postings of first term
        const first_tid: usize = @as(usize, tids[0]);
        let p_idx: u32 = bufs.terms[first_tid].head_posting;
        while (p_idx != FT_INVALID_IDX) {
            const p: FtPosting = bufs.postings[@as(usize, p_idx)];
            const d_us: usize = @as(usize, p.doc_slot);
            if (!bufs.docs[d_us].deleted && out_mask[d_us] == 0) {
                if (matchPhraseInDocField(tids[0..raw_cnt], p.doc_slot, p.field_id, slop)) {
                    out_mask[d_us] = 1;
                    // Score phrase match as sum of constituent term BM25F scores for this doc
                    let total_s: f64 = 0.0;
                    let t_i: usize = 0;
                    while (t_i < raw_cnt) : (t_i += 1) {
                        const tid_u: usize = @as(usize, tids[t_i]);
                        const idf: f64 = computeBm25fIdf(active_doc_count, @as(usize, bufs.terms[tid_u].df));
                        let ptf: f64 = 0.0;
                        let f: usize = 0;
                        while (f < cfg.field_count) : (f += 1) {
                            if (fp_idx := findPostingForDocField(tid_u, p.doc_slot, @as(u16, @intCast(f)))) {
                                const fp: FtPosting = bufs.postings[@as(usize, fp_idx)];
                                const fspec: FullTextFieldSpec = bufs.field_specs[f];
                                const fl_off: usize = @as(usize, bufs.docs[d_us].field_len_offset);
                                const dl: f64 = @as(f64, @floatFromInt(bufs.doc_field_lens[fl_off + f]));
                                const avgdl: f64 = avgFieldLen(f);
                                const norm: f64 = if (avgdl > 0.0) (1.0 - fspec.b) + fspec.b * (dl / avgdl) else 1.0;
                                ptf += fspec.weight * (@as(f64, @floatFromInt(fp.tf)) / norm);
                            }
                        }
                        total_s += computeBm25fTermScore(idf, ptf, cfg.k1);
                    }
                    if (score_scale > 0.0) {
                        out_scores[d_us] += total_s * score_scale;
                        out_matched[d_us] += @as(u32, @intCast(raw_cnt));
                    }
                }
            }
            p_idx = p.next_posting;
        }
    }

    /// Evaluates a prefix query (`prefix*`) by scanning the dictionary for matching terms.
    fn evalPrefixInto(
        raw_prefix: []const u8,
        score_scale: f64,
        out_scores: *[]f64,
        out_mask: *[]u8,
        out_matched: *[]u32,
    ) {
        let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
        let norm_slice: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
        const plen: usize = normalizeSingleTerm(raw_prefix, false, *norm_slice);
        if (plen == 0) return;
        const prefix: []const u8 = norm_arr[0..plen];

        let t: usize = 0;
        while (t < term_count) : (t += 1) {
            if (bufs.terms[t].df == 0) continue;
            if (bytesStartsWith(termText(t), prefix)) {
                scoreTermInto(t, score_scale, out_scores, out_mask, out_matched);
            }
        }
    }

    /// Evaluates a fuzzy query (`term~max_edits`) via Damerau-Levenshtein dictionary scan.
    fn evalFuzzyInto(
        raw_term: []const u8,
        max_edits: usize,
        score_scale: f64,
        out_scores: *[]f64,
        out_mask: *[]u8,
        out_matched: *[]u32,
    ) {
        let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
        let norm_slice: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
        const qlen: usize = normalizeSingleTerm(raw_term, false, *norm_slice);
        if (qlen == 0) return;
        const qterm: []const u8 = norm_arr[0..qlen];

        let stem_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
        let stem_slice: []u8 = stem_arr[0..FT_MAX_TOKEN_BYTES];
        const slen: usize = normalizeSingleTerm(raw_term, cfg.enable_stemming, *stem_slice);
        const sterm: []const u8 = stem_arr[0..slen];

        const clamped_edits: usize = if (max_edits > 2) 2 else max_edits;
        let t: usize = 0;
        while (t < term_count) : (t += 1) {
            if (bufs.terms[t].df == 0) continue;
            const dict_t: []const u8 = termText(t);
            const d1: usize = damerauLevenshteinBounded(qterm, dict_t, clamped_edits);
            const d2: usize = if (cfg.enable_stemming)
                damerauLevenshteinBounded(sterm, dict_t, clamped_edits)
            else
                d1;
            const dist: usize = if (d1 < d2) d1 else d2;
            if (dist <= clamped_edits) {
                const max_len: usize = if (qlen > dict_t.len) qlen else dict_t.len;
                const penalty: f64 = if (max_len > 0)
                    1.0 - (@as(f64, @floatFromInt(dist)) / @as(f64, @floatFromInt(max_len + 1)))
                else
                    1.0;
                const boost: f64 = (if (penalty > 0.2) penalty else 0.2) * score_scale;
                scoreTermInto(t, boost, out_scores, out_mask, out_matched);
            }
        }
    }

    /// Evaluates a plain term (which may tokenize into multiple CJK unigrams/bigrams or a stemmed word).
    fn evalPlainTermInto(
        raw_term: []const u8,
        score_scale: f64,
        out_scores: *[]f64,
        out_mask: *[]u8,
        out_matched: *[]u32,
    ) {
        let spans: [FT_MAX_PHRASE_TERMS]FtTokenSpan = [FtTokenSpan{}; FT_MAX_PHRASE_TERMS];
        let span_slice: []FtTokenSpan = spans[0..FT_MAX_PHRASE_TERMS];
        const cnt: usize = tokenizeText(
            raw_term,
            cfg.enable_stemming,
            cfg.enable_stopwords,
            cfg.enable_cjk_bigrams,
            *span_slice,
        );
        let i: usize = 0;
        while (i < cnt) : (i += 1) {
            if (tid := findTerm(spans[i].text())) {
                const boost: f64 = (if (spans[i].is_bigram) 1.25 else 1.0) * score_scale;
                scoreTermInto(@as(usize, tid), boost, out_scores, out_mask, out_matched);
            }
        }
    }

    /// Collects top-K active documents where `doc_mask[d] != 0`, sorted by `(score DESC, doc_id ASC)`.
    fn collectTopKHits(
        scores: []const f64,
        doc_mask: []const u8,
        matched_cnt: []const u32,
        out_hits: *[]FtHit,
    ): usize {
        let count: usize = 0;
        let d: usize = 0;
        while (d < doc_count) : (d += 1) {
            if (bufs.docs[d].deleted || doc_mask[d] == 0) continue;
            const cand: FtHit = FtHit{
                doc_id: bufs.docs[d].doc_id,
                doc_slot: @as(u32, @intCast(d)),
                score: scores[d],
                matched_terms: matched_cnt[d],
            };
            insertTopKHit(out_hits, *count, cand);
        }
        return count;
    }

    /// Executes a standalone phrase / proximity search (`"phrase"~slop`) and returns top-K hits.
    fn searchPhrase(
        phrase_text: []const u8,
        slop: u32,
        scratch: *FullTextScratch,
        out_hits: *[]FtHit,
    ): usize ! FullTextError {
        try validateQueryScratch(doc_count, scratch);
        zeroQueryArrays(doc_count, scratch);
        evalPhraseInto(phrase_text, slop, 1.0, *scratch.scores, *scratch.doc_mask, *scratch.matched_terms_cnt);
        return collectTopKHits(scratch.scores, scratch.doc_mask, scratch.matched_terms_cnt, out_hits);
    }

    /// Executes a standalone prefix search (`prefix*`) and returns top-K hits.
    fn searchPrefix(
        prefix: []const u8,
        scratch: *FullTextScratch,
        out_hits: *[]FtHit,
    ): usize ! FullTextError {
        try validateQueryScratch(doc_count, scratch);
        zeroQueryArrays(doc_count, scratch);
        evalPrefixInto(prefix, 1.0, *scratch.scores, *scratch.doc_mask, *scratch.matched_terms_cnt);
        return collectTopKHits(scratch.scores, scratch.doc_mask, scratch.matched_terms_cnt, out_hits);
    }

    /// Executes a standalone Damerau-Levenshtein fuzzy search (`max_edits` in `1..=2`) and returns top-K hits.
    fn searchFuzzy(
        term: []const u8,
        max_edits: usize,
        scratch: *FullTextScratch,
        out_hits: *[]FtHit,
    ): usize ! FullTextError {
        try validateQueryScratch(doc_count, scratch);
        zeroQueryArrays(doc_count, scratch);
        evalFuzzyInto(term, max_edits, 1.0, *scratch.scores, *scratch.doc_mask, *scratch.matched_terms_cnt);
        return collectTopKHits(scratch.scores, scratch.doc_mask, scratch.matched_terms_cnt, out_hits);
    }

    /// Executes a full query string supporting boolean operators (`AND`, `OR`, `NOT`),
    /// arbitrary parentheses `( ... )`, unary `+`/`-`, exact/slop phrases (`"a b"~2`),
    /// prefix (`comp*`), and fuzzy (`term~1|2`) clauses with zero heap allocation.
    fn searchQuery(
        query: []const u8,
        scratch: *FullTextScratch,
        out_hits: *[]FtHit,
    ): usize ! FullTextError {
        try validateQueryScratch(doc_count, scratch);
        zeroQueryArrays(doc_count, scratch);
        if (doc_count == 0 || active_doc_count == 0) return 0;

        let parser: QueryParser = QueryParser{
            src: query,
            pos: 0,
            depth: 0,
            aux_offset: 0,
        };
        parser.skipSpaces();
        if (parser.pos >= parser.src.len) return 0;

        const has_expr: bool = try evalOrExpr(
            _,
            *parser,
            *scratch.scores,
            *scratch.doc_mask,
            *scratch.matched_terms_cnt,
            *scratch.doc_mask_aux,
        );
        if (!has_expr) return 0;
        parser.skipSpaces();
        if (parser.pos < parser.src.len) {
            return FullTextError.InvalidQuery;
        }
        return collectTopKHits(scratch.scores, scratch.doc_mask, scratch.matched_terms_cnt, out_hits);
    }

    /// Returns the exact byte size required to serialize this index with CRC32 trailer.
    fn snapshotByteSize(): usize {
        const specs_bytes: usize = cfg.field_count * 16;
        const sums_bytes: usize = cfg.field_count * 8;
        const docs_bytes: usize = doc_count * 20;
        const dfl_bytes: usize = doc_count * cfg.field_count * 4;
        const terms_bytes_meta: usize = term_count * 28;
        const postings_bytes: usize = posting_count * 24;
        const positions_bytes: usize = position_count * 4;
        return FT_SNAPSHOT_HEADER_BYTES +
            specs_bytes +
            sums_bytes +
            docs_bytes +
            dfl_bytes +
            terms_bytes_meta +
            term_bytes_used +
            postings_bytes +
            positions_bytes +
            FT_SNAPSHOT_CRC_BYTES;
    }

    /// Serializes the entire inverted index into `out` with an IEEE CRC32 integrity trailer.
    fn serializeSnapshot(out: *[]u8): usize ! FullTextError {
        const total_bytes: usize = snapshotByteSize();
        if (out.len < total_bytes) return FullTextError.BufferTooSmall;

        let off: usize = 0;
        writeU32Le(out, off + 0, FT_SNAPSHOT_MAGIC);
        writeU32Le(out, off + 4, FT_SNAPSHOT_VERSION);
        const flags: u32 = (if (cfg.enable_stemming) @as(u32, 1) else 0) |
            (if (cfg.enable_stopwords) @as(u32, 2) else 0) |
            (if (cfg.enable_cjk_bigrams) @as(u32, 4) else 0);
        writeU32Le(out, off + 8, flags);
        writeU32Le(out, off + 12, @as(u32, @intCast(cfg.field_count)));
        writeF64Le(out, off + 16, cfg.k1);
        writeU32Le(out, off + 24, @as(u32, @intCast(doc_count)));
        writeU32Le(out, off + 28, @as(u32, @intCast(active_doc_count)));
        writeU32Le(out, off + 32, @as(u32, @intCast(term_count)));
        writeU32Le(out, off + 36, @as(u32, @intCast(posting_count)));
        writeU32Le(out, off + 40, @as(u32, @intCast(position_count)));
        writeU32Le(out, off + 44, @as(u32, @intCast(term_bytes_used)));
        writeU64Le(out, off + 48, 0);
        writeU64Le(out, off + 56, 0);
        off += FT_SNAPSHOT_HEADER_BYTES;

        // Field specs
        let f: usize = 0;
        while (f < cfg.field_count) : (f += 1) {
            writeF64Le(out, off, bufs.field_specs[f].weight);
            writeF64Le(out, off + 8, bufs.field_specs[f].b);
            off += 16;
        }
        // Field length sums
        f = 0;
        while (f < cfg.field_count) : (f += 1) {
            writeU64Le(out, off, bufs.field_len_sums[f]);
            off += 8;
        }
        // Docs
        let d: usize = 0;
        while (d < doc_count) : (d += 1) {
            const dm: FtDocMeta = bufs.docs[d];
            writeU64Le(out, off, dm.doc_id);
            writeU32Le(out, off + 8, if (dm.deleted) 1 else 0);
            writeU32Le(out, off + 12, dm.field_len_offset);
            writeU32Le(out, off + 16, dm.total_tokens);
            off += 20;
        }
        // Doc field lens
        const dfl_total: usize = doc_count * cfg.field_count;
        let i: usize = 0;
        while (i < dfl_total) : (i += 1) {
            writeU32Le(out, off, bufs.doc_field_lens[i]);
            off += 4;
        }
        // Terms metadata
        let t: usize = 0;
        while (t < term_count) : (t += 1) {
            const te: FtTermEntry = bufs.terms[t];
            writeU64Le(out, off, te.hash);
            writeU32Le(out, off + 8, te.text_offset);
            writeU32Le(out, off + 12, @as(u32, te.text_len));
            writeU32Le(out, off + 16, te.df);
            writeU32Le(out, off + 20, te.head_posting);
            writeU32Le(out, off + 24, te.tail_posting);
            off += 28;
        }
        // Term bytes arena
        i = 0;
        while (i < term_bytes_used) : (i += 1) {
            out[off + i] = bufs.term_bytes[i];
        }
        off += term_bytes_used;
        // Postings
        let p: usize = 0;
        while (p < posting_count) : (p += 1) {
            const pe: FtPosting = bufs.postings[p];
            writeU32Le(out, off, pe.doc_slot);
            writeU32Le(out, off + 4, @as(u32, pe.field_id));
            writeU32Le(out, off + 8, pe.tf);
            writeU32Le(out, off + 12, pe.pos_offset);
            writeU32Le(out, off + 16, pe.pos_len);
            writeU32Le(out, off + 20, pe.next_posting);
            off += 24;
        }
        // Positions
        i = 0;
        while (i < position_count) : (i += 1) {
            writeU32Le(out, off, bufs.positions[i]);
            off += 4;
        }

        const crc: u32 = crc32Ieee(out[0..off]);
        writeU32Le(out, off, crc);
        off += FT_SNAPSHOT_CRC_BYTES;
        return off;
    }

    /// Deserializes and verifies an IEEE CRC32 binary snapshot into caller-provided `bufs`.
    static fn deserializeSnapshot(data: []const u8, bufs: FullTextBuffers): FullTextIndex ! FullTextError {
        if (data.len < FT_SNAPSHOT_HEADER_BYTES + FT_SNAPSHOT_CRC_BYTES) {
            return FullTextError.InvalidSnapshot;
        }
        const magic: u32 = readU32Le(data, 0);
        const version: u32 = readU32Le(data, 4);
        if (magic != FT_SNAPSHOT_MAGIC || version != FT_SNAPSHOT_VERSION) {
            return FullTextError.InvalidSnapshot;
        }

        const flags: u32 = readU32Le(data, 8);
        const field_count: usize = @as(usize, readU32Le(data, 12));
        const k1: f64 = readF64Le(data, 16);
        const doc_cnt: usize = @as(usize, readU32Le(data, 24));
        const active_cnt: usize = @as(usize, readU32Le(data, 28));
        const term_cnt: usize = @as(usize, readU32Le(data, 32));
        const posting_cnt: usize = @as(usize, readU32Le(data, 36));
        const pos_cnt: usize = @as(usize, readU32Le(data, 40));
        const tb_used: usize = @as(usize, readU32Le(data, 44));

        const cfg: FullTextConfig = FullTextConfig{
            k1: k1,
            field_count: field_count,
            enable_stemming: (flags & 1) != 0,
            enable_stopwords: (flags & 2) != 0,
            enable_cjk_bigrams: (flags & 4) != 0,
        };
        if (!cfg.isValid() || active_cnt > doc_cnt) {
            return FullTextError.InvalidSnapshot;
        }

        const expected_len: usize = FT_SNAPSHOT_HEADER_BYTES +
            field_count * 16 +
            field_count * 8 +
            doc_cnt * 20 +
            doc_cnt * field_count * 4 +
            term_cnt * 28 +
            tb_used +
            posting_cnt * 24 +
            pos_cnt * 4 +
            FT_SNAPSHOT_CRC_BYTES;
        if (data.len < expected_len) {
            return FullTextError.InvalidSnapshot;
        }

        const payload_len: usize = expected_len - FT_SNAPSHOT_CRC_BYTES;
        const stored_crc: u32 = readU32Le(data, payload_len);
        const computed_crc: u32 = crc32Ieee(data[0..payload_len]);
        if (stored_crc != computed_crc) {
            return FullTextError.ChecksumMismatch;
        }

        // Validate destination buffer capacities
        if (bufs.field_specs.len < field_count || bufs.field_len_sums.len < field_count) {
            return FullTextError.BufferTooSmall;
        }
        if (bufs.docs.len < doc_cnt || bufs.doc_field_lens.len < doc_cnt * field_count) {
            return FullTextError.BufferTooSmall;
        }
        if (bufs.terms.len < term_cnt || bufs.term_hash_table.len <= term_cnt) {
            return FullTextError.BufferTooSmall;
        }
        if (bufs.term_bytes.len < tb_used || bufs.postings.len < posting_cnt || bufs.positions.len < pos_cnt) {
            return FullTextError.BufferTooSmall;
        }

        let idx: FullTextIndex = FullTextIndex{
            cfg: cfg,
            bufs: bufs,
            doc_count: doc_cnt,
            active_doc_count: active_cnt,
            term_count: term_cnt,
            posting_count: posting_cnt,
            position_count: pos_cnt,
            term_bytes_used: tb_used,
        };

        let off: usize = FT_SNAPSHOT_HEADER_BYTES;
        let f: usize = 0;
        while (f < field_count) : (f += 1) {
            idx.bufs.field_specs[f] = FullTextFieldSpec{
                weight: readF64Le(data, off),
                b: readF64Le(data, off + 8),
            };
            off += 16;
        }
        f = 0;
        while (f < field_count) : (f += 1) {
            idx.bufs.field_len_sums[f] = readU64Le(data, off);
            off += 8;
        }
        let d: usize = 0;
        while (d < doc_cnt) : (d += 1) {
            idx.bufs.docs[d] = FtDocMeta{
                doc_id: readU64Le(data, off),
                deleted: readU32Le(data, off + 8) != 0,
                field_len_offset: readU32Le(data, off + 12),
                total_tokens: readU32Le(data, off + 16),
            };
            off += 20;
        }
        const dfl_total: usize = doc_cnt * field_count;
        let i: usize = 0;
        while (i < dfl_total) : (i += 1) {
            idx.bufs.doc_field_lens[i] = readU32Le(data, off);
            off += 4;
        }
        let t: usize = 0;
        while (t < term_cnt) : (t += 1) {
            idx.bufs.terms[t] = FtTermEntry{
                hash: readU64Le(data, off),
                text_offset: readU32Le(data, off + 8),
                text_len: @as(u16, @intCast(readU32Le(data, off + 12))),
                df: readU32Le(data, off + 16),
                head_posting: readU32Le(data, off + 20),
                tail_posting: readU32Le(data, off + 24),
            };
            off += 28;
        }
        i = 0;
        while (i < tb_used) : (i += 1) {
            idx.bufs.term_bytes[i] = data[off + i];
        }
        off += tb_used;
        let p: usize = 0;
        while (p < posting_cnt) : (p += 1) {
            idx.bufs.postings[p] = FtPosting{
                doc_slot: readU32Le(data, off),
                field_id: @as(u16, @intCast(readU32Le(data, off + 4))),
                tf: readU32Le(data, off + 8),
                pos_offset: readU32Le(data, off + 12),
                pos_len: readU32Le(data, off + 16),
                next_posting: readU32Le(data, off + 20),
            };
            off += 24;
        }
        i = 0;
        while (i < pos_cnt) : (i += 1) {
            idx.bufs.positions[i] = readU32Le(data, off);
            off += 4;
        }

        try idx.rebuildHashTable();
        return idx;
    }
}

// ============================================================================
// 6. Internal Sorting, Positional Binary Search & Recursive-Descent Query Engine
// ============================================================================

fn pairLess(tids: []const u32, pos: []const u32, a: usize, b: usize): bool {
    if (tids[a] != tids[b]) return tids[a] < tids[b];
    return pos[a] < pos[b];
}

fn siftDownPair(tids: *[]u32, pos: *[]u32, start: usize, end: usize) {
    let root: usize = start;
    while (root * 2 + 1 < end) {
        let child: usize = root * 2 + 1;
        let swap_idx: usize = root;
        if (pairLess(tids, pos, swap_idx, child)) swap_idx = child;
        if (child + 1 < end && pairLess(tids, pos, swap_idx, child + 1)) swap_idx = child + 1;
        if (swap_idx == root) return;
        const tmp_t: u32 = tids[root];
        tids[root] = tids[swap_idx];
        tids[swap_idx] = tmp_t;
        const tmp_p: u32 = pos[root];
        pos[root] = pos[swap_idx];
        pos[swap_idx] = tmp_p;
        root = swap_idx;
    }
}

fn sortTermPosPairs(tids: *[]u32, pos: *[]u32, n: usize) {
    if (n <= 1) return;
    let start: usize = n / 2;
    while (start > 0) {
        start -= 1;
        siftDownPair(tids, pos, start, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp_t: u32 = tids[0];
        tids[0] = tids[end];
        tids[end] = tmp_t;
        const tmp_p: u32 = pos[0];
        pos[0] = pos[end];
        pos[end] = tmp_p;
        siftDownPair(tids, pos, 0, end);
    }
}

fn posSliceContains(positions: []const u32, target: u32): bool {
    let lo: usize = 0;
    let hi: usize = positions.len;
    while (lo < hi) {
        const mid: usize = lo + (hi - lo) / 2;
        if (positions[mid] == target) return true;
        if (positions[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return false;
}

fn posSliceLowerBound(positions: []const u32, min_val: u32): u32? {
    let lo: usize = 0;
    let hi: usize = positions.len;
    while (lo < hi) {
        const mid: usize = lo + (hi - lo) / 2;
        if (positions[mid] < min_val) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < positions.len) return positions[lo];
    return null;
}

fn posSliceClosest(positions: []const u32, target: u32): u32? {
    if (positions.len == 0) return null;
    let lo: usize = 0;
    let hi: usize = positions.len;
    while (lo < hi) {
        const mid: usize = lo + (hi - lo) / 2;
        if (positions[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo == 0) return positions[0];
    if (lo >= positions.len) return positions[positions.len - 1];
    const p_hi: u32 = positions[lo];
    const p_lo: u32 = positions[lo - 1];
    const d_hi: u32 = p_hi - target;
    const d_lo: u32 = target - p_lo;
    return if (d_lo <= d_hi) p_lo else p_hi;
}

fn hitBetter(a: FtHit, b: FtHit): bool {
    if (a.score > b.score) return true;
    if (a.score < b.score) return false;
    return a.doc_id < b.doc_id;
}

fn insertTopKHit(out_hits: *[]FtHit, count: *usize, cand: FtHit) {
    const cap: usize = out_hits.len;
    if (cap == 0) return;
    if (count.* < cap) {
        let idx: usize = count.*;
        out_hits[idx] = cand;
        count.* += 1;
        while (idx > 0 && hitBetter(out_hits[idx], out_hits[idx - 1])) {
            const tmp: FtHit = out_hits[idx - 1];
            out_hits[idx - 1] = out_hits[idx];
            out_hits[idx] = tmp;
            idx -= 1;
        }
    } else if (hitBetter(cand, out_hits[cap - 1])) {
        let idx: usize = cap - 1;
        out_hits[idx] = cand;
        while (idx > 0 && hitBetter(out_hits[idx], out_hits[idx - 1])) {
            const tmp: FtHit = out_hits[idx - 1];
            out_hits[idx - 1] = out_hits[idx];
            out_hits[idx] = tmp;
            idx -= 1;
        }
    }
}

fn validateQueryScratch(doc_count: usize, scratch: *FullTextScratch): void ! FullTextError {
    if (scratch.scores.len < doc_count ||
        scratch.doc_mask.len < doc_count ||
        scratch.matched_terms_cnt.len < doc_count)
    {
        return FullTextError.BufferTooSmall;
    }
}

fn zeroQueryArrays(doc_count: usize, scratch: *FullTextScratch) {
    let i: usize = 0;
    while (i < doc_count) : (i += 1) {
        scratch.scores[i] = 0.0;
        scratch.doc_mask[i] = 0;
        scratch.matched_terms_cnt[i] = 0;
    }
}

struct QueryParser {
    src: []const u8 = "";
    pos: usize = 0;
    depth: usize = 0;
    aux_offset: usize = 0;
    score_enabled: bool = true;

    fn skipSpaces(*_) {
        while (_.pos < _.src.len) {
            const c: u8 = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peekKeyword(kw: []const u8): bool {
        if (pos + kw.len > src.len) return false;
        let i: usize = 0;
        while (i < kw.len) : (i += 1) {
            if (src[pos + i] != kw[i]) return false;
        }
        if (pos + kw.len < src.len) {
            const next_c: u8 = src[pos + kw.len];
            if (next_c != ' ' && next_c != '\t' && next_c != '\r' && next_c != '\n' &&
                next_c != '(' && next_c != ')' && next_c != '"')
            {
                return false;
            }
        }
        return true;
    }

    fn consumeKeyword(*_, kw: []const u8): bool {
        _.skipSpaces();
        if (_.peekKeyword(kw)) {
            _.pos += kw.len;
            return true;
        }
        return false;
    }
}

fn allocAuxMask(
    parser: *QueryParser,
    aux_buf: *[]u8,
    doc_count: usize,
): usize ! FullTextError {
    if (parser.aux_offset + doc_count > aux_buf.len) {
        return FullTextError.BufferTooSmall;
    }
    const base: usize = parser.aux_offset;
    parser.aux_offset += doc_count;
    let i: usize = 0;
    while (i < doc_count) : (i += 1) {
        aux_buf[base + i] = 0;
    }
    return base;
}

fn parseUintSuffix(src: []const u8, pos: *usize): u32 {
    let val: u32 = 0;
    while (pos.* < src.len) {
        const c: u8 = src[pos.*];
        if (c >= '0' && c <= '9') {
            val = val * 10 + @as(u32, c - '0');
            pos.* += 1;
        } else {
            break;
        }
    }
    return val;
}

/// Evaluates `or_expr := and_expr ( ("OR" | implicit) and_expr )*`.
fn evalOrExpr(
    idx: FullTextIndex,
    parser: *QueryParser,
    scores: *[]f64,
    out_mask: *[]u8,
    matched_cnt: *[]u32,
    aux_buf: *[]u8,
): bool ! FullTextError {
    const dc: usize = idx.doc_count;
    const lhs_ok: bool = try evalAndExpr(idx, parser, scores, out_mask, matched_cnt, aux_buf);
    if (!lhs_ok) return false;

    while (true) {
        parser.skipSpaces();
        if (parser.pos >= parser.src.len || parser.src[parser.pos] == ')') break;

        let explicit_or: bool = false;
        if (parser.consumeKeyword("OR")) {
            explicit_or = true;
        } else if (parser.peekKeyword("AND")) {
            break;
        }

        const rhs_base: usize = try allocAuxMask(parser, aux_buf, dc);
        let rhs_mask: []u8 = aux_buf[rhs_base..rhs_base + dc];
        const rhs_ok: bool = try evalAndExpr(idx, parser, scores, *rhs_mask, matched_cnt, aux_buf);
        if (!rhs_ok) {
            parser.aux_offset = rhs_base;
            if (explicit_or) return FullTextError.InvalidQuery;
            break;
        }

        let d: usize = 0;
        while (d < dc) : (d += 1) {
            if (rhs_mask[d] != 0) out_mask[d] = 1;
        }
        parser.aux_offset = rhs_base;
    }

    return true;
}

/// Evaluates `and_expr := not_expr ( ("AND" | "+" | "-") not_expr )*`.
fn evalAndExpr(
    idx: FullTextIndex,
    parser: *QueryParser,
    scores: *[]f64,
    out_mask: *[]u8,
    matched_cnt: *[]u32,
    aux_buf: *[]u8,
): bool ! FullTextError {
    const dc: usize = idx.doc_count;
    const lhs_ok: bool = try evalNotExpr(idx, parser, scores, out_mask, matched_cnt, aux_buf);
    if (!lhs_ok) return false;

    while (true) {
        parser.skipSpaces();
        if (parser.pos >= parser.src.len || parser.src[parser.pos] == ')') break;

        let is_and: bool = false;
        let is_must_not: bool = false;
        if (parser.consumeKeyword("AND")) {
            is_and = true;
        } else if (parser.src[parser.pos] == '+') {
            parser.pos += 1;
            is_and = true;
        } else if (parser.src[parser.pos] == '-') {
            parser.pos += 1;
            is_must_not = true;
        } else if (parser.peekKeyword("NOT")) {
            // Implicit AND NOT (e.g. `foo NOT bar`)
            is_and = true;
        } else {
            break;
        }

        const rhs_base: usize = try allocAuxMask(parser, aux_buf, dc);
        let rhs_mask: []u8 = aux_buf[rhs_base..rhs_base + dc];
        const saved_score_en: bool = parser.score_enabled;
        if (is_must_not) parser.score_enabled = false;
        const rhs_ok: bool = try evalNotExpr(idx, parser, scores, *rhs_mask, matched_cnt, aux_buf);
        parser.score_enabled = saved_score_en;
        if (!rhs_ok) {
            parser.aux_offset = rhs_base;
            return FullTextError.InvalidQuery;
        }

        let d: usize = 0;
        if (is_must_not) {
            while (d < dc) : (d += 1) {
                if (rhs_mask[d] != 0) out_mask[d] = 0;
            }
        } else if (is_and) {
            while (d < dc) : (d += 1) {
                if (out_mask[d] != 0 && rhs_mask[d] != 0) {
                    out_mask[d] = 1;
                } else {
                    out_mask[d] = 0;
                }
            }
        }
        parser.aux_offset = rhs_base;
    }

    return true;
}

/// Evaluates `not_expr := "NOT" not_expr | "-" primary | "+" primary | primary`.
fn evalNotExpr(
    idx: FullTextIndex,
    parser: *QueryParser,
    scores: *[]f64,
    out_mask: *[]u8,
    matched_cnt: *[]u32,
    aux_buf: *[]u8,
): bool ! FullTextError {
    parser.skipSpaces();
    if (parser.pos >= parser.src.len) return false;

    let is_neg: bool = false;
    if (parser.consumeKeyword("NOT")) {
        is_neg = true;
    } else if (parser.src[parser.pos] == '-') {
        parser.pos += 1;
        is_neg = true;
    }
    if (is_neg) {
        const dc: usize = idx.doc_count;
        const sub_base: usize = try allocAuxMask(parser, aux_buf, dc);
        let sub_mask: []u8 = aux_buf[sub_base..sub_base + dc];
        const saved_score_en: bool = parser.score_enabled;
        parser.score_enabled = false;
        const ok: bool = try evalNotExpr(idx, parser, scores, *sub_mask, matched_cnt, aux_buf);
        parser.score_enabled = saved_score_en;
        if (!ok) {
            parser.aux_offset = sub_base;
            return FullTextError.InvalidQuery;
        }
        let d: usize = 0;
        while (d < dc) : (d += 1) {
            if (!idx.bufs.docs[d].deleted && sub_mask[d] == 0) {
                out_mask[d] = 1;
            } else {
                out_mask[d] = 0;
            }
        }
        parser.aux_offset = sub_base;
        return true;
    }

    if (parser.src[parser.pos] == '+') {
        parser.pos += 1;
    }
    return try evalPrimaryExpr(idx, parser, scores, out_mask, matched_cnt, aux_buf);
}

/// Evaluates primary expressions: `( or_expr )`, `"phrase"[~slop]`, `prefix*`, `term~[1|2]`, or `term`.
fn evalPrimaryExpr(
    idx: FullTextIndex,
    parser: *QueryParser,
    scores: *[]f64,
    out_mask: *[]u8,
    matched_cnt: *[]u32,
    aux_buf: *[]u8,
): bool ! FullTextError {
    parser.skipSpaces();
    if (parser.pos >= parser.src.len) return false;

    const c: u8 = parser.src[parser.pos];
    if (c == ')') return false;

    // Parenthesized sub-expression (bounded depth prevents stack exhaustion on adversarial queries)
    if (c == '(') {
        if (parser.depth >= 32) return FullTextError.InvalidQuery;
        parser.pos += 1;
        parser.depth += 1;
        const ok: bool = try evalOrExpr(idx, parser, scores, out_mask, matched_cnt, aux_buf);
        parser.skipSpaces();
        if (!ok || parser.pos >= parser.src.len || parser.src[parser.pos] != ')') {
            return FullTextError.InvalidQuery;
        }
        parser.pos += 1;
        parser.depth -= 1;
        return true;
    }

    // Quoted phrase with optional ~slop
    if (c == '"') {
        parser.pos += 1;
        const start: usize = parser.pos;
        while (parser.pos < parser.src.len && parser.src[parser.pos] != '"') {
            parser.pos += 1;
        }
        if (parser.pos >= parser.src.len) return FullTextError.InvalidQuery;
        const phrase: []const u8 = parser.src[start..parser.pos];
        parser.pos += 1; // consume closing '"'
        let slop: u32 = 0;
        if (parser.pos < parser.src.len && parser.src[parser.pos] == '~') {
            parser.pos += 1;
            slop = parseUintSuffix(parser.src, *parser.pos);
        }
        const sc: f64 = if (parser.score_enabled) 1.0 else 0.0;
        idx.evalPhraseInto(phrase, slop, sc, scores, out_mask, matched_cnt);
        return true;
    }

    // Atom token: scan until whitespace or delimiter
    const tok_start: usize = parser.pos;
    while (parser.pos < parser.src.len) {
        const ch: u8 = parser.src[parser.pos];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' ||
            ch == '(' || ch == ')' || ch == '"')
        {
            break;
        }
        parser.pos += 1;
    }
    const raw_tok: []const u8 = parser.src[tok_start..parser.pos];
    if (raw_tok.len == 0) return false;
    const sc: f64 = if (parser.score_enabled) 1.0 else 0.0;

    // Check prefix wildcard: `foo*`
    if (raw_tok.len >= 2 && raw_tok[raw_tok.len - 1] == '*') {
        idx.evalPrefixInto(raw_tok[0..raw_tok.len - 1], sc, scores, out_mask, matched_cnt);
        return true;
    }

    // Check fuzzy operator: `foo~` / `foo~1` / `foo~2`
    let tilde_pos: usize = raw_tok.len;
    let ti: usize = 0;
    while (ti < raw_tok.len) : (ti += 1) {
        if (raw_tok[ti] == '~') {
            tilde_pos = ti;
            break;
        }
    }
    if (tilde_pos > 0 && tilde_pos < raw_tok.len) {
        let max_edits: usize = 1;
        if (tilde_pos + 1 < raw_tok.len) {
            let p_sub: usize = tilde_pos + 1;
            const parsed: u32 = parseUintSuffix(raw_tok, *p_sub);
            if (parsed > 0) max_edits = @as(usize, parsed);
        }
        idx.evalFuzzyInto(raw_tok[0..tilde_pos], max_edits, sc, scores, out_mask, matched_cnt);
        return true;
    }

    idx.evalPlainTermInto(raw_tok, sc, scores, out_mask, matched_cnt);
    return true;
}

// ============================================================================
// 7. Dense-Window Keyword Snippet Extractor with `<mark>...</mark>` Highlighting
// ============================================================================

struct SnippetHitSpan {
    start: usize = 0;
    end: usize = 0;
    query_idx: u16 = 0;
}

const SNIPPET_MAX_HITS: usize = 256;
const SNIPPET_MAX_QUERY_TERMS: usize = 32;

fn appendBytesChecked(dst: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > dst.len) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn isUtf8Continuation(b: u8): bool {
    return (b & 0xC0) == 0x80;
}

fn alignUtf8Forward(text: []const u8, off: usize): usize {
    let i: usize = off;
    while (i < text.len && isUtf8Continuation(text[i])) : (i += 1) {}
    return i;
}

fn alignUtf8Backward(text: []const u8, off: usize): usize {
    let i: usize = if (off > text.len) text.len else off;
    while (i > 0 && isUtf8Continuation(text[i])) : (i -= 1) {}
    return i;
}

/// Extracts the highest-density context snippet around `query_terms` from `text` within `max_bytes`,
/// wrapping matched spans with `open_tag` and `close_tag` (e.g. `"<mark>"` / `"</mark>"`).
/// Merges overlapping spans (such as CJK unigram+bigram matches) and writes cleanly into `out_buf`.
pub fn extractBestSnippet(
    text: []const u8,
    query_terms: []const []const u8,
    max_bytes: usize,
    open_tag: []const u8,
    close_tag: []const u8,
    enable_stemming: bool,
    out_buf: *[]u8,
): usize ! FullTextError {
    if (max_bytes == 0 || out_buf.len == 0) return FullTextError.BufferTooSmall;
    if (text.len == 0) return 0;

    // Normalize query terms into stack table
    let q_norms: [SNIPPET_MAX_QUERY_TERMS][FT_MAX_TOKEN_BYTES]u8 = [[0; FT_MAX_TOKEN_BYTES]; SNIPPET_MAX_QUERY_TERMS];
    let q_lens: [SNIPPET_MAX_QUERY_TERMS]usize = [0; SNIPPET_MAX_QUERY_TERMS];
    const q_count: usize = if (query_terms.len > SNIPPET_MAX_QUERY_TERMS) SNIPPET_MAX_QUERY_TERMS else query_terms.len;
    let qi: usize = 0;
    while (qi < q_count) : (qi += 1) {
        let q_slice: []u8 = q_norms[qi][0..FT_MAX_TOKEN_BYTES];
        q_lens[qi] = normalizeSingleTerm(query_terms[qi], enable_stemming, *q_slice);
    }

    // Scan text for all matching token spans (ASCII/Latin words + CJK unigrams & 2-grams)
    let hits: [SNIPPET_MAX_HITS]SnippetHitSpan = [SnippetHitSpan{}; SNIPPET_MAX_HITS];
    let hit_count: usize = 0;
    let i: usize = 0;
    let prev_cjk_start: usize = 0;
    let prev_cjk_len: usize = 0;
    let has_prev_cjk: bool = false;

    while (i < text.len && hit_count < SNIPPET_MAX_HITS) {
        const u8dec: Utf8Decoded = decodeUtf8At(text, i);
        if (u8dec.len == 0) break;
        if (isUnicodeSeparatorOrPunct(u8dec.cp)) {
            has_prev_cjk = false;
            i += u8dec.len;
            continue;
        }

        if (isCjkCodepoint(u8dec.cp)) {
            const cur_start: usize = i;
            const cur_len: usize = u8dec.len;
            const cur_slice: []const u8 = text[cur_start..cur_start + cur_len];

            qi = 0;
            while (qi < q_count) : (qi += 1) {
                if (q_lens[qi] > 0 && bytesEqual(cur_slice, q_norms[qi][0..q_lens[qi]])) {
                    if (hit_count < SNIPPET_MAX_HITS) {
                        hits[hit_count] = SnippetHitSpan{
                            start: cur_start,
                            end: cur_start + cur_len,
                            query_idx: @as(u16, @intCast(qi)),
                        };
                        hit_count += 1;
                    }
                    break;
                }
            }

            if (has_prev_cjk && prev_cjk_len + cur_len <= FT_MAX_TOKEN_BYTES) {
                let bi_buf: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
                let k: usize = 0;
                while (k < prev_cjk_len) : (k += 1) bi_buf[k] = text[prev_cjk_start + k];
                let m: usize = 0;
                while (m < cur_len) : (m += 1) bi_buf[prev_cjk_len + m] = text[cur_start + m];
                const bi_slice: []const u8 = bi_buf[0..prev_cjk_len + cur_len];
                qi = 0;
                while (qi < q_count) : (qi += 1) {
                    if (q_lens[qi] > 0 && bytesEqual(bi_slice, q_norms[qi][0..q_lens[qi]])) {
                        if (hit_count < SNIPPET_MAX_HITS) {
                            hits[hit_count] = SnippetHitSpan{
                                start: prev_cjk_start,
                                end: cur_start + cur_len,
                                query_idx: @as(u16, @intCast(qi)),
                            };
                            hit_count += 1;
                        }
                        break;
                    }
                }
            }

            prev_cjk_start = cur_start;
            prev_cjk_len = cur_len;
            has_prev_cjk = true;
            i += cur_len;
            continue;
        }

        has_prev_cjk = false;
        const w_start: usize = i;
        let norm_arr: [FT_MAX_TOKEN_BYTES]u8 = [0; FT_MAX_TOKEN_BYTES];
        let norm_len: usize = 0;
        while (i < text.len) {
            const wdec: Utf8Decoded = decodeUtf8At(text, i);
            if (wdec.len == 0) break;
            if (isUnicodeSeparatorOrPunct(wdec.cp) || isCjkCodepoint(wdec.cp)) break;
            if (wdec.len == 1) {
                if (norm_len < FT_MAX_TOKEN_BYTES) {
                    norm_arr[norm_len] = foldAsciiLower(text[i]);
                    norm_len += 1;
                }
            } else if (norm_len + wdec.len <= FT_MAX_TOKEN_BYTES) {
                let k: usize = 0;
                while (k < wdec.len) : (k += 1) norm_arr[norm_len + k] = text[i + k];
                norm_len += wdec.len;
            }
            i += wdec.len;
        }
        const w_end: usize = i;
        if (norm_len == 0) continue;

        if (enable_stemming && norm_len > 2) {
            let ns: []u8 = norm_arr[0..FT_MAX_TOKEN_BYTES];
            norm_len = porterLiteStemInPlace(*ns, norm_len);
        }
        const w_norm: []const u8 = norm_arr[0..norm_len];
        qi = 0;
        while (qi < q_count) : (qi += 1) {
            if (q_lens[qi] > 0 && bytesEqual(w_norm, q_norms[qi][0..q_lens[qi]])) {
                if (hit_count < SNIPPET_MAX_HITS) {
                    hits[hit_count] = SnippetHitSpan{
                        start: w_start,
                        end: w_end,
                        query_idx: @as(u16, @intCast(qi)),
                    };
                    hit_count += 1;
                }
                break;
            }
        }
    }

    // Determine best window [win_start, win_end] maximizing (distinct_queries * 3 + total_hits)
    let win_start: usize = 0;
    let win_end: usize = if (text.len <= max_bytes) text.len else alignUtf8Backward(text, max_bytes);

    if (hit_count > 0 && text.len > max_bytes) {
        let best_score: usize = 0;
        let best_first_hit: usize = 0;
        let left: usize = 0;
        while (left < hit_count) : (left += 1) {
            const anchor_start: usize = hits[left].start;
            let seen_mask: u32 = 0;
            let total_in_win: usize = 0;
            let distinct_in_win: usize = 0;
            let right: usize = left;
            while (right < hit_count) : (right += 1) {
                if (hits[right].end > anchor_start + max_bytes) break;
                total_in_win += 1;
                const bit: u32 = @as(u32, 1) << @as(u5, @intCast(hits[right].query_idx & 31));
                if ((seen_mask & bit) == 0) {
                    seen_mask |= bit;
                    distinct_in_win += 1;
                }
            }
            const score: usize = distinct_in_win * 3 + total_in_win;
            if (score > best_score) {
                best_score = score;
                best_first_hit = left;
            }
        }

        // Center window with leading context padding (1/4 of max_bytes) around best_first_hit
        const pad: usize = max_bytes / 4;
        const raw_start: usize = if (hits[best_first_hit].start > pad)
            hits[best_first_hit].start - pad
        else
            0;
        win_start = alignUtf8Forward(text, raw_start);
        // Snap forward to next word boundary if we landed mid-ASCII-word
        if (win_start > 0 && win_start < hits[best_first_hit].start) {
            while (win_start < hits[best_first_hit].start &&
                text[win_start] >= 'a' && text[win_start] <= 'z')
            {
                win_start += 1;
            }
            while (win_start < hits[best_first_hit].start && text[win_start] == ' ') {
                win_start += 1;
            }
        }
        const raw_end: usize = if (win_start + max_bytes < text.len) win_start + max_bytes else text.len;
        win_end = alignUtf8Backward(text, raw_end);
        if (win_end < hits[best_first_hit].end && hits[best_first_hit].end <= text.len) {
            win_end = hits[best_first_hit].end;
        }
    }

    // Sort and merge overlapping hit spans inside [win_start, win_end]
    let m_starts: [SNIPPET_MAX_HITS]usize = [0; SNIPPET_MAX_HITS];
    let m_ends: [SNIPPET_MAX_HITS]usize = [0; SNIPPET_MAX_HITS];
    let m_count: usize = 0;

    // Insertion sort hits by start offset
    let s_i: usize = 1;
    while (s_i < hit_count) : (s_i += 1) {
        const key: SnippetHitSpan = hits[s_i];
        let j: usize = s_i;
        while (j > 0 && hits[j - 1].start > key.start) : (j -= 1) {
            hits[j] = hits[j - 1];
        }
        hits[j] = key;
    }

    let h_i: usize = 0;
    while (h_i < hit_count) : (h_i += 1) {
        if (hits[h_i].end <= win_start || hits[h_i].start >= win_end) continue;
        const cs: usize = if (hits[h_i].start < win_start) win_start else hits[h_i].start;
        const ce: usize = if (hits[h_i].end > win_end) win_end else hits[h_i].end;
        if (m_count > 0 && cs <= m_ends[m_count - 1]) {
            if (ce > m_ends[m_count - 1]) m_ends[m_count - 1] = ce;
        } else {
            m_starts[m_count] = cs;
            m_ends[m_count] = ce;
            m_count += 1;
        }
    }

    // Emit snippet with ellipsis and highlight tags
    let out_pos: usize = 0;
    if (win_start > 0) {
        if (!appendBytesChecked(out_buf, *out_pos, "...")) return FullTextError.BufferTooSmall;
    }

    let cursor: usize = win_start;
    let mi: usize = 0;
    while (mi < m_count) : (mi += 1) {
        if (m_starts[mi] > cursor) {
            if (!appendBytesChecked(out_buf, *out_pos, text[cursor..m_starts[mi]])) {
                return FullTextError.BufferTooSmall;
            }
        }
        if (!appendBytesChecked(out_buf, *out_pos, open_tag)) return FullTextError.BufferTooSmall;
        if (!appendBytesChecked(out_buf, *out_pos, text[m_starts[mi]..m_ends[mi]])) {
            return FullTextError.BufferTooSmall;
        }
        if (!appendBytesChecked(out_buf, *out_pos, close_tag)) return FullTextError.BufferTooSmall;
        cursor = m_ends[mi];
    }

    if (cursor < win_end) {
        if (!appendBytesChecked(out_buf, *out_pos, text[cursor..win_end])) {
            return FullTextError.BufferTooSmall;
        }
    }
    if (win_end < text.len) {
        if (!appendBytesChecked(out_buf, *out_pos, "...")) return FullTextError.BufferTooSmall;
    }

    return out_pos;
}

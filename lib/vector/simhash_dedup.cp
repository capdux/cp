// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc 64-bit Charikar SimHash & 4-Band LSH Near-Duplicate RAG Dedup) | Created: 2026-11-22
// Pure CP zero-heap-allocation 64-bit Charikar SimHash fingerprinting and Pigeonhole 4x16-bit LSH band
// near-duplicate document deduplication index for RAG ingestion, agent web-crawl memory & code chunking:
//   - Hardware `@popCount(a ^ b)` 64-bit Hamming distance (`hammingDistanceU64`)
//   - Zero-alloc ASCII case-folding tokenizer + unigram & 2-shingle FNV-1a/SplitMix64 SimHash (`computeSimHash64`)
//   - Fixed 256-slot inline index (`SimHashDedupIndex`) with 4-band 16-bit zero-false-negative pre-filter
//     for `max_hamming_dist <= 3` and exact full `@popCount` scan for `max_hamming_dist > 3`

// ============================================================================
// 1. Constants & Internal Hash / Tokenization Helpers
// ============================================================================

/// Maximum resident active documents in `SimHashDedupIndex` (zero dynamic heap).
pub const SIMHASH_MAX_DOCS: usize = 256;

/// Number of 16-bit LSH bands partitioning each 64-bit SimHash fingerprint (4 x 16 = 64 bits).
pub const SIMHASH_NUM_BANDS: usize = 4;

const FNV1A_64_OFFSET: u64 = 0xcbf29ce484222325;
const FNV1A_64_PRIME: u64 = 0x100000001b3;

fn foldAsciiLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn isTokenByte(b: u8): bool {
    if (b >= 97 && b <= 122) return true; // 'a'..'z'
    if (b >= 65 && b <= 90) return true;  // 'A'..'Z'
    if (b >= 48 && b <= 57) return true;  // '0'..'9'
    if (b == 95) return true;             // '_'
    if (b >= 128) return true;            // UTF-8 multi-byte word chars
    return false;
}

/// SplitMix64 finalizer for strong 64-bit avalanche uniformity across all SimHash bit positions.
fn mix64(x_in: u64): u64 {
    let z: u64 = x_in +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn accumulateHashWeights(weights: *[]i32, h: u64) {
    let bit: usize = 0;
    while (bit < 64) : (bit += 1) {
        const shift: u6 = @intCast(bit);
        if (((h >> shift) & 1) != 0) {
            weights[bit] += 1;
        } else {
            weights[bit] -= 1;
        }
    }
}

/// Pigeonhole 4x16-bit band pre-filter: returns `true` iff `a` and `b` share at least one
/// identical 16-bit band (guaranteed 100% zero false negatives whenever `hammingDistanceU64(a, b) <= 3`).
fn sharesAnyBand16(a: u64, b: u64): bool {
    const x: u64 = a ^ b;
    if ((x & 0xFFFF) == 0) return true;
    if (((x >> 16) & 0xFFFF) == 0) return true;
    if (((x >> 32) & 0xFFFF) == 0) return true;
    if (((x >> 48) & 0xFFFF) == 0) return true;
    return false;
}

// ============================================================================
// 2. Public Models: Config, DocEntry, Match & Stats
// ============================================================================

/// Configuration for `SimHashDedupIndex`.
pub struct SimHashDedupConfig {
    max_hamming_dist: u32 = 3;
    min_token_len: usize = 2;

    /// Validates `max_hamming_dist <= 16` and `min_token_len >= 1`.
    fn isValid(): bool {
        if (_.max_hamming_dist > 16) return false;
        if (_.min_token_len == 0) return false;
        return true;
    }
}

/// Inline resident document fingerprint entry in `SimHashDedupIndex`.
pub struct SimHashDocEntry {
    doc_id: u64 = 0;
    fingerprint: u64 = 0;
    active: bool = false;
}

/// Result returned by `query_nearest_fp`, `query_nearest`, and `check_and_insert`.
pub struct SimHashMatch {
    is_duplicate: bool = false;
    matched_doc_id: u64 = 0;
    hamming_dist: u32 = 64;
    fingerprint: u64 = 0;
    inserted: bool = false;
}

/// Operational statistics snapshot for `SimHashDedupIndex`.
pub struct SimHashDedupStats {
    active_docs: usize = 0;
    total_checked: u64 = 0;
    duplicates_rejected: u64 = 0;
    unique_accepted: u64 = 0;
}

// ============================================================================
// 3. Pure SimHash & Hardware Hamming Distance Kernels
// ============================================================================

/// Computes the exact 64-bit Hamming distance between two fingerprints using hardware `@popCount`.
pub fn hammingDistanceU64(a: u64, b: u64): u32 {
    return @intCast(@popCount(a ^ b));
}

/// Computes a 64-bit Charikar SimHash fingerprint for `text` with zero dynamic heap allocation.
/// Uses ASCII case-folding tokenization, FNV-1a + SplitMix64 hashing of unigrams and 2-shingles,
/// and 64-dimensional signed bit-weight accumulation. Returns `0` for empty or tokenless input.
pub fn computeSimHash64(text: []const u8, min_token_len: usize): u64 {
    if (text.len == 0) return 0;
    const eff_min_len: usize = if (min_token_len == 0) 1 else min_token_len;

    let weights_buf: [64]i32 = [0; 64];
    let weights: []i32 = weights_buf[0..];

    let has_prev: bool = false;
    let prev_hash: u64 = 0;
    let token_count: usize = 0;

    let i: usize = 0;
    while (i < text.len) {
        // Skip delimiters / punctuation / whitespace
        while (i < text.len && !isTokenByte(text[i])) : (i += 1) {}
        if (i >= text.len) break;

        // Scan token and compute case-folded FNV-1a 64-bit hash
        const start: usize = i;
        let h_fnv: u64 = FNV1A_64_OFFSET;
        while (i < text.len && isTokenByte(text[i])) : (i += 1) {
            const c: u8 = foldAsciiLower(text[i]);
            h_fnv = (h_fnv ^ (c as u64)) *% FNV1A_64_PRIME;
        }

        const tok_len: usize = i - start;
        if (tok_len >= eff_min_len) {
            const h_unigram: u64 = mix64(h_fnv);
            accumulateHashWeights(weights, h_unigram);

            if (has_prev) {
                const bigram_seed: u64 = (prev_hash *% 0x9e3779b185ebca87) ^ h_unigram;
                const h_bigram: u64 = mix64(bigram_seed);
                accumulateHashWeights(weights, h_bigram);
            }
            prev_hash = h_unigram;
            has_prev = true;
            token_count += 1;
        }
    }

    if (token_count == 0) return 0;

    let fp: u64 = 0;
    let bit: usize = 0;
    while (bit < 64) : (bit += 1) {
        if (weights_buf[bit] > 0) {
            const shift: u6 = @intCast(bit);
            fp = fp | ((1 as u64) << shift);
        }
    }
    return fp;
}

// ============================================================================
// 4. Zero-Alloc 4-Band LSH Near-Duplicate RAG Deduplication Index
// ============================================================================

/// Zero-heap-allocation 256-document near-duplicate deduplication index using 64-bit SimHash
/// and 4x16-bit Pigeonhole LSH band pre-filtering.
pub struct SimHashDedupIndex {
    cfg: SimHashDedupConfig = SimHashDedupConfig{},
    entries: [SIMHASH_MAX_DOCS]SimHashDocEntry = [SimHashDocEntry{}; SIMHASH_MAX_DOCS],
    active_docs: usize = 0,
    total_checked: u64 = 0,
    duplicates_rejected: u64 = 0,
    unique_accepted: u64 = 0,

    /// Initializes a validated `SimHashDedupIndex`. Returns `null` if `cfg.isValid()` is `false`.
    static fn init(cfg: SimHashDedupConfig): SimHashDedupIndex? {
        if (!cfg.isValid()) return null;
        return SimHashDedupIndex{
            cfg: cfg,
            entries: [SimHashDocEntry{}; SIMHASH_MAX_DOCS],
            active_docs: 0,
            total_checked: 0,
            duplicates_rejected: 0,
            unique_accepted: 0,
        };
    }

    /// Queries the nearest resident document within `cfg.max_hamming_dist` of `fp`.
    /// - When `max_hamming_dist <= 3`, applies Pigeonhole 4x16-bit band pre-filtering (zero false negatives).
    /// - When `max_hamming_dist > 3`, automatically performs an exact full `@popCount` scan.
    /// - Ties in Hamming distance are broken deterministically by ascending `doc_id`.
    fn query_nearest_fp(fp: u64): SimHashMatch {
        const use_band_filter: bool = _.cfg.max_hamming_dist <= 3;
        let found: bool = false;
        let best_dist: u32 = 65;
        let best_doc_id: u64 = 0;

        let i: usize = 0;
        while (i < SIMHASH_MAX_DOCS) : (i += 1) {
            if (!_.entries[i].active) continue;
            const entry_fp: u64 = _.entries[i].fingerprint;

            if (use_band_filter && !sharesAnyBand16(fp, entry_fp)) {
                continue;
            }

            const dist: u32 = hammingDistanceU64(fp, entry_fp);
            if (dist <= _.cfg.max_hamming_dist) {
                const entry_id: u64 = _.entries[i].doc_id;
                if (!found || dist < best_dist || (dist == best_dist && entry_id < best_doc_id)) {
                    found = true;
                    best_dist = dist;
                    best_doc_id = entry_id;
                }
            }
        }

        if (!found) {
            return SimHashMatch{
                is_duplicate: false,
                matched_doc_id: 0,
                hamming_dist: 64,
                fingerprint: fp,
                inserted: false,
            };
        }

        return SimHashMatch{
            is_duplicate: true,
            matched_doc_id: best_doc_id,
            hamming_dist: best_dist,
            fingerprint: fp,
            inserted: false,
        };
    }

    /// Computes `computeSimHash64(text, cfg.min_token_len)` and queries the nearest resident match.
    /// Returns a non-duplicate sentinel (`fingerprint == 0`) for empty or tokenless text.
    fn query_nearest(text: []const u8): SimHashMatch {
        const fp: u64 = computeSimHash64(text, _.cfg.min_token_len);
        if (text.len == 0 || fp == 0) {
            return SimHashMatch{
                is_duplicate: false,
                matched_doc_id: 0,
                hamming_dist: 64,
                fingerprint: 0,
                inserted: false,
            };
        }
        return _.query_nearest_fp(fp);
    }

    /// Checks whether `text` is a near-duplicate of any resident document (or reuses an active `doc_id`).
    /// - Rejects duplicates (incrementing `duplicates_rejected`) without inserting.
    /// - Rejects empty/tokenless text or full-index insertions safely (`inserted = false`).
    /// - Otherwise stores the fingerprint in an available slot and increments `unique_accepted`.
    fn check_and_insert(*_, doc_id: u64, text: []const u8): SimHashMatch {
        _.total_checked += 1;

        if (text.len == 0) {
            return SimHashMatch{
                is_duplicate: false,
                matched_doc_id: 0,
                hamming_dist: 64,
                fingerprint: 0,
                inserted: false,
            };
        }

        const fp: u64 = computeSimHash64(text, _.cfg.min_token_len);
        if (fp == 0) {
            return SimHashMatch{
                is_duplicate: false,
                matched_doc_id: 0,
                hamming_dist: 64,
                fingerprint: 0,
                inserted: false,
            };
        }

        // Guard against duplicate active doc_id and locate first free slot in one pass.
        let free_slot: usize = SIMHASH_MAX_DOCS;
        let i: usize = 0;
        while (i < SIMHASH_MAX_DOCS) : (i += 1) {
            if (_.entries[i].active) {
                if (_.entries[i].doc_id == doc_id) {
                    _.duplicates_rejected += 1;
                    return SimHashMatch{
                        is_duplicate: true,
                        matched_doc_id: doc_id,
                        hamming_dist: hammingDistanceU64(fp, _.entries[i].fingerprint),
                        fingerprint: fp,
                        inserted: false,
                    };
                }
            } else if (free_slot == SIMHASH_MAX_DOCS) {
                free_slot = i;
            }
        }

        // Near-duplicate check against active fingerprints.
        const match_res: SimHashMatch = _.query_nearest_fp(fp);
        if (match_res.is_duplicate) {
            _.duplicates_rejected += 1;
            return match_res;
        }

        // Capacity guard: index full (256 active docs).
        if (free_slot >= SIMHASH_MAX_DOCS) {
            return SimHashMatch{
                is_duplicate: false,
                matched_doc_id: 0,
                hamming_dist: 64,
                fingerprint: fp,
                inserted: false,
            };
        }

        _.entries[free_slot] = SimHashDocEntry{
            doc_id: doc_id,
            fingerprint: fp,
            active: true,
        };
        _.active_docs += 1;
        _.unique_accepted += 1;

        return SimHashMatch{
            is_duplicate: false,
            matched_doc_id: 0,
            hamming_dist: 64,
            fingerprint: fp,
            inserted: true,
        };
    }

    /// Logically removes `doc_id` and frees its slot for future reuse. Returns `true` if found.
    fn remove_doc(*_, doc_id: u64): bool {
        let i: usize = 0;
        while (i < SIMHASH_MAX_DOCS) : (i += 1) {
            if (_.entries[i].active && _.entries[i].doc_id == doc_id) {
                _.entries[i] = SimHashDocEntry{};
                _.active_docs -= 1;
                return true;
            }
        }
        return false;
    }

    /// Returns a snapshot of current active document count and deduplication counters.
    fn stats(): SimHashDedupStats {
        return SimHashDedupStats{
            active_docs: _.active_docs,
            total_checked: _.total_checked,
            duplicates_rejected: _.duplicates_rejected,
            unique_accepted: _.unique_accepted,
        };
    }
}

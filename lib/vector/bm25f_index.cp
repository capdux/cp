// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc BM25F Multi-Field Weighted Inverted Index) | Created: 2026-11-22
// Pure CP zero-heap-allocation Robertson/Zaragoza BM25F multi-field weighted retrieval engine
// for code search, structured RAG & metadata-boosted document retrieval:
//   - Independent per-field length normalization `b_f` & boost `w_f` aggregated into `pseudo_tf`
//     before unified `k1` term-frequency saturation (preventing multi-field TF stuffing)
//   - Lucene / BM25+ non-negative smoothed IDF: `ln(1 + (N - df + 0.5) / (df + 0.5))`
//   - Fixed inline pools (`BM25F_MAX_FIELDS = 4`, `BM25F_MAX_DOCS = 128`, `BM25F_MAX_POSTINGS = 2048`)

const std = @import("std");

// ============================================================================
// 1. Constants & Internal Tokenization / Hashing Helpers
// ============================================================================

/// Maximum fields per document in `Bm25fIndex` (e.g., title, symbol, body, tags).
pub const BM25F_MAX_FIELDS: usize = 4;

/// Maximum resident active documents in `Bm25fIndex` (zero dynamic heap).
pub const BM25F_MAX_DOCS: usize = 128;

/// Maximum `(doc, term)` posting entries across the inline posting pool.
pub const BM25F_MAX_POSTINGS: usize = 2048;

const BM25F_MAX_QUERY_TERMS: usize = 64;
const FNV1A_64_OFFSET: u64 = 0xcbf29ce484222325;
const FNV1A_64_PRIME: u64 = 0x100000001b3;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn foldAsciiLower(b: u8): u8 {
    if (b >= 65 && b <= 90) return b + 32;
    return b;
}

fn isTokenByte(b: u8): bool {
    if (b >= 97 && b <= 122) return true;
    if (b >= 65 && b <= 90) return true;
    if (b >= 48 && b <= 57) return true;
    if (b == 95) return true;
    if (b >= 128) return true;
    return false;
}

// ============================================================================
// 2. Public Models: FieldSpec, Config, Hit & Stats
// ============================================================================

/// Per-field boost weight (`weight_f32 >= 0`) and length normalization (`b_f32 in [0, 1]`).
pub struct Bm25fFieldSpec {
    weight_f32: f32 = 1.0;
    b_f32: f32 = 0.75;

    /// Validates finite non-negative weight and `b_f32 in [0.0, 1.0]`.
    fn isValid(): bool {
        if (!isFiniteF32(_.weight_f32) || _.weight_f32 < 0.0) return false;
        if (!isFiniteF32(_.b_f32) || _.b_f32 < 0.0 || _.b_f32 > 1.0) return false;
        return true;
    }
}

/// Configuration for `Bm25fIndex`.
pub struct Bm25fConfig {
    k1_f32: f32 = 1.2;
    field_count: usize = 1;
    fields: [BM25F_MAX_FIELDS]Bm25fFieldSpec = [Bm25fFieldSpec{}; BM25F_MAX_FIELDS];

    /// Preset 4-field configuration tuned for code search:
    /// `0: title (w=3.0, b=0.75)`, `1: symbol (w=4.0, b=0.50)`, `2: body (w=1.0, b=0.75)`, `3: tags (w=2.0, b=0.40)`.
    static fn defaultsCodeSearch(): Bm25fConfig {
        return Bm25fConfig{
            k1_f32: 1.2,
            field_count: 4,
            fields: [
                Bm25fFieldSpec{ weight_f32: 3.0, b_f32: 0.75 },
                Bm25fFieldSpec{ weight_f32: 4.0, b_f32: 0.50 },
                Bm25fFieldSpec{ weight_f32: 1.0, b_f32: 0.75 },
                Bm25fFieldSpec{ weight_f32: 2.0, b_f32: 0.40 },
            ],
        };
    }

    /// Validates `k1_f32 > 0`, `1 <= field_count <= 4`, all active field specs, and at least one positive weight.
    fn isValid(): bool {
        if (!isFiniteF32(_.k1_f32) || _.k1_f32 <= 0.0) return false;
        if (_.field_count == 0 || _.field_count > BM25F_MAX_FIELDS) return false;
        let has_positive_weight: bool = false;
        let f: usize = 0;
        while (f < _.field_count) : (f += 1) {
            if (!_.fields[f].isValid()) return false;
            if (_.fields[f].weight_f32 > 0.0) {
                has_positive_weight = true;
            }
        }
        return has_positive_weight;
    }
}

/// Ranked document search hit returned by `Bm25fIndex.search_topk`.
pub struct Bm25fHit {
    doc_id: u64 = 0;
    score_f32: f32 = 0.0;
    matched_terms: usize = 0;
}

/// Operational statistics snapshot for `Bm25fIndex`.
pub struct Bm25fIndexStats {
    active_docs: usize = 0;
    posting_count: usize = 0;
    field_count: usize = 0;
    avg_field_lens: [BM25F_MAX_FIELDS]f32 = [0.0; BM25F_MAX_FIELDS];
}

// ============================================================================
// 3. Internal Inline Storage Structures
// ============================================================================

struct Bm25fDocSlot {
    doc_id: u64 = 0;
    field_lens: [BM25F_MAX_FIELDS]u32 = [0; BM25F_MAX_FIELDS];
    posting_start: usize = 0;
    posting_len: usize = 0;
    active: bool = false;
}

struct Bm25fPosting {
    doc_slot: u16 = 0;
    term_hash: u64 = 0;
    field_tf: [BM25F_MAX_FIELDS]u16 = [0; BM25F_MAX_FIELDS];
    active: bool = false;
}

// ============================================================================
// 4. Zero-Alloc BM25F Multi-Field Weighted Inverted Index
// ============================================================================

/// Zero-heap-allocation Robertson/Zaragoza BM25F multi-field weighted inverted index.
pub struct Bm25fIndex {
    cfg: Bm25fConfig = Bm25fConfig{},
    docs: [BM25F_MAX_DOCS]Bm25fDocSlot = [Bm25fDocSlot{}; BM25F_MAX_DOCS],
    postings: [BM25F_MAX_POSTINGS]Bm25fPosting = [Bm25fPosting{}; BM25F_MAX_POSTINGS],
    active_docs: usize = 0;
    posting_used: usize = 0;
    active_postings: usize = 0;
    total_field_lens: [BM25F_MAX_FIELDS]u64 = [0; BM25F_MAX_FIELDS];

    /// Initializes a validated `Bm25fIndex`. Returns `null` if `cfg.isValid()` is `false`.
    static fn init(cfg: Bm25fConfig): Bm25fIndex? {
        if (!cfg.isValid()) return null;
        return Bm25fIndex{
            cfg: cfg,
            docs: [Bm25fDocSlot{}; BM25F_MAX_DOCS],
            postings: [Bm25fPosting{}; BM25F_MAX_POSTINGS],
            active_docs: 0,
            posting_used: 0,
            active_postings: 0,
            total_field_lens: [0; BM25F_MAX_FIELDS],
        };
    }

    /// Indexes a document with `field_texts.len == cfg.field_count` fields.
    /// - Enforces field count match, unique active `doc_id`, and atomic rollback on posting pool overflow.
    /// - Tokenizes each field with ASCII case-folding and FNV-1a 64-bit term hashing.
    fn add_document(*_, doc_id: u64, field_texts: []const []const u8): bool {
        if (field_texts.len != _.cfg.field_count) return false;

        // Check duplicate active doc_id & locate free doc slot
        let free_slot: usize = BM25F_MAX_DOCS;
        let s: usize = 0;
        while (s < BM25F_MAX_DOCS) : (s += 1) {
            if (_.docs[s].active) {
                if (_.docs[s].doc_id == doc_id) return false;
            } else if (free_slot == BM25F_MAX_DOCS) {
                free_slot = s;
            }
        }
        if (free_slot >= BM25F_MAX_DOCS) return false;

        const p_start: usize = _.posting_used;
        let p_end: usize = p_start;
        let doc_field_lens: [BM25F_MAX_FIELDS]u32 = [0; BM25F_MAX_FIELDS];

        let f: usize = 0;
        while (f < _.cfg.field_count) : (f += 1) {
            const text: []const u8 = field_texts[f];
            let i: usize = 0;
            while (i < text.len) {
                while (i < text.len && !isTokenByte(text[i])) : (i += 1) {}
                if (i >= text.len) break;

                let h: u64 = FNV1A_64_OFFSET;
                while (i < text.len && isTokenByte(text[i])) : (i += 1) {
                    const c: u8 = foldAsciiLower(text[i]);
                    h = (h ^ (c as u64)) *% FNV1A_64_PRIME;
                }
                doc_field_lens[f] += 1;

                // Look up existing posting for (this doc, h)
                let found_idx: usize = p_end;
                let p: usize = p_start;
                while (p < p_end) : (p += 1) {
                    if (_.postings[p].term_hash == h) {
                        found_idx = p;
                        break;
                    }
                }

                if (found_idx < p_end) {
                    if (_.postings[found_idx].field_tf[f] < 0xFFFF) {
                        _.postings[found_idx].field_tf[f] += 1;
                    }
                } else {
                    if (p_end >= BM25F_MAX_POSTINGS) {
                        // Atomic rollback of partial postings on pool exhaustion
                        let rb: usize = p_start;
                        while (rb < p_end) : (rb += 1) {
                            _.postings[rb] = Bm25fPosting{};
                        }
                        return false;
                    }
                    _.postings[p_end] = Bm25fPosting{
                        doc_slot: @intCast(free_slot),
                        term_hash: h,
                        field_tf: [0; BM25F_MAX_FIELDS],
                        active: true,
                    };
                    _.postings[p_end].field_tf[f] = 1;
                    p_end += 1;
                }
            }
        }

        // Commit document slot & update field length accumulators
        _.docs[free_slot] = Bm25fDocSlot{
            doc_id: doc_id,
            field_lens: doc_field_lens,
            posting_start: p_start,
            posting_len: p_end - p_start,
            active: true,
        };
        let fl: usize = 0;
        while (fl < _.cfg.field_count) : (fl += 1) {
            _.total_field_lens[fl] += doc_field_lens[fl] as u64;
        }
        _.active_docs += 1;
        _.posting_used = p_end;
        _.active_postings += (p_end - p_start);
        return true;
    }

    /// Logically removes `doc_id`, updating `active_docs`, active posting count, and `avg_field_lens`.
    fn remove_document(*_, doc_id: u64): bool {
        let s: usize = 0;
        while (s < BM25F_MAX_DOCS) : (s += 1) {
            if (_.docs[s].active && _.docs[s].doc_id == doc_id) {
                let f: usize = 0;
                while (f < _.cfg.field_count) : (f += 1) {
                    const len_u64: u64 = _.docs[s].field_lens[f] as u64;
                    if (_.total_field_lens[f] >= len_u64) {
                        _.total_field_lens[f] -= len_u64;
                    } else {
                        _.total_field_lens[f] = 0;
                    }
                }
                const p_start: usize = _.docs[s].posting_start;
                const p_end: usize = p_start + _.docs[s].posting_len;
                let p: usize = p_start;
                while (p < p_end) : (p += 1) {
                    if (_.postings[p].active) {
                        _.postings[p].active = false;
                        if (_.active_postings > 0) {
                            _.active_postings -= 1;
                        }
                    }
                }
                _.docs[s] = Bm25fDocSlot{};
                _.active_docs -= 1;
                return true;
            }
        }
        return false;
    }

    /// Searches the index with BM25F multi-field scoring and writes up to `min(k, out_hits.len)`
    /// hits in descending `score_f32` order (ties broken by ascending `doc_id`).
    fn search_topk(
        query_text: []const u8,
        k: usize,
        out_hits: *[]Bm25fHit,
    ): usize {
        if (k == 0 || out_hits.len == 0 || _.active_docs == 0 || query_text.len == 0) {
            return 0;
        }

        // 1. Tokenize & deduplicate query terms
        let q_terms: [BM25F_MAX_QUERY_TERMS]u64 = [0; BM25F_MAX_QUERY_TERMS];
        let q_count: usize = 0;
        let qi: usize = 0;
        while (qi < query_text.len && q_count < BM25F_MAX_QUERY_TERMS) {
            while (qi < query_text.len && !isTokenByte(query_text[qi])) : (qi += 1) {}
            if (qi >= query_text.len) break;

            let h: u64 = FNV1A_64_OFFSET;
            while (qi < query_text.len && isTokenByte(query_text[qi])) : (qi += 1) {
                const c: u8 = foldAsciiLower(query_text[qi]);
                h = (h ^ (c as u64)) *% FNV1A_64_PRIME;
            }

            let dup: bool = false;
            let d: usize = 0;
            while (d < q_count) : (d += 1) {
                if (q_terms[d] == h) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                q_terms[q_count] = h;
                q_count += 1;
            }
        }
        if (q_count == 0) return 0;

        // 2. Compute per-field average lengths & non-negative smoothed IDF for each query term
        const n_docs_f64: f64 = _.active_docs as f64;
        let avg_lens: [BM25F_MAX_FIELDS]f64 = [1.0; BM25F_MAX_FIELDS];
        let f: usize = 0;
        while (f < _.cfg.field_count) : (f += 1) {
            const avg: f64 = (_.total_field_lens[f] as f64) / n_docs_f64;
            avg_lens[f] = if (avg > 0.0) avg else 1.0;
        }

        let q_idf: [BM25F_MAX_QUERY_TERMS]f64 = [0.0; BM25F_MAX_QUERY_TERMS];
        let t_idx: usize = 0;
        while (t_idx < q_count) : (t_idx += 1) {
            const target_h: u64 = q_terms[t_idx];
            let df: usize = 0;
            let p: usize = 0;
            while (p < _.posting_used) : (p += 1) {
                if (_.postings[p].active && _.postings[p].term_hash == target_h) {
                    df += 1;
                }
            }
            if (df > 0) {
                const df_f64: f64 = df as f64;
                const inner: f64 = 1.0 + (n_docs_f64 - df_f64 + 0.5) / (df_f64 + 0.5);
                q_idf[t_idx] = std.math.log(f64, std.math.e, inner);
            }
        }

        // 3. Score each active document via BM25F pseudo_tf aggregation + k1 saturation
        const k1: f64 = _.cfg.k1_f32 as f64;
        const limit: usize = if (k < out_hits.len) k else out_hits.len;
        let hit_count: usize = 0;

        let s: usize = 0;
        while (s < BM25F_MAX_DOCS) : (s += 1) {
            if (!_.docs[s].active) continue;
            const doc = _.docs[s];
            const p_start: usize = doc.posting_start;
            const p_end: usize = p_start + doc.posting_len;

            let doc_score: f64 = 0.0;
            let matched_terms: usize = 0;

            let p: usize = p_start;
            while (p < p_end) : (p += 1) {
                if (!_.postings[p].active) continue;
                const ph: u64 = _.postings[p].term_hash;

                let match_q: usize = q_count;
                let qj: usize = 0;
                while (qj < q_count) : (qj += 1) {
                    if (q_terms[qj] == ph) {
                        match_q = qj;
                        break;
                    }
                }
                if (match_q >= q_count) continue;

                matched_terms += 1;

                // Aggregate weighted length-normalized pseudo_tf across all fields
                let pseudo_tf: f64 = 0.0;
                let fi: usize = 0;
                while (fi < _.cfg.field_count) : (fi += 1) {
                    const tf_f: u16 = _.postings[p].field_tf[fi];
                    if (tf_f == 0) continue;
                    const w_f: f64 = _.cfg.fields[fi].weight_f32 as f64;
                    if (w_f <= 0.0) continue;
                    const b_f: f64 = _.cfg.fields[fi].b_f32 as f64;
                    const len_f: f64 = doc.field_lens[fi] as f64;
                    const norm_f: f64 = (1.0 - b_f) + b_f * (len_f / avg_lens[fi]);
                    if (norm_f > 0.0) {
                        pseudo_tf += (w_f * (tf_f as f64)) / norm_f;
                    }
                }

                if (pseudo_tf > 0.0) {
                    const tf_sat: f64 = (pseudo_tf * (k1 + 1.0)) / (k1 + pseudo_tf);
                    doc_score += q_idf[match_q] * tf_sat;
                }
            }

            if (matched_terms == 0 || doc_score <= 0.0) continue;

            const cand = Bm25fHit{
                doc_id: doc.doc_id,
                score_f32: @floatCast(doc_score),
                matched_terms: matched_terms,
            };

            // Insert into sorted top-k buffer (score desc, doc_id asc)
            let pos: usize = hit_count;
            if (hit_count < limit) {
                hit_count += 1;
            } else {
                const worst = out_hits[limit - 1];
                const better: bool = (cand.score_f32 > worst.score_f32) ||
                    (cand.score_f32 == worst.score_f32 && cand.doc_id < worst.doc_id);
                if (!better) continue;
                pos = limit - 1;
            }

            while (pos > 0) {
                const prev = out_hits[pos - 1];
                const better_prev: bool = (cand.score_f32 > prev.score_f32) ||
                    (cand.score_f32 == prev.score_f32 && cand.doc_id < prev.doc_id);
                if (!better_prev) break;
                out_hits[pos] = prev;
                pos -= 1;
            }
            out_hits[pos] = cand;
        }

        return hit_count;
    }

    /// Returns a snapshot of active documents, active postings, field count, and average field lengths.
    fn stats(): Bm25fIndexStats {
        let avgs: [BM25F_MAX_FIELDS]f32 = [0.0; BM25F_MAX_FIELDS];
        if (_.active_docs > 0) {
            const denom: f64 = _.active_docs as f64;
            let f: usize = 0;
            while (f < _.cfg.field_count) : (f += 1) {
                avgs[f] = @floatCast((_.total_field_lens[f] as f64) / denom);
            }
        }
        return Bm25fIndexStats{
            active_docs: _.active_docs,
            posting_count: _.active_postings,
            field_count: _.cfg.field_count,
            avg_field_lens: avgs,
        };
    }
}

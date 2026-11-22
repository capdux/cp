// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Query HyDE Centroid & Multi-View RRF/CombMNZ Fusion Retriever) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-query expansion & multi-view retrieval fusion engine:
//   - Multi-view query vector weighted centroid fusion (`fuseQueryVectorsF32`): combines original query +
//     up to 8 HyDE/rewritten sub-query embeddings with `@fma` spherical weighted averaging & L2 normalization,
//     automatically filtering NaN/Inf/zero vectors and non-positive weights
//   - Multi-view ranked result deduplication & fusion (`MultiQueryFusion`): up to 8 query views x 32 hits each,
//     supporting `weighted_rrf` (per-view weights + configurable `rrf_k`) and `comb_mnz` (consensus-amplified
//     min-max normalized fusion), tracking per-doc `consensus_mask: u8`, `consensus_count` & `best_rank`
//   - Diversity & coverage telemetry (`QueryFusionReport`: raw hits, unique docs, consensus & unanimous counts)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of query views (original query + HyDE / rewritten sub-queries).
pub const QFUSION_MAX_VIEWS: usize = 8;

/// Maximum ranked hits accepted per query view.
pub const QFUSION_MAX_HITS_PER_VIEW: usize = 32;

/// Maximum unique documents aggregated across all views (`8 * 32 = 256`, zero dynamic heap).
pub const QFUSION_MAX_UNIQUE_DOCS: usize = 256;

/// Standard RRF damping constant `k = 60.0`.
pub const QFUSION_DEFAULT_RRF_K: f32 = 60.0;

/// Multi-view score fusion strategy.
pub enum QueryFusionStrategy {
    weighted_rrf,
    comb_mnz,
}

/// Telemetry report returned by `fuseQueryVectorsF32`.
pub struct QueryCentroidReport {
    valid: bool = false,
    dim: usize = 0,
    total_views: usize = 0,
    used_views: usize = 0,
    skipped_views: usize = 0,
    total_weight: f32 = 0.0,
}

/// Single ranked hit produced by one sub-query view.
pub struct QueryViewHit {
    doc_id: u64 = 0,
    score: f32 = 0.0,
}

/// Deduplicated, fused retrieval hit with multi-view attribution.
pub struct FusedDocHit {
    doc_id: u64 = 0,
    fused_score: f32 = 0.0,
    /// 8-bit bitmask where bit `v` is set iff view `v` retrieved this document.
    consensus_mask: u8 = 0,
    /// Number of distinct query views that retrieved this document (`1..=8`).
    consensus_count: u8 = 0,
    /// Best (lowest) 0-based rank achieved across all views that retrieved this document.
    best_rank: u16 = 0,
}

/// Configuration for `MultiQueryFusion`.
pub struct QueryFusionConfig {
    strategy: QueryFusionStrategy = QueryFusionStrategy.weighted_rrf,
    /// Damping constant `k > 0` used by `weighted_rrf` (non-positive falls back to 60.0).
    rrf_k: f32 = QFUSION_DEFAULT_RRF_K,
}

/// Diversity, coverage & consensus telemetry returned by `MultiQueryFusion.fuse`.
pub struct QueryFusionReport {
    views_added: usize = 0,
    total_raw_hits: usize = 0,
    unique_docs: usize = 0,
    /// Number of unique documents retrieved by at least 2 distinct views.
    consensus_docs: usize = 0,
    /// Number of unique documents retrieved by every added view (`consensus_count == views_added`).
    unanimous_docs: usize = 0,
    output_count: usize = 0,
}

// ============================================================================
// 2. Internal Numeric Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return (x - x) == 0.0;
}

fn fusedHitPrecedes(a: FusedDocHit, b: FusedDocHit): bool {
    if (a.fused_score != b.fused_score) return a.fused_score > b.fused_score;
    if (a.consensus_count != b.consensus_count) return a.consensus_count > b.consensus_count;
    if (a.best_rank != b.best_rank) return a.best_rank < b.best_rank;
    return a.doc_id < b.doc_id;
}

// ============================================================================
// 3. Multi-View HyDE / Sub-Query Weighted Vector Centroid Fusion
// ============================================================================

/// Computes a weighted spherical centroid from up to `QFUSION_MAX_VIEWS` query embeddings
/// using hardware `@fma` and writes the L2-normalized result into `out_centroid[0..dim]`.
/// Automatically skips views with NaN/Inf components, zero L2 norm, or non-positive weights.
pub fn fuseQueryVectorsF32(
    vecs_flat: []const f32,
    weights: []const f32,
    num_views: usize,
    dim: usize,
    out_centroid: *[]f32,
): QueryCentroidReport {
    let rep = QueryCentroidReport{
        valid: false,
        dim: dim,
        total_views: num_views,
    };

    if (dim == 0 || num_views == 0 || num_views > QFUSION_MAX_VIEWS) return rep;
    if (vecs_flat.len < num_views * dim || weights.len < num_views || out_centroid.len < dim) {
        return rep;
    }

    let d: usize = 0;
    while (d < dim) : (d += 1) {
        out_centroid[d] = 0.0;
    }

    let v: usize = 0;
    while (v < num_views) : (v += 1) {
        const w = weights[v];
        if (!isFiniteF32(w) || w <= 0.0) {
            rep.skipped_views += 1;
            continue;
        }

        const base = v * dim;
        let norm_sq: f32 = 0.0;
        let ok_vec: bool = true;
        let i: usize = 0;
        while (i < dim) : (i += 1) {
            const val = vecs_flat[base + i];
            if (!isFiniteF32(val)) {
                ok_vec = false;
                break;
            }
            norm_sq = @fma(val, val, norm_sq);
        }

        if (!ok_vec || !isFiniteF32(norm_sq) || norm_sq <= 1.0e-12) {
            rep.skipped_views += 1;
            continue;
        }

        const scaled_w: f32 = w / @sqrt(norm_sq);
        let j: usize = 0;
        while (j < dim) : (j += 1) {
            out_centroid[j] = @fma(vecs_flat[base + j], scaled_w, out_centroid[j]);
        }

        rep.used_views += 1;
        rep.total_weight += w;
    }

    if (rep.used_views == 0) return rep;

    // L2 normalize the final weighted centroid
    let c_norm_sq: f32 = 0.0;
    let k: usize = 0;
    while (k < dim) : (k += 1) {
        c_norm_sq = @fma(out_centroid[k], out_centroid[k], c_norm_sq);
    }
    if (!isFiniteF32(c_norm_sq) || c_norm_sq <= 1.0e-12) {
        return rep;
    }

    const inv_c_norm: f32 = 1.0 / @sqrt(c_norm_sq);
    let m: usize = 0;
    while (m < dim) : (m += 1) {
        out_centroid[m] *= inv_c_norm;
    }

    rep.valid = true;
    return rep;
}

// ============================================================================
// 4. Multi-View Result Deduplication & RRF / CombMNZ Fusion Engine
// ============================================================================

struct StoredQueryView {
    active: bool = false,
    weight: f32 = 1.0,
    hit_count: usize = 0,
    hits: [QFUSION_MAX_HITS_PER_VIEW]QueryViewHit = [QueryViewHit{}; QFUSION_MAX_HITS_PER_VIEW],
}

/// Zero-heap-allocation multi-query ranked result deduplicator & RRF / CombMNZ fusion engine.
pub struct MultiQueryFusion {
    config: QueryFusionConfig = QueryFusionConfig{},
    views: [QFUSION_MAX_VIEWS]StoredQueryView = [StoredQueryView{}; QFUSION_MAX_VIEWS],
    view_count: usize = 0,

    /// Initializes a fusion session with `cfg` (non-positive `rrf_k` normalizes to 60.0).
    pub fn init(cfg: QueryFusionConfig): MultiQueryFusion {
        const k = if (!isFiniteF32(cfg.rrf_k) || cfg.rrf_k <= 0.0) QFUSION_DEFAULT_RRF_K else cfg.rrf_k;
        return MultiQueryFusion{
            config: QueryFusionConfig{
                strategy: cfg.strategy,
                rrf_k: k,
            },
        };
    }

    /// Clears all added query views while preserving the fusion configuration.
    pub fn reset(*_) {
        let i: usize = 0;
        while (i < QFUSION_MAX_VIEWS) : (i += 1) {
            _.views[i] = StoredQueryView{};
        }
        _.view_count = 0;
    }

    /// Returns the number of query views currently registered.
    pub fn viewCount(): usize {
        return _.view_count;
    }

    /// Adds a ranked hit list for one query view with positive `weight`.
    /// Deduplicates repeated `doc_id` entries within the same view (retaining highest rank)
    /// and caps stored hits at `QFUSION_MAX_HITS_PER_VIEW` (32).
    pub fn addView(*_, weight: f32, hits: []const QueryViewHit): bool {
        if (_.view_count >= QFUSION_MAX_VIEWS) return false;
        if (!isFiniteF32(weight) || weight <= 0.0) return false;

        const v_idx = _.view_count;
        _.views[v_idx] = StoredQueryView{
            active: true,
            weight: weight,
            hit_count: 0,
        };

        let i: usize = 0;
        while (i < hits.len && _.views[v_idx].hit_count < QFUSION_MAX_HITS_PER_VIEW) : (i += 1) {
            const h = hits[i];
            // Deduplicate within the same view (keep best/first occurrence)
            let dup: bool = false;
            let j: usize = 0;
            while (j < _.views[v_idx].hit_count) : (j += 1) {
                if (_.views[v_idx].hits[j].doc_id == h.doc_id) {
                    dup = true;
                    break;
                }
            }
            if (dup) continue;

            const clean_score: f32 = if (isFiniteF32(h.score)) h.score else 0.0;
            const dst = _.views[v_idx].hit_count;
            _.views[v_idx].hits[dst] = QueryViewHit{
                doc_id: h.doc_id,
                score: clean_score,
            };
            _.views[v_idx].hit_count += 1;
        }

        _.view_count += 1;
        return true;
    }

    /// Fuses all registered views into a deduplicated, ranked list written to `out_hits`.
    /// Returns coverage, diversity & consensus attribution telemetry.
    pub fn fuse(out_hits: *[]FusedDocHit): QueryFusionReport {
        let rep = QueryFusionReport{
            views_added: _.view_count,
        };
        if (_.view_count == 0) return rep;

        let docs: [QFUSION_MAX_UNIQUE_DOCS]FusedDocHit = [FusedDocHit{}; QFUSION_MAX_UNIQUE_DOCS];
        let unique_n: usize = 0;

        let v: usize = 0;
        while (v < _.view_count) : (v += 1) {
            const view = _.views[v];
            if (!view.active || view.hit_count == 0) continue;

            // Precompute min/max scores for CombMNZ min-max normalization
            let min_s: f32 = view.hits[0].score;
            let max_s: f32 = view.hits[0].score;
            if (_.config.strategy == QueryFusionStrategy.comb_mnz) {
                let s_i: usize = 1;
                while (s_i < view.hit_count) : (s_i += 1) {
                    const sc = view.hits[s_i].score;
                    if (sc < min_s) min_s = sc;
                    if (sc > max_s) max_s = sc;
                }
            }
            const span: f32 = max_s - min_s;
            const view_bit: u8 = (1 as u8) << (v as u3);

            let r: usize = 0;
            while (r < view.hit_count) : (r += 1) {
                const hit = view.hits[r];
                rep.total_raw_hits += 1;

                const contrib: f32 = if (_.config.strategy == QueryFusionStrategy.weighted_rrf)
                    (view.weight / (_.config.rrf_k + (r as f32) + 1.0))
                else
                    (view.weight * (if (span > 1.0e-12) ((hit.score - min_s) / span) else 1.0));

                // Locate or insert document in deduplicated accumulator
                let found_idx: usize? = null;
                let u: usize = 0;
                while (u < unique_n) : (u += 1) {
                    if (docs[u].doc_id == hit.doc_id) {
                        found_idx = u;
                        break;
                    }
                }

                if (idx := found_idx) {
                    docs[idx].fused_score += contrib;
                    docs[idx].consensus_mask |= view_bit;
                    docs[idx].consensus_count += 1;
                    if ((r as u16) < docs[idx].best_rank) {
                        docs[idx].best_rank = r as u16;
                    }
                } else if (unique_n < QFUSION_MAX_UNIQUE_DOCS) {
                    docs[unique_n] = FusedDocHit{
                        doc_id: hit.doc_id,
                        fused_score: contrib,
                        consensus_mask: view_bit,
                        consensus_count: 1,
                        best_rank: r as u16,
                    };
                    unique_n += 1;
                }
            }
        }

        // CombMNZ consensus amplification & consensus/unanimous telemetry counting
        let d: usize = 0;
        while (d < unique_n) : (d += 1) {
            if (_.config.strategy == QueryFusionStrategy.comb_mnz) {
                docs[d].fused_score *= docs[d].consensus_count as f32;
            }
            if (docs[d].consensus_count >= 2) {
                rep.consensus_docs += 1;
            }
            if ((docs[d].consensus_count as usize) == _.view_count) {
                rep.unanimous_docs += 1;
            }
        }

        rep.unique_docs = unique_n;

        // In-place insertion sort by descending fused_score (tie-break: consensus_count, best_rank, doc_id)
        let k: usize = 1;
        while (k < unique_n) : (k += 1) {
            const key = docs[k];
            let pos: usize = k;
            while (pos > 0 && fusedHitPrecedes(key, docs[pos - 1])) {
                docs[pos] = docs[pos - 1];
                pos -= 1;
            }
            docs[pos] = key;
        }

        const written = if (unique_n < out_hits.len) unique_n else out_hits.len;
        let w: usize = 0;
        while (w < written) : (w += 1) {
            out_hits[w] = docs[w];
        }
        rep.output_count = written;
        return rep;
    }
}

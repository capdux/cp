// Origin: Pure-CP Stdlib (Zero-Alloc Streaming Semantic Near-Duplicate Detector & Online Centroid Clustering) | Created: 2026-11-23
// Pure CP zero-heap-allocation streaming semantic deduplication & online centroid clustering engine
// for RAG corpus ingestion pipelines:
//   - Fixed 128-cluster semantic table (supports dimensions 1..64) storing L2-normalized centroids,
//     canonical representative `doc_id`, member count, and intra-cluster min/avg similarity
//   - Single-pass SIMD `@dot` streaming `ingest(doc_id, vec)` with NaN/Inf/zero-vector protection,
//     configurable cosine similarity threshold, and optional online running-mean centroid updates
//   - Zero-alloc batch corpus cleaning (`dedupBatch`), Top-K nearest cluster search (`findNearestClusters`),
//     and weighted centroid cluster compaction (`mergeClusters`)
//   - Full deduplication telemetry snapshot (`SemanticDedupStats` with `dedup_ratio_bps`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of active semantic clusters stored inline (zero dynamic heap).
pub const SEM_DEDUP_MAX_CLUSTERS: usize = 128;

/// Maximum supported embedding dimension (`1..64`).
pub const SEM_DEDUP_MAX_DIM: usize = 64;

/// Basis-point scale (10000 bps = 100.00%) for deduplication ratio telemetry.
pub const SEM_DEDUP_BPS_SCALE: u32 = 10_000;

/// Verdict returned by `SemanticDeduplicator.ingest`.
pub enum SemanticDedupStatus {
    novel,
    duplicate,
    capacity_full,
    invalid_vector,
}

/// Configuration for `SemanticDeduplicator`.
pub struct SemanticDedupConfig {
    dim: usize = 8;
    sim_threshold: f32 = 0.92;
    update_centroid: bool = true;
    min_norm_eps: f32 = 0.000001;

    /// Construct a validated default config for `dim` and `sim_threshold`.
    static fn make(dim: usize, sim_threshold: f32, update_centroid: bool): SemanticDedupConfig {
        return SemanticDedupConfig{
            dim: dim,
            sim_threshold: sim_threshold,
            update_centroid: update_centroid,
            min_norm_eps: 0.000001,
        };
    }

    pub fn isValid(): bool {
        if (dim == 0 or dim > SEM_DEDUP_MAX_DIM) return false;
        if (!isFiniteF32(sim_threshold)) return false;
        if (sim_threshold < -1.0 or sim_threshold > 1.0) return false;
        if (!isFiniteF32(min_norm_eps) or min_norm_eps <= 0.0) return false;
        return true;
    }
}

/// Inline semantic cluster storing an L2-normalized centroid and membership statistics.
pub struct SemanticCluster {
    active: bool = false;
    cluster_id: u32 = 0;
    canonical_doc_id: u64 = 0;
    member_count: u32 = 0;
    min_member_sim: f32 = 1.0;
    sum_member_sim: f32 = 1.0;
    centroid: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];

    pub fn avgMemberSim(): f32 {
        if (member_count == 0) return 0.0;
        return sum_member_sim / @as(f32, @floatFromInt(member_count));
    }
}

/// Decision returned for a single document ingested into `SemanticDeduplicator`.
pub struct SemanticDedupDecision {
    status: SemanticDedupStatus = SemanticDedupStatus.invalid_vector;
    kept: bool = false;
    cluster_id: u32 = 0;
    canonical_doc_id: u64 = 0;
    similarity: f32 = 0.0;
    cluster_member_count: u32 = 0;
}

/// Nearest-cluster match returned by `findNearestClusters`.
pub struct SemanticClusterHit {
    cluster_id: u32 = 0;
    canonical_doc_id: u64 = 0;
    similarity: f32 = 0.0;
    member_count: u32 = 0;
}

/// Summary report returned by `dedupBatch`.
pub struct SemanticDedupBatchReport {
    processed: usize = 0;
    novel_count: usize = 0;
    duplicate_count: usize = 0;
    invalid_count: usize = 0;
    capacity_full_count: usize = 0;
    kept_written: usize = 0;
    dedup_ratio_bps: u32 = 0;
}

/// Zero-heap global telemetry snapshot for `SemanticDeduplicator`.
pub struct SemanticDedupStats {
    dim: usize = 0;
    active_clusters: usize = 0;
    cluster_capacity: usize = SEM_DEDUP_MAX_CLUSTERS;
    total_ingested: u64 = 0;
    novel_count: u64 = 0;
    duplicate_count: u64 = 0;
    invalid_count: u64 = 0;
    capacity_full_count: u64 = 0;
    merged_clusters: u64 = 0;
    dedup_ratio_bps: u32 = 0;
}

// ============================================================================
// 2. Internal SIMD & Numerical Hygiene Helpers
// ============================================================================

fn isFiniteF32(v: f32): bool {
    if (v != v) return false; // NaN
    if (v > 3.4e38 or v < -3.4e38) return false; // +/-Inf
    return true;
}

fn clampCosineF32(v: f32): f32 {
    if (v > 1.0) return 1.0;
    if (v < -1.0) return -1.0;
    return v;
}

/// Validate `vec` for length, finite components, and non-zero L2 norm, writing unit vector to `out_unit`.
fn normalizeVecF32(
    vec: []const f32,
    dim: usize,
    min_norm_eps: f32,
    out_unit: *[]f32,
): bool {
    if (vec.len != dim or out_unit.len < dim) return false;
    let i: usize = 0;
    while (i < dim) : (i += 1) {
        if (!isFiniteF32(vec[i])) return false;
    }
    const norm_sq: f32 = @dot(vec, vec);
    if (!isFiniteF32(norm_sq)) return false;
    const min_sq = min_norm_eps * min_norm_eps;
    if (norm_sq <= min_sq) return false;

    const inv_norm: f32 = 1.0 / @sqrt(norm_sq);
    if (!isFiniteF32(inv_norm)) return false;

    i = 0;
    while (i < dim) : (i += 1) {
        out_unit[i] = vec[i] * inv_norm;
    }
    return true;
}

fn computeDedupBps(novel: u64, dup: u64): u32 {
    const valid_total = novel + dup;
    if (valid_total == 0) return 0;
    return @as(u32, @intCast((dup * @as(u64, SEM_DEDUP_BPS_SCALE)) / valid_total));
}

// ============================================================================
// 3. SemanticDeduplicator — Zero-Heap Streaming Near-Duplicate Engine
// ============================================================================

/// Zero-heap-allocation streaming semantic near-duplicate detector & online centroid clusterer.
pub struct SemanticDeduplicator {
    config: SemanticDedupConfig = SemanticDedupConfig{},
    clusters: [SEM_DEDUP_MAX_CLUSTERS]SemanticCluster = [SemanticCluster{}; SEM_DEDUP_MAX_CLUSTERS];
    active_clusters: usize = 0;
    next_cluster_id: u32 = 1;

    total_ingested: u64 = 0;
    novel_count: u64 = 0;
    duplicate_count: u64 = 0;
    invalid_count: u64 = 0;
    capacity_full_count: u64 = 0;
    merged_clusters: u64 = 0;

    /// Initialize a streaming semantic deduplicator; returns `null` if config is invalid.
    static fn init(cfg: SemanticDedupConfig): SemanticDeduplicator? {
        if (!cfg.isValid()) return null;
        let dedup = SemanticDeduplicator{};
        dedup.config = cfg;
        return dedup;
    }

    /// Clear all clusters and reset counters while preserving configuration.
    pub fn reset(*_) {
        let i: usize = 0;
        while (i < SEM_DEDUP_MAX_CLUSTERS) : (i += 1) {
            clusters[i] = SemanticCluster{};
        }
        active_clusters = 0;
        next_cluster_id = 1;
        total_ingested = 0;
        novel_count = 0;
        duplicate_count = 0;
        invalid_count = 0;
        capacity_full_count = 0;
        merged_clusters = 0;
    }

    /// Return a read-only copy of a cluster by its `cluster_id`, or `null` if not found.
    pub fn getClusterById(cluster_id: u32): SemanticCluster? {
        if (cluster_id == 0) return null;
        let i: usize = 0;
        while (i < SEM_DEDUP_MAX_CLUSTERS) : (i += 1) {
            if (clusters[i].active and clusters[i].cluster_id == cluster_id) {
                return clusters[i];
            }
        }
        return null;
    }

    /// Single-pass online streaming deduplication using SIMD `@dot` cosine similarity.
    pub fn ingest(*_, doc_id: u64, vec: []const f32): SemanticDedupDecision {
        total_ingested += 1;
        const dim = config.dim;

        let unit_buf: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];
        let unit_s: []f32 = unit_buf[0..dim];
        if (!normalizeVecF32(vec, dim, config.min_norm_eps, *unit_s)) {
            invalid_count += 1;
            return SemanticDedupDecision{
                status: SemanticDedupStatus.invalid_vector,
                kept: false,
            };
        }

        const unit_vec: []const f32 = unit_buf[0..dim];

        // Find the nearest active cluster via SIMD `@dot`.
        let best_slot: usize = SEM_DEDUP_MAX_CLUSTERS;
        let best_sim: f32 = -2.0;
        let free_slot: usize = SEM_DEDUP_MAX_CLUSTERS;

        let i: usize = 0;
        while (i < SEM_DEDUP_MAX_CLUSTERS) : (i += 1) {
            if (clusters[i].active) {
                const c_slice: []const f32 = clusters[i].centroid[0..dim];
                const sim = clampCosineF32(@dot(unit_vec, c_slice));
                if (sim > best_sim) {
                    best_sim = sim;
                    best_slot = i;
                }
            } else if (free_slot == SEM_DEDUP_MAX_CLUSTERS) {
                free_slot = i;
            }
        }

        // Duplicate hit: best_sim >= sim_threshold
        if (best_slot != SEM_DEDUP_MAX_CLUSTERS and best_sim >= config.sim_threshold) {
            duplicate_count += 1;
            const old_cnt = clusters[best_slot].member_count;
            clusters[best_slot].member_count = old_cnt + 1;
            if (best_sim < clusters[best_slot].min_member_sim) {
                clusters[best_slot].min_member_sim = best_sim;
            }
            clusters[best_slot].sum_member_sim += best_sim;

            if (config.update_centroid) {
                const w_old = @as(f32, @floatFromInt(old_cnt));
                let tmp_buf: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];
                let k: usize = 0;
                while (k < dim) : (k += 1) {
                    tmp_buf[k] = clusters[best_slot].centroid[k] * w_old + unit_buf[k];
                }
                let renorm_buf: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];
                let renorm_s: []f32 = renorm_buf[0..dim];
                if (normalizeVecF32(tmp_buf[0..dim], dim, config.min_norm_eps, *renorm_s)) {
                    k = 0;
                    while (k < dim) : (k += 1) {
                        clusters[best_slot].centroid[k] = renorm_buf[k];
                    }
                }
            }

            return SemanticDedupDecision{
                status: SemanticDedupStatus.duplicate,
                kept: false,
                cluster_id: clusters[best_slot].cluster_id,
                canonical_doc_id: clusters[best_slot].canonical_doc_id,
                similarity: best_sim,
                cluster_member_count: clusters[best_slot].member_count,
            };
        }

        // Novel document: allocate a new semantic cluster if capacity allows.
        if (free_slot == SEM_DEDUP_MAX_CLUSTERS) {
            capacity_full_count += 1;
            return SemanticDedupDecision{
                status: SemanticDedupStatus.capacity_full,
                kept: false,
                similarity: if (best_slot != SEM_DEDUP_MAX_CLUSTERS) best_sim else 0.0,
            };
        }

        const cid = next_cluster_id;
        next_cluster_id += 1;

        let c = SemanticCluster{};
        c.active = true;
        c.cluster_id = cid;
        c.canonical_doc_id = doc_id;
        c.member_count = 1;
        c.min_member_sim = 1.0;
        c.sum_member_sim = 1.0;
        let k: usize = 0;
        while (k < dim) : (k += 1) {
            c.centroid[k] = unit_buf[k];
        }

        clusters[free_slot] = c;
        active_clusters += 1;
        novel_count += 1;

        return SemanticDedupDecision{
            status: SemanticDedupStatus.novel,
            kept: true,
            cluster_id: cid,
            canonical_doc_id: doc_id,
            similarity: if (best_slot != SEM_DEDUP_MAX_CLUSTERS) best_sim else 0.0,
            cluster_member_count: 1,
        };
    }

    /// Clean a flat batch of `(doc_ids[i], vecs_flat[i*dim .. (i+1)*dim])` embeddings in one pass.
    /// Writes kept (novel) `doc_id`s into `out_kept_ids` (safely bounded by `out_kept_ids.len`).
    pub fn dedupBatch(
        *_,
        doc_ids: []const u64,
        vecs_flat: []const f32,
        out_kept_ids: *[]u64,
    ): SemanticDedupBatchReport {
        const dim = config.dim;
        const max_vecs = vecs_flat.len / dim;
        const count = if (doc_ids.len < max_vecs) doc_ids.len else max_vecs;

        let report = SemanticDedupBatchReport{};
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const base = i * dim;
            const verdict = ingest(doc_ids[i], vecs_flat[base .. base + dim]);
            report.processed += 1;
            if (verdict.status == SemanticDedupStatus.novel) {
                report.novel_count += 1;
                if (report.kept_written < out_kept_ids.len) {
                    out_kept_ids[report.kept_written] = doc_ids[i];
                    report.kept_written += 1;
                }
            } else if (verdict.status == SemanticDedupStatus.duplicate) {
                report.duplicate_count += 1;
            } else if (verdict.status == SemanticDedupStatus.capacity_full) {
                report.capacity_full_count += 1;
            } else {
                report.invalid_count += 1;
            }
        }

        report.dedup_ratio_bps = computeDedupBps(
            @as(u64, @intCast(report.novel_count)),
            @as(u64, @intCast(report.duplicate_count)),
        );
        return report;
    }

    /// Query Top-K nearest active semantic clusters to `vec` in descending cosine similarity order.
    pub fn findNearestClusters(
        vec: []const f32,
        top_k: usize,
        out_hits: *[]SemanticClusterHit,
    ): usize {
        const k_limit = if (top_k < out_hits.len) top_k else out_hits.len;
        if (k_limit == 0 or active_clusters == 0) return 0;

        const dim = config.dim;
        let unit_buf: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];
        let unit_s: []f32 = unit_buf[0..dim];
        if (!normalizeVecF32(vec, dim, config.min_norm_eps, *unit_s)) return 0;
        const unit_vec: []const f32 = unit_buf[0..dim];

        let count: usize = 0;
        let i: usize = 0;
        while (i < SEM_DEDUP_MAX_CLUSTERS) : (i += 1) {
            if (!clusters[i].active) continue;
            const c_slice: []const f32 = clusters[i].centroid[0..dim];
            const sim = clampCosineF32(@dot(unit_vec, c_slice));
            const hit = SemanticClusterHit{
                cluster_id: clusters[i].cluster_id,
                canonical_doc_id: clusters[i].canonical_doc_id,
                similarity: sim,
                member_count: clusters[i].member_count,
            };

            if (count < k_limit) {
                out_hits[count] = hit;
                count += 1;
            } else if (sim > out_hits[count - 1].similarity) {
                out_hits[count - 1] = hit;
            } else {
                continue;
            }

            // Insertion bubble-up to keep `out_hits[0..count]` sorted descending by similarity.
            let pos = count - 1;
            while (pos > 0 and out_hits[pos].similarity > out_hits[pos - 1].similarity) : (pos -= 1) {
                const tmp = out_hits[pos - 1];
                out_hits[pos - 1] = out_hits[pos];
                out_hits[pos] = tmp;
            }
        }
        return count;
    }

    /// Compact active clusters by merging pairs whose centroids have cosine similarity `>= merge_threshold`.
    /// Returns the number of merged (freed) clusters.
    pub fn mergeClusters(*_, merge_threshold: f32): usize {
        if (!isFiniteF32(merge_threshold) or merge_threshold < -1.0 or merge_threshold > 1.0) {
            return 0;
        }
        const dim = config.dim;
        let merges: usize = 0;

        let i: usize = 0;
        while (i < SEM_DEDUP_MAX_CLUSTERS) : (i += 1) {
            if (!clusters[i].active) continue;
            let j: usize = i + 1;
            while (j < SEM_DEDUP_MAX_CLUSTERS) : (j += 1) {
                if (!clusters[j].active) continue;
                const ci: []const f32 = clusters[i].centroid[0..dim];
                const cj: []const f32 = clusters[j].centroid[0..dim];
                const sim = clampCosineF32(@dot(ci, cj));
                if (sim >= merge_threshold) {
                    // Merge into the larger cluster (tie-break: lower slot `i`).
                    const keep_idx = if (clusters[i].member_count >= clusters[j].member_count) i else j;
                    const drop_idx = if (keep_idx == i) j else i;

                    const w_keep = @as(f32, @floatFromInt(clusters[keep_idx].member_count));
                    const w_drop = @as(f32, @floatFromInt(clusters[drop_idx].member_count));
                    let tmp_buf: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];
                    let k: usize = 0;
                    while (k < dim) : (k += 1) {
                        tmp_buf[k] = clusters[keep_idx].centroid[k] * w_keep +
                            clusters[drop_idx].centroid[k] * w_drop;
                    }
                    let renorm_buf: [SEM_DEDUP_MAX_DIM]f32 = [0.0; SEM_DEDUP_MAX_DIM];
                    let renorm_s: []f32 = renorm_buf[0..dim];
                    if (normalizeVecF32(tmp_buf[0..dim], dim, config.min_norm_eps, *renorm_s)) {
                        k = 0;
                        while (k < dim) : (k += 1) {
                            clusters[keep_idx].centroid[k] = renorm_buf[k];
                        }
                    }

                    clusters[keep_idx].member_count += clusters[drop_idx].member_count;
                    clusters[keep_idx].sum_member_sim += clusters[drop_idx].sum_member_sim;
                    if (clusters[drop_idx].min_member_sim < clusters[keep_idx].min_member_sim) {
                        clusters[keep_idx].min_member_sim = clusters[drop_idx].min_member_sim;
                    }
                    if (sim < clusters[keep_idx].min_member_sim) {
                        clusters[keep_idx].min_member_sim = sim;
                    }

                    clusters[drop_idx] = SemanticCluster{};
                    if (active_clusters > 0) active_clusters -= 1;
                    merges += 1;
                    merged_clusters += 1;

                    // If outer cluster `i` was dropped, break inner loop.
                    if (drop_idx == i) break;
                }
            }
        }
        return merges;
    }

    /// Return a zero-heap global telemetry snapshot (including `dedup_ratio_bps`).
    pub fn stats(): SemanticDedupStats {
        return SemanticDedupStats{
            dim: config.dim,
            active_clusters: active_clusters,
            cluster_capacity: SEM_DEDUP_MAX_CLUSTERS,
            total_ingested: total_ingested,
            novel_count: novel_count,
            duplicate_count: duplicate_count,
            invalid_count: invalid_count,
            capacity_full_count: capacity_full_count,
            merged_clusters: merged_clusters,
            dedup_ratio_bps: computeDedupBps(novel_count, duplicate_count),
        };
    }
}

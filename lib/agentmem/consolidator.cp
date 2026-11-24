// Origin: Pure-CP Stdlib (Zero-Alloc Agent Memory Consolidation, Near-Duplicate FMA Merging, Conflict Superseding & Ebbinghaus Decay Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation long-horizon AI Agent memory consolidation engine:
//   - Fixed-capacity inline episodic (`MEM_CONSOL_MAX_EPISODIC = 64`) and semantic (`MEM_CONSOL_MAX_SEMANTIC = 64`)
//     memory pools with normalized embeddings (up to `MEM_CONSOL_MAX_DIM = 32`)
//   - Native `@dot` cosine similarity & `@fma` importance-weighted centroid fusion
//   - Ebbinghaus exponential decay + importance/access-frequency retention scoring (`computeEntryRetentionScore`)
//   - Unified consolidation pass (`runConsolidationPass`):
//     1) Entity conflict/overwrite detection (`entity_key_hash` match + low cosine -> supersede older record)
//     2) Near-duplicate episodic clustering, `@fma` centroid merging & promotion to semantic memory
//     3) TTL expiration & low-retention archival pruning with strict `pinned` protection

const std = @import("std");

/// Maximum episodic memory entries stored inline (zero dynamic heap).
pub const MEM_CONSOL_MAX_EPISODIC: usize = 64;

/// Maximum semantic memory entries stored inline (zero dynamic heap).
pub const MEM_CONSOL_MAX_SEMANTIC: usize = 64;

/// Maximum embedding dimension stored inline per memory entry.
pub const MEM_CONSOL_MAX_DIM: usize = 32;

/// Sentinel empty memory identifier.
pub const MEM_CONSOL_NO_ID: u64 = 0;

const NORM_SQ_EPS: f32 = 1.0e-12;
const LN2_F64: f64 = 0.6931471805599453;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn allFiniteF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

fn clamp01F32(x: f32): f32 {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

// ============================================================================
// 1. Public Enums, Data Models & Retention Scoring Function
// ============================================================================

/// Lifecycle state of a memory record inside `AgentMemoryConsolidator`.
pub enum MemConsolStatus {
    active,
    merged,
    archived,
    evicted,
    superseded,
}

/// Memory record stored in either the episodic or semantic pool.
pub struct MemConsolEntry {
    memory_id: u64 = MEM_CONSOL_NO_ID;
    entity_key_hash: u64 = 0;
    source_tag: u32 = 0;
    importance: f32 = 0.5;
    access_count: u32 = 1;
    created_ms: u64 = 0;
    last_access_ms: u64 = 0;
    pinned: bool = false;
    status: MemConsolStatus = MemConsolStatus.active;
    superseded_by_id: u64 = MEM_CONSOL_NO_ID;
    merged_into_semantic_id: u64 = MEM_CONSOL_NO_ID;
    cluster_members: u32 = 1;
    vec: [MEM_CONSOL_MAX_DIM]f32 = [0.0; MEM_CONSOL_MAX_DIM];
    occupied: bool = false;
}

/// Policy parameters governing a consolidation pass.
pub struct MemConsolPassConfig {
    half_life_ms: u64 = 3_600_000;
    ttl_ms: u64 = 86_400_000;
    merge_threshold: f32 = 0.85;
    conflict_threshold: f32 = 0.35;
    prune_threshold: f32 = 0.20;
    importance_boost_on_merge: f32 = 0.10;
    min_cluster_size_for_promote: u32 = 2;

    /// Returns production-tuned default consolidation policy.
    static fn defaults(): MemConsolPassConfig {
        return MemConsolPassConfig{
            half_life_ms: 3_600_000,
            ttl_ms: 86_400_000,
            merge_threshold: 0.85,
            conflict_threshold: 0.35,
            prune_threshold: 0.20,
            importance_boost_on_merge: 0.10,
            min_cluster_size_for_promote: 2,
        };
    }

    /// Validates all thresholds, half-life, and cluster parameters.
    fn isValid(): bool {
        if (_.half_life_ms == 0) return false;
        if (_.min_cluster_size_for_promote == 0) return false;
        if (!isFiniteF32(_.merge_threshold) || _.merge_threshold < -1.0 || _.merge_threshold > 1.0) return false;
        if (!isFiniteF32(_.conflict_threshold) || _.conflict_threshold < -1.0 || _.conflict_threshold > 1.0) return false;
        if (!isFiniteF32(_.prune_threshold) || _.prune_threshold < 0.0 || _.prune_threshold > 1.0) return false;
        if (!isFiniteF32(_.importance_boost_on_merge) || _.importance_boost_on_merge < 0.0 || _.importance_boost_on_merge > 1.0) {
            return false;
        }
        return true;
    }
}

/// Detailed telemetry returned by `AgentMemoryConsolidator.runConsolidationPass`.
pub struct MemConsolPassReport {
    clusters_merged: usize = 0;
    episodic_merged_count: usize = 0;
    semantic_promoted_count: usize = 0;
    conflicts_superseded: usize = 0;
    pruned_archived_count: usize = 0;
    evicted_count: usize = 0;
    pinned_protected_count: usize = 0;
}

/// Aggregate operational statistics for `AgentMemoryConsolidator`.
pub struct MemConsolEngineStats {
    dim: usize = 0;
    active_episodic: usize = 0;
    active_semantic: usize = 0;
    total_episodic_slots: usize = 0;
    total_semantic_slots: usize = 0;
    total_passes: u64 = 0;
    total_promoted: u64 = 0;
    total_superseded: u64 = 0;
    total_pruned: u64 = 0;
}

/// Computes Ebbinghaus exponential decay + importance + access-frequency retention score in `[0.0, 1.0]`.
/// Pinned entries always return `1.0`.
pub fn computeEntryRetentionScore(entry: MemConsolEntry, now_ms: u64, half_life_ms: u64): f32 {
    if (!entry.occupied) return 0.0;
    if (entry.pinned) return 1.0;

    const hl: f64 = if (half_life_ms > 0) (half_life_ms as f64) else 1.0;
    const elapsed: f64 = if (now_ms > entry.last_access_ms) ((now_ms - entry.last_access_ms) as f64) else 0.0;

    // Higher access_count increases Ebbinghaus memory stability (slower decay).
    const freq_boost: f64 = 1.0 + 0.35 * @log((1 + entry.access_count) as f64);
    const decay: f64 = @exp((-LN2_F64 * elapsed) / (hl * freq_boost));

    const imp: f64 = clamp01F32(entry.importance) as f64;
    const capped_acc: u32 = if (entry.access_count > 10) 10 else entry.access_count;
    const freq_mult: f64 = 1.0 + 0.10 * ((capped_acc as f64) / 10.0);
    const raw: f64 = (0.55 * decay + 0.45 * imp) * freq_mult;
    return clamp01F32(raw as f32);
}

// ============================================================================
// 2. Zero-Alloc Agent Memory Consolidation & Decay Engine
// ============================================================================

/// Zero-heap-allocation episodic-to-semantic memory consolidator with `@dot`/`@fma` clustering,
/// entity conflict superseding, and Ebbinghaus decay pruning.
pub struct AgentMemoryConsolidator {
    dim: usize = 0;
    episodic: [MEM_CONSOL_MAX_EPISODIC]MemConsolEntry = [MemConsolEntry{}; MEM_CONSOL_MAX_EPISODIC];
    semantic: [MEM_CONSOL_MAX_SEMANTIC]MemConsolEntry = [MemConsolEntry{}; MEM_CONSOL_MAX_SEMANTIC];
    next_auto_semantic_id: u64 = 1_000_000;
    total_passes: u64 = 0;
    total_promoted: u64 = 0;
    total_superseded: u64 = 0;
    total_pruned: u64 = 0;

    /// Initializes a consolidator for normalized embeddings of dimension `dim` (`1..=32`).
    static fn init(dim: usize): AgentMemoryConsolidator? {
        if (dim == 0 || dim > MEM_CONSOL_MAX_DIM) return null;
        return AgentMemoryConsolidator{
            dim: dim,
            episodic: [MemConsolEntry{}; MEM_CONSOL_MAX_EPISODIC],
            semantic: [MemConsolEntry{}; MEM_CONSOL_MAX_SEMANTIC],
            next_auto_semantic_id: 1_000_000,
            total_passes: 0,
            total_promoted: 0,
            total_superseded: 0,
            total_pruned: 0,
        };
    }

    static fn normalizeInto(dst: *[MEM_CONSOL_MAX_DIM]f32, src: []const f32, dim: usize): bool {
        if (src.len != dim || !allFiniteF32(src)) return false;
        const nsq: f32 = @dot(src, src);
        if (!isFiniteF32(nsq) || nsq <= NORM_SQ_EPS) return false;
        const inv_n: f32 = 1.0 / @sqrt(nsq);
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            dst[d] = src[d] * inv_n;
        }
        while (d < MEM_CONSOL_MAX_DIM) : (d += 1) {
            dst[d] = 0.0;
        }
        return true;
    }

    fn cosineBetween(a: [MEM_CONSOL_MAX_DIM]f32, b: [MEM_CONSOL_MAX_DIM]f32): f32 {
        const d: usize = _.dim;
        return clampCosineF32(@dot(a[0..d], b[0..d]));
    }

    /// Inserts a new episodic memory entry with automatic L2 normalization.
    fn addEpisodic(
        *_,
        memory_id: u64,
        entity_key_hash: u64,
        source_tag: u32,
        importance: f32,
        created_ms: u64,
        pinned: bool,
        embedding: []const f32,
    ): bool {
        if (memory_id == MEM_CONSOL_NO_ID) return false;
        if (!isFiniteF32(importance) || importance < 0.0 || importance > 1.0) return false;

        let free_slot: usize = MEM_CONSOL_MAX_EPISODIC;
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_EPISODIC) : (i += 1) {
            if (_.episodic[i].occupied) {
                if (_.episodic[i].memory_id == memory_id) return false;
            } else if (free_slot == MEM_CONSOL_MAX_EPISODIC) {
                free_slot = i;
            }
        }
        if (free_slot >= MEM_CONSOL_MAX_EPISODIC) return false;

        let entry = MemConsolEntry{
            memory_id: memory_id,
            entity_key_hash: entity_key_hash,
            source_tag: source_tag,
            importance: importance,
            access_count: 1,
            created_ms: created_ms,
            last_access_ms: created_ms,
            pinned: pinned,
            status: MemConsolStatus.active,
            superseded_by_id: MEM_CONSOL_NO_ID,
            merged_into_semantic_id: MEM_CONSOL_NO_ID,
            cluster_members: 1,
            occupied: true,
        };
        if (!AgentMemoryConsolidator.normalizeInto(*entry.vec, embedding, _.dim)) return false;
        _.episodic[free_slot] = entry;
        return true;
    }

    /// Inserts a pre-existing semantic memory entry with automatic L2 normalization.
    fn addSemantic(
        *_,
        memory_id: u64,
        entity_key_hash: u64,
        source_tag: u32,
        importance: f32,
        created_ms: u64,
        pinned: bool,
        embedding: []const f32,
    ): bool {
        if (memory_id == MEM_CONSOL_NO_ID) return false;
        if (!isFiniteF32(importance) || importance < 0.0 || importance > 1.0) return false;

        let free_slot: usize = MEM_CONSOL_MAX_SEMANTIC;
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_SEMANTIC) : (i += 1) {
            if (_.semantic[i].occupied) {
                if (_.semantic[i].memory_id == memory_id) return false;
            } else if (free_slot == MEM_CONSOL_MAX_SEMANTIC) {
                free_slot = i;
            }
        }
        if (free_slot >= MEM_CONSOL_MAX_SEMANTIC) return false;

        let entry = MemConsolEntry{
            memory_id: memory_id,
            entity_key_hash: entity_key_hash,
            source_tag: source_tag,
            importance: importance,
            access_count: 1,
            created_ms: created_ms,
            last_access_ms: created_ms,
            pinned: pinned,
            status: MemConsolStatus.active,
            superseded_by_id: MEM_CONSOL_NO_ID,
            merged_into_semantic_id: MEM_CONSOL_NO_ID,
            cluster_members: 1,
            occupied: true,
        };
        if (!AgentMemoryConsolidator.normalizeInto(*entry.vec, embedding, _.dim)) return false;
        _.semantic[free_slot] = entry;
        return true;
    }

    /// Reinforces an episodic entry by incrementing `access_count` and updating `last_access_ms`.
    fn touchEpisodic(*_, memory_id: u64, now_ms: u64): bool {
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_EPISODIC) : (i += 1) {
            if (_.episodic[i].occupied && _.episodic[i].memory_id == memory_id &&
                _.episodic[i].status == MemConsolStatus.active) {
                _.episodic[i].access_count += 1;
                if (now_ms > _.episodic[i].last_access_ms) {
                    _.episodic[i].last_access_ms = now_ms;
                }
                return true;
            }
        }
        return false;
    }

    /// Reinforces a semantic entry by incrementing `access_count` and updating `last_access_ms`.
    fn touchSemantic(*_, memory_id: u64, now_ms: u64): bool {
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_SEMANTIC) : (i += 1) {
            if (_.semantic[i].occupied && _.semantic[i].memory_id == memory_id &&
                _.semantic[i].status == MemConsolStatus.active) {
                _.semantic[i].access_count += 1;
                if (now_ms > _.semantic[i].last_access_ms) {
                    _.semantic[i].last_access_ms = now_ms;
                }
                return true;
            }
        }
        return false;
    }

    /// Looks up an episodic memory entry by `memory_id`.
    fn getEpisodic(memory_id: u64): MemConsolEntry? {
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_EPISODIC) : (i += 1) {
            if (_.episodic[i].occupied && _.episodic[i].memory_id == memory_id) {
                return _.episodic[i];
            }
        }
        return null;
    }

    /// Looks up a semantic memory entry by `memory_id`.
    fn getSemantic(memory_id: u64): MemConsolEntry? {
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_SEMANTIC) : (i += 1) {
            if (_.semantic[i].occupied && _.semantic[i].memory_id == memory_id) {
                return _.semantic[i];
            }
        }
        return null;
    }

    fn allocAutoSemanticId(*_): u64 {
        let cand: u64 = _.next_auto_semantic_id;
        _.next_auto_semantic_id += 1;
        return cand;
    }

    /// Executes a complete three-stage consolidation pass:
    ///   1) Entity conflict/overwrite detection (`entity_key_hash` match + cosine < `conflict_threshold`)
    ///   2) Near-duplicate episodic clustering (`@dot >= merge_threshold`), `@fma` centroid fusion & semantic promotion
    ///   3) Ebbinghaus decay archival & hard-TTL eviction with strict `pinned` protection
    fn runConsolidationPass(*_, now_ms: u64, cfg: MemConsolPassConfig): MemConsolPassReport {
        let report = MemConsolPassReport{};
        if (!cfg.isValid()) return report;
        _.total_passes += 1;
        const dim: usize = _.dim;

        // --------------------------------------------------------------------
        // Stage 1: Entity conflict / overwrite superseding (episodic vs older episodic & semantic)
        // --------------------------------------------------------------------
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_EPISODIC) : (i += 1) {
            if (!_.episodic[i].occupied || _.episodic[i].status != MemConsolStatus.active) continue;
            const key_h: u64 = _.episodic[i].entity_key_hash;
            if (key_h == 0) continue;

            // Check against older episodic records with same entity_key_hash
            let j: usize = 0;
            while (j < MEM_CONSOL_MAX_EPISODIC) : (j += 1) {
                if (i == j || !_.episodic[j].occupied || _.episodic[j].status != MemConsolStatus.active) continue;
                if (_.episodic[j].entity_key_hash != key_h) continue;
                if (_.episodic[i].created_ms <= _.episodic[j].created_ms) continue;

                const sim: f32 = _.cosineBetween(_.episodic[i].vec, _.episodic[j].vec);
                if (sim < cfg.conflict_threshold) {
                    if (_.episodic[j].pinned) {
                        report.pinned_protected_count += 1;
                    } else {
                        _.episodic[j].status = MemConsolStatus.superseded;
                        _.episodic[j].superseded_by_id = _.episodic[i].memory_id;
                        report.conflicts_superseded += 1;
                    }
                }
            }

            // Check against older semantic records with same entity_key_hash
            let s: usize = 0;
            while (s < MEM_CONSOL_MAX_SEMANTIC) : (s += 1) {
                if (!_.semantic[s].occupied || _.semantic[s].status != MemConsolStatus.active) continue;
                if (_.semantic[s].entity_key_hash != key_h) continue;
                if (_.episodic[i].created_ms <= _.semantic[s].created_ms) continue;

                const sim_s: f32 = _.cosineBetween(_.episodic[i].vec, _.semantic[s].vec);
                if (sim_s < cfg.conflict_threshold) {
                    if (_.semantic[s].pinned) {
                        report.pinned_protected_count += 1;
                    } else {
                        _.semantic[s].status = MemConsolStatus.superseded;
                        _.semantic[s].superseded_by_id = _.episodic[i].memory_id;
                        report.conflicts_superseded += 1;
                    }
                }
            }
        }

        // --------------------------------------------------------------------
        // Stage 2: Near-duplicate clustering & @fma centroid fusion into semantic memory
        // --------------------------------------------------------------------
        let visited: [MEM_CONSOL_MAX_EPISODIC]bool = [false; MEM_CONSOL_MAX_EPISODIC];
        let seed: usize = 0;
        while (seed < MEM_CONSOL_MAX_EPISODIC) : (seed += 1) {
            if (visited[seed] || !_.episodic[seed].occupied || _.episodic[seed].status != MemConsolStatus.active) {
                continue;
            }

            let members: [MEM_CONSOL_MAX_EPISODIC]usize = [0; MEM_CONSOL_MAX_EPISODIC];
            let member_cnt: usize = 0;
            members[member_cnt] = seed;
            member_cnt += 1;
            visited[seed] = true;

            let cand: usize = seed + 1;
            while (cand < MEM_CONSOL_MAX_EPISODIC) : (cand += 1) {
                if (visited[cand] || !_.episodic[cand].occupied || _.episodic[cand].status != MemConsolStatus.active) {
                    continue;
                }
                const sim: f32 = _.cosineBetween(_.episodic[seed].vec, _.episodic[cand].vec);
                if (sim >= cfg.merge_threshold) {
                    members[member_cnt] = cand;
                    member_cnt += 1;
                    visited[cand] = true;
                }
            }

            if ((member_cnt as u32) < cfg.min_cluster_size_for_promote) continue;

            // Compute importance-weighted centroid via native @fma
            let centroid: [MEM_CONSOL_MAX_DIM]f32 = [0.0; MEM_CONSOL_MAX_DIM];
            let max_imp: f32 = 0.0;
            let sum_access: u32 = 0;
            let newest_created: u64 = 0;
            let newest_access: u64 = 0;
            let any_pinned: bool = false;
            let cluster_entity: u64 = _.episodic[seed].entity_key_hash;
            let cluster_tag: u32 = _.episodic[seed].source_tag;

            let m: usize = 0;
            while (m < member_cnt) : (m += 1) {
                const idx: usize = members[m];
                const e = _.episodic[idx];
                const w: f32 = if (e.importance > 0.05) e.importance else 0.05;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    centroid[d] = @fma(w, e.vec[d], centroid[d]);
                }
                if (e.importance > max_imp) max_imp = e.importance;
                sum_access += e.access_count;
                if (e.created_ms > newest_created) newest_created = e.created_ms;
                if (e.last_access_ms > newest_access) newest_access = e.last_access_ms;
                if (e.pinned) any_pinned = true;
            }

            const nsq: f32 = @dot(centroid[0..dim], centroid[0..dim]);
            if (!isFiniteF32(nsq) || nsq <= NORM_SQ_EPS) continue;
            const inv_n: f32 = 1.0 / @sqrt(nsq);
            let dn: usize = 0;
            while (dn < dim) : (dn += 1) {
                centroid[dn] *= inv_n;
            }

            const boosted_imp: f32 = clamp01F32(max_imp + cfg.importance_boost_on_merge);

            // Check if an existing active semantic memory matches this centroid (>= merge_threshold)
            let target_sem_slot: usize = MEM_CONSOL_MAX_SEMANTIC;
            let best_sem_sim: f32 = cfg.merge_threshold;
            let free_sem_slot: usize = MEM_CONSOL_MAX_SEMANTIC;
            let s_idx: usize = 0;
            while (s_idx < MEM_CONSOL_MAX_SEMANTIC) : (s_idx += 1) {
                if (_.semantic[s_idx].occupied) {
                    if (_.semantic[s_idx].status == MemConsolStatus.active) {
                        const s_sim: f32 = _.cosineBetween(centroid, _.semantic[s_idx].vec);
                        if (s_sim >= best_sem_sim) {
                            best_sem_sim = s_sim;
                            target_sem_slot = s_idx;
                        }
                    }
                } else if (free_sem_slot == MEM_CONSOL_MAX_SEMANTIC) {
                    free_sem_slot = s_idx;
                }
            }

            let target_sem_id: u64 = MEM_CONSOL_NO_ID;
            if (target_sem_slot < MEM_CONSOL_MAX_SEMANTIC) {
                // Fuse centroid into existing semantic memory via @fma
                const prev_cnt: f32 = _.semantic[target_sem_slot].cluster_members as f32;
                const add_cnt: f32 = member_cnt as f32;
                const total_cnt: f32 = prev_cnt + add_cnt;
                const old_w: f32 = prev_cnt / total_cnt;
                const new_w: f32 = add_cnt / total_cnt;

                let d2: usize = 0;
                while (d2 < dim) : (d2 += 1) {
                    _.semantic[target_sem_slot].vec[d2] = @fma(old_w, _.semantic[target_sem_slot].vec[d2], new_w * centroid[d2]);
                }
                const sem_v: []const f32 = _.semantic[target_sem_slot].vec[0..dim];
                const s_nsq: f32 = @dot(sem_v, sem_v);
                if (isFiniteF32(s_nsq) && s_nsq > NORM_SQ_EPS) {
                    const s_inv: f32 = 1.0 / @sqrt(s_nsq);
                    let d3: usize = 0;
                    while (d3 < dim) : (d3 += 1) {
                        _.semantic[target_sem_slot].vec[d3] *= s_inv;
                    }
                }
                _.semantic[target_sem_slot].importance = clamp01F32(
                    if (boosted_imp > _.semantic[target_sem_slot].importance) boosted_imp else _.semantic[target_sem_slot].importance
                );
                _.semantic[target_sem_slot].access_count += sum_access;
                _.semantic[target_sem_slot].cluster_members += member_cnt as u32;
                if (newest_access > _.semantic[target_sem_slot].last_access_ms) {
                    _.semantic[target_sem_slot].last_access_ms = newest_access;
                }
                if (any_pinned) _.semantic[target_sem_slot].pinned = true;
                target_sem_id = _.semantic[target_sem_slot].memory_id;
            } else if (free_sem_slot < MEM_CONSOL_MAX_SEMANTIC) {
                target_sem_id = _.allocAutoSemanticId();
                _.semantic[free_sem_slot] = MemConsolEntry{
                    memory_id: target_sem_id,
                    entity_key_hash: cluster_entity,
                    source_tag: cluster_tag,
                    importance: boosted_imp,
                    access_count: sum_access,
                    created_ms: newest_created,
                    last_access_ms: if (now_ms > newest_access) now_ms else newest_access,
                    pinned: any_pinned,
                    status: MemConsolStatus.active,
                    superseded_by_id: MEM_CONSOL_NO_ID,
                    merged_into_semantic_id: MEM_CONSOL_NO_ID,
                    cluster_members: member_cnt as u32,
                    vec: centroid,
                    occupied: true,
                };
            } else {
                // Semantic pool is full: skip promoting this cluster to avoid losing data
                continue;
            }

            report.clusters_merged += 1;
            report.semantic_promoted_count += 1;

            // Mark non-pinned episodic cluster members as merged
            let k: usize = 0;
            while (k < member_cnt) : (k += 1) {
                const idx: usize = members[k];
                _.episodic[idx].merged_into_semantic_id = target_sem_id;
                if (_.episodic[idx].pinned) {
                    report.pinned_protected_count += 1;
                } else {
                    _.episodic[idx].status = MemConsolStatus.merged;
                    report.episodic_merged_count += 1;
                }
            }
        }

        // --------------------------------------------------------------------
        // Stage 3: Ebbinghaus decay archival & hard-TTL eviction (pinned protected)
        // --------------------------------------------------------------------
        let p: usize = 0;
        while (p < MEM_CONSOL_MAX_EPISODIC) : (p += 1) {
            if (!_.episodic[p].occupied || _.episodic[p].status != MemConsolStatus.active) continue;
            if (_.episodic[p].pinned) {
                report.pinned_protected_count += 1;
                continue;
            }

            const elapsed_ms: u64 = if (now_ms > _.episodic[p].last_access_ms) (now_ms - _.episodic[p].last_access_ms) else 0;
            if (cfg.ttl_ms > 0 && elapsed_ms > cfg.ttl_ms) {
                _.episodic[p].status = MemConsolStatus.evicted;
                report.evicted_count += 1;
                continue;
            }

            const ret: f32 = computeEntryRetentionScore(_.episodic[p], now_ms, cfg.half_life_ms);
            if (ret < cfg.prune_threshold) {
                _.episodic[p].status = MemConsolStatus.archived;
                report.pruned_archived_count += 1;
            }
        }

        _.total_promoted += report.semantic_promoted_count as u64;
        _.total_superseded += report.conflicts_superseded as u64;
        _.total_pruned += (report.pruned_archived_count + report.evicted_count) as u64;
        return report;
    }

    /// Returns aggregate pool utilization and consolidation telemetry.
    fn stats(): MemConsolEngineStats {
        let act_ep: usize = 0;
        let tot_ep: usize = 0;
        let i: usize = 0;
        while (i < MEM_CONSOL_MAX_EPISODIC) : (i += 1) {
            if (_.episodic[i].occupied) {
                tot_ep += 1;
                if (_.episodic[i].status == MemConsolStatus.active) act_ep += 1;
            }
        }
        let act_sem: usize = 0;
        let tot_sem: usize = 0;
        let j: usize = 0;
        while (j < MEM_CONSOL_MAX_SEMANTIC) : (j += 1) {
            if (_.semantic[j].occupied) {
                tot_sem += 1;
                if (_.semantic[j].status == MemConsolStatus.active) act_sem += 1;
            }
        }
        return MemConsolEngineStats{
            dim: _.dim,
            active_episodic: act_ep,
            active_semantic: act_sem,
            total_episodic_slots: tot_ep,
            total_semantic_slots: tot_sem,
            total_passes: _.total_passes,
            total_promoted: _.total_promoted,
            total_superseded: _.total_superseded,
            total_pruned: _.total_pruned,
        };
    }
}

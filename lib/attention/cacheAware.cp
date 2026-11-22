// Origin: Pure-CP Stdlib (Zero-Alloc SGLang/Mooncake-Style Prefix-Cache-Aware LLM Replica Router) | Created: 2026-11-23
// Pure CP zero-heap-allocation prefix-cache-aware multi-replica LLM inference load balancer:
//   - Fixed 16-replica cluster table tracking health, active/max requests, KV utilization (0..10000 bps),
//     and an inline 64-slot approximate prompt-block-hash LRU cache per replica
//   - Joint prefix-affinity + KV-load + queue-load routing (`routePrompt`) with longest contiguous
//     prefix block matching and automatic overload diversion (`kv_util_bps >= overload_kv_bps`) to
//     prevent hot-prefix cascade snowballs
//   - Closed-loop request completion, load/health feedback & cluster telemetry snapshot

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of LLM inference replicas tracked inline (zero dynamic heap).
pub const CACHE_ROUTE_MAX_REPLICAS: usize = 16;

/// Number of recent prompt block hashes cached per replica (approximate LRU).
pub const CACHE_ROUTE_LRU_BLOCKS: usize = 64;

/// Basis-points scale for KV cache utilization (`10000` == 100.00%).
pub const CACHE_ROUTE_BPS_SCALE: u32 = 10000;

/// Outcome status of a `routePrompt` call.
pub enum CacheRouteStatus {
    routed,
    no_healthy_replica,
    all_replicas_saturated,
    invalid_input,
}

/// Tuning weights and overload circuit-breaker thresholds for `CacheAwareRouter`.
pub struct CacheRouteConfig {
    cache_affinity_weight: f32 = 0.55,
    kv_load_weight: f32 = 0.25,
    queue_load_weight: f32 = 0.20,
    overload_kv_bps: u32 = 8500,
    overload_penalty: f32 = 0.65,

    pub fn defaults(): CacheRouteConfig {
        return CacheRouteConfig{};
    }

    pub fn isValid(): bool {
        if (overload_kv_bps == 0 || overload_kv_bps > CACHE_ROUTE_BPS_SCALE) return false;
        if (cache_affinity_weight < 0.0 || kv_load_weight < 0.0 || queue_load_weight < 0.0 || overload_penalty < 0.0) {
            return false;
        }
        const sum = cache_affinity_weight + kv_load_weight + queue_load_weight;
        return !std.math.isNan(sum) && !std.math.isInf(sum) && sum > 0.0 &&
            !std.math.isNan(overload_penalty) && !std.math.isInf(overload_penalty);
    }
}

/// Single LRU prefix-block slot inside a replica's approximate prefix cache.
pub struct ReplicaBlockSlot {
    occupied: bool = false;
    block_hash: u64 = 0;
    last_touch_seq: u64 = 0;
}

/// Inline runtime state for a single LLM inference replica.
pub struct ReplicaNodeState {
    active: bool = false;
    replica_id: u32 = 0;
    healthy: bool = true;
    active_requests: u32 = 0;
    max_requests: u32 = 64;
    kv_util_bps: u32 = 0;
    lru_blocks: [CACHE_ROUTE_LRU_BLOCKS]ReplicaBlockSlot = [ReplicaBlockSlot{}; CACHE_ROUTE_LRU_BLOCKS];
    lru_count: usize = 0;
    total_routed: u64 = 0;
    total_cache_hits: u64 = 0;

    pub fn isAvailable(): bool {
        return active && healthy && active_requests < max_requests;
    }

    pub fn queueLoadRatio(): f32 {
        if (max_requests == 0) return 1.0;
        const r: f32 = (active_requests as f32) / (max_requests as f32);
        return if (r > 1.0) 1.0 else r;
    }

    pub fn kvLoadRatio(): f32 {
        const r: f32 = (kv_util_bps as f32) / (CACHE_ROUTE_BPS_SCALE as f32);
        return if (r > 1.0) 1.0 else r;
    }

    pub fn hasBlock(block_hash: u64): bool {
        let i: usize = 0;
        while (i < CACHE_ROUTE_LRU_BLOCKS) : (i += 1) {
            if (lru_blocks[i].occupied && lru_blocks[i].block_hash == block_hash) return true;
        }
        return false;
    }

    /// Computes the longest contiguous shared prefix length (in blocks) cached on this replica.
    pub fn matchedPrefixBlocks(prompt_hashes: []const u64): usize {
        let matched: usize = 0;
        while (matched < prompt_hashes.len) : (matched += 1) {
            if (!hasBlock(prompt_hashes[matched])) break;
        }
        return matched;
    }

    /// Inserts or refreshes `block_hash` in the 64-slot LRU cache, evicting the oldest entry if full.
    pub fn touchBlock(*_, block_hash: u64, touch_seq: u64) {
        let free_idx: usize = CACHE_ROUTE_LRU_BLOCKS;
        let oldest_idx: usize = 0;
        let oldest_seq: u64 = 0xFFFFFFFFFFFFFFFF;

        let i: usize = 0;
        while (i < CACHE_ROUTE_LRU_BLOCKS) : (i += 1) {
            if (lru_blocks[i].occupied) {
                if (lru_blocks[i].block_hash == block_hash) {
                    lru_blocks[i].last_touch_seq = touch_seq;
                    return;
                }
                if (lru_blocks[i].last_touch_seq < oldest_seq) {
                    oldest_seq = lru_blocks[i].last_touch_seq;
                    oldest_idx = i;
                }
            } else if (free_idx == CACHE_ROUTE_LRU_BLOCKS) {
                free_idx = i;
            }
        }

        if (free_idx < CACHE_ROUTE_LRU_BLOCKS) {
            lru_blocks[free_idx] = ReplicaBlockSlot{
                occupied: true,
                block_hash: block_hash,
                last_touch_seq: touch_seq,
            };
            lru_count += 1;
        } else {
            lru_blocks[oldest_idx] = ReplicaBlockSlot{
                occupied: true,
                block_hash: block_hash,
                last_touch_seq: touch_seq,
            };
        }
    }
}

/// Routing verdict returned by `CacheAwareRouter.routePrompt`.
pub struct CacheRouteDecision {
    status: CacheRouteStatus = CacheRouteStatus.no_healthy_replica;
    replica_id: u32 = 0;
    slot_index: usize = 0;
    matched_blocks: usize = 0;
    total_blocks: usize = 0;
    hit_ratio: f32 = 0.0;
    score: f32 = 0.0;
    diverted_by_overload: bool = false;
    hotspot_fallback: bool = false;
}

/// Cluster-wide telemetry snapshot for `CacheAwareRouter`.
pub struct CacheRouterStats {
    replica_count: usize = 0;
    healthy_replicas: usize = 0;
    capacity: usize = CACHE_ROUTE_MAX_REPLICAS;
    total_routes: u64 = 0;
    cache_hit_routes: u64 = 0;
    total_matched_blocks: u64 = 0;
    total_prompt_blocks: u64 = 0;
    overload_diversions: u64 = 0;
    hotspot_fallbacks: u64 = 0;
    rejected_routes: u64 = 0;
}

// ============================================================================
// 2. Core Engine (`CacheAwareRouter`)
// ============================================================================

/// Zero-heap SGLang/Mooncake-style prefix-cache-aware multi-replica LLM inference router.
pub struct CacheAwareRouter {
    replicas: [CACHE_ROUTE_MAX_REPLICAS]ReplicaNodeState = [ReplicaNodeState{}; CACHE_ROUTE_MAX_REPLICAS];
    cfg: CacheRouteConfig = CacheRouteConfig{},
    lru_seq: u64 = 0,
    total_routes: u64 = 0;
    cache_hit_routes: u64 = 0;
    total_matched_blocks: u64 = 0;
    total_prompt_blocks: u64 = 0;
    overload_diversions: u64 = 0;
    hotspot_fallbacks: u64 = 0;
    rejected_routes: u64 = 0;

    /// Initializes a router with default affinity/load weights and overload threshold (8500 bps).
    pub fn init(): CacheAwareRouter {
        return CacheAwareRouter{};
    }

    /// Initializes a router with custom `config`, or `null` if `config` is invalid.
    pub fn initWithConfig(config: CacheRouteConfig): CacheAwareRouter? {
        if (!config.isValid()) return null;
        return CacheAwareRouter{
            cfg: config,
        };
    }

    fn findReplicaSlot(replica_id: u32): usize? {
        if (replica_id == 0) return null;
        let i: usize = 0;
        while (i < CACHE_ROUTE_MAX_REPLICAS) : (i += 1) {
            if (replicas[i].active && replicas[i].replica_id == replica_id) return i;
        }
        return null;
    }

    /// Registers a healthy LLM replica with `replica_id > 0` and `max_requests > 0`.
    /// Returns slot index, or `null` on duplicate ID, invalid parameters, or full 16-replica table.
    pub fn registerReplica(*_, replica_id: u32, max_requests: u32): usize? {
        if (replica_id == 0 || max_requests == 0) return null;
        if (findReplicaSlot(replica_id) != null) return null;

        let i: usize = 0;
        while (i < CACHE_ROUTE_MAX_REPLICAS) : (i += 1) {
            if (!replicas[i].active) {
                replicas[i] = ReplicaNodeState{
                    active: true,
                    replica_id: replica_id,
                    healthy: true,
                    active_requests: 0,
                    max_requests: max_requests,
                    kv_util_bps: 0,
                };
                return i;
            }
        }
        return null;
    }

    /// Removes a replica from the cluster table.
    pub fn removeReplica(*_, replica_id: u32): bool {
        const slot = findReplicaSlot(replica_id) ?? return false;
        replicas[slot] = ReplicaNodeState{};
        return true;
    }

    /// Updates a replica's health status (unhealthy replicas are excluded from routing).
    pub fn setReplicaHealth(*_, replica_id: u32, healthy: bool): bool {
        const slot = findReplicaSlot(replica_id) ?? return false;
        replicas[slot].healthy = healthy;
        return true;
    }

    /// Updates a replica's in-flight request count and KV cache utilization (`0..=10000` bps).
    pub fn updateReplicaLoad(*_, replica_id: u32, active_requests: u32, kv_util_bps: u32): bool {
        if (kv_util_bps > CACHE_ROUTE_BPS_SCALE) return false;
        const slot = findReplicaSlot(replica_id) ?? return false;
        replicas[slot].active_requests = active_requests;
        replicas[slot].kv_util_bps = kv_util_bps;
        return true;
    }

    /// Marks one request completed on `replica_id` (underflow-safe) and updates `new_kv_util_bps`.
    pub fn completeRequest(*_, replica_id: u32, new_kv_util_bps: u32): bool {
        if (new_kv_util_bps > CACHE_ROUTE_BPS_SCALE) return false;
        const slot = findReplicaSlot(replica_id) ?? return false;
        if (replicas[slot].active_requests > 0) {
            replicas[slot].active_requests -= 1;
        }
        replicas[slot].kv_util_bps = new_kv_util_bps;
        return true;
    }

    /// Selects the optimal replica for `prompt_block_hashes` by combining longest contiguous
    /// prefix-block hit ratio, KV cache headroom, queue headroom, and overload diversion penalty.
    /// Automatically touches all prompt block hashes into the chosen replica's LRU cache and
    /// increments its `active_requests`.
    pub fn routePrompt(*_, prompt_block_hashes: []const u64, now_ms: u64): CacheRouteDecision {
        * = now_ms;
        let decision = CacheRouteDecision{
            total_blocks: prompt_block_hashes.len,
        };

        let healthy_seen: usize = 0;
        let avail_seen: usize = 0;

        let best_slot: usize = 0;
        let best_score: f32 = -1.0e30;
        let best_matched: usize = 0;
        let best_hit_ratio: f32 = 0.0;
        let best_overloaded: bool = false;

        // Track the replica with the highest prefix match to detect overload diversion.
        let top_cache_slot: usize = 0;
        let top_cache_matched: usize = 0;
        let top_cache_overloaded: bool = false;

        let i: usize = 0;
        while (i < CACHE_ROUTE_MAX_REPLICAS) : (i += 1) {
            if (!replicas[i].active || !replicas[i].healthy) continue;
            healthy_seen += 1;
            if (replicas[i].active_requests >= replicas[i].max_requests) continue;
            avail_seen += 1;

            const matched = replicas[i].matchedPrefixBlocks(prompt_block_hashes);
            let hit_ratio: f32 = 0.0;
            if (prompt_block_hashes.len > 0) {
                hit_ratio = (matched as f32) / (prompt_block_hashes.len as f32);
            }

            const overloaded = replicas[i].kv_util_bps >= cfg.overload_kv_bps;
            if (matched > top_cache_matched) {
                top_cache_matched = matched;
                top_cache_slot = i;
                top_cache_overloaded = overloaded;
            }

            const kv_headroom: f32 = 1.0 - replicas[i].kvLoadRatio();
            const q_headroom: f32 = 1.0 - replicas[i].queueLoadRatio();
            let score: f32 = cfg.cache_affinity_weight * hit_ratio +
                cfg.kv_load_weight * kv_headroom +
                cfg.queue_load_weight * q_headroom;
            if (overloaded) {
                score = score - cfg.overload_penalty;
            }

            const better = (avail_seen == 1) ||
                (score > best_score) ||
                (score == best_score && replicas[i].replica_id < replicas[best_slot].replica_id);
            if (better) {
                best_slot = i;
                best_score = score;
                best_matched = matched;
                best_hit_ratio = hit_ratio;
                best_overloaded = overloaded;
            }
        }

        if (healthy_seen == 0) {
            decision.status = CacheRouteStatus.no_healthy_replica;
            rejected_routes += 1;
            return decision;
        }
        if (avail_seen == 0) {
            decision.status = CacheRouteStatus.all_replicas_saturated;
            rejected_routes += 1;
            return decision;
        }

        // Admit request on best_slot and populate its 64-slot approximate prefix LRU cache.
        replicas[best_slot].active_requests += 1;
        replicas[best_slot].total_routed += 1;
        if (best_matched > 0) {
            replicas[best_slot].total_cache_hits += 1;
            cache_hit_routes += 1;
        }

        let b: usize = 0;
        while (b < prompt_block_hashes.len) : (b += 1) {
            lru_seq += 1;
            replicas[best_slot].touchBlock(prompt_block_hashes[b], lru_seq);
        }

        total_routes += 1;
        total_matched_blocks += best_matched as u64;
        total_prompt_blocks += prompt_block_hashes.len as u64;

        const diverted = top_cache_matched > 0 && top_cache_overloaded &&
            best_slot != top_cache_slot && !best_overloaded;
        if (diverted) {
            overload_diversions += 1;
        }
        if (best_overloaded) {
            hotspot_fallbacks += 1;
        }

        decision.status = CacheRouteStatus.routed;
        decision.replica_id = replicas[best_slot].replica_id;
        decision.slot_index = best_slot;
        decision.matched_blocks = best_matched;
        decision.hit_ratio = best_hit_ratio;
        decision.score = best_score;
        decision.diverted_by_overload = diverted;
        decision.hotspot_fallback = best_overloaded;
        return decision;
    }

    /// Returns a read-only copy of `replica_id`, or `null` if not registered.
    pub fn getReplica(replica_id: u32): ReplicaNodeState? {
        const slot = findReplicaSlot(replica_id) ?? return null;
        return replicas[slot];
    }

    /// Returns a point-in-time cluster telemetry snapshot.
    pub fn stats(): CacheRouterStats {
        let count: usize = 0;
        let healthy: usize = 0;
        let i: usize = 0;
        while (i < CACHE_ROUTE_MAX_REPLICAS) : (i += 1) {
            if (!replicas[i].active) continue;
            count += 1;
            if (replicas[i].healthy) healthy += 1;
        }
        return CacheRouterStats{
            replica_count: count,
            healthy_replicas: healthy,
            capacity: CACHE_ROUTE_MAX_REPLICAS,
            total_routes: total_routes,
            cache_hit_routes: cache_hit_routes,
            total_matched_blocks: total_matched_blocks,
            total_prompt_blocks: total_prompt_blocks,
            overload_diversions: overload_diversions,
            hotspot_fallbacks: hotspot_fallbacks,
            rejected_routes: rejected_routes,
        };
    }
}

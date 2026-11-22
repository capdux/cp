// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc A2A AgentCard Discovery Registry & Health-Weighted Router) | Created: 2026-11-22
// Pure CP zero-heap-allocation Agent2Agent (A2A) Mesh Service Discovery & Health-Weighted Node Router:
//   - Fixed inline 64-card registry (`A2A_REGISTRY_MAX_CARDS`) with 0 dynamic heap allocation
//   - Bitmask capability matching across 64-bit skills, 32-bit input/output modalities & protocol features
//   - Heartbeat TTL renewal (`heartbeat`) and stale-peer eviction (`prune_expired`)
//   - Health- and latency-weighted Top-K discovery (`health_score_bps / 10000.0 / (1.0 + avg_latency_ms / 200.0)`)
//     with deterministic `agent_id` ascending tie-breaking

// ============================================================================
// 1. Constants & Capability Bitmasks
// ============================================================================

/// Maximum AgentCard entries stored inline in `A2aCardRegistry` (zero dynamic heap).
pub const A2A_REGISTRY_MAX_CARDS: usize = 64;

/// Maximum byte length of an AgentCard display name.
pub const A2A_CARD_NAME_MAX_LEN: usize = 48;

/// Maximum byte length of an AgentCard endpoint URL.
pub const A2A_CARD_URL_MAX_LEN: usize = 128;

// Modality bitmask flags (u32)
pub const A2A_MOD_TEXT: u32 = 1 << 0;
pub const A2A_MOD_JSON: u32 = 1 << 1;
pub const A2A_MOD_IMAGE: u32 = 1 << 2;
pub const A2A_MOD_AUDIO: u32 = 1 << 3;
pub const A2A_MOD_FILE: u32 = 1 << 4;

// Protocol feature bitmask flags (u32)
pub const A2A_FEAT_STREAMING: u32 = 1 << 0;
pub const A2A_FEAT_PUSH_NOTIFY: u32 = 1 << 1;
pub const A2A_FEAT_STATE_HISTORY: u32 = 1 << 2;

// ============================================================================
// 2. Public Models
// ============================================================================

/// Fixed-capacity inline AgentCard entry stored in `A2aCardRegistry`.
pub struct A2aCardEntry {
    agent_id: u64 = 0;
    name_buf: [A2A_CARD_NAME_MAX_LEN]u8 = [0; A2A_CARD_NAME_MAX_LEN];
    name_len: usize = 0;
    url_buf: [A2A_CARD_URL_MAX_LEN]u8 = [0; A2A_CARD_URL_MAX_LEN];
    url_len: usize = 0;
    skill_mask: u64 = 0;
    input_modalities: u32 = 0;
    output_modalities: u32 = 0;
    feature_mask: u32 = 0;
    health_score_bps: u16 = 10000;
    avg_latency_ms: u32 = 0;
    last_heartbeat_ms: u64 = 0;
    ttl_ms: u64 = 30_000;
    active: bool = false;

    /// Constructs a validated `A2aCardEntry` from byte slices.
    /// Returns `null` if `agent_id == 0`, name/URL is empty or over-length, `health_score_bps > 10000`, or `ttl_ms == 0`.
    static fn create(
        agent_id: u64,
        name: []const u8,
        endpoint_url: []const u8,
        skill_mask: u64,
        input_modalities: u32,
        output_modalities: u32,
        feature_mask: u32,
        health_score_bps: u16,
        avg_latency_ms: u32,
        now_ms: u64,
        ttl_ms: u64,
    ): A2aCardEntry? {
        if (agent_id == 0 || health_score_bps > 10000 || ttl_ms == 0) return null;
        if (name.len == 0 || name.len > A2A_CARD_NAME_MAX_LEN) return null;
        if (endpoint_url.len == 0 || endpoint_url.len > A2A_CARD_URL_MAX_LEN) return null;

        let card = A2aCardEntry{
            agent_id: agent_id,
            name_buf: [0; A2A_CARD_NAME_MAX_LEN],
            name_len: name.len,
            url_buf: [0; A2A_CARD_URL_MAX_LEN],
            url_len: endpoint_url.len,
            skill_mask: skill_mask,
            input_modalities: input_modalities,
            output_modalities: output_modalities,
            feature_mask: feature_mask,
            health_score_bps: health_score_bps,
            avg_latency_ms: avg_latency_ms,
            last_heartbeat_ms: now_ms,
            ttl_ms: ttl_ms,
            active: true,
        };
        let i: usize = 0;
        while (i < name.len) : (i += 1) {
            card.name_buf[i] = name[i];
        }
        let j: usize = 0;
        while (j < endpoint_url.len) : (j += 1) {
            card.url_buf[j] = endpoint_url[j];
        }
        return card;
    }

    /// Returns the agent display name as a read-only byte slice.
    fn nameSlice(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    /// Returns the agent endpoint URL as a read-only byte slice.
    fn urlSlice(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    /// Validates non-zero ID, buffer lengths, health basis points (`<= 10000`), and positive TTL.
    fn isValid(): bool {
        return _.agent_id != 0 &&
            _.name_len <= A2A_CARD_NAME_MAX_LEN &&
            _.url_len <= A2A_CARD_URL_MAX_LEN &&
            _.health_score_bps <= 10000 &&
            _.ttl_ms > 0;
    }
}

/// Capability and SLA filter criteria for `A2aCardRegistry.discover_topk`.
pub struct A2aDiscoveryQuery {
    required_skills: u64 = 0;
    required_in_mods: u32 = 0;
    required_out_mods: u32 = 0;
    required_features: u32 = 0;
    min_health_bps: u16 = 0;
    max_latency_ms: u32 = 0;
}

/// Ranked candidate returned by `A2aCardRegistry.discover_topk`.
pub struct A2aDiscoveryHit {
    agent_id: u64 = 0;
    slot_idx: usize = 0;
    rank_score_f32: f32 = 0.0;
    health_score_bps: u16 = 0;
    avg_latency_ms: u32 = 0;
}

/// Telemetry snapshot returned by `A2aCardRegistry.stats()`.
pub struct A2aCardRegistryStats {
    active_cards: usize = 0;
    expired_pruned: u64 = 0;
    total_registrations: u64 = 0;
    total_queries: u64 = 0;
}

// ============================================================================
// 3. Internal Ranking Helper
// ============================================================================

/// Ordering for discovery hits: `(rank_score_f32 desc, agent_id asc)`.
fn discoveryHitBetter(a: A2aDiscoveryHit, b: A2aDiscoveryHit): bool {
    if (a.rank_score_f32 > b.rank_score_f32) return true;
    if (a.rank_score_f32 < b.rank_score_f32) return false;
    return a.agent_id < b.agent_id;
}

// ============================================================================
// 4. Zero-Alloc A2A AgentCard Discovery Registry
// ============================================================================

/// Zero-heap-allocation A2A AgentCard capability discovery registry and health-weighted router.
pub struct A2aCardRegistry {
    cards: [A2A_REGISTRY_MAX_CARDS]A2aCardEntry = [A2aCardEntry{}; A2A_REGISTRY_MAX_CARDS];
    active_cards: usize = 0;
    expired_pruned: u64 = 0;
    total_registrations: u64 = 0;
    total_queries: u64 = 0;

    /// Initializes an empty `A2aCardRegistry`.
    static fn init(): A2aCardRegistry {
        return A2aCardRegistry{
            cards: [A2aCardEntry{}; A2A_REGISTRY_MAX_CARDS],
            active_cards: 0,
            expired_pruned: 0,
            total_registrations: 0,
            total_queries: 0,
        };
    }

    /// Internal helper: returns `true` iff card `idx` has exceeded its heartbeat TTL at `now_ms`.
    fn isCardExpired(idx: usize, now_ms: u64): bool {
        if (now_ms <= _.cards[idx].last_heartbeat_ms) return false;
        return (now_ms - _.cards[idx].last_heartbeat_ms) > _.cards[idx].ttl_ms;
    }

    /// Registers a new AgentCard or updates an existing card with the same `agent_id` in place.
    /// Rejects invalid entries (`agent_id == 0`, `health_score_bps > 10000`, `ttl_ms == 0`) or full 64-slot pool.
    fn upsert_card(*_, entry: A2aCardEntry): bool {
        if (!entry.isValid()) return false;

        let free_idx: usize = A2A_REGISTRY_MAX_CARDS;
        let i: usize = 0;
        while (i < A2A_REGISTRY_MAX_CARDS) : (i += 1) {
            if (_.cards[i].active) {
                if (_.cards[i].agent_id == entry.agent_id) {
                    _.cards[i] = entry;
                    _.cards[i].active = true;
                    _.total_registrations += 1;
                    return true;
                }
            } else if (free_idx == A2A_REGISTRY_MAX_CARDS) {
                free_idx = i;
            }
        }

        if (free_idx == A2A_REGISTRY_MAX_CARDS) return false;

        _.cards[free_idx] = entry;
        _.cards[free_idx].active = true;
        _.active_cards += 1;
        _.total_registrations += 1;
        return true;
    }

    /// Refreshes heartbeat timestamp, health score, and average latency for an active `agent_id`.
    /// Returns `false` if `agent_id == 0`, `health_score_bps > 10000`, or the agent is not registered.
    fn heartbeat(
        *_,
        agent_id: u64,
        health_score_bps: u16,
        avg_latency_ms: u32,
        now_ms: u64,
    ): bool {
        if (agent_id == 0 || health_score_bps > 10000) return false;
        let i: usize = 0;
        while (i < A2A_REGISTRY_MAX_CARDS) : (i += 1) {
            if (_.cards[i].active && _.cards[i].agent_id == agent_id) {
                _.cards[i].health_score_bps = health_score_bps;
                _.cards[i].avg_latency_ms = avg_latency_ms;
                _.cards[i].last_heartbeat_ms = now_ms;
                return true;
            }
        }
        return false;
    }

    /// Deregisters an active `agent_id` and frees its slot for immediate reuse.
    fn deregister(*_, agent_id: u64): bool {
        if (agent_id == 0) return false;
        let i: usize = 0;
        while (i < A2A_REGISTRY_MAX_CARDS) : (i += 1) {
            if (_.cards[i].active && _.cards[i].agent_id == agent_id) {
                _.cards[i] = A2aCardEntry{};
                _.active_cards -= 1;
                return true;
            }
        }
        return false;
    }

    /// Evicts all active cards whose heartbeat age exceeds `ttl_ms` (`now_ms - last_heartbeat_ms > ttl_ms`).
    /// Returns the number of expired cards pruned.
    fn prune_expired(*_, now_ms: u64): usize {
        let pruned: usize = 0;
        let i: usize = 0;
        while (i < A2A_REGISTRY_MAX_CARDS) : (i += 1) {
            if (_.cards[i].active && _.isCardExpired(i, now_ms)) {
                _.cards[i] = A2aCardEntry{};
                _.active_cards -= 1;
                _.expired_pruned += 1;
                pruned += 1;
            }
        }
        return pruned;
    }

    /// Discovers up to `k` live agents matching all required skills, input/output modalities,
    /// protocol features, minimum health, and optional maximum latency (`0` = unlimited).
    /// Automatically skips expired agents, ranking matches by
    /// `health_score_bps / 10000.0 / (1.0 + avg_latency_ms / 200.0)` descending (tie-break: `agent_id` asc).
    fn discover_topk(
        *_,
        query: A2aDiscoveryQuery,
        now_ms: u64,
        k: usize,
        out_hits: *[]A2aDiscoveryHit,
    ): usize {
        _.total_queries += 1;
        if (k == 0 || out_hits.len == 0 || _.active_cards == 0) return 0;

        const eff_k: usize = if (k < out_hits.len) k else out_hits.len;
        let found: usize = 0;

        let i: usize = 0;
        while (i < A2A_REGISTRY_MAX_CARDS) : (i += 1) {
            if (!_.cards[i].active || _.isCardExpired(i, now_ms)) continue;

            if ((_.cards[i].skill_mask & query.required_skills) != query.required_skills) continue;
            if ((_.cards[i].input_modalities & query.required_in_mods) != query.required_in_mods) continue;
            if ((_.cards[i].output_modalities & query.required_out_mods) != query.required_out_mods) continue;
            if ((_.cards[i].feature_mask & query.required_features) != query.required_features) continue;
            if (_.cards[i].health_score_bps < query.min_health_bps) continue;
            if (query.max_latency_ms > 0 && _.cards[i].avg_latency_ms > query.max_latency_ms) continue;

            const health_norm: f32 = (_.cards[i].health_score_bps as f32) / 10000.0;
            const lat_factor: f32 = 1.0 + ((_.cards[i].avg_latency_ms as f32) / 200.0);
            const score: f32 = health_norm / lat_factor;

            const cand = A2aDiscoveryHit{
                agent_id: _.cards[i].agent_id,
                slot_idx: i,
                rank_score_f32: score,
                health_score_bps: _.cards[i].health_score_bps,
                avg_latency_ms: _.cards[i].avg_latency_ms,
            };

            if (found < eff_k) {
                let ins: usize = found;
                while (ins > 0 && discoveryHitBetter(cand, out_hits[ins - 1])) : (ins -= 1) {
                    out_hits[ins] = out_hits[ins - 1];
                }
                out_hits[ins] = cand;
                found += 1;
            } else if (discoveryHitBetter(cand, out_hits[eff_k - 1])) {
                let ins: usize = eff_k - 1;
                while (ins > 0 && discoveryHitBetter(cand, out_hits[ins - 1])) : (ins -= 1) {
                    out_hits[ins] = out_hits[ins - 1];
                }
                out_hits[ins] = cand;
            }
        }

        return found;
    }

    /// Returns a cumulative telemetry & active-card snapshot.
    fn stats(): A2aCardRegistryStats {
        return A2aCardRegistryStats{
            active_cards: _.active_cards,
            expired_pruned: _.expired_pruned,
            total_registrations: _.total_registrations,
            total_queries: _.total_queries,
        };
    }
}

// Origin: Pure-CP Stdlib (Zero-Alloc Multi-AZ Topology-Aware Envoy Locality Spillover Load Balancer) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-AZ topology-aware locality spillover load balancer:
//   - Fixed 64-endpoint registry partitioned across up to 8 Availability Zones (`zone_id: 0..7`),
//     tracking `endpoint_id`, `zone_id`, `weight`, `healthy`, and `active_conns`
//   - Envoy-style locality affinity & smooth panic-threshold spillover (`pickEndpoint`):
//       * Computes local AZ healthy weight ratio `local_health_bps` (`0..=10000`)
//       * When `local_health_bps >= panic_threshold_bps` (default 5000 = 50%), routes 100% locally
//       * When degraded (`< panic_threshold_bps`), retains `local_health_bps / panic_threshold_bps` locally
//         and smoothly spills the remainder across healthy remote AZs proportional to healthy weight
//       * On total global panic (zero healthy endpoints), optionally fails open across all endpoints
//   - Per-zone real-time health summary (`zoneSummary`) & zero-alloc routing telemetry snapshot (`stats`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum service endpoints registered inline (zero dynamic heap allocation).
pub const ZONE_ROUTER_MAX_ENDPOINTS: usize = 64;

/// Maximum number of Availability Zones (`zone_id` in `0..=7`).
pub const ZONE_ROUTER_MAX_ZONES: usize = 8;

/// Basis points scale (10000 = 100.00%).
pub const ZONE_ROUTER_BPS_SCALE: u32 = 10000;

/// Default Envoy-style panic threshold in basis points (5000 = 50.00%).
pub const ZONE_ROUTER_DEFAULT_PANIC_BPS: u16 = 5000;

/// Routing outcome mode returned by `pickEndpoint`.
pub enum ZonePickMode {
    local_hit,
    remote_spillover,
    panic_fallback,
    no_endpoint,
}

/// Operation status codes for endpoint mutations.
pub enum ZoneRouterOpStatus {
    ok,
    invalid_arg,
    duplicate_endpoint,
    capacity_full,
    not_found,
}

/// Configuration for `ZoneLocalityRouter`.
pub struct ZoneRouterConfig {
    /// Local AZ healthy weight ratio (`1..=10000` bps) below which traffic begins spilling to remote AZs.
    panic_threshold_bps: u16 = ZONE_ROUTER_DEFAULT_PANIC_BPS,
    /// When `true` and zero endpoints are healthy globally, fails open across all registered endpoints.
    fail_open_on_total_panic: bool = true,
}

/// Inline service endpoint record.
pub struct ZoneEndpoint {
    occupied: bool = false,
    endpoint_id: u64 = 0,
    zone_id: u8 = 0,
    weight: u32 = 1,
    healthy: bool = true,
    active_conns: u32 = 0,
}

/// Real-time per-zone health & capacity summary.
pub struct ZoneSummary {
    zone_id: u8 = 0,
    total_endpoints: usize = 0,
    healthy_endpoints: usize = 0,
    total_weight: u64 = 0,
    healthy_weight: u64 = 0,
    health_bps: u16 = 0,
    total_active_conns: u64 = 0,
}

/// Routing decision returned by `pickEndpoint`.
pub struct ZonePickDecision {
    mode: ZonePickMode = ZonePickMode.no_endpoint,
    endpoint_id: u64 = 0,
    selected_zone: u8 = 0,
    local_health_bps: u16 = 0,
    local_retain_bps: u16 = 0,
}

/// Global routing & health telemetry snapshot.
pub struct ZoneRouterStats {
    endpoint_count: usize = 0,
    capacity: usize = ZONE_ROUTER_MAX_ENDPOINTS,
    healthy_endpoints: usize = 0,
    total_picks: u64 = 0,
    local_hits: u64 = 0,
    remote_spillovers: u64 = 0,
    panic_fallbacks: u64 = 0,
    rejected_no_endpoint: u64 = 0,
}

// ============================================================================
// 2. Deterministic Hash Mixer
// ============================================================================

fn zoneSplitMix64(seed: u64): u64 {
    let z: u64 = seed +% 0x9E37_79B9_7F4A_7C15;
    z = (z ^ (z >> 30)) *% 0xBF58_476D_1CE4_E5B9;
    z = (z ^ (z >> 27)) *% 0x94D0_49BB_1331_11EB;
    return z ^ (z >> 31);
}

// ============================================================================
// 3. Core Zero-Alloc Multi-AZ Locality Spillover Router
// ============================================================================

/// Zero-heap-allocation Envoy-style multi-AZ locality spillover load balancer.
pub struct ZoneLocalityRouter {
    config: ZoneRouterConfig = ZoneRouterConfig{},
    endpoints: [ZONE_ROUTER_MAX_ENDPOINTS]ZoneEndpoint = [ZoneEndpoint{}; ZONE_ROUTER_MAX_ENDPOINTS],
    endpoint_count: usize = 0,
    total_picks: u64 = 0,
    local_hits: u64 = 0,
    remote_spillovers: u64 = 0,
    panic_fallbacks: u64 = 0,
    rejected_no_endpoint: u64 = 0,

    /// Initializes a router with `cfg`. Returns `null` if `panic_threshold_bps == 0 || > 10000`.
    pub static fn init(cfg: ZoneRouterConfig): ZoneLocalityRouter? {
        if (cfg.panic_threshold_bps == 0 || cfg.panic_threshold_bps > 10000) return null;
        return ZoneLocalityRouter{
            config: cfg,
        };
    }

    /// Initializes a router with default Envoy settings (50% panic threshold, fail-open enabled).
    pub static fn initDefault(): ZoneLocalityRouter {
        return ZoneLocalityRouter{};
    }

    // ---- Internal Helpers ----

    fn findSlot(endpoint_id: u64): usize? {
        if (endpoint_id == 0) return null;
        let i: usize = 0;
        while (i < ZONE_ROUTER_MAX_ENDPOINTS) : (i += 1) {
            if (_.endpoints[i].occupied && _.endpoints[i].endpoint_id == endpoint_id) return i;
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < ZONE_ROUTER_MAX_ENDPOINTS) : (i += 1) {
            if (!_.endpoints[i].occupied) return i;
        }
        return null;
    }

    /// Deterministic weighted pick from a filtered subset of endpoints.
    /// `filter_mode`: 0 = healthy in `target_zone`, 1 = healthy in remote zones (`!= target_zone`),
    ///                2 = any (incl. unhealthy) in `target_zone`, 3 = any globally.
    fn weightedPick(
        filter_mode: u8,
        target_zone: u8,
        total_w: u64,
        selector: u64,
    ): usize? {
        if (total_w == 0) return null;
        const ticket: u64 = selector % total_w;
        let acc: u64 = 0;
        let last_match: usize? = null;

        let i: usize = 0;
        while (i < ZONE_ROUTER_MAX_ENDPOINTS) : (i += 1) {
            const ep = _.endpoints[i];
            if (!ep.occupied) continue;

            const matches: bool = if (filter_mode == 0)
                (ep.healthy && ep.zone_id == target_zone)
            else if (filter_mode == 1)
                (ep.healthy && ep.zone_id != target_zone)
            else if (filter_mode == 2)
                (ep.zone_id == target_zone)
            else
                true;

            if (!matches) continue;
            last_match = i;
            acc += ep.weight as u64;
            if (ticket < acc) return i;
        }
        return last_match;
    }

    // ---- Endpoint Registry API ----

    /// Adds a weighted endpoint in `zone_id` (`0..=7`, `endpoint_id != 0`, `weight > 0`).
    pub fn addEndpoint(
        *_,
        endpoint_id: u64,
        zone_id: u8,
        weight: u32,
        healthy: bool,
    ): ZoneRouterOpStatus {
        if (endpoint_id == 0 || (zone_id as usize) >= ZONE_ROUTER_MAX_ZONES || weight == 0) {
            return ZoneRouterOpStatus.invalid_arg;
        }
        if (_.findSlot(endpoint_id) != null) return ZoneRouterOpStatus.duplicate_endpoint;
        const slot = _.findFreeSlot() ?? return ZoneRouterOpStatus.capacity_full;

        _.endpoints[slot] = ZoneEndpoint{
            occupied: true,
            endpoint_id: endpoint_id,
            zone_id: zone_id,
            weight: weight,
            healthy: healthy,
            active_conns: 0,
        };
        _.endpoint_count += 1;
        return ZoneRouterOpStatus.ok;
    }

    /// Removes an endpoint by `endpoint_id`.
    pub fn removeEndpoint(*_, endpoint_id: u64): ZoneRouterOpStatus {
        if (endpoint_id == 0) return ZoneRouterOpStatus.invalid_arg;
        const idx = _.findSlot(endpoint_id) ?? return ZoneRouterOpStatus.not_found;
        _.endpoints[idx] = ZoneEndpoint{};
        if (_.endpoint_count > 0) _.endpoint_count -= 1;
        return ZoneRouterOpStatus.ok;
    }

    /// Updates the health status of an endpoint.
    pub fn setHealth(*_, endpoint_id: u64, healthy: bool): ZoneRouterOpStatus {
        if (endpoint_id == 0) return ZoneRouterOpStatus.invalid_arg;
        const idx = _.findSlot(endpoint_id) ?? return ZoneRouterOpStatus.not_found;
        _.endpoints[idx].healthy = healthy;
        return ZoneRouterOpStatus.ok;
    }

    /// Updates the positive weight of an endpoint.
    pub fn setWeight(*_, endpoint_id: u64, weight: u32): ZoneRouterOpStatus {
        if (endpoint_id == 0 || weight == 0) return ZoneRouterOpStatus.invalid_arg;
        const idx = _.findSlot(endpoint_id) ?? return ZoneRouterOpStatus.not_found;
        _.endpoints[idx].weight = weight;
        return ZoneRouterOpStatus.ok;
    }

    /// Increments `active_conns` for `endpoint_id` (saturating).
    pub fn incActiveConns(*_, endpoint_id: u64): bool {
        const idx = _.findSlot(endpoint_id) ?? return false;
        _.endpoints[idx].active_conns +|= 1;
        return true;
    }

    /// Decrements `active_conns` for `endpoint_id` (saturating at 0).
    pub fn decActiveConns(*_, endpoint_id: u64): bool {
        const idx = _.findSlot(endpoint_id) ?? return false;
        _.endpoints[idx].active_conns -|= 1;
        return true;
    }

    // ---- Locality Spillover Routing Engine ----

    /// Selects an upstream endpoint for a request originating in `caller_zone` using deterministic
    /// `selector_hash`, applying Envoy-style locality affinity, smooth panic-threshold spillover,
    /// and optional fail-open total-panic fallback.
    pub fn pickEndpoint(*_, caller_zone: u8, selector_hash: u64): ZonePickDecision {
        _.total_picks += 1;
        if (_.endpoint_count == 0) {
            _.rejected_no_endpoint += 1;
            return ZonePickDecision{};
        }

        let local_total_w: u64 = 0;
        let local_healthy_w: u64 = 0;
        let remote_healthy_w: u64 = 0;
        let all_total_w: u64 = 0;

        let i: usize = 0;
        while (i < ZONE_ROUTER_MAX_ENDPOINTS) : (i += 1) {
            const ep = _.endpoints[i];
            if (!ep.occupied) continue;
            const w: u64 = ep.weight as u64;
            all_total_w += w;
            if (ep.zone_id == caller_zone) {
                local_total_w += w;
                if (ep.healthy) local_healthy_w += w;
            } else {
                if (ep.healthy) remote_healthy_w += w;
            }
        }

        const local_health_bps: u16 = if (local_total_w == 0)
            0
        else
            (((local_healthy_w * (ZONE_ROUTER_BPS_SCALE as u64)) / local_total_w) as u16);

        const h1 = zoneSplitMix64(selector_hash);
        const h2 = zoneSplitMix64(h1);

        // Case 1: Global total panic (zero healthy endpoints across all zones)
        if (local_healthy_w == 0 && remote_healthy_w == 0) {
            if (!_.config.fail_open_on_total_panic) {
                _.rejected_no_endpoint += 1;
                return ZonePickDecision{
                    mode: ZonePickMode.no_endpoint,
                    local_health_bps: 0,
                    local_retain_bps: 0,
                };
            }

            // Fail-open: prefer caller_zone endpoints if any exist, otherwise global pool
            const chosen_idx = if (local_total_w > 0)
                _.weightedPick(2, caller_zone, local_total_w, h2)
            else
                _.weightedPick(3, caller_zone, all_total_w, h2);

            if (idx := chosen_idx) {
                _.panic_fallbacks += 1;
                return ZonePickDecision{
                    mode: ZonePickMode.panic_fallback,
                    endpoint_id: _.endpoints[idx].endpoint_id,
                    selected_zone: _.endpoints[idx].zone_id,
                    local_health_bps: 0,
                    local_retain_bps: 0,
                };
            }
            _.rejected_no_endpoint += 1;
            return ZonePickDecision{};
        }

        // Case 2: Local zone has healthy endpoints at or above panic threshold -> 100% local affinity
        if (local_healthy_w > 0 && local_health_bps >= _.config.panic_threshold_bps) {
            if (idx := _.weightedPick(0, caller_zone, local_healthy_w, h2)) {
                _.local_hits += 1;
                return ZonePickDecision{
                    mode: ZonePickMode.local_hit,
                    endpoint_id: _.endpoints[idx].endpoint_id,
                    selected_zone: _.endpoints[idx].zone_id,
                    local_health_bps: local_health_bps,
                    local_retain_bps: ZONE_ROUTER_BPS_SCALE as u16,
                };
            }
        }

        // Case 3: Local zone degraded (< panic_threshold_bps) with some healthy endpoints
        if (local_healthy_w > 0) {
            if (remote_healthy_w == 0) {
                // No healthy remote zones available -> retain 100% locally
                if (idx := _.weightedPick(0, caller_zone, local_healthy_w, h2)) {
                    _.local_hits += 1;
                    return ZonePickDecision{
                        mode: ZonePickMode.local_hit,
                        endpoint_id: _.endpoints[idx].endpoint_id,
                        selected_zone: _.endpoints[idx].zone_id,
                        local_health_bps: local_health_bps,
                        local_retain_bps: ZONE_ROUTER_BPS_SCALE as u16,
                    };
                }
            }

            const retain_bps: u16 = (((local_health_bps as u64) * (ZONE_ROUTER_BPS_SCALE as u64)) /
                (_.config.panic_threshold_bps as u64)) as u16;
            const bucket: u16 = (h1 % (ZONE_ROUTER_BPS_SCALE as u64)) as u16;

            if (bucket < retain_bps) {
                if (idx := _.weightedPick(0, caller_zone, local_healthy_w, h2)) {
                    _.local_hits += 1;
                    return ZonePickDecision{
                        mode: ZonePickMode.local_hit,
                        endpoint_id: _.endpoints[idx].endpoint_id,
                        selected_zone: _.endpoints[idx].zone_id,
                        local_health_bps: local_health_bps,
                        local_retain_bps: retain_bps,
                    };
                }
            } else {
                if (idx := _.weightedPick(1, caller_zone, remote_healthy_w, h2)) {
                    _.remote_spillovers += 1;
                    return ZonePickDecision{
                        mode: ZonePickMode.remote_spillover,
                        endpoint_id: _.endpoints[idx].endpoint_id,
                        selected_zone: _.endpoints[idx].zone_id,
                        local_health_bps: local_health_bps,
                        local_retain_bps: retain_bps,
                    };
                }
            }
        }

        // Case 4: Local zone has zero healthy endpoints -> 100% spillover to healthy remote zones
        if (idx := _.weightedPick(1, caller_zone, remote_healthy_w, h2)) {
            _.remote_spillovers += 1;
            return ZonePickDecision{
                mode: ZonePickMode.remote_spillover,
                endpoint_id: _.endpoints[idx].endpoint_id,
                selected_zone: _.endpoints[idx].zone_id,
                local_health_bps: local_health_bps,
                local_retain_bps: 0,
            };
        }

        _.rejected_no_endpoint += 1;
        return ZonePickDecision{};
    }

    // ---- Queries & Telemetry ----

    /// Looks up an endpoint by `endpoint_id`.
    pub fn getEndpoint(endpoint_id: u64): ZoneEndpoint? {
        const idx = _.findSlot(endpoint_id) ?? return null;
        return _.endpoints[idx];
    }

    /// Computes a real-time health & weight summary for `zone_id`.
    pub fn zoneSummary(zone_id: u8): ZoneSummary {
        let sum = ZoneSummary{ zone_id: zone_id };
        if ((zone_id as usize) >= ZONE_ROUTER_MAX_ZONES) return sum;

        let i: usize = 0;
        while (i < ZONE_ROUTER_MAX_ENDPOINTS) : (i += 1) {
            const ep = _.endpoints[i];
            if (!ep.occupied || ep.zone_id != zone_id) continue;
            sum.total_endpoints += 1;
            sum.total_weight += ep.weight as u64;
            sum.total_active_conns += ep.active_conns as u64;
            if (ep.healthy) {
                sum.healthy_endpoints += 1;
                sum.healthy_weight += ep.weight as u64;
            }
        }

        if (sum.total_weight > 0) {
            sum.health_bps = (((sum.healthy_weight * (ZONE_ROUTER_BPS_SCALE as u64)) / sum.total_weight) as u16);
        }
        return sum;
    }

    /// Returns a global routing & health telemetry snapshot.
    pub fn stats(): ZoneRouterStats {
        let healthy_cnt: usize = 0;
        let i: usize = 0;
        while (i < ZONE_ROUTER_MAX_ENDPOINTS) : (i += 1) {
            if (_.endpoints[i].occupied && _.endpoints[i].healthy) {
                healthy_cnt += 1;
            }
        }

        return ZoneRouterStats{
            endpoint_count: _.endpoint_count,
            capacity: ZONE_ROUTER_MAX_ENDPOINTS,
            healthy_endpoints: healthy_cnt,
            total_picks: _.total_picks,
            local_hits: _.local_hits,
            remote_spillovers: _.remote_spillovers,
            panic_fallbacks: _.panic_fallbacks,
            rejected_no_endpoint: _.rejected_no_endpoint,
        };
    }
}

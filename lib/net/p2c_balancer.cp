// Origin: Pure-CP Stdlib (Zero-Alloc P2C + Peak-EWMA Adaptive Load Balancer & Outlier Ejection Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation Linkerd/Envoy-grade Power-of-Two-Choices (P2C) + Peak-EWMA
// adaptive load balancer with passive outlier ejection, half-open recovery probing,
// anti-cascade max_ejection_percent guard, and panic-mode fallback routing.

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const MAX_P2C_BACKENDS: usize = 64;
pub const MAX_BACKEND_ID_LEN: usize = 48;

const DEFAULT_INIT_EWMA_US: f32 = 1000.0;
const MIX_C1: u64 = 0xBF58476D1CE4E5B9;
const MIX_C2: u64 = 0x94D049BB133111EB;
const GOLDEN_GAMMA: u64 = 0x9E3779B97F4A7C15;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn mix64(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% MIX_C1;
    z = (z ^ (z >> 27)) *% MIX_C2;
    return z ^ (z >> 31);
}

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_u64: u64 = std.math.maxInt(u64);
    if (b > max_u64 - a) {
        return max_u64;
    }
    return a + b;
}

fn copyBytes(dst: *[]u8, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len && i < dst.len) : (i += 1) {
        dst[i] = src[i];
    }
    return i;
}

fn computeEffectiveLoad(slot: BackendSlot, inflight_penalty_us: f32): f32 {
    const base_lat: f32 = if (slot.ewma_latency_us > 1.0) slot.ewma_latency_us else 1.0;
    const w: f32 = slot.weight as f32;
    const inf: f32 = slot.inflight as f32;
    return @fma(inf, inflight_penalty_us, (base_lat * (1.0 + inf)) / w);
}

// ============================================================================
// 1. Health State, Config, Backend Slot, Decision & Stats Models
// ============================================================================

/// Passive health-check state of a backend node.
pub enum BackendHealthState {
    healthy,
    ejected,
    half_open,
}

/// Configuration for Peak-EWMA latency smoothing and passive outlier ejection.
pub struct P2cBalancerConfig {
    ewma_alpha_f32: f32 = 0.25;
    max_consecutive_errors: u32 = 3;
    ejection_cooldown_ms: u64 = 10000;
    max_ejection_percent: u8 = 50;
    inflight_penalty_us: f32 = 5000.0;

    /// Validates configuration parameters (`ewma_alpha_f32 ∈ (0, 1]`, `max_ejection_percent <= 100`, etc.).
    fn isValid(): bool {
        if (!isFiniteF32(_.ewma_alpha_f32) || _.ewma_alpha_f32 <= 0.0 || _.ewma_alpha_f32 > 1.0) {
            return false;
        }
        if (_.max_consecutive_errors == 0 || _.ejection_cooldown_ms == 0) {
            return false;
        }
        if (_.max_ejection_percent > 100) {
            return false;
        }
        if (!isFiniteF32(_.inflight_penalty_us) || _.inflight_penalty_us < 0.0) {
            return false;
        }
        return true;
    }
}

/// Fixed-size inline backend node state & Peak-EWMA telemetry slot.
pub struct BackendSlot {
    id_buf: [48]u8 = [0; 48];
    id_len: usize = 0;
    weight: u16 = 1;
    inflight: u32 = 0;
    ewma_latency_us: f32 = DEFAULT_INIT_EWMA_US;
    has_latency_sample: bool = false;
    consecutive_errors: u32 = 0;
    total_requests: u64 = 0;
    total_successes: u64 = 0;
    total_failures: u64 = 0;
    state: BackendHealthState = BackendHealthState.healthy;
    ejected_until_ms: u64 = 0;
    occupied: bool = false;

    /// Returns the backend identifier as a borrowed byte slice.
    fn idSlice(): []const u8 {
        return _.id_buf[0.._.id_len];
    }
}

/// Backend selection verdict returned by `P2cLoadBalancer.pick_backend`.
pub struct P2cPickDecision {
    found: bool = false;
    slot_idx: usize = 0;
    id_buf: [48]u8 = [0; 48];
    id_len: usize = 0;
    effective_load_score: f32 = 0.0;
    used_fallback_panic_mode: bool = false;

    /// Returns the selected backend identifier as a borrowed byte slice.
    fn idSlice(): []const u8 {
        return _.id_buf[0.._.id_len];
    }
}

/// Aggregate pool health and routing telemetry snapshot.
pub struct P2cBalancerStats {
    active_count: usize = 0;
    healthy_count: usize = 0;
    ejected_count: usize = 0;
    half_open_count: usize = 0;
    total_picks: u64 = 0;
    total_ejections: u64 = 0;
    panic_mode_picks: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc P2C + Peak-EWMA Adaptive Load Balancer
// ============================================================================

/// Zero-heap-allocation Power-of-Two-Choices + Peak-EWMA adaptive load balancer
/// backed by 64 fixed inline backend slots (`MAX_P2C_BACKENDS = 64`).
pub struct P2cLoadBalancer {
    cfg: P2cBalancerConfig = P2cBalancerConfig{};
    slots: [64]BackendSlot = [BackendSlot{}; 64];
    total_picks: u64 = 0;
    total_ejections: u64 = 0;
    panic_mode_picks: u64 = 0;

    /// Initializes a zero-alloc P2C load balancer. Returns `null` if `cfg` is invalid.
    static fn init(cfg: P2cBalancerConfig): P2cLoadBalancer? {
        if (!cfg.isValid()) {
            return null;
        }
        return P2cLoadBalancer{
            cfg: cfg,
            slots: [BackendSlot{}; 64],
            total_picks: 0,
            total_ejections: 0,
            panic_mode_picks: 0,
        };
    }

    /// Finds an occupied backend slot by `id`, or `null` if absent.
    fn find_backend(id: []const u8): usize? {
        let i: usize = 0;
        while (i < MAX_P2C_BACKENDS) : (i += 1) {
            if (_.slots[i].occupied && std.mem.eql(u8, _.slots[i].idSlice(), id)) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new backend with positive `weight`. Rejects empty/oversized IDs,
    /// duplicate IDs, zero weight, or full capacity (64 backends).
    fn register_backend(*_, id: []const u8, weight: u16): usize? {
        if (id.len == 0 || id.len > MAX_BACKEND_ID_LEN || weight == 0) {
            return null;
        }
        if (_.find_backend(id) != null) {
            return null;
        }
        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < MAX_P2C_BACKENDS) : (i += 1) {
            if (!_.slots[i].occupied) {
                free_idx = i;
                break;
            }
        }
        const idx = free_idx ?? return null;

        let slot = BackendSlot{
            weight: weight,
            inflight: 0,
            ewma_latency_us: DEFAULT_INIT_EWMA_US,
            has_latency_sample: false,
            consecutive_errors: 0,
            state: BackendHealthState.healthy,
            ejected_until_ms: 0,
            occupied: true,
        };
        let dst: []u8 = slot.id_buf[0..];
        slot.id_len = copyBytes(dst, id);
        _.slots[idx] = slot;
        return idx;
    }

    /// Removes a backend node at `slot_idx`. Returns `false` if invalid or unoccupied.
    fn remove_backend(*_, slot_idx: usize): bool {
        if (slot_idx >= MAX_P2C_BACKENDS || !_.slots[slot_idx].occupied) {
            return false;
        }
        _.slots[slot_idx] = BackendSlot{};
        return true;
    }

    /// Picks a backend via Power-of-Two-Choices + Peak-EWMA load scoring:
    /// - Automatically transitions cooldown-expired `ejected` nodes to `half_open`
    /// - Samples 2 distinct eligible nodes via deterministic SplitMix64 and picks the lower
    ///   `effective_load = ewma_latency_us * (1 + inflight) / weight + inflight * inflight_penalty_us`
    /// - Falls back to Panic Mode over all occupied backends if zero healthy/half-open nodes remain
    /// - Increments the chosen node's `inflight` counter
    fn pick_backend(*_, now_ms: u64, rng_seed: u64): P2cPickDecision {
        let eligible: [64]usize = [0; 64];
        let eligible_count: usize = 0;
        let all_active: [64]usize = [0; 64];
        let all_count: usize = 0;

        let i: usize = 0;
        while (i < MAX_P2C_BACKENDS) : (i += 1) {
            if (!_.slots[i].occupied) {
                continue;
            }
            all_active[all_count] = i;
            all_count += 1;

            if (_.slots[i].state == BackendHealthState.ejected && now_ms >= _.slots[i].ejected_until_ms) {
                _.slots[i].state = BackendHealthState.half_open;
            }
            if (_.slots[i].state != BackendHealthState.ejected) {
                eligible[eligible_count] = i;
                eligible_count += 1;
            }
        }

        if (all_count == 0) {
            return P2cPickDecision{ found: false };
        }

        let panic_mode = false;
        let pool_len: usize = eligible_count;
        if (eligible_count == 0) {
            panic_mode = true;
            pool_len = all_count;
        }

        let chosen_idx: usize = 0;
        let chosen_load: f32 = 0.0;
        const penalty: f32 = _.cfg.inflight_penalty_us;

        if (pool_len == 1) {
            chosen_idx = if (panic_mode) all_active[0] else eligible[0];
            chosen_load = computeEffectiveLoad(_.slots[chosen_idx], penalty);
        } else {
            const h1: u64 = mix64(rng_seed ^ (_.total_picks *% GOLDEN_GAMMA));
            const h2: u64 = mix64(h1 ^ GOLDEN_GAMMA);
            const pos_a: usize = (h1 % (pool_len as u64)) as usize;
            const rem_b: usize = (h2 % ((pool_len - 1) as u64)) as usize;
            const offset: usize = rem_b + 1;
            const pos_b: usize = (pos_a + offset) % pool_len;

            const idx_a: usize = if (panic_mode) all_active[pos_a] else eligible[pos_a];
            const idx_b: usize = if (panic_mode) all_active[pos_b] else eligible[pos_b];

            const load_a: f32 = computeEffectiveLoad(_.slots[idx_a], penalty);
            const load_b: f32 = computeEffectiveLoad(_.slots[idx_b], penalty);

            if (load_a < load_b || (load_a == load_b && idx_a < idx_b)) {
                chosen_idx = idx_a;
                chosen_load = load_a;
            } else {
                chosen_idx = idx_b;
                chosen_load = load_b;
            }
        }

        _.slots[chosen_idx].inflight += 1;
        _.slots[chosen_idx].total_requests = saturatingAddU64(_.slots[chosen_idx].total_requests, 1);
        _.total_picks = saturatingAddU64(_.total_picks, 1);
        if (panic_mode) {
            _.panic_mode_picks = saturatingAddU64(_.panic_mode_picks, 1);
        }

        let decision = P2cPickDecision{
            found: true,
            slot_idx: chosen_idx,
            effective_load_score: chosen_load,
            used_fallback_panic_mode: panic_mode,
        };
        let dst: []u8 = decision.id_buf[0..];
        decision.id_len = copyBytes(dst, _.slots[chosen_idx].idSlice());
        return decision;
    }

    /// Reports request completion for `slot_idx`:
    /// - Decrements `inflight` and updates Peak-EWMA latency (`@fma` decay on improvement, immediate jump on spike)
    /// - Recovers `half_open` -> `healthy` on success
    /// - Triggers passive outlier ejection on `max_consecutive_errors` (or failed `half_open` probe)
    ///   while strictly honoring `max_ejection_percent` anti-cascade ceiling
    fn report_outcome(*_, slot_idx: usize, ok: bool, latency_us: u64, now_ms: u64): bool {
        if (slot_idx >= MAX_P2C_BACKENDS || !_.slots[slot_idx].occupied) {
            return false;
        }
        if (_.slots[slot_idx].inflight > 0) {
            _.slots[slot_idx].inflight -= 1;
        }

        // Peak-EWMA latency update
        const sample_lat: f32 = latency_us as f32;
        if (!_.slots[slot_idx].has_latency_sample) {
            _.slots[slot_idx].ewma_latency_us = sample_lat;
            _.slots[slot_idx].has_latency_sample = true;
        } else if (sample_lat > _.slots[slot_idx].ewma_latency_us) {
            // Peak-EWMA reacts immediately to latency spikes
            _.slots[slot_idx].ewma_latency_us = sample_lat;
        } else {
            const alpha: f32 = _.cfg.ewma_alpha_f32;
            _.slots[slot_idx].ewma_latency_us = @fma(alpha, sample_lat, (1.0 - alpha) * _.slots[slot_idx].ewma_latency_us);
        }

        if (ok) {
            _.slots[slot_idx].total_successes = saturatingAddU64(_.slots[slot_idx].total_successes, 1);
            _.slots[slot_idx].consecutive_errors = 0;
            if (_.slots[slot_idx].state == BackendHealthState.half_open) {
                _.slots[slot_idx].state = BackendHealthState.healthy;
                _.slots[slot_idx].ejected_until_ms = 0;
            }
            return true;
        }

        // Failure path
        _.slots[slot_idx].total_failures = saturatingAddU64(_.slots[slot_idx].total_failures, 1);
        _.slots[slot_idx].consecutive_errors += 1;

        const should_eject: bool = _.slots[slot_idx].state == BackendHealthState.half_open ||
            (_.slots[slot_idx].state == BackendHealthState.healthy &&
                _.slots[slot_idx].consecutive_errors >= _.cfg.max_consecutive_errors);

        if (should_eject) {
            let active_n: usize = 0;
            let ejected_n: usize = 0;
            let k: usize = 0;
            while (k < MAX_P2C_BACKENDS) : (k += 1) {
                if (_.slots[k].occupied) {
                    active_n += 1;
                    if (_.slots[k].state == BackendHealthState.ejected) {
                        ejected_n += 1;
                    }
                }
            }
            const max_allowed: usize = (active_n * (_.cfg.max_ejection_percent as usize)) / 100;
            if (ejected_n < max_allowed) {
                _.slots[slot_idx].state = BackendHealthState.ejected;
                _.slots[slot_idx].ejected_until_ms = saturatingAddU64(now_ms, _.cfg.ejection_cooldown_ms);
                _.total_ejections = saturatingAddU64(_.total_ejections, 1);
            }
        }
        return true;
    }

    /// Returns a zero-allocation snapshot of pool health and routing statistics.
    fn stats(): P2cBalancerStats {
        let st = P2cBalancerStats{
            total_picks: _.total_picks,
            total_ejections: _.total_ejections,
            panic_mode_picks: _.panic_mode_picks,
        };
        let i: usize = 0;
        while (i < MAX_P2C_BACKENDS) : (i += 1) {
            if (!_.slots[i].occupied) {
                continue;
            }
            st.active_count += 1;
            if (_.slots[i].state == BackendHealthState.healthy) {
                st.healthy_count += 1;
            } else if (_.slots[i].state == BackendHealthState.ejected) {
                st.ejected_count += 1;
            } else {
                st.half_open_count += 1;
            }
        }
        return st;
    }
}

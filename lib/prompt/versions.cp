// Origin: Pure-CP Stdlib (Zero-Alloc AI Agent Prompt Version Registry & Canary A/B Traffic Router) | Created: 2026-11-22
// Pure CP zero-heap-allocation Prompt version lifecycle management, deterministic sticky
// basis-point canary A/B routing, shadow traffic mirroring, and automated rollout guard:
//   - Fixed-capacity inline registry (`MAX_PROMPT_VERSIONS = 16`, inline byte buffers, zero heap allocs)
//   - Deterministic FNV-1a + SplitMix64 hashing into [0, 10000) bps buckets with monotonic stickiness
//   - Atomic state transitions: `register_version`, `set_active`, `start_canary`, `promote_canary`, `rollback`
//   - Per-variant telemetry (`record_outcome`, `variant_error_rate_bps`, `variant_avg_latency_us`, `summary_stats`)
//   - Automated health guard (`should_auto_rollback` & `evaluate_and_auto_rollback`)

const std = @import("std");

// ============================================================================
// Constants & Internal Hashing / Safe Arithmetic Helpers
// ============================================================================

pub const MAX_PROMPT_VERSIONS: usize = 16;
pub const MAX_VERSION_ID_LEN: usize = 48;
pub const MAX_MODEL_HINT_LEN: usize = 48;
pub const MAX_PROMPT_SUMMARY_LEN: usize = 128;
pub const BPS_SCALE: u16 = 10000;

const FNV1A_OFFSET_64: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME_64: u64 = 0x100000001B3;
const MIX_C1: u64 = 0xBF58476D1CE4E5B9;
const MIX_C2: u64 = 0x94D049BB133111EB;

fn mix64(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% MIX_C1;
    z = (z ^ (z >> 27)) *% MIX_C2;
    return z ^ (z >> 31);
}

fn hashTenantToBucketBps(tenant_key: []const u8, seed: u64): u16 {
    let h: u64 = FNV1A_OFFSET_64 ^ mix64(seed);
    let i: usize = 0;
    while (i < tenant_key.len) : (i += 1) {
        h = (h ^ (tenant_key[i] as u64)) *% FNV1A_PRIME_64;
    }
    const mixed: u64 = mix64(h);
    return (mixed % (BPS_SCALE as u64)) as u16;
}

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_u64: u64 = std.math.maxInt(u64);
    if (b > max_u64 - a) {
        return max_u64;
    }
    return a + b;
}

fn copyFixedBytes(dst: *[]u8, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len && i < dst.len) : (i += 1) {
        dst[i] = src[i];
    }
    return i;
}

fn computeErrorRateBps(errors: u64, calls: u64): u16 {
    if (calls == 0) {
        return 0;
    }
    if (errors >= calls) {
        return BPS_SCALE;
    }
    const scale: u64 = BPS_SCALE as u64;
    // Prevent u64 overflow for very large error counts
    if (errors <= std.math.maxInt(u64) / scale) {
        return ((errors * scale) / calls) as u16;
    }
    const denom: u64 = calls / scale;
    if (denom == 0) {
        return BPS_SCALE;
    }
    const bps: u64 = errors / denom;
    return if (bps > scale) BPS_SCALE else (bps as u16);
}

// ============================================================================
// 1. Version Status, Entry, RouteDecision, Guard & Summary Models
// ============================================================================

/// Lifecycle status of a registered Prompt variant.
pub enum PromptVariantStatus {
    draft,
    canary,
    active,
    shadow,
    deprecated,
    rolled_back,
}

/// Fixed-size inline Prompt version metadata & per-variant runtime telemetry.
pub struct PromptVersionEntry {
    id_buf: [48]u8 = [0; 48];
    id_len: usize = 0;
    template_hash: u64 = 0;
    model_hint_buf: [48]u8 = [0; 48];
    model_hint_len: usize = 0;
    summary_buf: [128]u8 = [0; 128];
    summary_len: usize = 0;
    semver_major: u16 = 0;
    semver_minor: u16 = 0;
    semver_patch: u16 = 0;
    status: PromptVariantStatus = PromptVariantStatus.draft;
    weight_bps: u16 = 0;
    created_ms: i64 = 0;
    calls: u64 = 0;
    successes: u64 = 0;
    errors: u64 = 0;
    total_latency_us: u64 = 0;
    total_tokens: u64 = 0;
    occupied: bool = false;

    /// Returns the version ID as a borrowed byte slice.
    fn idSlice(): []const u8 {
        return _.id_buf[0.._.id_len];
    }

    /// Returns the model hint as a borrowed byte slice.
    fn modelHintSlice(): []const u8 {
        return _.model_hint_buf[0.._.model_hint_len];
    }

    /// Returns the system prompt summary/snippet as a borrowed byte slice.
    fn summarySlice(): []const u8 {
        return _.summary_buf[0.._.summary_len];
    }

    /// Returns error rate in basis points [0..=10000].
    fn errorRateBps(): u16 {
        return computeErrorRateBps(_.errors, _.calls);
    }

    /// Returns average latency in microseconds (0 if no calls recorded).
    fn avgLatencyUs(): u64 {
        if (_.calls == 0) {
            return 0;
        }
        return _.total_latency_us / _.calls;
    }
}

/// Deterministic routing result returned by `PromptVersionRegistry.route_request`.
pub struct RouteDecision {
    found: bool = false;
    slot_idx: usize = 0;
    id_buf: [48]u8 = [0; 48];
    id_len: usize = 0;
    status: PromptVariantStatus = PromptVariantStatus.draft;
    is_canary: bool = false;
    has_shadow: bool = false;
    shadow_slot_idx: usize = 0;
    bucket_bps: u16 = 0;
    is_fallback: bool = false;

    /// Returns the selected version ID as a borrowed byte slice.
    fn idSlice(): []const u8 {
        return _.id_buf[0.._.id_len];
    }
}

/// Automated rollout guard thresholds for canary / post-promotion health protection.
pub struct RolloutGuardConfig {
    min_samples: u64 = 20;
    max_error_bps: u16 = 500;
    max_avg_latency_us: u64 = 2000000;

    /// Constructs and validates a `RolloutGuardConfig`.
    /// Requires `min_samples > 0`, `max_error_bps <= 10000`, and `max_avg_latency_us > 0`.
    static fn init(
        min_samples: u64,
        max_error_bps: u16,
        max_avg_latency_us: u64,
    ): RolloutGuardConfig? {
        const cfg = RolloutGuardConfig{
            min_samples: min_samples,
            max_error_bps: max_error_bps,
            max_avg_latency_us: max_avg_latency_us,
        };
        if (!cfg.isValid()) {
            return null;
        }
        return cfg;
    }

    /// Validates threshold invariants.
    fn isValid(): bool {
        if (_.min_samples == 0 || _.max_error_bps > BPS_SCALE || _.max_avg_latency_us == 0) {
            return false;
        }
        return true;
    }
}

/// Zero-allocation aggregated registry telemetry snapshot.
pub struct RegistrySummaryStats {
    capacity: usize = MAX_PROMPT_VERSIONS;
    total_versions: usize = 0;
    has_active: bool = false;
    has_canary: bool = false;
    has_shadow: bool = false;
    active_slot_idx: usize = 0;
    canary_slot_idx: usize = 0;
    total_calls: u64 = 0;
    total_successes: u64 = 0;
    total_errors: u64 = 0;
    total_tokens: u64 = 0;
    overall_error_rate_bps: u16 = 0;
    overall_avg_latency_us: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc Prompt Version Registry & Canary A/B Traffic Router
// ============================================================================

/// Zero-heap-allocation Prompt version registry with deterministic sticky A/B routing,
/// shadow traffic support, atomic canary promotion/rollback, and automated health guards.
pub struct PromptVersionRegistry {
    entries: [16]PromptVersionEntry = [PromptVersionEntry{}; 16];
    count: usize = 0;
    prev_active_slot: usize? = null;

    /// Initializes an empty zero-alloc registry.
    static fn init(): PromptVersionRegistry {
        return PromptVersionRegistry{
            entries: [PromptVersionEntry{}; 16],
            count: 0,
            prev_active_slot: null,
        };
    }

    /// Finds the slot index of a registered version by `id`, or `null` if not found.
    fn find_slot(id: []const u8): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && std.mem.eql(u8, _.entries[i].idSlice(), id)) {
                return i;
            }
        }
        return null;
    }

    /// Returns the slot index of the current `active` version, if any.
    fn active_slot(): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].status == PromptVariantStatus.active) {
                return i;
            }
        }
        return null;
    }

    /// Returns the slot index of the current `canary` version, if any.
    fn canary_slot(): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].status == PromptVariantStatus.canary) {
                return i;
            }
        }
        return null;
    }

    /// Returns the slot index of the current `shadow` version, if any.
    fn shadow_slot(): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].status == PromptVariantStatus.shadow) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new Prompt version in `draft` status with duplicate-ID and capacity guards.
    /// Returns the assigned `slot_idx`, or `null` on empty/oversized fields, duplicate ID, or full registry.
    fn register_version(
        *_,
        id: []const u8,
        template_hash: u64,
        model_hint: []const u8,
        system_prompt_summary: []const u8,
        semver_major: u16,
        semver_minor: u16,
        semver_patch: u16,
        created_ms: i64,
    ): usize? {
        if (id.len == 0 || id.len > MAX_VERSION_ID_LEN) {
            return null;
        }
        if (model_hint.len > MAX_MODEL_HINT_LEN || system_prompt_summary.len > MAX_PROMPT_SUMMARY_LEN) {
            return null;
        }
        if (created_ms < 0 || _.count >= MAX_PROMPT_VERSIONS) {
            return null;
        }
        if (_.find_slot(id) != null) {
            return null;
        }

        let entry = PromptVersionEntry{
            template_hash: template_hash,
            semver_major: semver_major,
            semver_minor: semver_minor,
            semver_patch: semver_patch,
            status: PromptVariantStatus.draft,
            weight_bps: 0,
            created_ms: created_ms,
            occupied: true,
        };
        let id_dst: []u8 = entry.id_buf[0..];
        let mh_dst: []u8 = entry.model_hint_buf[0..];
        let sm_dst: []u8 = entry.summary_buf[0..];
        entry.id_len = copyFixedBytes(id_dst, id);
        entry.model_hint_len = copyFixedBytes(mh_dst, model_hint);
        entry.summary_len = copyFixedBytes(sm_dst, system_prompt_summary);

        const idx: usize = _.count;
        _.entries[idx] = entry;
        _.count += 1;
        return idx;
    }

    /// Sets `slot_idx` as the `active` production version, deprecating the previous active version
    /// and normalizing weights against any running canary.
    fn set_active(*_, slot_idx: usize): bool {
        if (slot_idx >= _.count || !_.entries[slot_idx].occupied) {
            return false;
        }
        if (old_active := _.active_slot()) {
            if (old_active != slot_idx) {
                _.entries[old_active].status = PromptVariantStatus.deprecated;
                _.entries[old_active].weight_bps = 0;
                _.prev_active_slot = old_active;
            }
        }
        let canary_w: u16 = 0;
        if (c_idx := _.canary_slot()) {
            if (c_idx != slot_idx) {
                canary_w = _.entries[c_idx].weight_bps;
            }
        }
        _.entries[slot_idx].status = PromptVariantStatus.active;
        _.entries[slot_idx].weight_bps = BPS_SCALE - canary_w;
        return true;
    }

    /// Designates `slot_idx` as the `shadow` traffic mirror (deprecating any prior shadow).
    /// Cannot shadow the currently active version.
    fn set_shadow(*_, slot_idx: usize): bool {
        if (slot_idx >= _.count || !_.entries[slot_idx].occupied) {
            return false;
        }
        if (_.entries[slot_idx].status == PromptVariantStatus.active) {
            return false;
        }
        if (old_shadow := _.shadow_slot()) {
            if (old_shadow != slot_idx) {
                _.entries[old_shadow].status = PromptVariantStatus.deprecated;
                _.entries[old_shadow].weight_bps = 0;
            }
        }
        _.entries[slot_idx].status = PromptVariantStatus.shadow;
        _.entries[slot_idx].weight_bps = 0;
        return true;
    }

    /// Starts (or updates weight of) a `canary` rollout on `slot_idx` with `canary_weight_bps ∈ [1..=10000]`.
    /// Normalizes the active version's weight to `10000 - canary_weight_bps`.
    fn start_canary(*_, slot_idx: usize, canary_weight_bps: u16): bool {
        if (slot_idx >= _.count || !_.entries[slot_idx].occupied) {
            return false;
        }
        if (canary_weight_bps == 0 || canary_weight_bps > BPS_SCALE) {
            return false;
        }
        if (_.entries[slot_idx].status == PromptVariantStatus.active) {
            return false;
        }
        if (old_canary := _.canary_slot()) {
            if (old_canary != slot_idx) {
                _.entries[old_canary].status = PromptVariantStatus.deprecated;
                _.entries[old_canary].weight_bps = 0;
            }
        }
        _.entries[slot_idx].status = PromptVariantStatus.canary;
        _.entries[slot_idx].weight_bps = canary_weight_bps;

        if (a_idx := _.active_slot()) {
            _.entries[a_idx].weight_bps = BPS_SCALE - canary_weight_bps;
        }
        return true;
    }

    /// Atomically promotes the current `canary` version to `active` (10000 bps) and marks
    /// the previous `active` version as `deprecated` (retained in `prev_active_slot` for rollback).
    fn promote_canary(*_): bool {
        const c_idx = _.canary_slot() ?? return false;
        if (old_active := _.active_slot()) {
            _.entries[old_active].status = PromptVariantStatus.deprecated;
            _.entries[old_active].weight_bps = 0;
            _.prev_active_slot = old_active;
        }
        _.entries[c_idx].status = PromptVariantStatus.active;
        _.entries[c_idx].weight_bps = BPS_SCALE;
        return true;
    }

    /// One-click atomic rollback:
    /// - If a `canary` is running, marks it `rolled_back` (0 bps) and restores `active` to 10000 bps.
    /// - Otherwise, if a post-promotion `prev_active_slot` exists, marks current `active` as
    ///   `rolled_back` (0 bps) and restores `prev_active_slot` to `active` (10000 bps).
    fn rollback(*_): bool {
        if (c_idx := _.canary_slot()) {
            _.entries[c_idx].status = PromptVariantStatus.rolled_back;
            _.entries[c_idx].weight_bps = 0;
            if (a_idx := _.active_slot()) {
                _.entries[a_idx].weight_bps = BPS_SCALE;
            }
            return true;
        }
        if (prev_idx := _.prev_active_slot) {
            if (prev_idx < _.count && _.entries[prev_idx].occupied) {
                if (cur_active := _.active_slot()) {
                    _.entries[cur_active].status = PromptVariantStatus.rolled_back;
                    _.entries[cur_active].weight_bps = 0;
                }
                _.entries[prev_idx].status = PromptVariantStatus.active;
                _.entries[prev_idx].weight_bps = BPS_SCALE;
                _.prev_active_slot = null;
                return true;
            }
        }
        return false;
    }

    /// Deterministic sticky traffic router mapping `(tenant_key, seed)` into `[0, 10000)` bps.
    /// Supports active + canary weighted split, shadow mirroring, and safe fallback if no active exists.
    fn route_request(tenant_key: []const u8, seed: u64): RouteDecision {
        const bucket: u16 = hashTenantToBucketBps(tenant_key, seed);
        const a_opt: usize? = _.active_slot();
        const c_opt: usize? = _.canary_slot();
        const s_opt: usize? = _.shadow_slot();

        let chosen_opt: usize? = null;
        let is_canary = false;
        let is_fallback = false;

        if (a_idx := a_opt) {
            if (c_idx := c_opt) {
                if (bucket < _.entries[c_idx].weight_bps) {
                    chosen_opt = c_idx;
                    is_canary = true;
                } else {
                    chosen_opt = a_idx;
                }
            } else {
                chosen_opt = a_idx;
            }
        } else if (c_idx := c_opt) {
            // Safe degradation: no active version, fall back to canary
            chosen_opt = c_idx;
            is_canary = true;
            is_fallback = true;
        } else {
            // Safe degradation: pick most recently registered non-rolled-back version
            let idx: usize = _.count;
            while (idx > 0) : (idx -= 1) {
                const i: usize = idx - 1;
                if (_.entries[i].occupied && _.entries[i].status != PromptVariantStatus.rolled_back) {
                    chosen_opt = i;
                    is_fallback = true;
                    break;
                }
            }
        }

        if (chosen := chosen_opt) {
            let decision = RouteDecision{
                found: true,
                slot_idx: chosen,
                status: _.entries[chosen].status,
                is_canary: is_canary,
                has_shadow: false,
                shadow_slot_idx: 0,
                bucket_bps: bucket,
                is_fallback: is_fallback,
            };
            let dst_id: []u8 = decision.id_buf[0..];
            decision.id_len = copyFixedBytes(dst_id, _.entries[chosen].idSlice());

            if (s_idx := s_opt) {
                if (s_idx != chosen) {
                    decision.has_shadow = true;
                    decision.shadow_slot_idx = s_idx;
                }
            }
            return decision;
        }

        return RouteDecision{
            found: false,
            bucket_bps: bucket,
        };
    }

    /// Records a single request outcome (`ok`, `latency_us`, `tokens`) for `slot_idx`.
    fn record_outcome(*_, slot_idx: usize, ok: bool, latency_us: u64, tokens: u64): bool {
        if (slot_idx >= _.count || !_.entries[slot_idx].occupied) {
            return false;
        }
        _.entries[slot_idx].calls = saturatingAddU64(_.entries[slot_idx].calls, 1);
        if (ok) {
            _.entries[slot_idx].successes = saturatingAddU64(_.entries[slot_idx].successes, 1);
        } else {
            _.entries[slot_idx].errors = saturatingAddU64(_.entries[slot_idx].errors, 1);
        }
        _.entries[slot_idx].total_latency_us = saturatingAddU64(_.entries[slot_idx].total_latency_us, latency_us);
        _.entries[slot_idx].total_tokens = saturatingAddU64(_.entries[slot_idx].total_tokens, tokens);
        return true;
    }

    /// Returns error rate in basis points [0..=10000] for `slot_idx`, or `null` if invalid.
    fn variant_error_rate_bps(slot_idx: usize): u16? {
        if (slot_idx >= _.count || !_.entries[slot_idx].occupied) {
            return null;
        }
        return _.entries[slot_idx].errorRateBps();
    }

    /// Returns average latency in microseconds for `slot_idx`, or `null` if invalid.
    fn variant_avg_latency_us(slot_idx: usize): u64? {
        if (slot_idx >= _.count || !_.entries[slot_idx].occupied) {
            return null;
        }
        return _.entries[slot_idx].avgLatencyUs();
    }

    /// Evaluates whether the currently monitored rollout (`canary`, or post-promotion `active`
    /// when a fallback `prev_active_slot` exists) breaches `guard` error-rate or latency thresholds.
    fn should_auto_rollback(guard: RolloutGuardConfig): bool {
        if (!guard.isValid()) {
            return false;
        }
        let target_opt: usize? = _.canary_slot();
        if (target_opt == null && _.prev_active_slot != null) {
            target_opt = _.active_slot();
        }
        if (idx := target_opt) {
            const e = _.entries[idx];
            if (e.calls < guard.min_samples) {
                return false;
            }
            if (e.errorRateBps() > guard.max_error_bps) {
                return true;
            }
            if (e.avgLatencyUs() > guard.max_avg_latency_us) {
                return true;
            }
        }
        return false;
    }

    /// Evaluates `guard` and immediately executes `rollback()` if a threshold is breached.
    /// Returns `true` if an automatic rollback was triggered and executed.
    fn evaluate_and_auto_rollback(*_, guard: RolloutGuardConfig): bool {
        if (_.should_auto_rollback(guard)) {
            return _.rollback();
        }
        return false;
    }

    /// Zero-allocation telemetry summary aggregated across all registered versions.
    fn summary_stats(): RegistrySummaryStats {
        let st = RegistrySummaryStats{
            capacity: MAX_PROMPT_VERSIONS,
            total_versions: _.count,
        };
        let total_lat: u64 = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const e = _.entries[i];
            if (!e.occupied) {
                continue;
            }
            if (e.status == PromptVariantStatus.active) {
                st.has_active = true;
                st.active_slot_idx = i;
            } else if (e.status == PromptVariantStatus.canary) {
                st.has_canary = true;
                st.canary_slot_idx = i;
            } else if (e.status == PromptVariantStatus.shadow) {
                st.has_shadow = true;
            }
            st.total_calls = saturatingAddU64(st.total_calls, e.calls);
            st.total_successes = saturatingAddU64(st.total_successes, e.successes);
            st.total_errors = saturatingAddU64(st.total_errors, e.errors);
            st.total_tokens = saturatingAddU64(st.total_tokens, e.total_tokens);
            total_lat = saturatingAddU64(total_lat, e.total_latency_us);
        }
        st.overall_error_rate_bps = computeErrorRateBps(st.total_errors, st.total_calls);
        st.overall_avg_latency_us = if (st.total_calls > 0) total_lat / st.total_calls else 0;
        return st;
    }
}

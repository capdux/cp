// Origin: Pure-CP Stdlib (Zero-Alloc Database Connection Pool Lifecycle, Health Probe & Backpressure State Machine) | Created: 2026-11-22
// Pure CP zero-heap-allocation transport-agnostic connection pool engine:
//   1. `DbPoolConfig` / `DbConnState` / `DbConnSlot` / `DbPoolStats`: configurable max_conns, min_idle, idle_timeout, max_lifetime & consecutive error threshold
//   2. `DbConnPool`: LIFO warm-connection reuse, automatic stale/expired eviction, backpressure `Exhausted` detection, unhealthy drop tracking & batch `evictExpired`

/// Configuration for `DbConnPool`.
pub struct DbPoolConfig {
    /// Maximum concurrent connections (active + idle). Must be > 0.
    max_conns: usize = 16;
    /// Minimum idle connections preserved during idle-timeout sweeps (`<= max_conns`).
    min_idle: usize = 0;
    /// Idle expiration threshold in milliseconds (`0` disables idle timeout).
    idle_timeout_ms: i64 = 300000;
    /// Maximum total lifetime of a connection in milliseconds (`0` disables max lifetime).
    max_lifetime_ms: i64 = 1800000;
    /// Consecutive unhealthy releases before retiring a connection (`0` = drop on first unhealthy release).
    max_consecutive_errors: u32 = 1;

    /// Constructs a validated `DbPoolConfig`. Returns `null` on invalid bounds.
    static fn init(
        max_conns: usize,
        min_idle: usize,
        idle_timeout_ms: i64,
        max_lifetime_ms: i64,
        max_consecutive_errors: u32,
    ): DbPoolConfig? {
        if (max_conns == 0 || min_idle > max_conns) return null;
        if (idle_timeout_ms < 0 || max_lifetime_ms < 0) return null;
        return DbPoolConfig{
            max_conns: max_conns,
            min_idle: min_idle,
            idle_timeout_ms: idle_timeout_ms,
            max_lifetime_ms: max_lifetime_ms,
            max_consecutive_errors: max_consecutive_errors,
        };
    }
}

/// Lifecycle state of an individual connection slot in `DbConnPool`.
pub enum DbConnState {
    Empty,
    Idle,
    Active,
}

/// Metadata for a single pooled connection slot.
pub struct DbConnSlot {
    state: DbConnState = DbConnState.Empty;
    conn_id: u64 = 0;
    handle_tag: u64 = 0;
    created_ms: i64 = 0;
    last_used_ms: i64 = 0;
    use_count: u64 = 0;
    consecutive_errors: u32 = 0;
}

/// Outcome classification for `DbConnPool.acquire`.
pub enum DbAcquireKind {
    Reused,
    Created,
    Exhausted,
}

/// Result returned by `DbConnPool.acquire`.
pub struct DbAcquireOutcome {
    kind: DbAcquireKind = DbAcquireKind.Exhausted;
    slot_index: usize = 0;
    conn_id: u64 = 0;
    handle_tag: u64 = 0;
}

/// Telemetry snapshot returned by `DbConnPool.stats`.
pub struct DbPoolStats {
    max_conns: usize = 0;
    active: usize = 0;
    idle: usize = 0;
    total_acquired: u64 = 0;
    total_created: u64 = 0;
    total_reused: u64 = 0;
    total_evicted: u64 = 0;
    total_exhausted: u64 = 0;
    total_unhealthy_drops: u64 = 0;
}

fn isLifetimeExpired(slot: DbConnSlot, max_lifetime_ms: i64, now_ms: i64): bool {
    if (max_lifetime_ms <= 0) return false;
    if (now_ms <= slot.created_ms) return false;
    return (now_ms - slot.created_ms) >= max_lifetime_ms;
}

fn isIdleExpired(slot: DbConnSlot, idle_timeout_ms: i64, now_ms: i64): bool {
    if (idle_timeout_ms <= 0) return false;
    if (now_ms <= slot.last_used_ms) return false;
    return (now_ms - slot.last_used_ms) >= idle_timeout_ms;
}

/// Zero-heap-allocation, transport-agnostic database connection pool state machine.
pub struct DbConnPool {
    slots: []DbConnSlot;
    config: DbPoolConfig;
    active_count: usize = 0;
    idle_count: usize = 0;
    next_conn_id: u64 = 1;
    total_acquired: u64 = 0;
    total_created: u64 = 0;
    total_reused: u64 = 0;
    total_evicted: u64 = 0;
    total_exhausted: u64 = 0;
    total_unhealthy_drops: u64 = 0;

    /// Initializes a connection pool over caller-provided `slots` (`slots.len >= config.max_conns`).
    /// Returns `null` if `config` is invalid or `slots` is smaller than `config.max_conns`.
    static fn init(slots: *[]DbConnSlot, config: DbPoolConfig): DbConnPool? {
        if (config.max_conns == 0 || config.min_idle > config.max_conns) return null;
        if (config.idle_timeout_ms < 0 || config.max_lifetime_ms < 0) return null;
        if (slots.len < config.max_conns) return null;

        let i: usize = 0;
        while (i < config.max_conns) : (i += 1) {
            slots[i] = DbConnSlot{};
        }

        return DbConnPool{
            slots: slots,
            config: config,
            active_count: 0,
            idle_count: 0,
            next_conn_id: 1,
            total_acquired: 0,
            total_created: 0,
            total_reused: 0,
            total_evicted: 0,
            total_exhausted: 0,
            total_unhealthy_drops: 0,
        };
    }

    /// Acquires a connection slot at timestamp `now_ms`:
    /// 1. Prefers the most-recently-used healthy, non-expired `.Idle` connection (LIFO warm reuse),
    ///    automatically evicting any expired idle connections encountered.
    /// 2. Otherwise allocates a fresh `.Empty` slot up to `max_conns` (`.Created`).
    /// 3. If all `max_conns` slots are `.Active`, increments `total_exhausted` and returns `.Exhausted`.
    fn acquire(*_, now_ms: i64): DbAcquireOutcome {
        const cap: usize = _.config.max_conns;

        // Pass 1: Evict expired idle slots and find warmest (largest last_used_ms) healthy idle slot.
        let best_idle_idx: usize = cap;
        let best_last_used: i64 = 0;
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (_.slots[i].state == DbConnState.Idle) {
                if (isLifetimeExpired(_.slots[i], _.config.max_lifetime_ms, now_ms) ||
                    isIdleExpired(_.slots[i], _.config.idle_timeout_ms, now_ms)) {
                    _.slots[i] = DbConnSlot{};
                    if (_.idle_count > 0) _.idle_count -= 1;
                    _.total_evicted += 1;
                    continue;
                }
                if (best_idle_idx == cap || _.slots[i].last_used_ms > best_last_used) {
                    best_idle_idx = i;
                    best_last_used = _.slots[i].last_used_ms;
                }
            }
        }

        if (best_idle_idx < cap) {
            _.slots[best_idle_idx].state = DbConnState.Active;
            _.slots[best_idle_idx].last_used_ms = now_ms;
            _.slots[best_idle_idx].use_count += 1;
            if (_.idle_count > 0) _.idle_count -= 1;
            _.active_count += 1;
            _.total_acquired += 1;
            _.total_reused += 1;
            return DbAcquireOutcome{
                kind: DbAcquireKind.Reused,
                slot_index: best_idle_idx,
                conn_id: _.slots[best_idle_idx].conn_id,
                handle_tag: _.slots[best_idle_idx].handle_tag,
            };
        }

        // Pass 2: Allocate first Empty slot.
        let e: usize = 0;
        while (e < cap) : (e += 1) {
            if (_.slots[e].state == DbConnState.Empty) {
                const cid: u64 = _.next_conn_id;
                _.next_conn_id += 1;
                _.slots[e] = DbConnSlot{
                    state: DbConnState.Active,
                    conn_id: cid,
                    handle_tag: 0,
                    created_ms: now_ms,
                    last_used_ms: now_ms,
                    use_count: 1,
                    consecutive_errors: 0,
                };
                _.active_count += 1;
                _.total_acquired += 1;
                _.total_created += 1;
                return DbAcquireOutcome{
                    kind: DbAcquireKind.Created,
                    slot_index: e,
                    conn_id: cid,
                    handle_tag: 0,
                };
            }
        }

        // Pass 3: Pool exhausted (backpressure).
        _.total_exhausted += 1;
        return DbAcquireOutcome{
            kind: DbAcquireKind.Exhausted,
            slot_index: 0,
            conn_id: 0,
            handle_tag: 0,
        };
    }

    /// Associates an opaque transport handle (e.g. socket fd or driver ID) with an active slot.
    fn bindHandle(*_, slot_idx: usize, handle_tag: u64): bool {
        if (slot_idx >= _.config.max_conns) return false;
        if (_.slots[slot_idx].state != DbConnState.Active) return false;
        _.slots[slot_idx].handle_tag = handle_tag;
        return true;
    }

    /// Releases an `.Active` connection slot back to the pool at `now_ms`.
    /// - If `healthy == true`: resets `consecutive_errors` and returns slot to `.Idle`
    ///   (or retires it if `max_lifetime_ms` has been reached).
    /// - If `healthy == false`: increments `consecutive_errors`; retires & drops the connection
    ///   (incrementing `total_unhealthy_drops`) once `max_consecutive_errors` or `max_lifetime_ms` is reached.
    /// Returns `false` if `slot_idx` is out of bounds or not currently `.Active`.
    fn release(*_, slot_idx: usize, healthy: bool, now_ms: i64): bool {
        if (slot_idx >= _.config.max_conns) return false;
        if (_.slots[slot_idx].state != DbConnState.Active) return false;

        if (_.active_count > 0) _.active_count -= 1;
        const life_expired: bool = isLifetimeExpired(_.slots[slot_idx], _.config.max_lifetime_ms, now_ms);

        if (!healthy) {
            _.slots[slot_idx].consecutive_errors += 1;
            const err_limit: u32 = _.config.max_consecutive_errors;
            if (err_limit == 0 || _.slots[slot_idx].consecutive_errors >= err_limit || life_expired) {
                _.slots[slot_idx] = DbConnSlot{};
                _.total_unhealthy_drops += 1;
                return true;
            }
            _.slots[slot_idx].state = DbConnState.Idle;
            _.slots[slot_idx].last_used_ms = now_ms;
            _.idle_count += 1;
            return true;
        }

        _.slots[slot_idx].consecutive_errors = 0;
        if (life_expired) {
            _.slots[slot_idx] = DbConnSlot{};
            _.total_evicted += 1;
            return true;
        }

        _.slots[slot_idx].state = DbConnState.Idle;
        _.slots[slot_idx].last_used_ms = now_ms;
        _.idle_count += 1;
        return true;
    }

    /// Batch-evicts expired `.Idle` connections at `now_ms`.
    /// - Connections exceeding `max_lifetime_ms` are always evicted.
    /// - Connections exceeding `idle_timeout_ms` are evicted while preserving at least `min_idle` idle slots.
    /// Returns the number of evicted connections.
    fn evictExpired(*_, now_ms: i64): usize {
        const cap: usize = _.config.max_conns;
        let evicted: usize = 0;

        // First evict hard max_lifetime_ms expirations unconditionally.
        let i: usize = 0;
        while (i < cap) : (i += 1) {
            if (_.slots[i].state == DbConnState.Idle &&
                isLifetimeExpired(_.slots[i], _.config.max_lifetime_ms, now_ms)) {
                _.slots[i] = DbConnSlot{};
                if (_.idle_count > 0) _.idle_count -= 1;
                _.total_evicted += 1;
                evicted += 1;
            }
        }

        // Next evict idle_timeout_ms expirations while respecting min_idle floor.
        let j: usize = 0;
        while (j < cap && _.idle_count > _.config.min_idle) : (j += 1) {
            if (_.slots[j].state == DbConnState.Idle &&
                isIdleExpired(_.slots[j], _.config.idle_timeout_ms, now_ms)) {
                _.slots[j] = DbConnSlot{};
                _.idle_count -= 1;
                _.total_evicted += 1;
                evicted += 1;
            }
        }

        return evicted;
    }

    /// Returns a copy of slot metadata at `slot_idx`, or `null` if out of bounds.
    fn slotAt(slot_idx: usize): DbConnSlot? {
        if (slot_idx >= _.config.max_conns) return null;
        return _.slots[slot_idx];
    }

    /// Returns a cumulative pool telemetry snapshot.
    fn stats(): DbPoolStats {
        return DbPoolStats{
            max_conns: _.config.max_conns,
            active: _.active_count,
            idle: _.idle_count,
            total_acquired: _.total_acquired,
            total_created: _.total_created,
            total_reused: _.total_reused,
            total_evicted: _.total_evicted,
            total_exhausted: _.total_exhausted,
            total_unhealthy_drops: _.total_unhealthy_drops,
        };
    }
}

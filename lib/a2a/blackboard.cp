// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Agent Shared Blackboard & Optimistic CAS Coordination Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-agent blackboard coordination workspace:
//   - Fixed-capacity versioned key-value slots with per-key exclusive leases and TTL expiration
//   - Optimistic concurrency control (compareAndSwap with expected_version == 0 create-if-absent,
//     returning current latest version on conflict) + automatic lease/TTL expiration sweeping
//   - Monotonic global change-log ring buffer supporting prefix-filtered incremental pollChanges
//     with ring-wrap lag detection (`lagged = true`)
//   - Complete coordination telemetry snapshot (active keys, CAS hits/conflicts, lease denials, sweeps, seq)

const std = @import("std");

// ============================================================================
// 1. Constants & Byte Helpers
// ============================================================================

/// Maximum versioned key-value slots stored inline in `SharedBlackboard`.
pub const BLACKBOARD_MAX_ENTRIES: usize = 64;

/// Maximum key byte length per blackboard slot.
pub const BLACKBOARD_MAX_KEY_LEN: usize = 48;

/// Maximum value byte length per blackboard slot.
pub const BLACKBOARD_MAX_VAL_LEN: usize = 256;

/// Capacity of the monotonic global change-log ring buffer.
pub const BLACKBOARD_EVENT_RING_CAP: usize = 128;

/// Sentinel lease owner ID meaning "no active lease".
pub const BLACKBOARD_NO_OWNER: u64 = 0;

/// Sentinel TTL value meaning "no automatic expiration".
pub const BLACKBOARD_NO_TTL: u64 = 0;

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

// ============================================================================
// 2. Public Enums & Data Models
// ============================================================================

/// Result status codes for blackboard mutations and lease operations.
pub enum BlackboardOpStatus {
    ok,
    not_found,
    version_conflict,
    lease_held_by_other,
    table_full,
    invalid_key,
    value_too_large,
    invalid_writer,
}

/// Event kinds recorded in the monotonic global change-log ring.
pub enum BlackboardEventKind {
    put,
    cas,
    delete,
    lease_expired,
    ttl_expired,
}

/// Versioned key-value slot with exclusive lease and TTL metadata.
pub struct BlackboardEntry {
    key: [48]u8 = [0; 48];
    key_len: usize = 0;
    value: [256]u8 = [0; 256];
    val_len: usize = 0;
    version: u64 = 0;
    writer_id: u64 = 0;
    lease_owner_id: u64 = BLACKBOARD_NO_OWNER;
    lease_expires_ms: u64 = 0;
    expires_ms: u64 = 0;
    updated_ms: u64 = 0;
    occupied: bool = false;

    /// Returns the valid key byte slice.
    fn keySlice(): []const u8 {
        return _.key[0.._.key_len];
    }

    /// Returns the valid value byte slice.
    fn valueSlice(): []const u8 {
        return _.value[0.._.val_len];
    }

    /// Returns `true` if this entry currently holds an unexpired exclusive lease at `now_ms`.
    fn hasActiveLease(now_ms: u64): bool {
        return _.lease_owner_id != BLACKBOARD_NO_OWNER && now_ms < _.lease_expires_ms;
    }

    /// Returns `true` if this entry's TTL has expired at `now_ms`.
    fn isExpired(now_ms: u64): bool {
        return _.expires_ms != 0 && now_ms >= _.expires_ms;
    }
}

/// Outcome returned by `put`, `compareAndSwap`, and `deleteKey`.
pub struct BlackboardWriteResult {
    status: BlackboardOpStatus = BlackboardOpStatus.ok;
    version: u64 = 0;
    global_seq: u64 = 0;
    lease_owner_id: u64 = BLACKBOARD_NO_OWNER;
}

/// Single change-log entry emitted on mutations and expiration events.
pub struct BlackboardEvent {
    seq: u64 = 0;
    kind: BlackboardEventKind = BlackboardEventKind.put;
    key: [48]u8 = [0; 48];
    key_len: usize = 0;
    version: u64 = 0;
    writer_id: u64 = 0;
    timestamp_ms: u64 = 0;

    /// Returns the affected key byte slice.
    fn keySlice(): []const u8 {
        return _.key[0.._.key_len];
    }
}

/// Summary returned by `pollChanges` including ring-wrap lag detection.
pub struct BlackboardPollResult {
    count: usize = 0;
    latest_seq: u64 = 0;
    oldest_available_seq: u64 = 0;
    lagged: bool = false;
}

/// Coordination telemetry snapshot returned by `SharedBlackboard.stats()`.
pub struct BlackboardStats {
    active_keys: usize = 0;
    capacity: usize = BLACKBOARD_MAX_ENTRIES;
    current_seq: u64 = 0;
    cas_successes: u64 = 0;
    cas_conflicts: u64 = 0;
    lease_denials: u64 = 0;
    expired_swept: u64 = 0;
    event_ring_count: usize = 0;
}

// ============================================================================
// 3. Zero-Alloc Shared Blackboard & CAS Coordination Engine
// ============================================================================

/// Zero-heap-allocation multi-agent shared blackboard with optimistic CAS,
/// exclusive edit leases, TTL expiration, and monotonic change-log polling.
pub struct SharedBlackboard {
    entries: [64]BlackboardEntry = [BlackboardEntry{}; 64],
    active_count: usize = 0,
    events: [128]BlackboardEvent = [BlackboardEvent{}; 128],
    event_head: usize = 0,
    event_count: usize = 0,
    global_seq: u64 = 0,
    cas_successes: u64 = 0,
    cas_conflicts: u64 = 0,
    lease_denials: u64 = 0,
    expired_swept: u64 = 0;

    /// Initializes an empty `SharedBlackboard`.
    static fn init(): SharedBlackboard {
        return SharedBlackboard{};
    }

    /// Clears all slots, change-log events, and counters.
    fn clear(*_) {
        let i: usize = 0;
        while (i < BLACKBOARD_MAX_ENTRIES) : (i += 1) {
            _.entries[i] = BlackboardEntry{};
        }
        _.active_count = 0;
        _.event_head = 0;
        _.event_count = 0;
        _.global_seq = 0;
        _.cas_successes = 0;
        _.cas_conflicts = 0;
        _.lease_denials = 0;
        _.expired_swept = 0;
    }

    fn emitEvent(
        *_,
        kind: BlackboardEventKind,
        key: []const u8,
        version: u64,
        writer_id: u64,
        now_ms: u64,
    ): u64 {
        _.global_seq += 1;
        let ev = BlackboardEvent{
            seq: _.global_seq,
            kind: kind,
            key_len: key.len,
            version: version,
            writer_id: writer_id,
            timestamp_ms: now_ms,
        };
        let i: usize = 0;
        while (i < key.len && i < BLACKBOARD_MAX_KEY_LEN) : (i += 1) {
            ev.key[i] = key[i];
        }
        _.events[_.event_head] = ev;
        _.event_head = (_.event_head + 1) % BLACKBOARD_EVENT_RING_CAP;
        if (_.event_count < BLACKBOARD_EVENT_RING_CAP) {
            _.event_count += 1;
        }
        return _.global_seq;
    }

    /// Reconciles lease expiration and TTL expiration on slot `idx` at `now_ms`.
    /// Returns `true` if the entry was evicted due to TTL expiration.
    fn reconcileSlot(*_, idx: usize, now_ms: u64): bool {
        if (!_.entries[idx].occupied) return false;

        // 1. Auto-release expired lease
        if (_.entries[idx].lease_owner_id != BLACKBOARD_NO_OWNER &&
            now_ms >= _.entries[idx].lease_expires_ms)
        {
            const prev_owner: u64 = _.entries[idx].lease_owner_id;
            const ver: u64 = _.entries[idx].version;
            const klen: usize = _.entries[idx].key_len;
            let kcopy: [48]u8 = _.entries[idx].key;
            _.entries[idx].lease_owner_id = BLACKBOARD_NO_OWNER;
            _.entries[idx].lease_expires_ms = 0;
            * = _.emitEvent(BlackboardEventKind.lease_expired, kcopy[0..klen], ver, prev_owner, now_ms);
        }

        // 2. Evict TTL-expired entry
        if (_.entries[idx].isExpired(now_ms)) {
            const ver: u64 = _.entries[idx].version;
            const wid: u64 = _.entries[idx].writer_id;
            const klen: usize = _.entries[idx].key_len;
            let kcopy: [48]u8 = _.entries[idx].key;
            _.entries[idx] = BlackboardEntry{};
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
            _.expired_swept += 1;
            * = _.emitEvent(BlackboardEventKind.ttl_expired, kcopy[0..klen], ver, wid, now_ms);
            return true;
        }
        return false;
    }

    fn findSlot(key: []const u8): usize? {
        let i: usize = 0;
        while (i < BLACKBOARD_MAX_ENTRIES) : (i += 1) {
            if (_.entries[i].occupied && bytesEq(_.entries[i].keySlice(), key)) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < BLACKBOARD_MAX_ENTRIES) : (i += 1) {
            if (!_.entries[i].occupied) return i;
        }
        return null;
    }

    /// Sweeps all slots at `now_ms`, automatically releasing expired leases and evicting
    /// TTL-expired entries. Returns the number of slots affected (evicted or lease released).
    fn sweepExpired(*_, now_ms: u64): usize {
        let affected: usize = 0;
        let i: usize = 0;
        while (i < BLACKBOARD_MAX_ENTRIES) : (i += 1) {
            if (!_.entries[i].occupied) continue;
            const had_exp_lease: bool = (_.entries[i].lease_owner_id != BLACKBOARD_NO_OWNER &&
                now_ms >= _.entries[i].lease_expires_ms);
            const evicted: bool = _.reconcileSlot(i, now_ms);
            if (had_exp_lease || evicted) {
                affected += 1;
            }
        }
        return affected;
    }

    /// Reads an entry by `key`, lazily reconciling lease/TTL expiration at `now_ms`.
    fn get(*_, key: []const u8, now_ms: u64): BlackboardEntry? {
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) return null;
        const idx = _.findSlot(key) orelse return null;
        if (_.reconcileSlot(idx, now_ms)) return null;
        return _.entries[idx];
    }

    /// Pure read-only lookup filtering out TTL-expired entries without mutating state.
    fn peek(key: []const u8, now_ms: u64): BlackboardEntry? {
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) return null;
        const idx = _.findSlot(key) orelse return null;
        if (_.entries[idx].isExpired(now_ms)) return null;
        return _.entries[idx];
    }

    fn writeValueIntoSlot(
        *_,
        idx: usize,
        key: []const u8,
        value: []const u8,
        writer_id: u64,
        ttl_ms: u64,
        now_ms: u64,
        is_new: bool,
    ): u64 {
        if (is_new) {
            let e = BlackboardEntry{};
            let ki: usize = 0;
            while (ki < key.len) : (ki += 1) {
                e.key[ki] = key[ki];
            }
            e.key_len = key.len;
            e.version = 1;
            e.occupied = true;
            _.entries[idx] = e;
            _.active_count += 1;
        } else {
            _.entries[idx].version += 1;
        }

        let vi: usize = 0;
        while (vi < value.len) : (vi += 1) {
            _.entries[idx].value[vi] = value[vi];
        }
        _.entries[idx].val_len = value.len;
        _.entries[idx].writer_id = writer_id;
        _.entries[idx].updated_ms = now_ms;
        _.entries[idx].expires_ms = if (ttl_ms > 0) (now_ms + ttl_ms) else 0;
        return _.entries[idx].version;
    }

    /// Unconditional versioned write (`put`). Respects active exclusive leases held by other agents.
    fn put(
        *_,
        key: []const u8,
        value: []const u8,
        writer_id: u64,
        ttl_ms: u64,
        now_ms: u64,
    ): BlackboardWriteResult {
        if (writer_id == 0) return BlackboardWriteResult{ status: BlackboardOpStatus.invalid_writer };
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) {
            return BlackboardWriteResult{ status: BlackboardOpStatus.invalid_key };
        }
        if (value.len > BLACKBOARD_MAX_VAL_LEN) {
            return BlackboardWriteResult{ status: BlackboardOpStatus.value_too_large };
        }

        if (idx := _.findSlot(key)) {
            if (!_.reconcileSlot(idx, now_ms)) {
                if (_.entries[idx].hasActiveLease(now_ms) && _.entries[idx].lease_owner_id != writer_id) {
                    _.lease_denials += 1;
                    return BlackboardWriteResult{
                        status: BlackboardOpStatus.lease_held_by_other,
                        version: _.entries[idx].version,
                        global_seq: _.global_seq,
                        lease_owner_id: _.entries[idx].lease_owner_id,
                    };
                }
                const new_ver: u64 = _.writeValueIntoSlot(idx, key, value, writer_id, ttl_ms, now_ms, false);
                const seq: u64 = _.emitEvent(BlackboardEventKind.put, key, new_ver, writer_id, now_ms);
                return BlackboardWriteResult{
                    status: BlackboardOpStatus.ok,
                    version: new_ver,
                    global_seq: seq,
                    lease_owner_id: _.entries[idx].lease_owner_id,
                };
            }
        }

        // Insert into free slot (sweep expired slots first if table is full).
        let free_opt = _.findFreeSlot();
        if (free_opt == null) {
            * = _.sweepExpired(now_ms);
            free_opt = _.findFreeSlot();
        }
        const free_idx = free_opt orelse {
            return BlackboardWriteResult{ status: BlackboardOpStatus.table_full };
        };

        const new_ver: u64 = _.writeValueIntoSlot(free_idx, key, value, writer_id, ttl_ms, now_ms, true);
        const seq: u64 = _.emitEvent(BlackboardEventKind.put, key, new_ver, writer_id, now_ms);
        return BlackboardWriteResult{
            status: BlackboardOpStatus.ok,
            version: new_ver,
            global_seq: seq,
            lease_owner_id: BLACKBOARD_NO_OWNER,
        };
    }

    /// Optimistic concurrency control Compare-And-Swap:
    /// - `expected_version == 0`: succeeds only if key is absent (create-if-absent).
    /// - `expected_version > 0`: succeeds only if key exists with `entry.version == expected_version`.
    /// On conflict, returns `version_conflict` with `version` set to the current latest version.
    fn compareAndSwap(
        *_,
        key: []const u8,
        expected_version: u64,
        new_val: []const u8,
        writer_id: u64,
        ttl_ms: u64,
        now_ms: u64,
    ): BlackboardWriteResult {
        if (writer_id == 0) return BlackboardWriteResult{ status: BlackboardOpStatus.invalid_writer };
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) {
            return BlackboardWriteResult{ status: BlackboardOpStatus.invalid_key };
        }
        if (new_val.len > BLACKBOARD_MAX_VAL_LEN) {
            return BlackboardWriteResult{ status: BlackboardOpStatus.value_too_large };
        }

        if (idx := _.findSlot(key)) {
            if (!_.reconcileSlot(idx, now_ms)) {
                if (_.entries[idx].hasActiveLease(now_ms) && _.entries[idx].lease_owner_id != writer_id) {
                    _.lease_denials += 1;
                    return BlackboardWriteResult{
                        status: BlackboardOpStatus.lease_held_by_other,
                        version: _.entries[idx].version,
                        global_seq: _.global_seq,
                        lease_owner_id: _.entries[idx].lease_owner_id,
                    };
                }
                if (_.entries[idx].version != expected_version) {
                    _.cas_conflicts += 1;
                    return BlackboardWriteResult{
                        status: BlackboardOpStatus.version_conflict,
                        version: _.entries[idx].version,
                        global_seq: _.global_seq,
                        lease_owner_id: _.entries[idx].lease_owner_id,
                    };
                }
                const new_ver: u64 = _.writeValueIntoSlot(idx, key, new_val, writer_id, ttl_ms, now_ms, false);
                _.cas_successes += 1;
                const seq: u64 = _.emitEvent(BlackboardEventKind.cas, key, new_ver, writer_id, now_ms);
                return BlackboardWriteResult{
                    status: BlackboardOpStatus.ok,
                    version: new_ver,
                    global_seq: seq,
                    lease_owner_id: _.entries[idx].lease_owner_id,
                };
            }
        }

        // Key does not exist (or was just TTL-evicted)
        if (expected_version != 0) {
            _.cas_conflicts += 1;
            return BlackboardWriteResult{
                status: BlackboardOpStatus.version_conflict,
                version: 0,
                global_seq: _.global_seq,
            };
        }

        let free_opt = _.findFreeSlot();
        if (free_opt == null) {
            * = _.sweepExpired(now_ms);
            free_opt = _.findFreeSlot();
        }
        const free_idx = free_opt orelse {
            return BlackboardWriteResult{ status: BlackboardOpStatus.table_full };
        };

        const new_ver: u64 = _.writeValueIntoSlot(free_idx, key, new_val, writer_id, ttl_ms, now_ms, true);
        _.cas_successes += 1;
        const seq: u64 = _.emitEvent(BlackboardEventKind.cas, key, new_ver, writer_id, now_ms);
        return BlackboardWriteResult{
            status: BlackboardOpStatus.ok,
            version: new_ver,
            global_seq: seq,
        };
    }

    /// Acquires (or renews) an exclusive edit lease on `key` for `owner_id` lasting `lease_ttl_ms`.
    fn acquireLease(
        *_,
        key: []const u8,
        owner_id: u64,
        lease_ttl_ms: u64,
        now_ms: u64,
    ): BlackboardOpStatus {
        if (owner_id == 0 || lease_ttl_ms == 0) return BlackboardOpStatus.invalid_writer;
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) return BlackboardOpStatus.invalid_key;

        const idx = _.findSlot(key) orelse return BlackboardOpStatus.not_found;
        if (_.reconcileSlot(idx, now_ms)) return BlackboardOpStatus.not_found;

        if (_.entries[idx].hasActiveLease(now_ms) && _.entries[idx].lease_owner_id != owner_id) {
            _.lease_denials += 1;
            return BlackboardOpStatus.lease_held_by_other;
        }
        _.entries[idx].lease_owner_id = owner_id;
        _.entries[idx].lease_expires_ms = now_ms + lease_ttl_ms;
        return BlackboardOpStatus.ok;
    }

    /// Releases an exclusive edit lease held by `owner_id`.
    fn releaseLease(
        *_,
        key: []const u8,
        owner_id: u64,
        now_ms: u64,
    ): BlackboardOpStatus {
        if (owner_id == 0) return BlackboardOpStatus.invalid_writer;
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) return BlackboardOpStatus.invalid_key;

        const idx = _.findSlot(key) orelse return BlackboardOpStatus.not_found;
        if (_.reconcileSlot(idx, now_ms)) return BlackboardOpStatus.not_found;

        if (_.entries[idx].lease_owner_id == BLACKBOARD_NO_OWNER) {
            return BlackboardOpStatus.ok;
        }
        if (_.entries[idx].lease_owner_id != owner_id) {
            _.lease_denials += 1;
            return BlackboardOpStatus.lease_held_by_other;
        }
        _.entries[idx].lease_owner_id = BLACKBOARD_NO_OWNER;
        _.entries[idx].lease_expires_ms = 0;
        return BlackboardOpStatus.ok;
    }

    /// Deletes a key and reclaims its slot. Respects active exclusive leases held by other agents.
    fn deleteKey(
        *_,
        key: []const u8,
        writer_id: u64,
        now_ms: u64,
    ): BlackboardWriteResult {
        if (writer_id == 0) return BlackboardWriteResult{ status: BlackboardOpStatus.invalid_writer };
        if (key.len == 0 || key.len > BLACKBOARD_MAX_KEY_LEN) {
            return BlackboardWriteResult{ status: BlackboardOpStatus.invalid_key };
        }
        const idx = _.findSlot(key) orelse {
            return BlackboardWriteResult{ status: BlackboardOpStatus.not_found };
        };
        if (_.reconcileSlot(idx, now_ms)) {
            return BlackboardWriteResult{ status: BlackboardOpStatus.not_found };
        }
        if (_.entries[idx].hasActiveLease(now_ms) && _.entries[idx].lease_owner_id != writer_id) {
            _.lease_denials += 1;
            return BlackboardWriteResult{
                status: BlackboardOpStatus.lease_held_by_other,
                version: _.entries[idx].version,
                global_seq: _.global_seq,
                lease_owner_id: _.entries[idx].lease_owner_id,
            };
        }

        const old_ver: u64 = _.entries[idx].version;
        _.entries[idx] = BlackboardEntry{};
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
        const seq: u64 = _.emitEvent(BlackboardEventKind.delete, key, old_ver, writer_id, now_ms);
        return BlackboardWriteResult{
            status: BlackboardOpStatus.ok,
            version: old_ver,
            global_seq: seq,
        };
    }

    /// Polls incremental change-log events matching `prefix` with `event.seq > since_seq`.
    /// Sets `lagged = true` if the ring buffer has wrapped past `since_seq + 1`.
    fn pollChanges(
        prefix: []const u8,
        since_seq: u64,
        out_events: *[]BlackboardEvent,
    ): BlackboardPollResult {
        if (_.event_count == 0) {
            return BlackboardPollResult{
                count: 0,
                latest_seq: _.global_seq,
                oldest_available_seq: 0,
                lagged: false,
            };
        }

        const start_idx: usize = if (_.event_count < BLACKBOARD_EVENT_RING_CAP) 0 else _.event_head;
        const oldest_seq: u64 = _.events[start_idx].seq;
        const lagged: bool = (oldest_seq > 1 && since_seq < oldest_seq - 1);

        let written: usize = 0;
        let i: usize = 0;
        while (i < _.event_count && written < out_events.len) : (i += 1) {
            const ev = _.events[(start_idx + i) % BLACKBOARD_EVENT_RING_CAP];
            if (ev.seq <= since_seq) continue;
            if (prefix.len > 0 && !startsWith(ev.keySlice(), prefix)) continue;
            out_events[written] = ev;
            written += 1;
        }

        return BlackboardPollResult{
            count: written,
            latest_seq: _.global_seq,
            oldest_available_seq: oldest_seq,
            lagged: lagged,
        };
    }

    /// Snapshots all live (unexpired at `now_ms`) entries matching `prefix` into `out_entries`.
    fn listKeysByPrefix(
        prefix: []const u8,
        now_ms: u64,
        out_entries: *[]BlackboardEntry,
    ): usize {
        if (out_entries.len == 0) return 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < BLACKBOARD_MAX_ENTRIES && written < out_entries.len) : (i += 1) {
            const e = _.entries[i];
            if (!e.occupied || e.isExpired(now_ms)) continue;
            if (prefix.len > 0 && !startsWith(e.keySlice(), prefix)) continue;
            out_entries[written] = e;
            written += 1;
        }
        return written;
    }

    /// Returns a diagnostic coordination statistics snapshot.
    fn stats(): BlackboardStats {
        return BlackboardStats{
            active_keys: _.active_count,
            capacity: BLACKBOARD_MAX_ENTRIES,
            current_seq: _.global_seq,
            cas_successes: _.cas_successes,
            cas_conflicts: _.cas_conflicts,
            lease_denials: _.lease_denials,
            expired_swept: _.expired_swept,
            event_ring_count: _.event_count,
        };
    }
}

// Origin: Pure-CP Stdlib (Zero-Alloc Observed-Remove Set CRDT & LWW-Register64) | Created: 2026-11-23
// Pure CP zero-heap-allocation State-Based CRDT engine complementing `lib/ds/crdtPn.cp`:
//   - Tombstone-free Add-Wins Observed-Remove Set (`OrSetCrdt`) backed by a compact
//     16-replica causal version vector (`OrSetCausalContext`) and 64-slot dot table
//   - Strictly satisfies join-semilattice laws:
//       * Commutativity:  merge(A, B) == merge(B, A)
//       * Associativity:  merge(merge(A, B), C) == merge(A, merge(B, C))
//       * Idempotency:    merge(A, A) == A
//   - Add-Wins concurrency semantics: concurrent `add(x)` and `remove(x)` across replicas
//     always preserve the unobserved `add(x)` dot while dropping only causally observed dots
//   - Deterministic Last-Write-Wins 64-bit register (`LwwRegister64`) with `(timestamp_ms, replica_id)`
//     total-order tie-breaking and idempotent semilattice `merge`

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of distinct replicas tracked in `OrSetCausalContext`.
pub const ORSET_MAX_REPLICAS: usize = 16;

/// Maximum number of active `(element, dot)` pairs stored inline in `OrSetCrdt`.
pub const ORSET_MAX_ENTRIES: usize = 64;

/// Maximum UTF-8 byte length of an element key stored inline in `OrSetEntry`.
pub const ORSET_KEY_MAX_LEN: usize = 48;

/// Globally unique causal dot `(replica_id, seq)` identifying a single `add` event.
pub struct OrSetDot {
    replica_id: u16 = 0;
    seq: u64 = 0;

    /// Return true if two causal dots represent the exact same event.
    fn equals(other: OrSetDot): bool {
        return replica_id == other.replica_id && seq == other.seq;
    }
}

/// Operation status returned by `OrSetCrdt` mutating methods.
pub enum OrSetOpStatus {
    ok,
    capacity_full,
    key_too_long,
    unknown_replica,
    not_found,
}

/// Single active `(key, dot)` entry stored inline inside `OrSetCrdt`.
pub struct OrSetEntry {
    active: bool = false;
    key_buf: [ORSET_KEY_MAX_LEN]u8 = [0; ORSET_KEY_MAX_LEN];
    key_len: u16 = 0;
    dot: OrSetDot = OrSetDot{};

    /// Return the element key slice.
    fn key(): []const u8 {
        return key_buf[0..@as(usize, key_len)];
    }
}

/// Zero-heap telemetry snapshot for `OrSetCrdt`.
pub struct OrSetStats {
    active_dots: usize = 0;
    distinct_elements: usize = 0;
    replicas_tracked: usize = 0;
    merge_count: u64 = 0;
}

// ============================================================================
// 2. Pure Helper Functions
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// 3. OrSetCausalContext — Compact 16-Replica Version Vector
// ============================================================================

/// Compact fixed-size causal context tracking the highest observed sequence number
/// for each replica `0 .. ORSET_MAX_REPLICAS - 1`.
pub struct OrSetCausalContext {
    clocks: [ORSET_MAX_REPLICAS]u64 = [0; ORSET_MAX_REPLICAS];

    /// Initialize an all-zero causal context.
    static fn init(): OrSetCausalContext {
        return OrSetCausalContext{};
    }

    /// Allocate the next monotonic causal dot for `replica_id`.
    /// Returns `null` if `replica_id >= ORSET_MAX_REPLICAS`.
    fn nextDot(*_, replica_id: u16): OrSetDot? {
        const idx: usize = @as(usize, replica_id);
        if (idx >= ORSET_MAX_REPLICAS) return null;
        clocks[idx] += 1;
        return OrSetDot{
            replica_id: replica_id,
            seq: clocks[idx],
        };
    }

    /// Return true if this causal context has observed `dot` (`1 <= dot.seq <= clocks[dot.replica_id]`).
    fn hasSeen(dot: OrSetDot): bool {
        const idx: usize = @as(usize, dot.replica_id);
        if (idx >= ORSET_MAX_REPLICAS || dot.seq == 0) return false;
        return dot.seq <= clocks[idx];
    }

    /// Join another causal context via element-wise maximum (join-semilattice LUB).
    fn merge(*_, other: OrSetCausalContext) {
        let i: usize = 0;
        while (i < ORSET_MAX_REPLICAS) : (i += 1) {
            if (other.clocks[i] > clocks[i]) {
                clocks[i] = other.clocks[i];
            }
        }
    }

    /// Return true if every replica clock in this context is `>=` the corresponding clock in `other`.
    fn dominates(other: OrSetCausalContext): bool {
        let i: usize = 0;
        while (i < ORSET_MAX_REPLICAS) : (i += 1) {
            if (clocks[i] < other.clocks[i]) return false;
        }
        return true;
    }

    /// Return true if two causal contexts have identical version vectors.
    fn equals(other: OrSetCausalContext): bool {
        let i: usize = 0;
        while (i < ORSET_MAX_REPLICAS) : (i += 1) {
            if (clocks[i] != other.clocks[i]) return false;
        }
        return true;
    }

    /// Count how many replicas have a non-zero clock entry in this context.
    fn trackedReplicas(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < ORSET_MAX_REPLICAS) : (i += 1) {
            if (clocks[i] > 0) count += 1;
        }
        return count;
    }
}

// ============================================================================
// 4. OrSetCrdt — Tombstone-Free Add-Wins Observed-Remove Set
// ============================================================================

/// Zero-heap-allocation Add-Wins Observed-Remove Set (AWORSet) CRDT.
pub struct OrSetCrdt {
    replica_id: u16 = 0;
    context: OrSetCausalContext = OrSetCausalContext{},
    entries: [ORSET_MAX_ENTRIES]OrSetEntry = [OrSetEntry{}; ORSET_MAX_ENTRIES];
    entry_count: usize = 0;
    merge_count: u64 = 0;

    /// Initialize an empty `OrSetCrdt` owned by `replica_id` (`0 .. ORSET_MAX_REPLICAS - 1`).
    static fn init(replica_id: u16): OrSetCrdt {
        let s = OrSetCrdt{};
        s.replica_id = replica_id;
        return s;
    }

    /// Add `elem` to the set by generating a fresh causal dot for this replica.
    /// Existing dots for `elem` are preserved so concurrent partial removes follow strict OR-Set semantics.
    /// Returns `.unknown_replica`, `.key_too_long`, or `.capacity_full` with zero side effects on error.
    fn add(*_, elem: []const u8): OrSetOpStatus {
        if (@as(usize, replica_id) >= ORSET_MAX_REPLICAS) {
            return OrSetOpStatus.unknown_replica;
        }
        if (elem.len > ORSET_KEY_MAX_LEN) {
            return OrSetOpStatus.key_too_long;
        }
        if (entry_count >= ORSET_MAX_ENTRIES) {
            return OrSetOpStatus.capacity_full;
        }

        const dot = context.nextDot(replica_id) ?? return OrSetOpStatus.unknown_replica;

        let e = OrSetEntry{};
        e.active = true;
        let i: usize = 0;
        while (i < elem.len) : (i += 1) {
            e.key_buf[i] = elem[i];
        }
        e.key_len = @as(u16, @intCast(elem.len));
        e.dot = dot;

        entries[entry_count] = e;
        entry_count += 1;
        return OrSetOpStatus.ok;
    }

    /// Remove all locally observed dots associated with `elem`.
    /// Because `context` retains all observed dots, concurrent unobserved `add(elem)` dots
    /// from remote replicas will automatically survive subsequent `merge` calls (Add-Wins).
    /// Returns `true` if at least one dot for `elem` was removed.
    fn remove(*_, elem: []const u8): bool {
        let removed_any = false;
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < entry_count) : (read_idx += 1) {
            const k: []const u8 = entries[read_idx].key_buf[0..@as(usize, entries[read_idx].key_len)];
            if (entries[read_idx].active && bytesEqual(k, elem)) {
                removed_any = true;
            } else {
                if (write_idx != read_idx) {
                    entries[write_idx] = entries[read_idx];
                }
                write_idx += 1;
            }
        }
        let clear_idx: usize = write_idx;
        while (clear_idx < entry_count) : (clear_idx += 1) {
            entries[clear_idx] = OrSetEntry{};
        }
        entry_count = write_idx;
        return removed_any;
    }

    /// Return true if `elem` currently has at least one active dot in this replica.
    fn contains(elem: []const u8): bool {
        let i: usize = 0;
        while (i < entry_count) : (i += 1) {
            if (!entries[i].active) continue;
            const k: []const u8 = entries[i].key_buf[0..@as(usize, entries[i].key_len)];
            if (bytesEqual(k, elem)) return true;
        }
        return false;
    }

    /// Return the total number of active `(element, dot)` pairs in this replica.
    fn dotCount(): usize {
        return entry_count;
    }

    /// Return the number of distinct element keys currently present in this replica.
    fn distinctCount(): usize {
        let distinct: usize = 0;
        let i: usize = 0;
        while (i < entry_count) : (i += 1) {
            if (!entries[i].active) continue;
            const ki: []const u8 = entries[i].key_buf[0..@as(usize, entries[i].key_len)];
            let seen_before = false;
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (!entries[j].active) continue;
                const kj: []const u8 = entries[j].key_buf[0..@as(usize, entries[j].key_len)];
                if (bytesEqual(ki, kj)) {
                    seen_before = true;
                    break;
                }
            }
            if (!seen_before) distinct += 1;
        }
        return distinct;
    }

    /// Internal helper: check whether an exact `(key, dot)` pair is present in this replica.
    fn hasExactPair(key: []const u8, dot: OrSetDot): bool {
        let i: usize = 0;
        while (i < entry_count) : (i += 1) {
            if (!entries[i].active) continue;
            if (!entries[i].dot.equals(dot)) continue;
            const k: []const u8 = entries[i].key_buf[0..@as(usize, entries[i].key_len)];
            if (bytesEqual(k, key)) return true;
        }
        return false;
    }

    /// Merge a remote `OrSetCrdt` state into this replica (strict join-semilattice LUB):
    ///   - Keeps every local `(e, d)` that is either present in `remote` OR not yet seen by `remote.context`
    ///   - Adds every remote `(e, d)` that is not already in local AND not yet seen by local `context`
    ///   - Joins `context` with `remote.context` via element-wise maximum
    /// Returns `.capacity_full` with zero side effects if the merged dot set exceeds `ORSET_MAX_ENTRIES`.
    fn merge(*_, remote: OrSetCrdt): OrSetOpStatus {
        let next_entries: [ORSET_MAX_ENTRIES]OrSetEntry = [OrSetEntry{}; ORSET_MAX_ENTRIES];
        let next_count: usize = 0;

        // 1. Retain local entries that are shared with remote OR unobserved by remote's context.
        let i: usize = 0;
        while (i < entry_count) : (i += 1) {
            if (!entries[i].active) continue;
            const k: []const u8 = entries[i].key_buf[0..@as(usize, entries[i].key_len)];
            const in_remote = remote.hasExactPair(k, entries[i].dot);
            if (in_remote || !remote.context.hasSeen(entries[i].dot)) {
                if (next_count >= ORSET_MAX_ENTRIES) return OrSetOpStatus.capacity_full;
                next_entries[next_count] = entries[i];
                next_count += 1;
            }
        }

        // 2. Import remote entries that are not already in local AND unobserved by local context.
        let j: usize = 0;
        while (j < remote.entry_count) : (j += 1) {
            if (!remote.entries[j].active) continue;
            const rk: []const u8 = remote.entries[j].key_buf[0..@as(usize, remote.entries[j].key_len)];
            if (!hasExactPair(rk, remote.entries[j].dot) && !context.hasSeen(remote.entries[j].dot)) {
                if (next_count >= ORSET_MAX_ENTRIES) return OrSetOpStatus.capacity_full;
                next_entries[next_count] = remote.entries[j];
                next_count += 1;
            }
        }

        // 3. Commit merged dot table and causal context atomically.
        let k_idx: usize = 0;
        while (k_idx < next_count) : (k_idx += 1) {
            entries[k_idx] = next_entries[k_idx];
        }
        while (k_idx < entry_count) : (k_idx += 1) {
            entries[k_idx] = OrSetEntry{};
        }
        entry_count = next_count;
        context.merge(remote.context);
        merge_count += 1;
        return OrSetOpStatus.ok;
    }

    /// Return true if this replica and `other` contain the exact same set of distinct elements.
    fn equalsElements(other: OrSetCrdt): bool {
        if (distinctCount() != other.distinctCount()) return false;
        let i: usize = 0;
        while (i < entry_count) : (i += 1) {
            if (!entries[i].active) continue;
            const k: []const u8 = entries[i].key_buf[0..@as(usize, entries[i].key_len)];
            if (!other.contains(k)) return false;
        }
        return true;
    }

    /// Return a zero-heap telemetry snapshot of this OR-Set replica.
    fn stats(): OrSetStats {
        return OrSetStats{
            active_dots: entry_count,
            distinct_elements: distinctCount(),
            replicas_tracked: context.trackedReplicas(),
            merge_count: merge_count,
        };
    }
}

// ============================================================================
// 5. LwwRegister64 — Zero-Heap Deterministic Last-Write-Wins 64-Bit Register
// ============================================================================

/// Zero-heap Last-Write-Wins 64-bit CRDT register with total-order tie-breaking
/// on `(timestamp_ms, replica_id, value)` guaranteeing commutative, associative,
/// and idempotent `merge`.
pub struct LwwRegister64 {
    local_replica_id: u16 = 0;
    replica_id: u16 = 0;
    timestamp_ms: u64 = 0;
    value: u64 = 0;
    initialized: bool = false;

    /// Initialize an empty `LwwRegister64` owned by `replica_id`.
    static fn init(replica_id: u16): LwwRegister64 {
        let reg = LwwRegister64{};
        reg.local_replica_id = replica_id;
        reg.replica_id = replica_id;
        return reg;
    }

    /// Initialize a `LwwRegister64` with an initial value and timestamp.
    static fn initWithValue(replica_id: u16, initial_value: u64, ts_ms: u64): LwwRegister64 {
        let reg = LwwRegister64{};
        reg.local_replica_id = replica_id;
        reg.replica_id = replica_id;
        reg.timestamp_ms = ts_ms;
        reg.value = initial_value;
        reg.initialized = true;
        return reg;
    }

    /// Attempt a local write at `ts_ms`.
    /// Succeeds if uninitialized, or `ts_ms > timestamp_ms`, or equal timestamp with higher/equal replica priority.
    /// Returns `true` if the write took effect, `false` if rejected as stale.
    fn set(*_, new_value: u64, ts_ms: u64): bool {
        if (!initialized) {
            value = new_value;
            timestamp_ms = ts_ms;
            replica_id = local_replica_id;
            initialized = true;
            return true;
        }
        if (ts_ms < timestamp_ms) return false;
        if (ts_ms == timestamp_ms && local_replica_id < replica_id) return false;
        value = new_value;
        timestamp_ms = ts_ms;
        replica_id = local_replica_id;
        return true;
    }

    /// Return the current winning value, or `null` if no write has been applied yet.
    fn get(): u64? {
        if (!initialized) return null;
        return value;
    }

    /// Merge a remote `LwwRegister64` into this register using deterministic `(timestamp_ms, replica_id, value)` ordering.
    /// Returns `true` if the local state was updated to the remote value.
    fn merge(*_, remote: LwwRegister64): bool {
        if (!remote.initialized) return false;
        if (!initialized) {
            value = remote.value;
            timestamp_ms = remote.timestamp_ms;
            replica_id = remote.replica_id;
            initialized = true;
            return true;
        }

        let remote_wins = false;
        if (remote.timestamp_ms > timestamp_ms) {
            remote_wins = true;
        } else if (remote.timestamp_ms == timestamp_ms) {
            if (remote.replica_id > replica_id) {
                remote_wins = true;
            } else if (remote.replica_id == replica_id && remote.value > value) {
                remote_wins = true;
            }
        }

        if (remote_wins) {
            value = remote.value;
            timestamp_ms = remote.timestamp_ms;
            replica_id = remote.replica_id;
            return true;
        }
        return false;
    }
}

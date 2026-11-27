// Origin: Pure-CP Stdlib (Zero-Alloc O(1) Briggs-Torczon SparseSet & ECS Dense SparseMapU64) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation O(1) SparseSet & ECS SparseMap engine:
//   - `SparseSet`: O(1) insert, O(1) swap-remove, O(1) membership test, O(1) zero-touch `clear()`,
//     cache-friendly contiguous `denseSlice()` iteration, and set algebra (`intersectInto`,
//     `unionInto`, `differenceInto`, `isSubsetOf`)
//   - `SparseMapU64`: ECS-style parallel dense arrays (`dense_ids` + `dense_vals`) with O(1)
//     `put`, `get`, `remove` (swap-remove preserving strict ID/value alignment), and `clear`

// ============================================================================
// 1. Constants & Telemetry
// ============================================================================

/// Fixed integer universe `[0, SPARSE_SET_UNIVERSE)` supported by `SparseSet` and `SparseMapU64`.
pub const SPARSE_SET_UNIVERSE: usize = 1024;

/// Sentinel index for uninitialized sparse slots (`0xFFFF`).
pub const SPARSE_SET_INVALID_IDX: u16 = 0xFFFF;

/// Operation telemetry snapshot for `SparseSet`.
pub struct SparseSetStats {
    len: usize = 0;
    universe: usize = SPARSE_SET_UNIVERSE;
    insert_ops: u64 = 0;
    remove_ops: u64 = 0;
    clear_ops: u64 = 0;
}

// ============================================================================
// 2. O(1) `SparseSet`
// ============================================================================

/// Zero-heap-allocation Briggs-Torczon O(1) integer set over `[0, 1024)`.
///
/// Membership is verified via the classic two-way invariant:
/// `id < SPARSE_SET_UNIVERSE && sparse[id] < dense_len && dense[sparse[id]] == id`,
/// enabling O(1) `clear()` without touching the `sparse` array.
pub struct SparseSet {
    dense: [SPARSE_SET_UNIVERSE]u16 = undefined;
    sparse: [SPARSE_SET_UNIVERSE]u16 = [SPARSE_SET_INVALID_IDX; SPARSE_SET_UNIVERSE];
    dense_len: usize = 0;
    insert_ops: u64 = 0;
    remove_ops: u64 = 0;
    clear_ops: u64 = 0;

    /// Initializes an empty `SparseSet`.
    static fn init(): SparseSet {
        return SparseSet{};
    }

    /// Returns the number of elements currently stored.
    fn len(): usize {
        return dense_len;
    }

    /// Returns `true` if the set contains zero elements.
    fn isEmpty(): bool {
        return dense_len == 0;
    }

    /// O(1) membership check immune to stale `sparse` entries left after `clear()`.
    fn contains(id: u16): bool {
        const uid: usize = @as(usize, id);
        if (uid >= SPARSE_SET_UNIVERSE) return false;
        const idx: usize = @as(usize, sparse[uid]);
        return idx < dense_len && dense[idx] == id;
    }

    /// O(1) insert. Returns `true` if newly inserted; `false` if out-of-range or already present.
    fn insert(*_, id: u16): bool {
        const uid: usize = @as(usize, id);
        if (uid >= SPARSE_SET_UNIVERSE) return false;
        if (contains(id)) return false;

        const idx: usize = dense_len;
        dense[idx] = id;
        sparse[uid] = @as(u16, @intCast(idx));
        dense_len = idx + 1;
        insert_ops += 1;
        return true;
    }

    /// O(1) swap-remove keeping `dense[0..len()]` tightly packed with zero holes.
    /// Returns `true` if removed, `false` if absent or out-of-range.
    fn remove(*_, id: u16): bool {
        const uid: usize = @as(usize, id);
        if (uid >= SPARSE_SET_UNIVERSE) return false;
        if (!contains(id)) return false;

        const idx: usize = @as(usize, sparse[uid]);
        const last_idx: usize = dense_len - 1;
        const last_id: u16 = dense[last_idx];
        dense[idx] = last_id;
        sparse[@as(usize, last_id)] = @as(u16, @intCast(idx));
        dense_len = last_idx;
        remove_ops += 1;
        return true;
    }

    /// O(1) zero-touch clear: resets `dense_len = 0` without clearing `sparse`.
    fn clear(*_) {
        dense_len = 0;
        clear_ops += 1;
    }

    /// Returns a cache-friendly contiguous read-only slice of all present element IDs.
    fn denseSlice(): []const u16 {
        return dense[0..dense_len];
    }

    /// Returns operation statistics snapshot.
    fn stats(): SparseSetStats {
        return SparseSetStats{
            len: dense_len,
            universe: SPARSE_SET_UNIVERSE,
            insert_ops: insert_ops,
            remove_ops: remove_ops,
            clear_ops: clear_ops,
        };
    }

    /// Writes `this ∩ other` into `out` (clears `out` first) and returns the intersection size.
    /// Iterates over the smaller dense array for O(min(|A|, |B|)) performance.
    fn intersectInto(other: SparseSet, out: *SparseSet): usize {
        out.clear();
        if (dense_len <= other.dense_len) {
            let i: usize = 0;
            while (i < dense_len) : (i += 1) {
                const id: u16 = dense[i];
                if (other.contains(id)) {
                    * = out.insert(id);
                }
            }
        } else {
            let i: usize = 0;
            while (i < other.dense_len) : (i += 1) {
                const id: u16 = other.dense[i];
                if (contains(id)) {
                    * = out.insert(id);
                }
            }
        }
        return out.len();
    }

    /// Writes `this ∪ other` into `out` (clears `out` first) and returns the union size.
    fn unionInto(other: SparseSet, out: *SparseSet): usize {
        out.clear();
        let i: usize = 0;
        while (i < dense_len) : (i += 1) {
            * = out.insert(dense[i]);
        }
        let j: usize = 0;
        while (j < other.dense_len) : (j += 1) {
            * = out.insert(other.dense[j]);
        }
        return out.len();
    }

    /// Writes `this \ other` into `out` (clears `out` first) and returns the difference size.
    fn differenceInto(other: SparseSet, out: *SparseSet): usize {
        out.clear();
        let i: usize = 0;
        while (i < dense_len) : (i += 1) {
            const id: u16 = dense[i];
            if (!other.contains(id)) {
                * = out.insert(id);
            }
        }
        return out.len();
    }

    /// Returns `true` if every element in `this` is also present in `other`.
    fn isSubsetOf(other: SparseSet): bool {
        if (dense_len > other.dense_len) return false;
        let i: usize = 0;
        while (i < dense_len) : (i += 1) {
            if (!other.contains(dense[i])) return false;
        }
        return true;
    }
}

// ============================================================================
// 3. O(1) ECS Dense `SparseMapU64`
// ============================================================================

/// Zero-heap-allocation ECS-style dense map from integer entity ID (`0..1023`) to `u64` value,
/// maintaining strictly aligned parallel slices `denseIds()` and `denseValues()`.
pub struct SparseMapU64 {
    dense_ids: [SPARSE_SET_UNIVERSE]u16 = undefined;
    dense_vals: [SPARSE_SET_UNIVERSE]u64 = undefined;
    sparse: [SPARSE_SET_UNIVERSE]u16 = [SPARSE_SET_INVALID_IDX; SPARSE_SET_UNIVERSE];
    dense_len: usize = 0;

    /// Initializes an empty `SparseMapU64`.
    static fn init(): SparseMapU64 {
        return SparseMapU64{};
    }

    /// Returns the number of stored key-value pairs.
    fn len(): usize {
        return dense_len;
    }

    /// O(1) membership test.
    fn contains(id: u16): bool {
        const uid: usize = @as(usize, id);
        if (uid >= SPARSE_SET_UNIVERSE) return false;
        const idx: usize = @as(usize, sparse[uid]);
        return idx < dense_len && dense_ids[idx] == id;
    }

    /// O(1) insert or overwrite. Returns `false` if `id >= SPARSE_SET_UNIVERSE`.
    fn put(*_, id: u16, val: u64): bool {
        const uid: usize = @as(usize, id);
        if (uid >= SPARSE_SET_UNIVERSE) return false;
        if (contains(id)) {
            const idx: usize = @as(usize, sparse[uid]);
            dense_vals[idx] = val;
            return true;
        }
        const idx: usize = dense_len;
        dense_ids[idx] = id;
        dense_vals[idx] = val;
        sparse[uid] = @as(u16, @intCast(idx));
        dense_len = idx + 1;
        return true;
    }

    /// O(1) lookup returning the stored `u64` value, or `null` if absent.
    fn get(id: u16): u64? {
        if (!contains(id)) return null;
        const idx: usize = @as(usize, sparse[@as(usize, id)]);
        return dense_vals[idx];
    }

    /// O(1) swap-remove preserving strict index alignment between `denseIds()` and `denseValues()`.
    /// Returns the removed `u64` value, or `null` if absent.
    fn remove(*_, id: u16): u64? {
        if (!contains(id)) return null;
        const idx: usize = @as(usize, sparse[@as(usize, id)]);
        const removed_val: u64 = dense_vals[idx];
        const last_idx: usize = dense_len - 1;
        const last_id: u16 = dense_ids[last_idx];
        const last_val: u64 = dense_vals[last_idx];

        dense_ids[idx] = last_id;
        dense_vals[idx] = last_val;
        sparse[@as(usize, last_id)] = @as(u16, @intCast(idx));
        dense_len = last_idx;
        return removed_val;
    }

    /// O(1) zero-touch clear.
    fn clear(*_) {
        dense_len = 0;
    }

    /// Returns contiguous slice of active entity IDs aligned with `denseValues()`.
    fn denseIds(): []const u16 {
        return dense_ids[0..dense_len];
    }

    /// Returns contiguous slice of active `u64` values aligned with `denseIds()`.
    fn denseValues(): []const u64 {
        return dense_vals[0..dense_len];
    }
}

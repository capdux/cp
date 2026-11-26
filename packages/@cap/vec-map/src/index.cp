// @cap/vec-map — Zero-allocation O(1) direct-indexed sparse integer-key map
// Inspired by crates.io:vec_map (https://github.com/contain-rs/vec-map)

pub const VECMAP_CAP: usize = 128;

fn vmapMask(bit: usize): u64 {
    return (1 as u64) << bit;
}

fn vmapHasBit(bits_lo: u64, bits_hi: u64, k: usize): bool {
    if (k >= VECMAP_CAP) {
        return false;
    }
    if (k < 64) {
        return (bits_lo & vmapMask(k)) != 0;
    }
    return (bits_hi & vmapMask(k - 64)) != 0;
}

/// Zero-heap O(1) direct-indexed sparse map for integer keys `0..127` -> `i64`.
/// Uses a 128-bit presence bitmap (`bits_lo`, `bits_hi`) alongside a 128-slot value array.
pub struct VecMapI64 {
    slots: [128]i64,
    bits_lo: u64,
    bits_hi: u64,
    count: usize,

    static fn new(): VecMapI64 {
        return VecMapI64{
            slots: [0; 128],
            bits_lo: 0,
            bits_hi: 0,
            count: 0,
        };
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn containsKey(k: usize): bool {
        return vmapHasBit(_.bits_lo, _.bits_hi, k);
    }

    fn get(k: usize): i64? {
        if (!vmapHasBit(_.bits_lo, _.bits_hi, k)) {
            return null;
        }
        return _.slots[k];
    }

    /// Inserts `(k, v)` for `k < VECMAP_CAP`.
    /// Returns the previous value if `k` was already present, or `null` otherwise.
    fn insert(*_, k: usize, v: i64): i64? {
        if (k >= VECMAP_CAP) {
            return null;
        }
        if (vmapHasBit(_.bits_lo, _.bits_hi, k)) {
            const prev: i64 = _.slots[k];
            _.slots[k] = v;
            return prev;
        }
        if (k < 64) {
            _.bits_lo = _.bits_lo | vmapMask(k);
        } else {
            _.bits_hi = _.bits_hi | vmapMask(k - 64);
        }
        _.slots[k] = v;
        _.count += 1;
        return null;
    }

    /// Removes key `k` if present and returns its previous value, or `null` if absent.
    fn remove(*_, k: usize): i64? {
        if (!vmapHasBit(_.bits_lo, _.bits_hi, k)) {
            return null;
        }
        const prev: i64 = _.slots[k];
        if (k < 64) {
            _.bits_lo = _.bits_lo & ~vmapMask(k);
        } else {
            _.bits_hi = _.bits_hi & ~vmapMask(k - 64);
        }
        _.slots[k] = 0;
        _.count -= 1;
        return prev;
    }

    /// Removes all entries in O(1).
    fn clear(*_) {
        _.bits_lo = 0;
        _.bits_hi = 0;
        _.count = 0;
    }

    /// Writes present keys in ascending order into `out`.
    /// Returns the number of keys written.
    fn keys(out: []usize): usize {
        let written: usize = 0;
        let k: usize = 0;
        while (k < VECMAP_CAP && written < out.len) : (k += 1) {
            if (vmapHasBit(_.bits_lo, _.bits_hi, k)) {
                out[written] = k;
                written += 1;
            }
        }
        return written;
    }

    /// Writes values of present keys in ascending key order into `out`.
    /// Returns the number of values written.
    fn values(out: []i64): usize {
        let written: usize = 0;
        let k: usize = 0;
        while (k < VECMAP_CAP && written < out.len) : (k += 1) {
            if (vmapHasBit(_.bits_lo, _.bits_hi, k)) {
                out[written] = _.slots[k];
                written += 1;
            }
        }
        return written;
    }

    /// Retains only entries whose value is strictly greater than `min_val`.
    /// Returns the number of retained entries.
    fn retainGreaterThan(*_, min_val: i64): usize {
        let k: usize = 0;
        while (k < VECMAP_CAP) : (k += 1) {
            if (vmapHasBit(_.bits_lo, _.bits_hi, k)) {
                if (_.slots[k] <= min_val) {
                    if (k < 64) {
                        _.bits_lo = _.bits_lo & ~vmapMask(k);
                    } else {
                        _.bits_hi = _.bits_hi & ~vmapMask(k - 64);
                    }
                    _.slots[k] = 0;
                    _.count -= 1;
                }
            }
        }
        return _.count;
    }
}

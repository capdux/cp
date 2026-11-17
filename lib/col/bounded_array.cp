// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/collections/bounded_array.cp
// Upstream Sync: Refer to bun/src/collections/bounded_array.zig for future upstream updates.
// ============================================================================
// lib/collections/bounded_array.cp
// Zero-heap stack-allocated bounded array with compile-time capacity
// Absorbed from Bun src/collections/bounded_array.cp

pub struct BoundedArray<T, const CAP: usize> {
    buffer: [CAP]T = undefined;
    len: usize = 0;

    pub static fn init(): BoundedArray<T, CAP> {
        return BoundedArray<T, CAP> {
            buffer: undefined,
            len: 0,
        };
    }

    pub fn capacity(): usize {
        return _.buffer.len;
    }

    pub fn isEmpty(): bool {
        return _.len == 0;
    }

    pub fn isFull(): bool {
        return _.len >= CAP;
    }

    pub fn unusedCapacity(): usize {
        return CAP - _.len;
    }

    pub fn clear(*_) {
        _.len = 0;
    }

    pub fn append(*_, item: T): bool {
        if (_.len >= CAP) {
            return false;
        }
        _.buffer[_.len] = item;
        _.len += 1;
        return true;
    }

    pub fn appendSlice(*_, items: []const T): bool {
        if (_.len + items.len > CAP) {
            return false;
        }
        let i: usize = 0;
        while (i < items.len) {
            _.buffer[_.len + i] = items[i];
            i += 1;
        }
        _.len += items.len;
        return true;
    }

    pub fn get(i: usize): T {
        return _.buffer[i];
    }

    pub fn set(*_, i: usize, item: T) {
        if (i < _.len) {
            _.buffer[i] = item;
        }
    }

    pub fn pop(*_): T? {
        if (_.len == 0) {
            return null;
        }
        _.len -= 1;
        return _.buffer[_.len];
    }

    pub fn orderedRemove(*_, idx: usize): T {
        const val = _.buffer[idx];
        let i: usize = idx;
        while (i + 1 < _.len) {
            _.buffer[i] = _.buffer[i + 1];
            i += 1;
        }
        _.len -= 1;
        return val;
    }

    pub fn swapRemove(*_, idx: usize): T {
        const val = _.buffer[idx];
        _.len -= 1;
        if (idx != _.len) {
            _.buffer[idx] = _.buffer[_.len];
        }
        return val;
    }
}

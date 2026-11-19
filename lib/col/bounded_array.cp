// Source: /data/dev/bun/src/collections/bounded_array.cp
// ============================================================================
// Absorbed from Bun: bun/src/collections/bounded_array.cp
// Upstream Sync: Refer to bun/src/collections/bounded_array.zig for future upstream updates.
// ============================================================================
// lib/col/bounded_array.cp
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

    pub static fn fromSlice(items: []const T): BoundedArray<T, CAP> {
        let ba = BoundedArray<T, CAP>.init();
        ba.appendSlice(items);
        return ba;
    }

    pub fn capacity(): usize {
        return _.buffer.len;
    }

    pub fn count(): usize {
        return _.len;
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

    pub fn slice(*_): []T {
        return _.buffer[0.._.len];
    }

    pub fn constSlice(): []const T {
        return _.buffer[0.._.len];
    }

    pub fn unusedCapacitySlice(*_): []T {
        return _.buffer[_.len..CAP];
    }

    pub fn resize(*_, new_len: usize): bool {
        if (new_len > CAP) {
            return false;
        }
        _.len = new_len;
        return true;
    }

    pub fn clear(*_) {
        _.len = 0;
    }

    pub fn ensureUnusedCapacity(additional_count: usize): bool {
        return (_.len + additional_count <= CAP);
    }

    pub fn get(i: usize): T {
        return _.buffer[i];
    }

    pub fn at(i: usize): T {
        return _.buffer[i];
    }

    pub fn first(): T? {
        if (_.len == 0) {
            return null;
        }
        return _.buffer[0];
    }

    pub fn last(): T? {
        if (_.len == 0) {
            return null;
        }
        return _.buffer[_.len - 1];
    }

    pub fn set(*_, i: usize, item: T) {
        if (i < _.len) {
            _.buffer[i] = item;
        }
    }

    pub fn append(*_, item: T): bool {
        if (_.len >= CAP) {
            return false;
        }
        _.buffer[_.len] = item;
        _.len += 1;
        return true;
    }

    pub fn appendAssumeCapacity(*_, item: T) {
        _.buffer[_.len] = item;
        _.len += 1;
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

    pub fn appendSliceAssumeCapacity(*_, items: []const T) {
        let i: usize = 0;
        while (i < items.len) {
            _.buffer[_.len + i] = items[i];
            i += 1;
        }
        _.len += items.len;
    }

    pub fn appendNTimes(*_, value: T, n: usize): bool {
        if (_.len + n > CAP) {
            return false;
        }
        let i: usize = 0;
        while (i < n) {
            _.buffer[_.len + i] = value;
            i += 1;
        }
        _.len += n;
        return true;
    }

    pub fn appendNTimesAssumeCapacity(*_, value: T, n: usize) {
        let i: usize = 0;
        while (i < n) {
            _.buffer[_.len + i] = value;
            i += 1;
        }
        _.len += n;
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

    pub fn insert(*_, i: usize, item: T): bool {
        if (i > _.len || _.len >= CAP) {
            return false;
        }
        let j: usize = _.len;
        while (j > i) {
            _.buffer[j] = _.buffer[j - 1];
            j -= 1;
        }
        _.buffer[i] = item;
        _.len += 1;
        return true;
    }

    pub fn insertSlice(*_, i: usize, items: []const T): bool {
        if (i > _.len || _.len + items.len > CAP) {
            return false;
        }
        const shift = items.len;
        let j: usize = _.len + shift;
        while (j > i + shift) {
            _.buffer[j - 1] = _.buffer[j - 1 - shift];
            j -= 1;
        }
        let k: usize = 0;
        while (k < shift) {
            _.buffer[i + k] = items[k];
            k += 1;
        }
        _.len += shift;
        return true;
    }

    pub fn replaceRange(*_, start: usize, len: usize, new_items: []const T): bool {
        if (start + len > _.len) {
            return false;
        }
        if (_.len - len + new_items.len > CAP) {
            return false;
        }
        if (len == new_items.len) {
            let i: usize = 0;
            while (i < len) {
                _.buffer[start + i] = new_items[i];
                i += 1;
            }
            return true;
        } else if (len < new_items.len) {
            const shift = new_items.len - len;
            let j: usize = _.len + shift;
            while (j > start + new_items.len) {
                _.buffer[j - 1] = _.buffer[j - 1 - shift];
                j -= 1;
            }
            let i: usize = 0;
            while (i < new_items.len) {
                _.buffer[start + i] = new_items[i];
                i += 1;
            }
            _.len += shift;
            return true;
        } else {
            const shift = len - new_items.len;
            let i: usize = 0;
            while (i < new_items.len) {
                _.buffer[start + i] = new_items[i];
                i += 1;
            }
            let j: usize = start + new_items.len;
            while (j + shift < _.len) {
                _.buffer[j] = _.buffer[j + shift];
                j += 1;
            }
            _.len -= shift;
            return true;
        }
    }

    pub fn contains(item: T): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.buffer[i] == item) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn findIndex(item: T): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.buffer[i] == item) {
                return i;
            }
            i += 1;
        }
        return null;
    }
}

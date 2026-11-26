// @cap/merge-descriptors — Pure CP zero-allocation property descriptor table
// & safe descriptor merge engine.
// Clean-room MIT implementation inspired by npm `merge-descriptors` v2.

pub const MAX_DESCRIPTORS: usize = 32;

fn mgdStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Property descriptor modeled after ECMAScript `PropertyDescriptor`.
pub struct PropDescriptor {
    pub key: string,
    pub value: string,
    pub writable: bool,
    pub enumerable: bool,
    pub configurable: bool,

    pub static fn data(key: string, value: string): PropDescriptor {
        return PropDescriptor {
            key: key,
            value: value,
            writable: true,
            enumerable: true,
            configurable: true
        };
    }

    pub static fn readonly(key: string, value: string): PropDescriptor {
        return PropDescriptor {
            key: key,
            value: value,
            writable: false,
            enumerable: true,
            configurable: false
        };
    }
}

fn mgdEmptyDesc(): PropDescriptor {
    return PropDescriptor {
        key: "",
        value: "",
        writable: false,
        enumerable: false,
        configurable: false
    };
}

/// Fixed-capacity (32-slot) zero-heap property descriptor table with
/// ECMAScript `configurable` guard enforcement.
pub struct DescriptorTable {
    pub entries: [32]PropDescriptor,
    pub count: usize,

    pub static fn new(): DescriptorTable {
        let empty = mgdEmptyDesc();
        return DescriptorTable {
            entries: [empty; 32],
            count: 0
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn has(key: string): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (mgdStrEq(_.entries[i].key, key)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    pub fn get(key: string): PropDescriptor? {
        let i: usize = 0;
        while (i < _.count) {
            if (mgdStrEq(_.entries[i].key, key)) {
                return _.entries[i];
            }
            i = i + 1;
        }
        return null;
    }

    /// Defines or redefines a property descriptor.
    /// Rejects empty keys, non-configurable existing keys, or overflow past 32 slots.
    pub fn define(*_, desc: PropDescriptor): bool {
        if (desc.key.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.count) {
            if (mgdStrEq(_.entries[i].key, desc.key)) {
                if (!_.entries[i].configurable) {
                    return false;
                }
                _.entries[i] = desc;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= MAX_DESCRIPTORS) {
            return false;
        }
        _.entries[_.count] = desc;
        _.count = _.count + 1;
        return true;
    }

    /// Deletes a property by key, honoring the `configurable` guard.
    /// Returns `false` if the key does not exist or is non-configurable.
    pub fn deleteKey(*_, key: string): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (mgdStrEq(_.entries[i].key, key)) {
                if (!_.entries[i].configurable) {
                    return false;
                }
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j = j + 1;
                }
                _.count = _.count - 1;
                _.entries[_.count] = mgdEmptyDesc();
                return true;
            }
            i = i + 1;
        }
        return false;
    }
}

/// Merges property descriptors from `src` into `dst` (parity with npm `merge-descriptors`).
/// - When `redefine == true`, overwrites existing configurable keys in `dst`.
/// - When `redefine == false`, skips keys already present in `dst` and only adds new ones.
/// - Non-configurable keys in `dst` are always protected from overwrite.
/// Returns the number of descriptors actually written into `dst`.
pub fn mergeDescriptors(dst: *DescriptorTable, src: DescriptorTable, redefine: bool): usize {
    let written: usize = 0;
    let i: usize = 0;
    while (i < src.count) {
        let desc = src.entries[i];
        if (!redefine && dst.*.has(desc.key)) {
            i = i + 1;
            continue;
        }
        if (dst.*.define(desc)) {
            written = written + 1;
        }
        i = i + 1;
    }
    return written;
}

/// Counts the number of enumerable descriptors in `tbl`.
pub fn countEnumerable(tbl: DescriptorTable): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < tbl.count) {
        if (tbl.entries[i].enumerable) {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

export {
    MAX_DESCRIPTORS,
    PropDescriptor,
    DescriptorTable,
    mergeDescriptors,
    countEnumerable
};

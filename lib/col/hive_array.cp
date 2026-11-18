// Source: bun/src/collections/hive_array.cp
// ============================================================================
// Absorbed from Bun: bun/src/collections/hive_array.cp
// Upstream Sync: Refer to bun/src/collections/hive_array.zig for future upstream updates.
// ============================================================================
// lib/col/hive_array.cp
// O(1) BitSet64 and HiveArray slot pool with pointer/index stability
// Absorbed from Bun src/collections/hive_array.cp and bit_set.zig

import { BitSet64 } from "./bit_set.cp";
export { BitSet64 };

pub struct HiveArray<T, const CAP: usize> {
    buffer: [CAP]T = undefined;
    used: BitSet64 = BitSet64 {};

    pub static fn init(): HiveArray<T, CAP> {
        return HiveArray<T, CAP> {
            buffer: undefined,
            used: BitSet64.initEmpty(),
        };
    }

    pub fn capacity(): usize {
        return _.buffer.len;
    }

    pub fn usedCount(): usize {
        return _.used.count();
    }

    pub fn availableCount(): usize {
        return CAP - _.used.count();
    }

    pub fn isEmpty(): bool {
        return _.used.count() == 0;
    }

    pub fn isFull(): bool {
        return _.used.count() >= CAP;
    }

    // Claims an unused slot in O(1) using hardware @ctz, writes `val`, and returns its stable slot index
    pub fn acquire(*_, val: T): usize? {
        if (slot := _.used.findFirstUnset(CAP)) {
            _.used.set(slot);
            _.buffer[slot] = val;
            return slot;
        }
        return null;
    }

    pub fn get(slot: usize): T? {
        if (!_.used.isSet(slot)) {
            return null;
        }
        return _.buffer[slot];
    }

    pub fn set(*_, slot: usize, val: T): bool {
        if (!_.used.isSet(slot)) {
            return false;
        }
        _.buffer[slot] = val;
        return true;
    }

    pub fn releaseSlot(*_, slot: usize): bool {
        if (!_.used.isSet(slot)) {
            return false;
        }
        _.used.unset(slot);
        return true;
    }

    pub fn clear(*_) {
        _.used = BitSet64.initEmpty();
    }
}

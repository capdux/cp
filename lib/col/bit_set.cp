// lib/col/bit_set.cp
// Multi-word static bit set (ArrayBitSet / BitSet256) with hardware @ctz/@popCount acceleration
// Absorbed from Bun src/collections/bit_set.cp

pub struct BitSet256 {
    masks: [4]u64 = [0, 0, 0, 0];

    pub static fn initEmpty(): BitSet256 {
        return BitSet256 {
            masks: [0, 0, 0, 0],
        };
    }

    pub static fn initFull(): BitSet256 {
        return BitSet256 {
            masks: [
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
                0xFFFFFFFFFFFFFFFF,
            ],
        };
    }

    pub fn capacity(): usize {
        return _.words.len * 64;
    }

    pub fn isSet(index: usize): bool {
        if (index >= 256) {
            return false;
        }
        const word_idx = index >> 6;
        const bit_idx = index & 63;
        const bit: u64 = (1 as u64) << bit_idx;
        return (_.masks[word_idx] & bit) != 0;
    }

    pub fn set(*_, index: usize) {
        if (index < 256) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] |= bit;
        }
    }

    pub fn unset(*_, index: usize) {
        if (index < 256) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] &= ~bit;
        }
    }

    pub fn toggle(*_, index: usize) {
        if (index < 256) {
            const word_idx = index >> 6;
            const bit_idx = index & 63;
            const bit: u64 = (1 as u64) << bit_idx;
            _.masks[word_idx] ^= bit;
        }
    }

    pub fn count(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 4) {
            total += (@popCount(_.masks[i]) as usize);
            i += 1;
        }
        return total;
    }

    pub fn findFirstSet(): usize? {
        let i: usize = 0;
        while (i < 4) {
            if (_.masks[i] != 0) {
                return (i << 6) + (@ctz(_.masks[i]) as usize);
            }
            i += 1;
        }
        return null;
    }

    pub fn setUnion(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] |= other.masks[i];
            i += 1;
        }
    }

    pub fn setIntersection(*_, other: BitSet256) {
        let i: usize = 0;
        while (i < 4) {
            _.masks[i] &= other.masks[i];
            i += 1;
        }
    }
}

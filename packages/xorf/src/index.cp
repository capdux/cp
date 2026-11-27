// @cap/xor-filter — Zero-allocation Xor8 static fingerprint filter for CP
// Clean-room MIT implementation of Graf & Lemire's Xor8 filter (ACM JEA 2020, Rust `xorf` parity).
//
// Features:
// - Up to `MAX_KEYS = 256` unique 64-bit keys stored in at most 384 bytes (3 blocks x <= 128 slots).
// - ~9.84 bits/key (~30% smaller than Bloom filters at equivalent ~0.39% false-positive rate).
// - O(1) query: 3 array lookups + 2 XORs (`B[h0] ^ B[h1] ^ B[h2] == fp`), 0% false negatives.
// - Automatic duplicate-key deduplication & up to 16 seed retries during peeling construction.
// - Zero heap allocation.

const std = @import("std");

pub const MAX_KEYS: usize = 256;
pub const MAX_SLOTS: usize = 384; // 3 blocks * 128 slots

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn hashKey64(key: u64, seed: u64) -> u64 {
    let buf: [8]u8 = undefined;
    buf[0] = (key & 0xFF) as u8;
    buf[1] = ((key >> 8) & 0xFF) as u8;
    buf[2] = ((key >> 16) & 0xFF) as u8;
    buf[3] = ((key >> 24) & 0xFF) as u8;
    buf[4] = ((key >> 32) & 0xFF) as u8;
    buf[5] = ((key >> 40) & 0xFF) as u8;
    buf[6] = ((key >> 48) & 0xFF) as u8;
    buf[7] = ((key >> 56) & 0xFF) as u8;
    const h: u64 = std.hash.Wyhash.hash(seed, buf[0..8]);
    return h;
}

struct Slots3 {
    h0: usize,
    h1: usize,
    h2: usize,
    fp: u8,
}

fn deriveSlots(h: u64, block_len: usize) -> Slots3 {
    const r0: u32 = (h & (0xFFFFFFFF as u64)) as u32;
    const r1: u32 = ((h >> 32) & (0xFFFFFFFF as u64)) as u32;
    const r2: u32 = ((h >> 16) & (0xFFFFFFFF as u64)) as u32;
    const h0 = (r0 as usize) % block_len;
    const h1 = block_len + ((r1 as usize) % block_len);
    const h2 = (block_len * 2) + (((r0 ^ r1 ^ r2) as usize) % block_len);
    const fp = (((h ^ (h >> 32)) >> 8) & (0xFF as u64)) as u8;
    return Slots3{ h0: h0, h1: h1, h2: h2, fp: fp };
}

/// Static 8-bit Xor fingerprint filter (zero heap allocation, up to 256 keys).
pub struct Xor8Filter {
    pub seed: u64,
    pub block_length: usize,
    pub fingerprints: [MAX_SLOTS]u8,
    pub key_count: usize,
    pub valid: bool,

    static fn emptyWithStatus(is_valid: bool) -> Xor8Filter {
        let fp: [MAX_SLOTS]u8 = undefined;
        let i: usize = 0;
        while (i < MAX_SLOTS) {
            fp[i] = 0;
            i += 1;
        }
        return Xor8Filter{
            seed: 0,
            block_length: 0,
            fingerprints: fp,
            key_count: 0,
            valid: is_valid,
        };
    }

    /// Builds an `Xor8Filter` from `keys` via peeling + reverse assignment (up to 16 seed retries).
    /// Returns a valid empty filter if `keys.len == 0`, or `valid: false` if `keys.len > 256`.
    pub static fn build(keys: []const u64) -> Xor8Filter {
        if (keys.len == 0) {
            return Xor8Filter.emptyWithStatus(true);
        }
        if (keys.len > MAX_KEYS) {
            return Xor8Filter.emptyWithStatus(false);
        }

        // Deduplicate keys on the stack so duplicate inputs never break 3-hypergraph peeling
        let unique: [MAX_KEYS]u64 = undefined;
        let n: usize = 0;
        let ki: usize = 0;
        while (ki < keys.len) {
            const k = keys[ki];
            let dup = false;
            let uj: usize = 0;
            while (uj < n) {
                if (unique[uj] == k) {
                    dup = true;
                    break;
                }
                uj += 1;
            }
            if (!dup) {
                unique[n] = k;
                n += 1;
            }
            ki += 1;
        }

        // Compute block_length: ~1.23 * n + 32 rounded up to a multiple of 3, clamped to [11, 128]
        const raw_cap = ((123 * n) / 100) + 32;
        let bl = (raw_cap + 2) / 3;
        if (bl < 11) {
            bl = 11;
        }
        if (bl > 128) {
            bl = 128;
        }
        const total_slots = bl * 3;

        let t2count: [MAX_SLOTS]u8 = undefined;
        let t2hash: [MAX_SLOTS]u64 = undefined;
        let alone: [MAX_SLOTS]u16 = undefined;
        let rev_h: [MAX_KEYS]u64 = undefined;
        let rev_slot: [MAX_KEYS]u16 = undefined;

        const seed_step = makeU64(0x9E3779B9 as u32, 0x7F4A7C15 as u32);
        let attempt: u64 = 0;
        while (attempt < 16) {
            const seed: u64 = (attempt + 1) ^ (seed_step >> ((attempt & 15) as u6));

            // Clear mapping arrays
            let si: usize = 0;
            while (si < total_slots) {
                t2count[si] = 0;
                t2hash[si] = 0;
                si += 1;
            }

            // Mapping phase
            let i: usize = 0;
            while (i < n) {
                const h = hashKey64(unique[i], seed);
                const sl = deriveSlots(h, bl);
                t2count[sl.h0] += 1;
                t2hash[sl.h0] = t2hash[sl.h0] ^ h;
                t2count[sl.h1] += 1;
                t2hash[sl.h1] = t2hash[sl.h1] ^ h;
                t2count[sl.h2] += 1;
                t2hash[sl.h2] = t2hash[sl.h2] ^ h;
                i += 1;
            }

            // Collect degree-1 slots
            let alone_len: usize = 0;
            si = 0;
            while (si < total_slots) {
                if (t2count[si] == 1) {
                    alone[alone_len] = si as u16;
                    alone_len += 1;
                }
                si += 1;
            }

            // Peeling phase
            let stack_len: usize = 0;
            let q_head: usize = 0;
            while (q_head < alone_len) {
                const slot = alone[q_head] as usize;
                q_head += 1;
                if (t2count[slot] != 1) {
                    continue;
                }
                const h = t2hash[slot];
                rev_h[stack_len] = h;
                rev_slot[stack_len] = slot as u16;
                stack_len += 1;

                const sl = deriveSlots(h, bl);
                t2count[sl.h0] -= 1;
                t2hash[sl.h0] = t2hash[sl.h0] ^ h;
                if (t2count[sl.h0] == 1) {
                    alone[alone_len] = sl.h0 as u16;
                    alone_len += 1;
                }

                t2count[sl.h1] -= 1;
                t2hash[sl.h1] = t2hash[sl.h1] ^ h;
                if (t2count[sl.h1] == 1) {
                    alone[alone_len] = sl.h1 as u16;
                    alone_len += 1;
                }

                t2count[sl.h2] -= 1;
                t2hash[sl.h2] = t2hash[sl.h2] ^ h;
                if (t2count[sl.h2] == 1) {
                    alone[alone_len] = sl.h2 as u16;
                    alone_len += 1;
                }
            }

            if (stack_len == n) {
                // Assignment phase (reverse peeling order)
                let filter = Xor8Filter.emptyWithStatus(true);
                filter.seed = seed;
                filter.block_length = bl;
                filter.key_count = n;

                let k = stack_len;
                while (k > 0) {
                    k -= 1;
                    const h = rev_h[k];
                    const target = rev_slot[k] as usize;
                    const sl = deriveSlots(h, bl);
                    // filter.fingerprints[target] is 0 before assignment
                    filter.fingerprints[target] =
                        sl.fp ^ filter.fingerprints[sl.h0] ^ filter.fingerprints[sl.h1] ^ filter.fingerprints[sl.h2];
                }
                return filter;
            }

            attempt += 1;
        }

        return Xor8Filter.emptyWithStatus(false);
    }

    /// O(1) membership test: 3 array lookups + 2 XORs. Guaranteed 0% false negatives for all built keys.
    pub fn contains(key: u64) -> bool {
        if (!_.valid || _.key_count == 0 || _.block_length == 0) {
            return false;
        }
        const h = hashKey64(key, _.seed);
        const sl = deriveSlots(h, _.block_length);
        const x = _.fingerprints[sl.h0] ^ _.fingerprints[sl.h1] ^ _.fingerprints[sl.h2];
        return x == sl.fp;
    }

    pub fn len() -> usize {
        return _.key_count;
    }

    pub fn fingerprintBytes() -> usize {
        return _.block_length * 3;
    }

    pub fn isValid() -> bool {
        return _.valid;
    }
}

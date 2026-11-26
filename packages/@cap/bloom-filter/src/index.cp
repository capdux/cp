// @cap/bloom-filter — Zero-heap classic Bloom filter in pure CP
// MIT License — Clean-room implementation using Kirsch-Mitzenmacher double hashing

const FNV_OFFSET_BASIS: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
const FNV_PRIME: u64 = ((0x00000100 as u64) << 32) | (0x000001b3 as u64);
const FMIX_C1: u64 = ((0xff51afd7 as u64) << 32) | (0xed558ccd as u64);
const FMIX_C2: u64 = ((0xc4ceb9fe as u64) << 32) | (0x1a85ec53 as u64);
const LN2: f64 = 0.6931471805599453;

/// Compute 64-bit FNV-1a hash (overflow-free wrapping arithmetic).
pub fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ (data[i] as u64);
        h = h *% FNV_PRIME;
        i += 1;
    }
    return h;
}

/// Compute secondary 64-bit hash using DJB2-xor + Murmur3 fmix64 finalizer.
/// Guarantees a non-zero odd stride for Kirsch-Mitzenmacher double hashing.
pub fn djb2Mix64(data: []const u8): u64 {
    let h: u64 = 5381;
    let i: usize = 0;
    while (i < data.len) {
        h = ((h << 5) +% h) ^ (data[i] as u64);
        i += 1;
    }
    h = h ^ (h >> 33);
    h = h *% FMIX_C1;
    h = h ^ (h >> 33);
    h = h *% FMIX_C2;
    h = h ^ (h >> 33);
    return h | 1;
}

/// Count total set bits (population count) across a byte slice.
pub fn popcount(bits: []const u8): u64 {
    let total: u64 = 0;
    let i: usize = 0;
    while (i < bits.len) {
        let b: u8 = bits[i];
        while (b != 0) {
            b = b & (b - 1);
            total += 1;
        }
        i += 1;
    }
    return total;
}

/// Return fraction of set bits in [0.0, 1.0]; returns 0.0 for empty slices.
pub fn fillRatio(bits: []const u8): f64 {
    if (bits.len == 0) {
        return 0.0;
    }
    const total_bits: f64 = (bits.len * 8) as f64;
    const set_bits: f64 = popcount(bits) as f64;
    return set_bits / total_bits;
}

/// Compute optimal number of hash functions k = round((m / n) * ln(2)), clamped to >= 1.
pub fn optimalNumHashes(bit_count: usize, expected_items: usize): u32 {
    if (bit_count == 0 || expected_items == 0) {
        return 1;
    }
    const ratio: f64 = (bit_count as f64) / (expected_items as f64);
    const k_f64: f64 = ratio * LN2 + 0.5;
    if (k_f64 < 1.0) {
        return 1;
    }
    if (k_f64 > 64.0) {
        return 64;
    }
    return k_f64 as u32;
}

/// Zero-heap Bloom filter operating over a caller-owned `[]u8` bitset buffer.
pub struct BloomFilter {
    pub num_hashes: u32,
    pub bit_count: usize,
    pub inserted: u64,

    /// Create a new BloomFilter and zero-initialize `bits`.
    pub static fn new(bits: []u8, num_hashes: u32): BloomFilter {
        let i: usize = 0;
        while (i < bits.len) {
            bits[i] = 0;
            i += 1;
        }
        const k: u32 = if (num_hashes == 0) 1 else num_hashes;
        return BloomFilter {
            num_hashes: k,
            bit_count: bits.len * 8,
            inserted: 0,
        };
    }

    /// Clear all bits in `bits` and reset `inserted` count to 0.
    pub fn clear(*_, bits: []u8) {
        let i: usize = 0;
        while (i < bits.len) {
            bits[i] = 0;
            i += 1;
        }
        _.inserted = 0;
    }

    /// Insert a raw byte slice into the Bloom filter.
    pub fn insertBytes(*_, bits: []u8, item: []const u8) {
        const cap_bits: usize = bits.len * 8;
        const m: usize = if (cap_bits < _.bit_count) cap_bits else _.bit_count;
        if (m == 0) {
            return;
        }
        const h1: u64 = fnv1a64(item);
        const h2: u64 = djb2Mix64(item);
        const m_u64: u64 = m as u64;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const idx_u64: u64 = (h1 +% ((i as u64) *% h2)) % m_u64;
            const bit_idx: usize = idx_u64 as usize;
            const byte_idx: usize = bit_idx >> 3;
            const shift: u3 = (bit_idx & 7) as u3;
            bits[byte_idx] = bits[byte_idx] | ((1 as u8) << shift);
            i += 1;
        }
        _.inserted += 1;
    }

    /// Insert a string item into the Bloom filter.
    pub fn insert(*_, bits: []u8, item: string) {
        _.insertBytes(bits, item);
    }

    /// Test whether a raw byte slice may be in the Bloom filter.
    /// Zero false negatives; bounded false positive rate.
    pub fn containsBytes(bits: []const u8, item: []const u8): bool {
        const cap_bits: usize = bits.len * 8;
        const m: usize = if (cap_bits < _.bit_count) cap_bits else _.bit_count;
        if (m == 0) {
            return false;
        }
        const h1: u64 = fnv1a64(item);
        const h2: u64 = djb2Mix64(item);
        const m_u64: u64 = m as u64;
        let i: u32 = 0;
        while (i < _.num_hashes) {
            const idx_u64: u64 = (h1 +% ((i as u64) *% h2)) % m_u64;
            const bit_idx: usize = idx_u64 as usize;
            const byte_idx: usize = bit_idx >> 3;
            const shift: u3 = (bit_idx & 7) as u3;
            if ((bits[byte_idx] & ((1 as u8) << shift)) == 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Test whether a string item may be in the Bloom filter.
    pub fn contains(bits: []const u8, item: string): bool {
        return _.containsBytes(bits, item);
    }

    /// Merge another same-sized Bloom filter bitset into `dst_bits` via bitwise OR.
    /// Returns false if buffer sizes do not match or do not cover `bit_count`.
    pub fn merge(*_, dst_bits: []u8, src_bits: []const u8): bool {
        if (dst_bits.len != src_bits.len) {
            return false;
        }
        if (dst_bits.len * 8 < _.bit_count) {
            return false;
        }
        let i: usize = 0;
        while (i < dst_bits.len) {
            dst_bits[i] = dst_bits[i] | src_bits[i];
            i += 1;
        }
        return true;
    }
}

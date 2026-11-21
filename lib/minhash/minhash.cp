// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Andrei Broder MinHash & LSH Banding Library)

/// Recommended default signature length (128 hash permutations).
pub const MINHASH_DEFAULT_K: usize = 128;

/// Sentinel value representing an empty MinHash signature slot (`u64::MAX`).
pub const MINHASH_EMPTY_VAL: u64 = 0xFFFFFFFFFFFFFFFF;

const FNV_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
const FNV_PRIME: u64 = 0x00000100000001B3;
const SM64_GAMMA: u64 = 0x9E3779B97F4A7C15;
const SM64_MUL1: u64 = 0xBF58476D1CE4E5B9;
const SM64_MUL2: u64 = 0x94D049BB133111EB;
const TWO_POW_64: f64 = 18446744073709551616.0;

fn splitmix64(x: u64): u64 {
    let z: u64 = x +% SM64_GAMMA;
    z = (z ^ (z >> 30)) *% SM64_MUL1;
    z = (z ^ (z >> 27)) *% SM64_MUL2;
    return z ^ (z >> 31);
}

fn permuteHash(elem_hash: u64, slot: usize): u64 {
    const salt: u64 = ((slot as u64) +% 1) *% SM64_GAMMA;
    return splitmix64(elem_hash ^ salt);
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ (data[i] as u64)) *% FNV_PRIME;
        i += 1;
    }
    return h;
}

fn isAsciiAlnum(ch: u8): bool {
    return (ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9');
}

fn toLowerAscii(ch: u8): u8 {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - ('A' as u8) + ('a' as u8);
    }
    return ch;
}

/// Zero-allocation MinHash sketch backed by a caller-provided `[]u64` signature slice.
pub struct MinHash {
    sig: []u64,

    /// Initializes a MinHash sketch over `sig` (all slots set to `MINHASH_EMPTY_VAL`).
    /// Returns `null` if `sig.len == 0`.
    static fn init(sig: []u64): MinHash? {
        if (sig.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < sig.len) {
            sig[i] = MINHASH_EMPTY_VAL;
            i += 1;
        }
        return MinHash { sig: sig };
    }

    /// Resets all signature slots back to `MINHASH_EMPTY_VAL`.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.sig.len) {
            _.sig[i] = MINHASH_EMPTY_VAL;
            i += 1;
        }
    }

    /// Returns the number of hash permutations (`k`) in this signature.
    fn len(): usize {
        return _.sig.len;
    }

    /// Updates all `k` signature slots with a pre-hashed 64-bit element digest.
    fn addHash(*_, elem_hash: u64) {
        let i: usize = 0;
        while (i < _.sig.len) {
            const h: u64 = permuteHash(elem_hash, i);
            if (h < _.sig[i]) {
                _.sig[i] = h;
            }
            i += 1;
        }
    }

    /// Hashes `item` with 64-bit FNV-1a and inserts it into the sketch.
    fn addBytes(*_, item: []const u8) {
        _.addHash(fnv1a64(item));
    }

    /// Tokenizes ASCII alphanumeric words in `text` (case-insensitive, zero-copy) and inserts each word.
    fn addTextWords(*_, text: []const u8) {
        let h: u64 = FNV_OFFSET_BASIS;
        let in_word: bool = false;
        let i: usize = 0;
        while (i < text.len) {
            const ch: u8 = text[i];
            if (isAsciiAlnum(ch)) {
                const lower: u8 = toLowerAscii(ch);
                h = (h ^ (lower as u64)) *% FNV_PRIME;
                in_word = true;
            } else if (in_word) {
                _.addHash(h);
                h = FNV_OFFSET_BASIS;
                in_word = false;
            }
            i += 1;
        }
        if (in_word) {
            _.addHash(h);
        }
    }

    /// Inserts all overlapping character `k`-shingles (`k`-grams) of `text` with zero allocation.
    fn addCharShingles(*_, text: []const u8, k: usize) {
        if (k == 0 || text.len < k) {
            return;
        }
        const limit: usize = text.len - k + 1;
        let i: usize = 0;
        while (i < limit) {
            _.addBytes(text[i..(i + k)]);
            i += 1;
        }
    }

    /// Monotonically merges `other` into this sketch (`sig[i] = min(sig[i], other.sig[i])`),
    /// producing the exact MinHash signature of the set union. Returns `false` on length mismatch.
    fn mergeFrom(*_, other: MinHash): bool {
        if (_.sig.len != other.sig.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.sig.len) {
            if (other.sig[i] < _.sig[i]) {
                _.sig[i] = other.sig[i];
            }
            i += 1;
        }
        return true;
    }

    /// Estimates the Jaccard similarity `|A ∩ B| / |A ∪ B|` with `other`.
    /// Returns `null` if signature lengths differ or are empty.
    fn jaccard(other: MinHash): f64? {
        return minhashJaccard(_.sig, other.sig);
    }
}

/// Estimates Jaccard similarity between two MinHash signature slices `a` and `b`.
/// Returns `null` if either slice is empty or lengths differ.
pub fn minhashJaccard(a: []const u64, b: []const u64): f64? {
    if (a.len == 0 || a.len != b.len) {
        return null;
    }
    let matches: usize = 0;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] == b[i]) {
            matches += 1;
        }
        i += 1;
    }
    return (matches as f64) / (a.len as f64);
}

/// Computes a 64-bit Locality-Sensitive Hashing (LSH) bucket key for band `band_idx`
/// spanning `rows_per_band` consecutive signature rows.
/// Returns `null` if `rows_per_band == 0` or the requested band exceeds `sig.len`.
pub fn minhashBandHash(sig: []const u64, band_idx: usize, rows_per_band: usize): u64? {
    if (rows_per_band == 0 || sig.len == 0) {
        return null;
    }
    const max_bands: usize = sig.len / rows_per_band;
    if (band_idx >= max_bands) {
        return null;
    }

    const start: usize = band_idx * rows_per_band;
    let h: u64 = splitmix64((band_idx as u64) +% 1);
    let r: usize = 0;
    while (r < rows_per_band) {
        h = splitmix64(h ^ sig[start + r]);
        r += 1;
    }
    return h;
}

/// Estimates the set cardinality (distinct element count) from a MinHash signature
/// using the order-statistic expectation `E[min(U_1..U_n)] = 1 / (n + 1)`.
/// Returns `0.0` for empty or unpopulated signatures.
pub fn minhashCardinality(sig: []const u64): f64 {
    if (sig.len == 0) {
        return 0.0;
    }

    let sum_u: f64 = 0.0;
    let populated: usize = 0;
    let i: usize = 0;
    while (i < sig.len) {
        if (sig[i] != MINHASH_EMPTY_VAL) {
            sum_u += (sig[i] as f64) / TWO_POW_64;
            populated += 1;
        }
        i += 1;
    }

    if (populated == 0 || sum_u <= 0.0) {
        return 0.0;
    }
    const mean_u: f64 = sum_u / (populated as f64);
    const est: f64 = (1.0 / mean_u) - 1.0;
    if (est < 0.0) {
        return 0.0;
    }
    return est;
}

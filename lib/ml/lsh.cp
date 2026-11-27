// Origin: Pure-CP Stdlib (Zero-Alloc Locality-Sensitive Hashing: Cosine Sign-Random-Projection LSH & E2LSH) | Created: 2026-11-22

const PI: f64 = 3.14159265358979323846;
const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;
const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;

/// Sentinel index for an empty hash bucket or end of a collision chain.
pub const LSH_NO_ENTRY: u32 = 0xFFFFFFFF;

/// Linked-list bucket entry stored in caller-provided zero-alloc entry pool.
pub struct LshEntry {
    item_id: u32,
    signature: u64,
    next: u32,
}

struct RngF64 {
    val: f64,
    next_state: u64,
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn vecAllFinite(v: []const f64): bool {
    let i: usize = 0;
    while (i < v.len) {
        if (!isFiniteF64(v[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn splitmixMix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn nextUniform01(state: u64): RngF64 {
    const ns: u64 = state +% SPLITMIX_INC;
    const raw: u64 = splitmixMix(ns);
    return RngF64 {
        val: ((raw >> 11) as f64) * INV_2_POW_53,
        next_state: ns,
    };
}

fn nextNormal01(state: u64): RngF64 {
    const s1 = nextUniform01(state);
    const s2 = nextUniform01(s1.next_state);
    const u_a = if (s1.val > 1.0e-15) s1.val else 1.0e-15;
    const mag = @sqrt(-2.0 * @log(u_a));
    const z = mag * @cos(2.0 * PI * s2.val);
    return RngF64 {
        val: z,
        next_state: s2.next_state,
    };
}

fn popcount64(x: u64): usize {
    let v: u64 = x;
    let cnt: usize = 0;
    while (v != 0) {
        v = v & (v - 1);
        cnt += 1;
    }
    return cnt;
}

fn floorToI64(x: f64): i64 {
    if (x >= 9.0e18) {
        return 9000000000000000000;
    }
    if (x <= -9.0e18) {
        return -9000000000000000000;
    }
    const fl = @floor(x);
    return fl as i64;
}

fn cosineFromHammingImpl(sig_a: u64, sig_b: u64, num_bits: usize): f64 {
    if (num_bits == 0) {
        return 0.0;
    }
    const eff_bits: usize = if (num_bits > 64) 64 else num_bits;
    let diff: u64 = sig_a ^ sig_b;
    if (eff_bits < 64) {
        const mask: u64 = ((1 as u64) << (eff_bits as u6)) - 1;
        diff = diff & mask;
    }
    const hamming = popcount64(diff);
    return @cos(PI * (hamming as f64) / (eff_bits as f64));
}

/// Estimates cosine similarity from the Hamming distance of two `num_bits`-bit random-hyperplane signatures:
/// `cos(pi * hamming(sig_a, sig_b) / num_bits)`.
pub fn estimatedCosineFromHamming(sig_a: u64, sig_b: u64, num_bits: usize): f64 {
    return cosineFromHammingImpl(sig_a, sig_b, num_bits);
}

fn containsU32(ids: []const u32, count: usize, target: u32): bool {
    let i: usize = 0;
    while (i < count) {
        if (ids[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Random Hyperplane (Sign-Random-Projection) Cosine LSH index (zero-alloc).
pub struct CosineLsh {
    dim: usize,
    num_tables: usize,
    num_bits: usize,
    num_buckets: usize,
    entry_count: usize,
    planes: []f64,
    heads: []u32,
    entries: []LshEntry,

    /// Initializes a zero-alloc Cosine LSH index using caller-supplied buffers.
    /// Required buffer lengths:
    /// - `planes_buf >= num_tables * num_bits * dim`
    /// - `heads_buf >= num_tables * num_buckets`
    /// - `entries_buf >= 1` (each `insert` consumes `num_tables` entries)
    static fn init(
        dim: usize,
        num_tables: usize,
        num_bits: usize,
        num_buckets: usize,
        seed: u64,
        planes_buf: *[]f64,
        heads_buf: *[]u32,
        entries_buf: *[]LshEntry,
    ): CosineLsh? {
        if (dim == 0 || num_tables == 0 || num_bits == 0 || num_bits > 64 || num_buckets == 0) {
            return null;
        }
        if ((planes_buf.*.len / dim) / num_tables < num_bits) {
            return null;
        }
        if (heads_buf.*.len / num_tables < num_buckets) {
            return null;
        }
        if (entries_buf.*.len == 0) {
            return null;
        }

        const total_weights = num_tables * num_bits * dim;
        let rng_state: u64 = seed;
        let i: usize = 0;
        while (i < total_weights) {
            const g = nextNormal01(rng_state);
            rng_state = g.next_state;
            planes_buf.*[i] = g.val;
            i += 1;
        }

        const total_heads = num_tables * num_buckets;
        i = 0;
        while (i < total_heads) {
            heads_buf.*[i] = LSH_NO_ENTRY;
            i += 1;
        }

        return CosineLsh {
            dim: dim,
            num_tables: num_tables,
            num_bits: num_bits,
            num_buckets: num_buckets,
            entry_count: 0,
            planes: planes_buf.*,
            heads: heads_buf.*,
            entries: entries_buf.*,
        };
    }

    /// Computes the `num_bits`-bit random-hyperplane signature for `vec` in table `table_idx`.
    fn hashVector(vec: []const f64, table_idx: usize): u64? {
        if (table_idx >= _.num_tables || vec.len != _.dim || !vecAllFinite(vec)) {
            return null;
        }
        let sig: u64 = 0;
        const table_offset = table_idx * _.num_bits * _.dim;
        let b: usize = 0;
        while (b < _.num_bits) {
            const w_offset = table_offset + b * _.dim;
            let dot: f64 = 0.0;
            let j: usize = 0;
            while (j < _.dim) {
                dot += _.planes[w_offset + j] * vec[j];
                j += 1;
            }
            if (dot >= 0.0) {
                sig = sig | ((1 as u64) << (b as u6));
            }
            b += 1;
        }
        return sig;
    }

    /// Inserts `item_id` with vector `vec` across all `num_tables` hash tables.
    /// Returns `false` if `vec` is invalid or the entry pool lacks space for `num_tables` entries.
    fn insert(*_, item_id: u32, vec: []const f64): bool {
        if (vec.len != _.dim || !vecAllFinite(vec)) {
            return false;
        }
        if (_.entries.len - _.entry_count < _.num_tables) {
            return false;
        }

        let t: usize = 0;
        while (t < _.num_tables) {
            const sig_opt = _.hashVector(vec, t);
            if (sig_opt == null) {
                return false;
            }
            const sig = sig_opt ?? 0;
            const bucket = (splitmixMix(sig) % (_.num_buckets as u64)) as usize;
            const head_idx = t * _.num_buckets + bucket;
            const slot = _.entry_count;
            _.entries[slot] = LshEntry {
                item_id: item_id,
                signature: sig,
                next: _.heads[head_idx],
            };
            _.heads[head_idx] = slot as u32;
            _.entry_count += 1;
            t += 1;
        }
        return true;
    }

    /// Retrieves deduplicated candidate `item_id`s across all hash tables into `out_ids`.
    /// Uses `seen_bitset` (automatically cleared per query) for O(1) deduplication.
    fn queryCandidates(vec: []const f64, out_ids: *[]u32, seen_bitset: *[]u8): usize? {
        if (vec.len != _.dim || !vecAllFinite(vec)) {
            return null;
        }
        let z: usize = 0;
        while (z < seen_bitset.*.len) {
            seen_bitset.*[z] = 0;
            z += 1;
        }

        let count: usize = 0;
        let t: usize = 0;
        while (t < _.num_tables) {
            const sig_opt = _.hashVector(vec, t);
            if (sig_opt == null) {
                return null;
            }
            const sig = sig_opt ?? 0;
            const bucket = (splitmixMix(sig) % (_.num_buckets as u64)) as usize;
            let cur = _.heads[t * _.num_buckets + bucket];
            let guard: usize = 0;
            while (cur != LSH_NO_ENTRY && guard < _.entry_count) {
                const entry = _.entries[cur as usize];
                if (entry.signature == sig) {
                    const id_u = entry.item_id as usize;
                    const byte_idx = id_u >> 3;
                    const bit_mask: u8 = (1 as u8) << ((id_u & 7) as u3);
                    let is_dup: bool = false;
                    if (byte_idx < seen_bitset.*.len) {
                        if ((seen_bitset.*[byte_idx] & bit_mask) != 0) {
                            is_dup = true;
                        } else {
                            seen_bitset.*[byte_idx] = seen_bitset.*[byte_idx] | bit_mask;
                        }
                    } else {
                        is_dup = containsU32(out_ids.*[0..count], count, entry.item_id);
                    }
                    if (!is_dup && count < out_ids.*.len) {
                        out_ids.*[count] = entry.item_id;
                        count += 1;
                    }
                }
                cur = entry.next;
                guard += 1;
            }
            t += 1;
        }
        return count;
    }

    /// Static convenience forwarder to `estimatedCosineFromHamming`.
    static fn estimatedCosineFromHamming(sig_a: u64, sig_b: u64, num_bits: usize): f64 {
        return cosineFromHammingImpl(sig_a, sig_b, num_bits);
    }
}

/// p-Stable Distribution Euclidean LSH (E2LSH, Datar-Immorlica-Indyk-Mirrokni 2004, zero-alloc).
pub struct EuclideanLsh {
    dim: usize,
    num_tables: usize,
    num_proj: usize,
    bucket_width: f64,
    num_buckets: usize,
    entry_count: usize,
    proj_a: []f64,
    proj_b: []f64,
    heads: []u32,
    entries: []LshEntry,

    /// Initializes a zero-alloc E2LSH index using caller-supplied buffers.
    /// Required buffer lengths:
    /// - `proj_a_buf >= num_tables * num_proj * dim`
    /// - `proj_b_buf >= num_tables * num_proj`
    /// - `heads_buf >= num_tables * num_buckets`
    /// - `entries_buf >= 1` (each `insert` consumes `num_tables` entries)
    static fn init(
        dim: usize,
        num_tables: usize,
        num_proj: usize,
        bucket_width: f64,
        num_buckets: usize,
        seed: u64,
        proj_a_buf: *[]f64,
        proj_b_buf: *[]f64,
        heads_buf: *[]u32,
        entries_buf: *[]LshEntry,
    ): EuclideanLsh? {
        if (
            dim == 0 ||
            num_tables == 0 ||
            num_proj == 0 ||
            num_buckets == 0 ||
            !isFiniteF64(bucket_width) ||
            bucket_width <= 0.0
        ) {
            return null;
        }
        if ((proj_a_buf.*.len / dim) / num_tables < num_proj) {
            return null;
        }
        if (proj_b_buf.*.len / num_tables < num_proj) {
            return null;
        }
        if (heads_buf.*.len / num_tables < num_buckets) {
            return null;
        }
        if (entries_buf.*.len == 0) {
            return null;
        }

        let rng_state: u64 = seed;
        const total_a = num_tables * num_proj * dim;
        let i: usize = 0;
        while (i < total_a) {
            const g = nextNormal01(rng_state);
            rng_state = g.next_state;
            proj_a_buf.*[i] = g.val;
            i += 1;
        }

        const total_b = num_tables * num_proj;
        i = 0;
        while (i < total_b) {
            const u = nextUniform01(rng_state);
            rng_state = u.next_state;
            proj_b_buf.*[i] = u.val * bucket_width;
            i += 1;
        }

        const total_heads = num_tables * num_buckets;
        i = 0;
        while (i < total_heads) {
            heads_buf.*[i] = LSH_NO_ENTRY;
            i += 1;
        }

        return EuclideanLsh {
            dim: dim,
            num_tables: num_tables,
            num_proj: num_proj,
            bucket_width: bucket_width,
            num_buckets: num_buckets,
            entry_count: 0,
            proj_a: proj_a_buf.*,
            proj_b: proj_b_buf.*,
            heads: heads_buf.*,
            entries: entries_buf.*,
        };
    }

    /// Computes the combined 64-bit E2LSH bucket signature for `vec` in table `table_idx`.
    fn hashVector(vec: []const f64, table_idx: usize): u64? {
        if (table_idx >= _.num_tables || vec.len != _.dim || !vecAllFinite(vec)) {
            return null;
        }
        let h: u64 = 0xCBF29CE484222325;
        const table_a_off = table_idx * _.num_proj * _.dim;
        const table_b_off = table_idx * _.num_proj;
        let p: usize = 0;
        while (p < _.num_proj) {
            const a_off = table_a_off + p * _.dim;
            let dot: f64 = 0.0;
            let j: usize = 0;
            while (j < _.dim) {
                dot += _.proj_a[a_off + j] * vec[j];
                j += 1;
            }
            const scaled = (dot + _.proj_b[table_b_off + p]) / _.bucket_width;
            if (!isFiniteF64(scaled)) {
                return null;
            }
            const q_i64 = floorToI64(scaled);
            const q_u64: u64 = @bitCast(q_i64);
            h = splitmixMix(h ^ q_u64 ^ ((p as u64) *% SPLITMIX_INC));
            p += 1;
        }
        return h;
    }

    /// Inserts `item_id` with vector `vec` across all `num_tables` E2LSH tables.
    /// Returns `false` if `vec` is invalid or the entry pool lacks space for `num_tables` entries.
    fn insert(*_, item_id: u32, vec: []const f64): bool {
        if (vec.len != _.dim || !vecAllFinite(vec)) {
            return false;
        }
        if (_.entries.len - _.entry_count < _.num_tables) {
            return false;
        }

        let t: usize = 0;
        while (t < _.num_tables) {
            const sig_opt = _.hashVector(vec, t);
            if (sig_opt == null) {
                return false;
            }
            const sig = sig_opt ?? 0;
            const bucket = (splitmixMix(sig) % (_.num_buckets as u64)) as usize;
            const head_idx = t * _.num_buckets + bucket;
            const slot = _.entry_count;
            _.entries[slot] = LshEntry {
                item_id: item_id,
                signature: sig,
                next: _.heads[head_idx],
            };
            _.heads[head_idx] = slot as u32;
            _.entry_count += 1;
            t += 1;
        }
        return true;
    }

    /// Retrieves deduplicated candidate `item_id`s across all E2LSH tables into `out_ids`.
    fn queryCandidates(vec: []const f64, out_ids: *[]u32, seen_bitset: *[]u8): usize? {
        if (vec.len != _.dim || !vecAllFinite(vec)) {
            return null;
        }
        let z: usize = 0;
        while (z < seen_bitset.*.len) {
            seen_bitset.*[z] = 0;
            z += 1;
        }

        let count: usize = 0;
        let t: usize = 0;
        while (t < _.num_tables) {
            const sig_opt = _.hashVector(vec, t);
            if (sig_opt == null) {
                return null;
            }
            const sig = sig_opt ?? 0;
            const bucket = (splitmixMix(sig) % (_.num_buckets as u64)) as usize;
            let cur = _.heads[t * _.num_buckets + bucket];
            let guard: usize = 0;
            while (cur != LSH_NO_ENTRY && guard < _.entry_count) {
                const entry = _.entries[cur as usize];
                if (entry.signature == sig) {
                    const id_u = entry.item_id as usize;
                    const byte_idx = id_u >> 3;
                    const bit_mask: u8 = (1 as u8) << ((id_u & 7) as u3);
                    let is_dup: bool = false;
                    if (byte_idx < seen_bitset.*.len) {
                        if ((seen_bitset.*[byte_idx] & bit_mask) != 0) {
                            is_dup = true;
                        } else {
                            seen_bitset.*[byte_idx] = seen_bitset.*[byte_idx] | bit_mask;
                        }
                    } else {
                        is_dup = containsU32(out_ids.*[0..count], count, entry.item_id);
                    }
                    if (!is_dup && count < out_ids.*.len) {
                        out_ids.*[count] = entry.item_id;
                        count += 1;
                    }
                }
                cur = entry.next;
                guard += 1;
            }
            t += 1;
        }
        return count;
    }
}

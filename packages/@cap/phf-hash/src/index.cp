// @cap/phf-hash — Zero-allocation CHD Displace-Hash-Compress perfect hash table
// Inspired by Rust `phf_generator` / `phf_shared`
// MIT License

const MASK32: u64 = 0xFFFFFFFF;
const MAX_KEYS: usize = 256;

const GOLDEN_RATIO: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
const FNV_OFFSET: u64 = ((0xCBF29CE4 as u64) << 32) | (0x84222325 as u64);
const FNV_PRIME: u64 = ((0x00000100 as u64) << 32) | (0x000001B3 as u64);
const MIX_K1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
const MIX_K2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
const SEED_XOR: u64 = ((0x517CC1B7 as u64) << 32) | (0x27220A95 as u64);

fn add64(a: u64, b: u64): u64 {
    let lo: u64 = (a & MASK32) + (b & MASK32);
    let hi: u64 = (a >> 32) + (b >> 32) + (lo >> 32);
    return (lo & MASK32) | ((hi & MASK32) << 32);
}

fn mul64(a: u64, b: u64): u64 {
    let a_lo: u64 = a & MASK32;
    let a_hi: u64 = a >> 32;
    let b_lo: u64 = b & MASK32;
    let b_hi: u64 = b >> 32;
    let p0: u64 = a_lo * b_lo;
    let mid: u64 = (p0 >> 32) + ((a_lo * b_hi) & MASK32) + ((a_hi * b_lo) & MASK32);
    return (p0 & MASK32) | ((mid & MASK32) << 32);
}

fn mix64(z_in: u64): u64 {
    let z: u64 = mul64(z_in ^ (z_in >> 30), MIX_K1);
    z = mul64(z ^ (z >> 27), MIX_K2);
    return z ^ (z >> 31);
}

fn strEq(a: string, b: string): bool {
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

pub struct PhfHashes {
    pub g: u32,
    pub f1: u32,
    pub f2: u32,
}

pub struct PhfBuildResult {
    pub ok: bool,
    pub seed: u64,
    pub table_len: usize,
    pub disp_len: usize,
}

pub fn hashKey(key: string, seed: u64): PhfHashes {
    let h1: u64 = add64(FNV_OFFSET, mix64(seed));
    let h2: u64 = add64(GOLDEN_RATIO, mix64(seed ^ SEED_XOR));
    let i: usize = 0;
    while (i < key.len) {
        let b: u64 = key[i] as u64;
        h1 = mul64(h1 ^ b, FNV_PRIME);
        h2 = mul64(add64(h2, b + 1), SEED_XOR);
        h2 = h2 ^ (h2 >> 29);
        i = i + 1;
    }
    h1 = mix64(add64(h1, key.len as u64));
    h2 = mix64(h2 ^ ((key.len as u64) << 17));
    return PhfHashes {
        g: ((h1 >> 32) ^ (h1 & MASK32)) as u32,
        f1: (h2 & MASK32) as u32,
        f2: (h2 >> 32) as u32,
    };
}

pub fn displace(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize {
    if (len == 0) {
        return 0;
    }
    let m: u64 = len as u64;
    let p: u64 = ((f1 as u64) % m) * ((d1 as u64) % m);
    let s: u64 = ((d2 as u64) % m) + (p % m) + ((f2 as u64) % m);
    return (s % m) as usize;
}

pub fn buildPhfWithSeed(
    keys: []const string,
    start_seed: u64,
    disps_d1: []u32,
    disps_d2: []u32,
    slot_to_key: []i32
): PhfBuildResult {
    let n: usize = keys.len;
    let table_len: usize = slot_to_key.len;
    let disp_len: usize = disps_d1.len;
    if (disps_d2.len < disp_len) {
        disp_len = disps_d2.len;
    }

    if (n == 0) {
        let s: usize = 0;
        while (s < table_len) {
            slot_to_key[s] = -1;
            s = s + 1;
        }
        return PhfBuildResult {
            ok: true,
            seed: start_seed,
            table_len: table_len,
            disp_len: disp_len,
        };
    }

    if (n > MAX_KEYS || disp_len == 0 || disp_len > MAX_KEYS || table_len < n) {
        return PhfBuildResult {
            ok: false,
            seed: 0,
            table_len: table_len,
            disp_len: disp_len,
        };
    }

    let bucket_head: [256]i32 = [-1; 256];
    let bucket_size: [256]u32 = [0; 256];
    let bucket_order: [256]usize = [0; 256];
    let key_next: [256]i32 = [-1; 256];
    let key_f1: [256]u32 = [0; 256];
    let key_f2: [256]u32 = [0; 256];
    let trial_slots: [256]usize = [0; 256];

    let attempt: u64 = 0;
    while (attempt < 64) {
        let cur_seed: u64 = add64(start_seed, mul64(attempt, GOLDEN_RATIO));

        // Reset structures
        let b_init: usize = 0;
        while (b_init < disp_len) {
            bucket_head[b_init] = -1;
            bucket_size[b_init] = 0;
            bucket_order[b_init] = b_init;
            disps_d1[b_init] = 0;
            disps_d2[b_init] = 0;
            b_init = b_init + 1;
        }
        let s_init: usize = 0;
        while (s_init < table_len) {
            slot_to_key[s_init] = -1;
            s_init = s_init + 1;
        }

        // Hash keys into buckets
        let ki: usize = 0;
        while (ki < n) {
            let h: PhfHashes = hashKey(keys[ki], cur_seed);
            key_f1[ki] = h.f1;
            key_f2[ki] = h.f2;
            let b: usize = (h.g as usize) % disp_len;
            key_next[ki] = bucket_head[b];
            bucket_head[b] = ki as i32;
            bucket_size[b] = bucket_size[b] + 1;
            ki = ki + 1;
        }

        // Sort buckets descending by size (largest buckets placed first)
        let bi: usize = 1;
        while (bi < disp_len) {
            let cur_b: usize = bucket_order[bi];
            let cur_sz: u32 = bucket_size[cur_b];
            let bj: usize = bi;
            while (bj > 0 && bucket_size[bucket_order[bj - 1]] < cur_sz) {
                bucket_order[bj] = bucket_order[bj - 1];
                bj = bj - 1;
            }
            bucket_order[bj] = cur_b;
            bi = bi + 1;
        }

        // Place each non-empty bucket
        let all_placed: bool = true;
        let oi: usize = 0;
        while (oi < disp_len) {
            let b: usize = bucket_order[oi];
            if (bucket_size[b] == 0) {
                break;
            }

            let found_disp: bool = false;
            let d1: u32 = 0;
            let max_d1: u32 = table_len as u32;
            if (bucket_size[b] == 1) {
                max_d1 = 1;
            }

            while (d1 < max_d1 && !found_disp) {
                let d2: u32 = 0;
                while (d2 < (table_len as u32) && !found_disp) {
                    let count: usize = 0;
                    let collision: bool = false;
                    let cur_k: i32 = bucket_head[b];
                    while (cur_k >= 0) {
                        let ku: usize = cur_k as usize;
                        let slot: usize = displace(key_f1[ku], key_f2[ku], d1, d2, table_len);
                        if (slot_to_key[slot] >= 0) {
                            collision = true;
                            break;
                        }
                        // Check collision within the same bucket
                        let prev: usize = 0;
                        while (prev < count) {
                            if (trial_slots[prev] == slot) {
                                collision = true;
                                break;
                            }
                            prev = prev + 1;
                        }
                        if (collision) {
                            break;
                        }
                        trial_slots[count] = slot;
                        count = count + 1;
                        cur_k = key_next[ku];
                    }

                    if (!collision) {
                        // Commit bucket placements
                        disps_d1[b] = d1;
                        disps_d2[b] = d2;
                        let commit_k: i32 = bucket_head[b];
                        let ci: usize = 0;
                        while (commit_k >= 0) {
                            slot_to_key[trial_slots[ci]] = commit_k;
                            ci = ci + 1;
                            commit_k = key_next[commit_k as usize];
                        }
                        found_disp = true;
                    }
                    d2 = d2 + 1;
                }
                d1 = d1 + 1;
            }

            if (!found_disp) {
                all_placed = false;
                break;
            }
            oi = oi + 1;
        }

        if (all_placed) {
            return PhfBuildResult {
                ok: true,
                seed: cur_seed,
                table_len: table_len,
                disp_len: disp_len,
            };
        }
        attempt = attempt + 1;
    }

    return PhfBuildResult {
        ok: false,
        seed: 0,
        table_len: table_len,
        disp_len: disp_len,
    };
}

pub fn buildPhf(
    keys: []const string,
    disps_d1: []u32,
    disps_d2: []u32,
    slot_to_key: []i32
): PhfBuildResult {
    return buildPhfWithSeed(keys, 0, disps_d1, disps_d2, slot_to_key);
}

pub fn lookupIndex(
    key: string,
    seed: u64,
    keys: []const string,
    disps_d1: []const u32,
    disps_d2: []const u32,
    slot_to_key: []const i32
): i64 {
    let disp_len: usize = disps_d1.len;
    if (disps_d2.len < disp_len) {
        disp_len = disps_d2.len;
    }
    if (keys.len == 0 || disp_len == 0 || slot_to_key.len == 0) {
        return -1;
    }
    let h: PhfHashes = hashKey(key, seed);
    let b: usize = (h.g as usize) % disp_len;
    let slot: usize = displace(h.f1, h.f2, disps_d1[b], disps_d2[b], slot_to_key.len);
    let k_idx: i32 = slot_to_key[slot];
    if (k_idx < 0 || (k_idx as usize) >= keys.len) {
        return -1;
    }
    if (strEq(key, keys[k_idx as usize])) {
        return k_idx as i64;
    }
    return -1;
}

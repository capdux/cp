// Source: cp/lib/reed_solomon/rs.cp (Pure CP GF(2^8) Cauchy Reed-Solomon Erasure Coding)
const std = @import("std");

pub const RsError = error{
    InvalidShardCount,
    ShardCountMismatch,
    EmptyShard,
    ShardSizeMismatch,
    TooFewShards,
    SingularMatrix,
    OutOfMemory,
};

// GF(2^8) primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1 (0x11d)
const GF_POLY: u16 = 0x11d;

pub fn gfAdd(a: u8, b: u8): u8 {
    return a ^ b;
}

pub fn gfMul(a_in: u8, b_in: u8): u8 {
    let a: u16 = a_in;
    let b: u16 = b_in;
    let res: u16 = 0;
    let i: u8 = 0;
    while (i < 8) : (i += 1) {
        if ((b & 1) != 0) {
            res ^= a;
        }
        const hi = (a & 0x80) != 0;
        a <<= 1;
        if (hi) {
            a ^= GF_POLY;
        }
        b >>= 1;
    }
    return @truncate(res & 0xFF);
}

pub fn gfPow(base_in: u8, exp_in: u8): u8 {
    let res: u8 = 1;
    let base = base_in;
    let exp = exp_in;
    while (exp > 0) {
        if ((exp & 1) != 0) {
            res = gfMul(res, base);
        }
        base = gfMul(base, base);
        exp >>= 1;
    }
    return res;
}

pub fn gfInv(a: u8): u8 {
    if (a == 0) { return 0; }
    // a^(2^8 - 2) = a^254 in GF(2^8)
    return gfPow(a, 254);
}

pub fn gfDiv(a: u8, b: u8): u8 {
    if (a == 0) { return 0; }
    return gfMul(a, gfInv(b));
}

pub const ReedSolomon = struct {
    alloc: std.mem.Allocator,
    data_shards: usize,
    parity_shards: usize,
    total_shards: usize,
    // parity_rows: parity_shards * data_shards systematic Cauchy matrix
    parity_rows: []u8,

    pub fn init(alloc: std.mem.Allocator, data_shards: usize, parity_shards: usize): RsError!ReedSolomon {
        if (data_shards == 0 or parity_shards == 0 or data_shards + parity_shards > 256) {
            return RsError.InvalidShardCount;
        }
        const total = data_shards + parity_shards;
        let parity_rows = try alloc.alloc(u8, parity_shards * data_shards);
        errdefer alloc.free(parity_rows);

        // Build systematic Cauchy matrix:
        // Choose x_i = data_shards + i (for i in 0..parity_shards), y_j = j (for j in 0..data_shards).
        // All x_i != y_j since x_i >= data_shards > y_j, so 1 / (x_i ^ y_j) is always valid and any k x k submatrix is invertible.
        let r: usize = 0;
        while (r < parity_shards) : (r += 1) {
            const xi: u8 = @intCast(data_shards + r);
            let c: usize = 0;
            while (c < data_shards) : (c += 1) {
                const yj: u8 = @intCast(c);
                parity_rows[r * data_shards + c] = gfInv(xi ^ yj);
            }
        }

        return ReedSolomon{
            .alloc = alloc,
            .data_shards = data_shards,
            .parity_shards = parity_shards,
            .total_shards = total,
            .parity_rows = parity_rows,
        };
    }

    pub fn deinit() {
        _.alloc.free(_.parity_rows);
    }

    fn validateShards(shards: []const []u8): RsError!usize {
        if (shards.len != _.total_shards) { return RsError.ShardCountMismatch; }
        const shard_len = shards[0].len;
        if (shard_len == 0) { return RsError.EmptyShard; }
        for (s of shards) {
            if (s.len != shard_len) { return RsError.ShardSizeMismatch; }
        }
        return shard_len;
    }

    pub fn encode(shards: []const []u8): RsError!void {
        const shard_len = try _.validateShards(shards);
        let p: usize = 0;
        while (p < _.parity_shards) : (p += 1) {
            let dst = shards[_.data_shards + p];
            @memset(dst, 0);
            let d: usize = 0;
            while (d < _.data_shards) : (d += 1) {
                const coeff = _.parity_rows[p * _.data_shards + d];
                if (coeff == 0) { continue; }
                const src = shards[d];
                let byte_i: usize = 0;
                while (byte_i < shard_len) : (byte_i += 1) {
                    dst[byte_i] ^= gfMul(coeff, src[byte_i]);
                }
            }
        }
    }

    pub fn verify(alloc: std.mem.Allocator, shards: []const []u8): RsError!bool {
        const shard_len = try _.validateShards(shards);
        let tmp = try alloc.alloc(u8, shard_len);
        defer alloc.free(tmp);

        let p: usize = 0;
        while (p < _.parity_shards) : (p += 1) {
            @memset(tmp, 0);
            let d: usize = 0;
            while (d < _.data_shards) : (d += 1) {
                const coeff = _.parity_rows[p * _.data_shards + d];
                const src = shards[d];
                let byte_i: usize = 0;
                while (byte_i < shard_len) : (byte_i += 1) {
                    tmp[byte_i] ^= gfMul(coeff, src[byte_i]);
                }
            }
            if (!std.mem.eql(u8, tmp, shards[_.data_shards + p])) {
                return false;
            }
        }
        return true;
    }

    fn rowForShard(shard_idx: usize, col: usize): u8 {
        if (shard_idx < _.data_shards) {
            return if (shard_idx == col) 1 else 0;
        }
        const p = shard_idx - _.data_shards;
        return _.parity_rows[p * _.data_shards + col];
    }

    pub fn reconstruct(alloc: std.mem.Allocator, shards: []const []u8, present: []const bool): RsError!void {
        if (shards.len != _.total_shards or present.len != _.total_shards) {
            return RsError.ShardCountMismatch;
        }
        const shard_len = try _.validateShards(shards);

        // Count present shards and check if any data shard is missing
        let present_count: usize = 0;
        let data_missing = false;
        let parity_missing = false;
        let i: usize = 0;
        while (i < _.total_shards) : (i += 1) {
            if (present[i]) {
                present_count += 1;
            } else if (i < _.data_shards) {
                data_missing = true;
            } else {
                parity_missing = true;
            }
        }
        if (!data_missing and !parity_missing) { return; }
        if (present_count < _.data_shards) { return RsError.TooFewShards; }

        if (data_missing) {
            const k = _.data_shards;
            // Pick first k present shards to form k x k invertible submatrix
            let chosen = try alloc.alloc(usize, k);
            defer alloc.free(chosen);
            let ci: usize = 0;
            i = 0;
            while (i < _.total_shards and ci < k) : (i += 1) {
                if (present[i]) {
                    chosen[ci] = i;
                    ci += 1;
                }
            }

            // Augmented matrix [A | I] of size k x 2k
            let mat = try alloc.alloc(u8, k * 2 * k);
            defer alloc.free(mat);
            @memset(mat, 0);

            let r: usize = 0;
            while (r < k) : (r += 1) {
                const s_idx = chosen[r];
                let c: usize = 0;
                while (c < k) : (c += 1) {
                    mat[r * (2 * k) + c] = _.rowForShard(s_idx, c);
                }
                mat[r * (2 * k) + k + r] = 1;
            }

            // Gauss-Jordan elimination over GF(2^8)
            let col: usize = 0;
            while (col < k) : (col += 1) {
                let pivot = col;
                while (pivot < k and mat[pivot * (2 * k) + col] == 0) : (pivot += 1) {}
                if (pivot == k) { return RsError.SingularMatrix; }
                if (pivot != col) {
                    let j: usize = 0;
                    while (j < 2 * k) : (j += 1) {
                        const tmp = mat[col * (2 * k) + j];
                        mat[col * (2 * k) + j] = mat[pivot * (2 * k) + j];
                        mat[pivot * (2 * k) + j] = tmp;
                    }
                }
                const inv_p = gfInv(mat[col * (2 * k) + col]);
                let j: usize = 0;
                while (j < 2 * k) : (j += 1) {
                    mat[col * (2 * k) + j] = gfMul(mat[col * (2 * k) + j], inv_p);
                }
                r = 0;
                while (r < k) : (r += 1) {
                    if (r == col) { continue; }
                    const factor = mat[r * (2 * k) + col];
                    if (factor == 0) { continue; }
                    j = 0;
                    while (j < 2 * k) : (j += 1) {
                        mat[r * (2 * k) + j] ^= gfMul(factor, mat[col * (2 * k) + j]);
                    }
                }
            }

            // Reconstruct missing data shards using A^{-1}
            r = 0;
            while (r < k) : (r += 1) {
                if (present[r]) { continue; }
                let dst = shards[r];
                @memset(dst, 0);
                let c: usize = 0;
                while (c < k) : (c += 1) {
                    const coeff = mat[r * (2 * k) + k + c];
                    if (coeff == 0) { continue; }
                    const src = shards[chosen[c]];
                    let byte_i: usize = 0;
                    while (byte_i < shard_len) : (byte_i += 1) {
                        dst[byte_i] ^= gfMul(coeff, src[byte_i]);
                    }
                }
            }
        }

        // Recompute any missing parity shards from now-complete data shards
        if (parity_missing) {
            let p: usize = 0;
            while (p < _.parity_shards) : (p += 1) {
                if (present[_.data_shards + p]) { continue; }
                let dst = shards[_.data_shards + p];
                @memset(dst, 0);
                let d: usize = 0;
                while (d < _.data_shards) : (d += 1) {
                    const coeff = _.parity_rows[p * _.data_shards + d];
                    const src = shards[d];
                    let byte_i: usize = 0;
                    while (byte_i < shard_len) : (byte_i += 1) {
                        dst[byte_i] ^= gfMul(coeff, src[byte_i]);
                    }
                }
            }
        }
    }
};

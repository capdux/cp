// @cap/reed-solomon — Zero-allocation GF(2^8) Cauchy Reed-Solomon erasure coding for CP
// Clean-room MIT implementation inspired by Rust `reed-solomon-erasure`.

pub const MAX_DATA: usize = 16;
pub const MAX_PARITY: usize = 8;
pub const MAX_TOTAL: usize = 24;

// Primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1 (0x11D -> low byte 0x1D)
const GF_POLY_LOW: u8 = 0x1D;

// ---------------------------------------------------------------------------
// 1. GF(2^8) Finite Field Arithmetic (Primitive Polynomial 0x11D)
// ---------------------------------------------------------------------------

/// Multiplies two elements in GF(2^8) modulo `0x11D` (constant-iteration Russian peasant algorithm).
pub fn gfMul(a: u8, b: u8) -> u8 {
    let p: u8 = 0;
    let aa = a;
    let bb = b;
    let bit: usize = 0;
    while (bit < 8) {
        if ((bb & 1) != 0) {
            p ^= aa;
        }
        const carry = (aa & 0x80) != 0;
        aa <<= 1;
        if (carry) {
            aa ^= GF_POLY_LOW;
        }
        bb >>= 1;
        bit += 1;
    }
    return p;
}

/// Raises `a` to the `n`-th power in GF(2^8).
pub fn gfPow(a: u8, n: u32) -> u8 {
    let res: u8 = 1;
    let base = a;
    let exp = n;
    while (exp > 0) {
        if ((exp & 1) != 0) {
            res = gfMul(res, base);
        }
        base = gfMul(base, base);
        exp >>= 1;
    }
    return res;
}

/// Computes the multiplicative inverse of `a` in GF(2^8) (`a^254`). Returns `0` for `a == 0`.
pub fn gfInv(a: u8) -> u8 {
    if (a == 0) {
        return 0;
    }
    return gfPow(a, 254 as u32);
}

// ---------------------------------------------------------------------------
// 2. Cauchy Systematic Reed-Solomon Codec (`MAX_DATA = 16`, `MAX_PARITY = 8`)
// ---------------------------------------------------------------------------

pub struct RsCodec {
    pub data_count: usize,
    pub parity_count: usize,
    /// Flattened `MAX_PARITY x MAX_DATA` (8 x 16 = 128) Cauchy parity generator matrix.
    pub parity_matrix: [128]u8,

    /// Initializes a systematic Cauchy Reed-Solomon codec with `data_shards` (`1..=16`)
    /// and `parity_shards` (`1..=8`). Values outside bounds are safely clamped.
    pub static fn init(data_shards: usize, parity_shards: usize) -> RsCodec {
        let k: usize = data_shards;
        if (k < 1) k = 1 as usize;
        if (k > MAX_DATA) k = MAX_DATA;

        let m: usize = parity_shards;
        if (m < 1) m = 1 as usize;
        if (m > MAX_PARITY) m = MAX_PARITY;

        let mat: [128]u8 = undefined;
        let idx: usize = 0;
        while (idx < 128) {
            mat[idx] = 0;
            idx += 1;
        }

        // Cauchy matrix: M[r][c] = 1 / ((k + r) ^ c) in GF(2^8) — every square submatrix is invertible!
        let r: usize = 0;
        while (r < m) {
            const xr = (k + r) as u8;
            let c: usize = 0;
            while (c < k) {
                const yc = c as u8;
                mat[r * MAX_DATA + c] = gfInv(xr ^ yc);
                c += 1;
            }
            r += 1;
        }

        return RsCodec{
            data_count: k,
            parity_count: m,
            parity_matrix: mat,
        };
    }

    pub fn dataShards() -> usize {
        return _.data_count;
    }

    pub fn parityShards() -> usize {
        return _.parity_count;
    }

    pub fn totalShards() -> usize {
        return _.data_count + _.parity_count;
    }

    /// Encodes `data_shards` bytes into `parity_shards` parity bytes written into `parity_out`.
    pub fn encodeByte(data: []const u8, parity_out: []u8) -> bool {
        const k = _.data_count;
        const m = _.parity_count;
        if (data.len < k || parity_out.len < m) {
            return false;
        }
        let r: usize = 0;
        while (r < m) {
            let acc: u8 = 0;
            let c: usize = 0;
            while (c < k) {
                acc ^= gfMul(_.parity_matrix[r * MAX_DATA + c], data[c]);
                c += 1;
            }
            parity_out[r] = acc;
            r += 1;
        }
        return true;
    }

    /// Verifies that `parity[0..parity_shards]` matches `data[0..data_shards]`.
    pub fn verifyByte(data: []const u8, parity: []const u8) -> bool {
        const k = _.data_count;
        const m = _.parity_count;
        if (data.len < k || parity.len < m) {
            return false;
        }
        let r: usize = 0;
        while (r < m) {
            let acc: u8 = 0;
            let c: usize = 0;
            while (c < k) {
                acc ^= gfMul(_.parity_matrix[r * MAX_DATA + c], data[c]);
                c += 1;
            }
            if (acc != parity[r]) {
                return false;
            }
            r += 1;
        }
        return true;
    }

    /// Reconstructs all missing data and parity bytes in `shards[0..totalShards()]` given
    /// the availability mask `present[0..totalShards()]`.
    /// Succeeds whenever at least `dataShards()` shards are present; returns `false` otherwise.
    pub fn reconstructByte(shards: []u8, present: []const bool) -> bool {
        const k = _.data_count;
        const m = _.parity_count;
        const total = k + m;
        if (shards.len < total || present.len < total) {
            return false;
        }

        // Collect the first `k` surviving shard indices
        let surv_rows: [16]usize = undefined;
        let surv_count: usize = 0;
        let all_present = true;
        let idx: usize = 0;
        while (idx < total) {
            if (present[idx]) {
                if (surv_count < k) {
                    surv_rows[surv_count] = idx;
                    surv_count += 1;
                }
            } else {
                all_present = false;
            }
            idx += 1;
        }

        if (all_present) {
            return true;
        }
        if (surv_count < k) {
            return false;
        }

        // Build k x k system matrix `mat` and RHS vector `rhs`
        let mat: [256]u8 = undefined;
        let rhs: [16]u8 = undefined;
        let r: usize = 0;
        while (r < k) {
            const orig_row = surv_rows[r];
            let c: usize = 0;
            while (c < k) {
                if (orig_row < k) {
                    mat[r * MAX_DATA + c] = if (c == orig_row) 1 as u8 else 0 as u8;
                } else {
                    mat[r * MAX_DATA + c] = _.parity_matrix[(orig_row - k) * MAX_DATA + c];
                }
                c += 1;
            }
            rhs[r] = shards[orig_row];
            r += 1;
        }

        // Gauss-Jordan elimination over GF(2^8)
        let col: usize = 0;
        while (col < k) {
            // Find pivot row
            let piv = col;
            while (piv < k && mat[piv * MAX_DATA + col] == 0) {
                piv += 1;
            }
            if (piv >= k) {
                return false;
            }
            if (piv != col) {
                let c: usize = 0;
                while (c < k) {
                    const tmp = mat[col * MAX_DATA + c];
                    mat[col * MAX_DATA + c] = mat[piv * MAX_DATA + c];
                    mat[piv * MAX_DATA + c] = tmp;
                    c += 1;
                }
                const tr = rhs[col];
                rhs[col] = rhs[piv];
                rhs[piv] = tr;
            }

            // Normalize pivot row
            const inv_piv = gfInv(mat[col * MAX_DATA + col]);
            let c: usize = 0;
            while (c < k) {
                mat[col * MAX_DATA + c] = gfMul(mat[col * MAX_DATA + c], inv_piv);
                c += 1;
            }
            rhs[col] = gfMul(rhs[col], inv_piv);

            // Eliminate column from all other rows
            let row: usize = 0;
            while (row < k) {
                if (row != col) {
                    const factor = mat[row * MAX_DATA + col];
                    if (factor != 0) {
                        let cc: usize = 0;
                        while (cc < k) {
                            mat[row * MAX_DATA + cc] ^= gfMul(factor, mat[col * MAX_DATA + cc]);
                            cc += 1;
                        }
                        rhs[row] ^= gfMul(factor, rhs[col]);
                    }
                }
                row += 1;
            }
            col += 1;
        }

        // Write recovered data shards
        let d: usize = 0;
        while (d < k) {
            shards[d] = rhs[d];
            d += 1;
        }

        // Recompute any missing parity shards from recovered data
        let p: usize = 0;
        while (p < m) {
            if (!present[k + p]) {
                let acc: u8 = 0;
                let c: usize = 0;
                while (c < k) {
                    acc ^= gfMul(_.parity_matrix[p * MAX_DATA + c], shards[c]);
                    c += 1;
                }
                shards[k + p] = acc;
            }
            p += 1;
        }

        return true;
    }
}

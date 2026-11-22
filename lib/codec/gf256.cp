// Origin: Pure-CP Stdlib (Zero-Alloc GF(2^8) Finite Field, Polynomial, Lagrange & Matrix Library) | Created: 2026-11-22

/// AES / Rijndael irreducible polynomial low 8 bits: x^8 + x^4 + x^3 + x + 1 (0x11B).
pub const GF256_POLY_AES: u8 = 0x1B;

/// Reed-Solomon / QR Code / RAID-6 primitive polynomial low 8 bits: x^8 + x^4 + x^3 + x^2 + 1 (0x11D).
pub const GF256_POLY_RS: u8 = 0x1D;

/// Constant-time, table-free GF(2^8) multiplication modulo `0x100 | poly_low8`.
/// Executes 8 branchless iterations suitable for side-channel-resistant cryptography.
pub fn gf256MulConstTime(a: u8, b: u8, poly_low8: u8): u8 {
    let p: u8 = 0;
    let x: u8 = a;
    let y: u8 = b;
    let i: usize = 0;
    while (i < 8) {
        const b_mask: u8 = (0 as u8) -% (y & 1);
        p ^= x & b_mask;
        const hi_mask: u8 = (0 as u8) -% ((x >> 7) & 1);
        x = (x << 1) ^ (poly_low8 & hi_mask);
        y >>= 1;
        i += 1;
    }
    return p;
}

/// Zero-allocation GF(2^8) finite field engine backed by stack-resident 512B exp and 256B log tables.
pub struct Gf256Field {
    poly_low8: u8,
    generator: u8,
    exp_table: [512]u8,
    log_table: [256]u8,

    /// Initializes lookup tables for `GF(2^8)` defined by `0x100 | poly_low8` and primitive `generator`.
    static fn init(poly_low8: u8, generator: u8): Gf256Field {
        let field: Gf256Field = Gf256Field {
            poly_low8: poly_low8,
            generator: generator,
            exp_table: [0; 512],
            log_table: [0; 256],
        };

        let x: u8 = 1;
        let i: usize = 0;
        while (i < 255) {
            field.exp_table[i] = x;
            field.exp_table[i + 255] = x;
            field.log_table[x as usize] = i as u8;
            x = gf256MulConstTime(x, generator, poly_low8);
            i += 1;
        }
        field.exp_table[510] = field.exp_table[0];
        field.exp_table[511] = field.exp_table[1];
        field.log_table[0] = 0;
        return field;
    }

    /// Convenience factory for the AES / Rijndael field (`0x11B`, generator `0x03`).
    static fn aes(): Gf256Field {
        return Gf256Field.init(GF256_POLY_AES, 0x03);
    }

    /// Convenience factory for the Reed-Solomon / QR / RAID-6 field (`0x11D`, generator `0x02`).
    static fn rs(): Gf256Field {
        return Gf256Field.init(GF256_POLY_RS, 0x02);
    }

    /// Field addition (`a ^ b`).
    fn add(a: u8, b: u8): u8 {
        return a ^ b;
    }

    /// Field subtraction (`a ^ b`, identical to addition in characteristic 2).
    fn sub(a: u8, b: u8): u8 {
        return a ^ b;
    }

    /// Fast O(1) table-driven field multiplication.
    fn mul(a: u8, b: u8): u8 {
        if (a == 0 || b == 0) {
            return 0;
        }
        const sum: usize = (_.log_table[a as usize] as usize) + (_.log_table[b as usize] as usize);
        return _.exp_table[sum];
    }

    /// Field division `a / b`; returns `null` if `b == 0`.
    fn div(a: u8, b: u8): u8? {
        if (b == 0) {
            return null;
        }
        if (a == 0) {
            return 0;
        }
        const idx: usize = ((_.log_table[a as usize] as usize) + 255) - (_.log_table[b as usize] as usize);
        return _.exp_table[idx];
    }

    /// Multiplicative inverse `a^-1`; returns `null` if `a == 0`.
    fn inv(a: u8): u8? {
        if (a == 0) {
            return null;
        }
        const idx: usize = (255 as usize) - (_.log_table[a as usize] as usize);
        return _.exp_table[idx];
    }

    /// Field exponentiation `a^exp` (`0^0 == 1`).
    fn pow(a: u8, exp: u32): u8 {
        if (exp == 0) {
            return 1;
        }
        if (a == 0) {
            return 0;
        }
        const idx: usize = (((_.log_table[a as usize] as u64) * (exp as u64)) % 255) as usize;
        return _.exp_table[idx];
    }

    /// Fused multiply-accumulate on byte slices: `dst[i] ^= mul(src[i], coeff)`.
    /// Returns `false` if `dst.len != src.len`.
    fn mulAddSlice(dst: *[]u8, src: []const u8, coeff: u8): bool {
        if (dst.*.len != src.len) {
            return false;
        }
        if (coeff == 0) {
            return true;
        }
        let i: usize = 0;
        if (coeff == 1) {
            while (i < src.len) {
                dst.*[i] ^= src[i];
                i += 1;
            }
            return true;
        }
        while (i < src.len) {
            dst.*[i] ^= _.mul(src[i], coeff);
            i += 1;
        }
        return true;
    }

    /// Scales every byte in `dst` in-place: `dst[i] = mul(dst[i], coeff)`.
    fn scaleSlice(dst: *[]u8, coeff: u8) {
        if (coeff == 1) {
            return;
        }
        let i: usize = 0;
        if (coeff == 0) {
            while (i < dst.*.len) {
                dst.*[i] = 0;
                i += 1;
            }
            return;
        }
        while (i < dst.*.len) {
            dst.*[i] = _.mul(dst.*[i], coeff);
            i += 1;
        }
    }

    /// Evaluates polynomial `coeffs[0] + coeffs[1]*x + ... + coeffs[n-1]*x^(n-1)` (lowest degree first)
    /// at point `x` using Horner's method.
    fn polyEval(coeffs: []const u8, x: u8): u8 {
        if (coeffs.len == 0) {
            return 0;
        }
        let acc: u8 = 0;
        let i: usize = coeffs.len;
        while (i > 0) {
            i -= 1;
            acc = _.mul(acc, x) ^ coeffs[i];
        }
        return acc;
    }

    /// Multiplies two lowest-degree-first polynomials `a` and `b`, writing `a.len + b.len - 1` coefficients into `out`.
    /// Returns output degree count, `0` if either input is empty, or `null` if `out` is too small.
    fn polyMul(a: []const u8, b: []const u8, out: *[]u8): usize? {
        if (a.len == 0 || b.len == 0) {
            return 0;
        }
        const out_len: usize = a.len + b.len - 1;
        if (out.*.len < out_len) {
            return null;
        }

        let z: usize = 0;
        while (z < out_len) {
            out.*[z] = 0;
            z += 1;
        }

        let i: usize = 0;
        while (i < a.len) {
            const ai: u8 = a[i];
            if (ai != 0) {
                let j: usize = 0;
                while (j < b.len) {
                    out.*[i + j] ^= _.mul(ai, b[j]);
                    j += 1;
                }
            }
            i += 1;
        }
        return out_len;
    }

    /// Evaluates the unique Lagrange interpolating polynomial passing through `(xs[i], ys[i])` at `x_target`.
    /// Passing `x_target = 0` directly recovers the secret constant term in Shamir Secret Sharing.
    /// Returns `null` if `xs.len == 0`, `xs.len != ys.len`, or `xs` contains duplicate x-coordinates.
    fn lagrangeInterpolateAt(xs: []const u8, ys: []const u8, x_target: u8): u8? {
        if (xs.len == 0 || xs.len != ys.len) {
            return null;
        }

        // Reject duplicate x nodes.
        let i: usize = 0;
        while (i < xs.len) {
            let j: usize = i + 1;
            while (j < xs.len) {
                if (xs[i] == xs[j]) {
                    return null;
                }
                j += 1;
            }
            i += 1;
        }

        let result: u8 = 0;
        let idx: usize = 0;
        while (idx < xs.len) {
            if (xs[idx] == x_target) {
                return ys[idx];
            }
            let num: u8 = 1;
            let den: u8 = 1;
            let k: usize = 0;
            while (k < xs.len) {
                if (k != idx) {
                    num = _.mul(num, x_target ^ xs[k]);
                    den = _.mul(den, xs[idx] ^ xs[k]);
                }
                k += 1;
            }
            if (basis := _.div(num, den)) {
                result ^= _.mul(ys[idx], basis);
            } else {
                return null;
            }
            idx += 1;
        }
        return result;
    }

    /// Inverts an `n x n` row-major matrix over GF(2^8) via Gauss-Jordan elimination (zero-alloc).
    /// `work` is mutated in-place into the identity matrix; `inv_out` receives the inverse.
    /// Returns `false` if `n == 0`, buffer sizes != `n * n`, or the matrix is singular.
    fn invertMatrix(work: *[]u8, inv_out: *[]u8, n: usize): bool {
        if (n == 0) {
            return false;
        }
        const total: usize = n * n;
        if (work.*.len != total || inv_out.*.len != total) {
            return false;
        }

        // Initialize inv_out to identity matrix.
        let r0: usize = 0;
        while (r0 < n) {
            let c0: usize = 0;
            while (c0 < n) {
                if (r0 == c0) {
                    inv_out.*[r0 * n + c0] = 1;
                } else {
                    inv_out.*[r0 * n + c0] = 0;
                }
                c0 += 1;
            }
            r0 += 1;
        }

        let col: usize = 0;
        while (col < n) {
            // Find non-zero pivot row.
            let pivot: usize = col;
            while (pivot < n && work.*[pivot * n + col] == 0) {
                pivot += 1;
            }
            if (pivot == n) {
                return false; // Singular matrix.
            }

            // Swap rows if pivot != col.
            if (pivot != col) {
                let s: usize = 0;
                while (s < n) {
                    const tmp_w: u8 = work.*[col * n + s];
                    work.*[col * n + s] = work.*[pivot * n + s];
                    work.*[pivot * n + s] = tmp_w;

                    const tmp_i: u8 = inv_out.*[col * n + s];
                    inv_out.*[col * n + s] = inv_out.*[pivot * n + s];
                    inv_out.*[pivot * n + s] = tmp_i;
                    s += 1;
                }
            }

            // Normalize pivot row.
            const pv: u8 = work.*[col * n + col];
            if (pv != 1) {
                if (pv_inv := _.inv(pv)) {
                    let s: usize = 0;
                    while (s < n) {
                        work.*[col * n + s] = _.mul(work.*[col * n + s], pv_inv);
                        inv_out.*[col * n + s] = _.mul(inv_out.*[col * n + s], pv_inv);
                        s += 1;
                    }
                } else {
                    return false;
                }
            }

            // Eliminate all other rows in this column.
            let row: usize = 0;
            while (row < n) {
                if (row != col) {
                    const factor: u8 = work.*[row * n + col];
                    if (factor != 0) {
                        let s: usize = 0;
                        while (s < n) {
                            work.*[row * n + s] ^= _.mul(work.*[col * n + s], factor);
                            inv_out.*[row * n + s] ^= _.mul(inv_out.*[col * n + s], factor);
                            s += 1;
                        }
                    }
                }
                row += 1;
            }

            col += 1;
        }
        return true;
    }
}

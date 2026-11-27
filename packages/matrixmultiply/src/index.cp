// @cap/matrixmultiply — Zero-heap general dense matrix multiplication (SGEMM / DGEMM)
// Clean-room CP implementation inspired by crates.io:matrixmultiply (bluss/matrixmultiply)

const MMUL_TILE: usize = 32;

fn mmulMin(a: usize, b: usize): usize {
    if (a < b) {
        return a;
    }
    return b;
}

/// General single-precision matrix multiplication: C = alpha * A * B + beta * C
/// A is m x k with row stride rsa and col stride csa.
/// B is k x n with row stride rsb and col stride csb.
/// C is m x n with row stride rsc and col stride csc.
pub fn sgemm(
    m: usize,
    k: usize,
    n: usize,
    alpha: f32,
    a: []const f32,
    rsa: usize,
    csa: usize,
    b: []const f32,
    rsb: usize,
    csb: usize,
    beta: f32,
    c: []f32,
    rsc: usize,
    csc: usize,
) {
    if (m == 0 || n == 0) {
        return;
    }

    // Scale or zero C according to beta
    if (beta == 0.0) {
        let i: usize = 0;
        while (i < m) : (i += 1) {
            const c_row = i * rsc;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                const ci = c_row + j * csc;
                if (ci < c.len) {
                    c[ci] = 0.0;
                }
            }
        }
    } else if (beta != 1.0) {
        let i: usize = 0;
        while (i < m) : (i += 1) {
            const c_row = i * rsc;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                const ci = c_row + j * csc;
                if (ci < c.len) {
                    c[ci] = c[ci] * beta;
                }
            }
        }
    }

    if (alpha == 0.0 || k == 0) {
        return;
    }

    // Cache-blocked i-p-j accumulation supporting arbitrary strides
    let row0: usize = 0;
    while (row0 < m) : (row0 += MMUL_TILE) {
        const i_end = mmulMin(row0 + MMUL_TILE, m);
        let kk0: usize = 0;
        while (kk0 < k) : (kk0 += MMUL_TILE) {
            const p_end = mmulMin(kk0 + MMUL_TILE, k);
            let col0: usize = 0;
            while (col0 < n) : (col0 += MMUL_TILE) {
                const j_end = mmulMin(col0 + MMUL_TILE, n);
                let i: usize = row0;
                while (i < i_end) : (i += 1) {
                    const a_row = i * rsa;
                    const c_row = i * rsc;
                    let p: usize = kk0;
                    while (p < p_end) : (p += 1) {
                        const ai = a_row + p * csa;
                        if (ai >= a.len) {
                            continue;
                        }
                        const a_val = alpha * a[ai];
                        if (a_val == 0.0) {
                            continue;
                        }
                        const b_row = p * rsb;
                        let j: usize = col0;
                        while (j < j_end) : (j += 1) {
                            const bi = b_row + j * csb;
                            const ci = c_row + j * csc;
                            if (bi < b.len && ci < c.len) {
                                c[ci] = c[ci] + a_val * b[bi];
                            }
                        }
                    }
                }
            }
        }
    }
}

/// General double-precision matrix multiplication: C = alpha * A * B + beta * C
/// A is m x k with row stride rsa and col stride csa.
/// B is k x n with row stride rsb and col stride csb.
/// C is m x n with row stride rsc and col stride csc.
pub fn dgemm(
    m: usize,
    k: usize,
    n: usize,
    alpha: f64,
    a: []const f64,
    rsa: usize,
    csa: usize,
    b: []const f64,
    rsb: usize,
    csb: usize,
    beta: f64,
    c: []f64,
    rsc: usize,
    csc: usize,
) {
    if (m == 0 || n == 0) {
        return;
    }

    // Scale or zero C according to beta
    if (beta == 0.0) {
        let i: usize = 0;
        while (i < m) : (i += 1) {
            const c_row = i * rsc;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                const ci = c_row + j * csc;
                if (ci < c.len) {
                    c[ci] = 0.0;
                }
            }
        }
    } else if (beta != 1.0) {
        let i: usize = 0;
        while (i < m) : (i += 1) {
            const c_row = i * rsc;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                const ci = c_row + j * csc;
                if (ci < c.len) {
                    c[ci] = c[ci] * beta;
                }
            }
        }
    }

    if (alpha == 0.0 || k == 0) {
        return;
    }

    // Cache-blocked i-p-j accumulation supporting arbitrary strides
    let row0: usize = 0;
    while (row0 < m) : (row0 += MMUL_TILE) {
        const i_end = mmulMin(row0 + MMUL_TILE, m);
        let kk0: usize = 0;
        while (kk0 < k) : (kk0 += MMUL_TILE) {
            const p_end = mmulMin(kk0 + MMUL_TILE, k);
            let col0: usize = 0;
            while (col0 < n) : (col0 += MMUL_TILE) {
                const j_end = mmulMin(col0 + MMUL_TILE, n);
                let i: usize = row0;
                while (i < i_end) : (i += 1) {
                    const a_row = i * rsa;
                    const c_row = i * rsc;
                    let p: usize = kk0;
                    while (p < p_end) : (p += 1) {
                        const ai = a_row + p * csa;
                        if (ai >= a.len) {
                            continue;
                        }
                        const a_val = alpha * a[ai];
                        if (a_val == 0.0) {
                            continue;
                        }
                        const b_row = p * rsb;
                        let j: usize = col0;
                        while (j < j_end) : (j += 1) {
                            const bi = b_row + j * csb;
                            const ci = c_row + j * csc;
                            if (bi < b.len && ci < c.len) {
                                c[ci] = c[ci] + a_val * b[bi];
                            }
                        }
                    }
                }
            }
        }
    }
}

/// Row-major f64 matrix multiplication convenience wrapper: C = A * B
/// A is m x k, B is k x n, C is m x n (all contiguous row-major).
pub fn matmulF64(m: usize, k: usize, n: usize, a: []const f64, b: []const f64, c: []f64) {
    dgemm(m, k, n, 1.0, a, k, 1, b, n, 1, 0.0, c, n, 1);
}

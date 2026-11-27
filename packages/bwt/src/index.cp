// @cap/bwt — Zero-allocation Burrows-Wheeler Transform (BWT) & Move-To-Front (MTF)
// Clean-room MIT implementation in pure CP.

pub const MAX_BLOCK: usize = 512;

pub struct BwtResult {
    pub primary_index: usize,
    pub len: usize,
    pub ok: bool,
}

fn cmpRotations(src: []const u8, a: usize, b: usize) -> i32 {
    if (a == b) {
        return 0;
    }
    const n = src.len;
    let pa = a;
    let pb = b;
    let k: usize = 0;
    while (k < n) {
        const ca = src[pa];
        const cb = src[pb];
        if (ca < cb) {
            return -1;
        }
        if (ca > cb) {
            return 1;
        }
        pa += 1;
        if (pa == n) {
            pa = 0;
        }
        pb += 1;
        if (pb == n) {
            pb = 0;
        }
        k += 1;
    }
    if (a < b) {
        return -1;
    }
    return 1;
}

/// Forward Burrows-Wheeler Transform (BWT).
/// Writes the last column L into `dst[0..src.len]` and returns the primary index.
/// Rejects inputs larger than `MAX_BLOCK` (512) or when `dst.len < src.len`.
pub fn bwtForward(src: []const u8, dst: []u8) -> BwtResult {
    const n = src.len;
    if (n > MAX_BLOCK || dst.len < n) {
        return BwtResult {
            primary_index: 0,
            len: 0,
            ok: false,
        };
    }
    if (n == 0) {
        return BwtResult {
            primary_index: 0,
            len: 0,
            ok: true,
        };
    }

    let rot: [512]u16 = undefined;
    let tmp: [512]u16 = undefined;

    let i: usize = 0;
    while (i < n) {
        rot[i] = i as u16;
        i += 1;
    }

    // Bottom-up iterative merge sort (O(N log N) comparisons, zero recursion, zero heap)
    let width: usize = 1;
    while (width < n) {
        let left: usize = 0;
        while (left < n) {
            let mid = left + width;
            if (mid > n) {
                mid = n;
            }
            let right = left + width * 2;
            if (right > n) {
                right = n;
            }

            let li = left;
            let ri = mid;
            let out = left;
            while (li < mid && ri < right) {
                if (cmpRotations(src, rot[li] as usize, rot[ri] as usize) <= 0) {
                    tmp[out] = rot[li];
                    li += 1;
                } else {
                    tmp[out] = rot[ri];
                    ri += 1;
                }
                out += 1;
            }
            while (li < mid) {
                tmp[out] = rot[li];
                li += 1;
                out += 1;
            }
            while (ri < right) {
                tmp[out] = rot[ri];
                ri += 1;
                out += 1;
            }
            left += width * 2;
        }

        let copy_i: usize = 0;
        while (copy_i < n) {
            rot[copy_i] = tmp[copy_i];
            copy_i += 1;
        }
        width *= 2;
    }

    let primary: usize = 0;
    let j: usize = 0;
    while (j < n) {
        const r = rot[j] as usize;
        if (r == 0) {
            primary = j;
            dst[j] = src[n - 1];
        } else {
            dst[j] = src[r - 1];
        }
        j += 1;
    }

    return BwtResult {
        primary_index: primary,
        len: n,
        ok: true,
    };
}

/// Inverse Burrows-Wheeler Transform using O(N) LF-Mapping.
/// Restores the original block into `dst[0..src.len]`.
pub fn bwtInverse(src: []const u8, primary_index: usize, dst: []u8) -> bool {
    const n = src.len;
    if (n > MAX_BLOCK || dst.len < n) {
        return false;
    }
    if (n == 0) {
        return primary_index == 0;
    }
    if (primary_index >= n) {
        return false;
    }

    let counts: [256]usize = undefined;
    let c: usize = 0;
    while (c < 256) {
        counts[c] = 0;
        c += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const b = src[i] as usize;
        counts[b] += 1;
        i += 1;
    }

    let starts: [256]usize = undefined;
    let total: usize = 0;
    c = 0;
    while (c < 256) {
        starts[c] = total;
        total += counts[c];
        c += 1;
    }

    let lf: [512]u16 = undefined;
    i = 0;
    while (i < n) {
        const b = src[i] as usize;
        lf[i] = starts[b] as u16;
        starts[b] += 1;
        i += 1;
    }

    let idx = primary_index;
    let k = n;
    while (k > 0) {
        k -= 1;
        dst[k] = src[idx];
        idx = lf[idx] as usize;
    }

    return true;
}

/// 256-symbol Move-To-Front (MTF) encoder.
/// Maps repeated local symbols to small indices (especially 0 for runs).
pub fn mtfEncode(src: []const u8, dst: []u8) -> bool {
    const n = src.len;
    if (dst.len < n) {
        return false;
    }

    let table: [256]u8 = undefined;
    let t: usize = 0;
    while (t < 256) {
        table[t] = t as u8;
        t += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const sym = src[i];
        let pos: usize = 0;
        while (pos < 256 && table[pos] != sym) {
            pos += 1;
        }
        dst[i] = pos as u8;
        let k = pos;
        while (k > 0) {
            table[k] = table[k - 1];
            k -= 1;
        }
        table[0] = sym;
        i += 1;
    }

    return true;
}

/// 256-symbol Move-To-Front (MTF) decoder.
pub fn mtfDecode(src: []const u8, dst: []u8) -> bool {
    const n = src.len;
    if (dst.len < n) {
        return false;
    }

    let table: [256]u8 = undefined;
    let t: usize = 0;
    while (t < 256) {
        table[t] = t as u8;
        t += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const pos = src[i] as usize;
        const sym = table[pos];
        dst[i] = sym;
        let k = pos;
        while (k > 0) {
            table[k] = table[k - 1];
            k -= 1;
        }
        table[0] = sym;
        i += 1;
    }

    return true;
}

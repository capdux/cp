// Origin: Pure-CP Stdlib (Zero-Alloc BWT & MTF Compression Frontend) | Created: 2026-11-22

/// Result of a Burrows-Wheeler Transform encoding pass.
pub struct BwtResult {
    primary_index: usize,
    len: usize,
}

fn compareRotations(src: []const u8, a: u32, b: u32): i8 {
    if (a == b) {
        return 0;
    }
    const n = src.len;
    let ia = a as usize;
    let ib = b as usize;
    let k: usize = 0;
    while (k < n) {
        const ca = src[ia];
        const cb = src[ib];
        if (ca != cb) {
            if (ca < cb) {
                return -1;
            }
            return 1;
        }
        ia += 1;
        if (ia == n) {
            ia = 0;
        }
        ib += 1;
        if (ib == n) {
            ib = 0;
        }
        k += 1;
    }
    if (a < b) {
        return -1;
    }
    return 1;
}

fn siftDown(src: []const u8, arr: *[]u32, start: usize, end: usize) {
    let root = start;
    while (true) {
        const left = root * 2 + 1;
        if (left >= end) {
            break;
        }
        let swap_idx = root;
        if (compareRotations(src, arr.*[swap_idx], arr.*[left]) < 0) {
            swap_idx = left;
        }
        const right = left + 1;
        if (right < end && compareRotations(src, arr.*[swap_idx], arr.*[right]) < 0) {
            swap_idx = right;
        }
        if (swap_idx == root) {
            break;
        }
        const tmp = arr.*[root];
        arr.*[root] = arr.*[swap_idx];
        arr.*[swap_idx] = tmp;
        root = swap_idx;
    }
}

/// Zero-allocation Burrows-Wheeler Transform forward encoder.
/// Requires `dst.*.len >= src.len` and `scratch_idx.*.len >= src.len`.
/// Outputs the last column `L` into `dst.*[0..src.len]` and returns `{ primary_index, len }`.
pub fn bwtEncode(src: []const u8, dst: *[]u8, scratch_idx: *[]u32): BwtResult? {
    const n = src.len;
    if (n == 0) {
        return BwtResult {
            primary_index: 0,
            len: 0,
        };
    }
    if (dst.*.len < n || scratch_idx.*.len < n || n > 0xFFFFFFFF) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        scratch_idx.*[i] = i as u32;
        i += 1;
    }

    // In-place heapsort over rotation indices (zero heap alloc, no recursion)
    if (n > 1) {
        let start = n / 2;
        while (start > 0) {
            start -= 1;
            siftDown(src, scratch_idx, start, n);
        }
        let end = n;
        while (end > 1) {
            end -= 1;
            const tmp = scratch_idx.*[0];
            scratch_idx.*[0] = scratch_idx.*[end];
            scratch_idx.*[end] = tmp;
            siftDown(src, scratch_idx, 0, end);
        }
    }

    let primary_index: usize = 0;
    let row: usize = 0;
    while (row < n) {
        const rot = scratch_idx.*[row] as usize;
        if (rot == 0) {
            primary_index = row;
            dst.*[row] = src[n - 1];
        } else {
            dst.*[row] = src[rot - 1];
        }
        row += 1;
    }

    return BwtResult {
        primary_index: primary_index,
        len: n,
    };
}

/// Zero-allocation O(n) Burrows-Wheeler Transform inverse decoder via bucket-counting LF-Mapping.
/// Strictly validates `primary_index < src.len` (or `0` when empty) and buffer capacities.
pub fn bwtDecode(src: []const u8, primary_index: usize, dst: *[]u8, scratch_lf: *[]u32): usize? {
    const n = src.len;
    if (n == 0) {
        if (primary_index != 0) {
            return null;
        }
        return 0;
    }
    if (primary_index >= n || dst.*.len < n || scratch_lf.*.len < n || n > 0xFFFFFFFF) {
        return null;
    }

    let counts: [256]u32 = undefined;
    let c: usize = 0;
    while (c < 256) {
        counts[c] = 0;
        c += 1;
    }

    let i: usize = 0;
    while (i < n) {
        counts[src[i] as usize] += 1;
        i += 1;
    }

    let starts: [256]u32 = undefined;
    let sum: u32 = 0;
    c = 0;
    while (c < 256) {
        starts[c] = sum;
        sum += counts[c];
        c += 1;
    }

    i = 0;
    while (i < n) {
        const ch = src[i] as usize;
        scratch_lf.*[i] = starts[ch];
        starts[ch] += 1;
        i += 1;
    }

    let idx = primary_index;
    let k = n;
    while (k > 0) {
        k -= 1;
        dst.*[k] = src[idx];
        idx = scratch_lf.*[idx] as usize;
    }
    return n;
}

/// Zero-allocation Move-To-Front (MTF) encoder using a 256-byte stack symbol table.
pub fn mtfEncode(src: []const u8, dst: *[]u8): usize? {
    if (dst.*.len < src.len) {
        return null;
    }
    let table: [256]u8 = undefined;
    let t: usize = 0;
    while (t < 256) {
        table[t] = t as u8;
        t += 1;
    }

    let i: usize = 0;
    while (i < src.len) {
        const target = src[i];
        let pos: usize = 0;
        while (pos < 256 && table[pos] != target) {
            pos += 1;
        }
        dst.*[i] = pos as u8;
        let j = pos;
        while (j > 0) {
            table[j] = table[j - 1];
            j -= 1;
        }
        table[0] = target;
        i += 1;
    }
    return src.len;
}

/// Zero-allocation Move-To-Front (MTF) decoder using a 256-byte stack symbol table.
pub fn mtfDecode(src: []const u8, dst: *[]u8): usize? {
    if (dst.*.len < src.len) {
        return null;
    }
    let table: [256]u8 = undefined;
    let t: usize = 0;
    while (t < 256) {
        table[t] = t as u8;
        t += 1;
    }

    let i: usize = 0;
    while (i < src.len) {
        const pos = src[i] as usize;
        const sym = table[pos];
        dst.*[i] = sym;
        let j = pos;
        while (j > 0) {
            table[j] = table[j - 1];
            j -= 1;
        }
        table[0] = sym;
        i += 1;
    }
    return src.len;
}

/// Combined BWT + MTF compression frontend pipeline (zero heap allocation).
pub fn bwtMtfEncode(src: []const u8, dst: *[]u8, tmp_bwt: *[]u8, scratch_idx: *[]u32): BwtResult? {
    if (res := bwtEncode(src, tmp_bwt, scratch_idx)) {
        if (_ := mtfEncode(tmp_bwt.*[0..res.len], dst)) {
            return res;
        }
    }
    return null;
}

/// Combined MTF + BWT inverse decoding pipeline (zero heap allocation).
pub fn bwtMtfDecode(src: []const u8, primary_index: usize, dst: *[]u8, tmp_bwt: *[]u8, scratch_lf: *[]u32): usize? {
    if (n := mtfDecode(src, tmp_bwt)) {
        return bwtDecode(tmp_bwt.*[0..n], primary_index, dst, scratch_lf);
    }
    return null;
}

/// Returns the ratio of zero bytes in `data` in permille (`0..1000`).
pub fn zeroByteRatioPermille(data: []const u8): u32 {
    if (data.len == 0) {
        return 0;
    }
    let zeros: u64 = 0;
    let i: usize = 0;
    while (i < data.len) {
        if (data[i] == 0) {
            zeros += 1;
        }
        i += 1;
    }
    return ((zeros * 1000) / (data.len as u64)) as u32;
}

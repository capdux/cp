// Origin: Pure-CP Stdlib (Zero-Alloc O(1) Sparse Table RMQ, Range GCD, Euler-Tour LCA & O(n) Monotonic Deque Sliding Window) | Created: 2026-11-22

/// Computes `floor(log2(n))` for `n >= 1` (returns `0` for `n == 0` or `n == 1`).
pub fn floorLog2(n: usize): usize {
    let lg: usize = 0;
    let v = n;
    while (v > 1) {
        v = v >> 1;
        lg += 1;
    }
    return lg;
}

/// Computes the exact number of flat table cells required by a Sparse Table for `n` elements:
/// `n * (floorLog2(n) + 1)` (returns `0` when `n == 0`).
pub fn sparseTableCells(n: usize): usize {
    if (n == 0) {
        return 0;
    }
    return n * (floorLog2(n) + 1);
}

/// Computes the greatest common divisor of two `u64` integers via iterative Euclidean algorithm.
/// Note: `gcdU64(0, 0) == 0`, `gcdU64(a, 0) == a`, `gcdU64(0, b) == b`.
pub fn gcdU64(a: u64, b: u64): u64 {
    let x = a;
    let y = b;
    while (y != 0) {
        const t = x % y;
        x = y;
        y = t;
    }
    return x;
}

/// O(1) Range Minimum Query (RMQ) Sparse Table over `i64` values.
///
/// Stores rows in row-major order `table[level * n + i]` inside caller-provided scratch memory.
pub struct RmqMin {
    n: usize;
    levels: usize;
    table: []i64;

    /// Builds an `RmqMin` Sparse Table in O(n log n) time and O(1) extra stack space.
    /// Returns `null` if `values.len == 0` or `table_buf.len < sparseTableCells(values.len)`.
    static fn build(values: []const i64, table_buf: []i64): RmqMin? {
        const n = values.len;
        if (n == 0) {
            return null;
        }
        const levels = floorLog2(n) + 1;
        const needed = n * levels;
        if (table_buf.len < needed) {
            return null;
        }

        let i: usize = 0;
        while (i < n) {
            table_buf[i] = values[i];
            i += 1;
        }

        let k: usize = 1;
        while (k < levels) {
            const half: usize = (1 as usize) << (k - 1);
            const span: usize = (1 as usize) << k;
            const prev_row = (k - 1) * n;
            const cur_row = k * n;
            const limit = n + 1 - span;
            let j: usize = 0;
            while (j < limit) {
                const a = table_buf[prev_row + j];
                const b = table_buf[prev_row + j + half];
                table_buf[cur_row + j] = if (a <= b) a else b;
                j += 1;
            }
            k += 1;
        }

        return RmqMin {
            n: n,
            levels: levels,
            table: table_buf[0..needed],
        };
    }

    /// Queries the minimum value on the closed interval `[l, r]` in O(1) time.
    /// Returns `null` if the table is empty or `l > r` or `r >= n`.
    fn query(l: usize, r: usize): i64? {
        if (_.n == 0 || l > r || r >= _.n) {
            return null;
        }
        const len = r - l + 1;
        const k = floorLog2(len);
        const span: usize = (1 as usize) << k;
        const row = k * _.n;
        const a = _.table[row + l];
        const b = _.table[row + (r + 1 - span)];
        return if (a <= b) a else b;
    }
}

/// O(1) Range Maximum Query (RMQ) Sparse Table over `i64` values.
///
/// Stores rows in row-major order `table[level * n + i]` inside caller-provided scratch memory.
pub struct RmqMax {
    n: usize;
    levels: usize;
    table: []i64;

    /// Builds an `RmqMax` Sparse Table in O(n log n) time.
    /// Returns `null` if `values.len == 0` or `table_buf.len < sparseTableCells(values.len)`.
    static fn build(values: []const i64, table_buf: []i64): RmqMax? {
        const n = values.len;
        if (n == 0) {
            return null;
        }
        const levels = floorLog2(n) + 1;
        const needed = n * levels;
        if (table_buf.len < needed) {
            return null;
        }

        let i: usize = 0;
        while (i < n) {
            table_buf[i] = values[i];
            i += 1;
        }

        let k: usize = 1;
        while (k < levels) {
            const half: usize = (1 as usize) << (k - 1);
            const span: usize = (1 as usize) << k;
            const prev_row = (k - 1) * n;
            const cur_row = k * n;
            const limit = n + 1 - span;
            let j: usize = 0;
            while (j < limit) {
                const a = table_buf[prev_row + j];
                const b = table_buf[prev_row + j + half];
                table_buf[cur_row + j] = if (a >= b) a else b;
                j += 1;
            }
            k += 1;
        }

        return RmqMax {
            n: n,
            levels: levels,
            table: table_buf[0..needed],
        };
    }

    /// Queries the maximum value on the closed interval `[l, r]` in O(1) time.
    /// Returns `null` if the table is empty or `l > r` or `r >= n`.
    fn query(l: usize, r: usize): i64? {
        if (_.n == 0 || l > r || r >= _.n) {
            return null;
        }
        const len = r - l + 1;
        const k = floorLog2(len);
        const span: usize = (1 as usize) << k;
        const row = k * _.n;
        const a = _.table[row + l];
        const b = _.table[row + (r + 1 - span)];
        return if (a >= b) a else b;
    }
}

/// O(1) Range ArgMin Query Sparse Table storing indices into `values`.
///
/// Guarantees returning the **first (smallest) index** achieving the minimum on `[l, r]`.
pub struct RmqArgMin {
    n: usize;
    levels: usize;
    table: []usize;

    /// Builds an `RmqArgMin` Sparse Table in O(n log n) time.
    /// Returns `null` if `values.len == 0` or `idx_buf.len < sparseTableCells(values.len)`.
    static fn build(values: []const i64, idx_buf: []usize): RmqArgMin? {
        const n = values.len;
        if (n == 0) {
            return null;
        }
        const levels = floorLog2(n) + 1;
        const needed = n * levels;
        if (idx_buf.len < needed) {
            return null;
        }

        let i: usize = 0;
        while (i < n) {
            idx_buf[i] = i;
            i += 1;
        }

        let k: usize = 1;
        while (k < levels) {
            const half: usize = (1 as usize) << (k - 1);
            const span: usize = (1 as usize) << k;
            const prev_row = (k - 1) * n;
            const cur_row = k * n;
            const limit = n + 1 - span;
            let j: usize = 0;
            while (j < limit) {
                const ia = idx_buf[prev_row + j];
                const ib = idx_buf[prev_row + j + half];
                idx_buf[cur_row + j] = if (values[ia] <= values[ib]) ia else ib;
                j += 1;
            }
            k += 1;
        }

        return RmqArgMin {
            n: n,
            levels: levels,
            table: idx_buf[0..needed],
        };
    }

    /// Queries the first (smallest) index achieving the minimum value on `[l, r]` in O(1) time.
    /// Returns `null` if the table is empty, `values.len < n`, `l > r`, or `r >= n`.
    fn queryIndex(values: []const i64, l: usize, r: usize): usize? {
        if (_.n == 0 || values.len < _.n || l > r || r >= _.n) {
            return null;
        }
        const len = r - l + 1;
        const k = floorLog2(len);
        const span: usize = (1 as usize) << k;
        const row = k * _.n;
        const ia = _.table[row + l];
        const ib = _.table[row + (r + 1 - span)];
        const va = values[ia];
        const vb = values[ib];
        if (va < vb) {
            return ia;
        }
        if (vb < va) {
            return ib;
        }
        return if (ia <= ib) ia else ib;
    }
}

/// O(1) Range Greatest Common Divisor (GCD) Sparse Table over `u64` values.
pub struct RangeGcd {
    n: usize;
    levels: usize;
    table: []u64;

    /// Builds a `RangeGcd` Sparse Table in O(n log n log M) time.
    /// Returns `null` if `values.len == 0` or `table_buf.len < sparseTableCells(values.len)`.
    static fn build(values: []const u64, table_buf: []u64): RangeGcd? {
        const n = values.len;
        if (n == 0) {
            return null;
        }
        const levels = floorLog2(n) + 1;
        const needed = n * levels;
        if (table_buf.len < needed) {
            return null;
        }

        let i: usize = 0;
        while (i < n) {
            table_buf[i] = values[i];
            i += 1;
        }

        let k: usize = 1;
        while (k < levels) {
            const half: usize = (1 as usize) << (k - 1);
            const span: usize = (1 as usize) << k;
            const prev_row = (k - 1) * n;
            const cur_row = k * n;
            const limit = n + 1 - span;
            let j: usize = 0;
            while (j < limit) {
                const a = table_buf[prev_row + j];
                const b = table_buf[prev_row + j + half];
                table_buf[cur_row + j] = gcdU64(a, b);
                j += 1;
            }
            k += 1;
        }

        return RangeGcd {
            n: n,
            levels: levels,
            table: table_buf[0..needed],
        };
    }

    /// Queries the GCD of all elements on the closed interval `[l, r]` in O(1) time.
    /// Returns `null` if the table is empty or `l > r` or `r >= n`.
    fn query(l: usize, r: usize): u64? {
        if (_.n == 0 || l > r || r >= _.n) {
            return null;
        }
        const len = r - l + 1;
        const k = floorLog2(len);
        const span: usize = (1 as usize) << k;
        const row = k * _.n;
        const a = _.table[row + l];
        const b = _.table[row + (r + 1 - span)];
        return gcdU64(a, b);
    }
}

/// O(1) Lowest Common Ancestor (LCA) query over a tree's Euler tour using `RmqArgMin`.
///
/// Zero-allocation: borrows `euler_nodes`, `euler_depth`, `first_occ`, and the `RmqArgMin` scratch table.
pub struct EulerLca {
    num_nodes: usize;
    euler_nodes: []const usize;
    euler_depth: []const i64;
    first_occ: []const usize;
    rmq: RmqArgMin;

    /// Builds an O(1) `EulerLca` structure from a precomputed Euler tour.
    /// Validates `num_nodes > 0`, `euler_nodes.len == euler_depth.len > 0`, `first_occ.len >= num_nodes`,
    /// every `first_occ[u] < euler_nodes.len`, and `idx_buf.len >= sparseTableCells(euler_depth.len)`.
    static fn build(
        num_nodes: usize,
        euler_nodes: []const usize,
        euler_depth: []const i64,
        first_occ: []const usize,
        idx_buf: []usize,
    ): EulerLca? {
        const m = euler_nodes.len;
        if (num_nodes == 0 || m == 0 || euler_depth.len != m || first_occ.len < num_nodes) {
            return null;
        }
        let u: usize = 0;
        while (u < num_nodes) {
            if (first_occ[u] >= m) {
                return null;
            }
            u += 1;
        }
        if (rmq := RmqArgMin.build(euler_depth, idx_buf)) {
            return EulerLca {
                num_nodes: num_nodes,
                euler_nodes: euler_nodes,
                euler_depth: euler_depth,
                first_occ: first_occ,
                rmq: rmq,
            };
        }
        return null;
    }

    /// Queries the Lowest Common Ancestor of tree vertices `u` and `v` in O(1) time.
    /// Returns `null` if `u >= num_nodes` or `v >= num_nodes`.
    fn query(u: usize, v: usize): usize? {
        if (u >= _.num_nodes || v >= _.num_nodes) {
            return null;
        }
        const a = _.first_occ[u];
        const b = _.first_occ[v];
        const l = if (a <= b) a else b;
        const r = if (a <= b) b else a;
        if (idx := _.rmq.queryIndex(_.euler_depth, l, r)) {
            return _.euler_nodes[idx];
        }
        return null;
    }
}

/// Computes all `n - k + 1` sliding window minimums of width `k` in O(n) time via a monotonic deque.
///
/// Writes outputs into `out[0..(n - k + 1)]` using `deque_scratch[0..n]` as working space.
/// Returns the number of output windows (`n - k + 1`), or `null` if `values.len == 0`, `k == 0`,
/// `k > values.len`, `out.len < n - k + 1`, or `deque_scratch.len < n`.
pub fn slidingWindowMin(
    values: []const i64,
    k: usize,
    out: []i64,
    deque_scratch: []usize,
): usize? {
    const n = values.len;
    if (n == 0 || k == 0 || k > n) {
        return null;
    }
    const out_len = n - k + 1;
    if (out.len < out_len || deque_scratch.len < n) {
        return null;
    }

    let head: usize = 0;
    let tail: usize = 0;
    let i: usize = 0;
    while (i < n) {
        if (head < tail && deque_scratch[head] + k <= i) {
            head += 1;
        }
        while (head < tail && values[deque_scratch[tail - 1]] >= values[i]) {
            tail -= 1;
        }
        deque_scratch[tail] = i;
        tail += 1;

        if (i + 1 >= k) {
            out[i + 1 - k] = values[deque_scratch[head]];
        }
        i += 1;
    }

    return out_len;
}

/// Computes all `n - k + 1` sliding window maximums of width `k` in O(n) time via a monotonic deque.
///
/// Writes outputs into `out[0..(n - k + 1)]` using `deque_scratch[0..n]` as working space.
/// Returns the number of output windows (`n - k + 1`), or `null` if `values.len == 0`, `k == 0`,
/// `k > values.len`, `out.len < n - k + 1`, or `deque_scratch.len < n`.
pub fn slidingWindowMax(
    values: []const i64,
    k: usize,
    out: []i64,
    deque_scratch: []usize,
): usize? {
    const n = values.len;
    if (n == 0 || k == 0 || k > n) {
        return null;
    }
    const out_len = n - k + 1;
    if (out.len < out_len || deque_scratch.len < n) {
        return null;
    }

    let head: usize = 0;
    let tail: usize = 0;
    let i: usize = 0;
    while (i < n) {
        if (head < tail && deque_scratch[head] + k <= i) {
            head += 1;
        }
        while (head < tail && values[deque_scratch[tail - 1]] <= values[i]) {
            tail -= 1;
        }
        deque_scratch[tail] = i;
        tail += 1;

        if (i + 1 >= k) {
            out[i + 1 - k] = values[deque_scratch[head]];
        }
        i += 1;
    }

    return out_len;
}

// @cap/natsort — Zero-allocation natural / alphanumeric comparison & in-place sorting for CP
// Clean-room MIT implementation inspired by npm `natural-compare` / `natsort` and Rust `natord` / `alphanumeric-sort`.
//
// Highlights:
// - Arbitrary-length numeric runs compared without integer parsing or overflow (30+ digits supported).
// - Strict total ordering: compares normalized (natural) order first, then raw bytes as deterministic tie-breaker.
// - Zero heap allocation: pure CP in-place sorting (insertion sort for small slices, heapsort for larger slices).

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn compareRawBytes(a: string, b: string) -> i8 {
    const min_len = if (a.len < b.len) { a.len } else { b.len };
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn naturalCompareImpl(a: string, b: string, ignore_case: bool) -> i8 {
    let ia: usize = 0;
    let ib: usize = 0;

    while (ia < a.len && ib < b.len) {
        const ca = a[ia];
        const cb = b[ib];

        if (isDigit(ca) && isDigit(cb)) {
            // Find full numeric runs [ia, end_a) and [ib, end_b)
            let end_a = ia;
            while (end_a < a.len && isDigit(a[end_a])) {
                end_a += 1;
            }
            let end_b = ib;
            while (end_b < b.len && isDigit(b[end_b])) {
                end_b += 1;
            }

            // Skip leading zeros in each run
            let nz_a = ia;
            while (nz_a < end_a && a[nz_a] == '0') {
                nz_a += 1;
            }
            let nz_b = ib;
            while (nz_b < end_b && b[nz_b] == '0') {
                nz_b += 1;
            }

            // Compare significant digit counts first (no overflow for arbitrary-length integers)
            const sig_a = end_a - nz_a;
            const sig_b = end_b - nz_b;
            if (sig_a < sig_b) {
                return -1;
            }
            if (sig_a > sig_b) {
                return 1;
            }

            // Same significant length: compare digit by digit
            let k: usize = 0;
            while (k < sig_a) {
                const da = a[nz_a + k];
                const db = b[nz_b + k];
                if (da < db) {
                    return -1;
                }
                if (da > db) {
                    return 1;
                }
                k += 1;
            }

            ia = end_a;
            ib = end_b;
        } else {
            const na = if (ignore_case) { toLowerAscii(ca) } else { ca };
            const nb = if (ignore_case) { toLowerAscii(cb) } else { cb };
            if (na < nb) {
                return -1;
            }
            if (na > nb) {
                return 1;
            }
            ia += 1;
            ib += 1;
        }
    }

    if (ia < a.len) {
        return 1;
    }
    if (ib < b.len) {
        return -1;
    }

    // Normalized natural representations are identical; deterministic raw-byte tie-breaker
    return compareRawBytes(a, b);
}

/// Natural / alphanumeric comparison (case-sensitive).
/// Returns -1 if `a < b`, 0 if `a == b`, 1 if `a > b`.
/// Supports arbitrary-length numeric segments without overflow and deterministic leading-zero tie-breaking.
pub fn naturalCompare(a: string, b: string) -> i8 {
    return naturalCompareImpl(a, b, false);
}

/// ASCII case-insensitive natural comparison.
/// Falls back to raw byte order as a deterministic tie-breaker when case-folded forms are equal.
pub fn naturalCompareIgnoreCase(a: string, b: string) -> i8 {
    return naturalCompareImpl(a, b, true);
}

/// Returns `true` if `a` comes strictly before `b` in natural order.
pub fn naturalLess(a: string, b: string) -> bool {
    return naturalCompareImpl(a, b, false) < 0;
}

/// Returns `true` if `a` comes strictly before `b` in ASCII case-insensitive natural order.
pub fn naturalLessIgnoreCase(a: string, b: string) -> bool {
    return naturalCompareImpl(a, b, true) < 0;
}

/// Returns `true` if `a` and `b` are equal under natural comparison (`naturalCompare(a, b) == 0`).
pub fn naturalEq(a: string, b: string) -> bool {
    return naturalCompareImpl(a, b, false) == 0;
}

fn comesBefore(a: string, b: string, desc: bool, ignore_case: bool) -> bool {
    const c = naturalCompareImpl(a, b, ignore_case);
    if (desc) {
        return c > 0;
    }
    return c < 0;
}

fn siftDown(items: []string, start: usize, end: usize, desc: bool, ignore_case: bool) {
    let root = start;
    while (true) {
        const left = root * 2 + 1;
        if (left >= end) {
            break;
        }
        let best = root;
        if (comesBefore(items[best], items[left], desc, ignore_case)) {
            best = left;
        }
        const right = left + 1;
        if (right < end && comesBefore(items[best], items[right], desc, ignore_case)) {
            best = right;
        }
        if (best == root) {
            break;
        }
        const tmp = items[root];
        items[root] = items[best];
        items[best] = tmp;
        root = best;
    }
}

fn sortInPlace(items: []string, desc: bool, ignore_case: bool) {
    const n = items.len;
    if (n <= 1) {
        return;
    }

    // Insertion sort for small slices (fast, cache-friendly, zero allocations)
    if (n <= 16) {
        let i: usize = 1;
        while (i < n) {
            const key = items[i];
            let j = i;
            while (j > 0 && comesBefore(key, items[j - 1], desc, ignore_case)) {
                items[j] = items[j - 1];
                j -= 1;
            }
            items[j] = key;
            i += 1;
        }
        return;
    }

    // Heapsort for larger slices: guaranteed O(n log n) worst-case, O(1) auxiliary space
    let start = n / 2;
    while (start > 0) {
        start -= 1;
        siftDown(items, start, n, desc, ignore_case);
    }

    let end = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDown(items, 0, end, desc, ignore_case);
    }
}

/// In-place ascending natural sort (zero heap allocation).
pub fn sortNatural(items: []string) {
    sortInPlace(items, false, false);
}

/// In-place descending natural sort (zero heap allocation).
pub fn sortNaturalDesc(items: []string) {
    sortInPlace(items, true, false);
}

/// In-place ascending ASCII case-insensitive natural sort (zero heap allocation).
pub fn sortNaturalIgnoreCase(items: []string) {
    sortInPlace(items, false, true);
}

/// Returns `true` if `items` is already sorted in ascending natural order.
pub fn isSortedNatural(items: []string) -> bool {
    if (items.len <= 1) {
        return true;
    }
    let i: usize = 0;
    while (i + 1 < items.len) {
        if (naturalCompareImpl(items[i], items[i + 1], false) > 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

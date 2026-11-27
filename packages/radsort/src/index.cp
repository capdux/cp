// @cap/radix-sort — Zero-allocation O(N) 8-bit LSD radix sort for u32/i32/u64/i64/f64
// Clean-room MIT implementation inspired by Rust `radsort` and `voracious_radix_sort`.

const U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
const SIGN_BIT_64: u64 = (1 as u64) << 63;
const SIGN_BIT_32: u32 = 0x80000000;

// ---------------------------------------------------------------------------
// Order-Preserving Bit Key Mappings
// ---------------------------------------------------------------------------

fn i32ToSortable(v: i32) -> u32 {
    const bits: u32 = @bitCast(v);
    return bits ^ SIGN_BIT_32;
}

fn i64ToSortable(v: i64) -> u64 {
    const bits: u64 = @bitCast(v);
    return bits ^ SIGN_BIT_64;
}

/// IEEE-754 total-order bijection: flips all bits for negative floats, only sign bit for non-negative.
fn f64ToSortable(v: f64) -> u64 {
    const bits: u64 = @bitCast(v);
    if ((bits & SIGN_BIT_64) != 0) {
        return bits ^ U64_MAX;
    }
    return bits ^ SIGN_BIT_64;
}

// ---------------------------------------------------------------------------
// 1. sortU32 (4 passes, 256 buckets, skips uniform-byte passes)
// ---------------------------------------------------------------------------

/// Sorts `data` in ascending order using 8-bit LSD radix sort with caller-provided `scratch`.
/// Returns `false` if `scratch.len < data.len`.
pub fn sortU32(data: []u32, scratch: []u32) -> bool {
    const n = data.len;
    if (scratch.len < n) {
        return false;
    }
    if (n <= 1) {
        return true;
    }

    let counts: [256]usize = undefined;
    let in_scratch = false;
    let pass: u32 = 0;

    while (pass < 4) {
        const shift = pass * 8;
        let b: usize = 0;
        while (b < 256) {
            counts[b] = 0;
            b += 1;
        }

        let i: usize = 0;
        while (i < n) {
            const v = if (in_scratch) scratch[i] else data[i];
            const bucket = ((v >> shift) & 0xFF) as usize;
            counts[bucket] += 1;
            i += 1;
        }

        // Skip pass if all elements share the same byte at this position
        const first_v = if (in_scratch) scratch[0] else data[0];
        const first_b = ((first_v >> shift) & 0xFF) as usize;
        if (counts[first_b] == n) {
            pass += 1;
            continue;
        }

        let sum: usize = 0;
        b = 0;
        while (b < 256) {
            const c = counts[b];
            counts[b] = sum;
            sum += c;
            b += 1;
        }

        i = 0;
        if (!in_scratch) {
            while (i < n) {
                const v = data[i];
                const bucket = ((v >> shift) & 0xFF) as usize;
                scratch[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = true;
        } else {
            while (i < n) {
                const v = scratch[i];
                const bucket = ((v >> shift) & 0xFF) as usize;
                data[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = false;
        }
        pass += 1;
    }

    if (in_scratch) {
        let i: usize = 0;
        while (i < n) {
            data[i] = scratch[i];
            i += 1;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 2. sortI32 (sign-bit flipped 4-pass LSD)
// ---------------------------------------------------------------------------

/// Sorts signed `i32` slice `data` in ascending order using `scratch`.
pub fn sortI32(data: []i32, scratch: []i32) -> bool {
    const n = data.len;
    if (scratch.len < n) {
        return false;
    }
    if (n <= 1) {
        return true;
    }

    let counts: [256]usize = undefined;
    let in_scratch = false;
    let pass: u32 = 0;

    while (pass < 4) {
        const shift = pass * 8;
        let b: usize = 0;
        while (b < 256) {
            counts[b] = 0;
            b += 1;
        }

        let i: usize = 0;
        while (i < n) {
            const v = if (in_scratch) scratch[i] else data[i];
            const bucket = ((i32ToSortable(v) >> shift) & 0xFF) as usize;
            counts[bucket] += 1;
            i += 1;
        }

        const first_v = if (in_scratch) scratch[0] else data[0];
        const first_b = ((i32ToSortable(first_v) >> shift) & 0xFF) as usize;
        if (counts[first_b] == n) {
            pass += 1;
            continue;
        }

        let sum: usize = 0;
        b = 0;
        while (b < 256) {
            const c = counts[b];
            counts[b] = sum;
            sum += c;
            b += 1;
        }

        i = 0;
        if (!in_scratch) {
            while (i < n) {
                const v = data[i];
                const bucket = ((i32ToSortable(v) >> shift) & 0xFF) as usize;
                scratch[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = true;
        } else {
            while (i < n) {
                const v = scratch[i];
                const bucket = ((i32ToSortable(v) >> shift) & 0xFF) as usize;
                data[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = false;
        }
        pass += 1;
    }

    if (in_scratch) {
        let i: usize = 0;
        while (i < n) {
            data[i] = scratch[i];
            i += 1;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 3. sortU64 (8-pass LSD with uniform-byte skip)
// ---------------------------------------------------------------------------

/// Sorts unsigned `u64` slice `data` in ascending order using `scratch`.
pub fn sortU64(data: []u64, scratch: []u64) -> bool {
    const n = data.len;
    if (scratch.len < n) {
        return false;
    }
    if (n <= 1) {
        return true;
    }

    let counts: [256]usize = undefined;
    let in_scratch = false;
    let pass: u64 = 0;

    while (pass < 8) {
        const shift = pass * 8;
        let b: usize = 0;
        while (b < 256) {
            counts[b] = 0;
            b += 1;
        }

        let i: usize = 0;
        while (i < n) {
            const v = if (in_scratch) scratch[i] else data[i];
            const bucket = ((v >> shift) & 0xFF) as usize;
            counts[bucket] += 1;
            i += 1;
        }

        const first_v = if (in_scratch) scratch[0] else data[0];
        const first_b = ((first_v >> shift) & 0xFF) as usize;
        if (counts[first_b] == n) {
            pass += 1;
            continue;
        }

        let sum: usize = 0;
        b = 0;
        while (b < 256) {
            const c = counts[b];
            counts[b] = sum;
            sum += c;
            b += 1;
        }

        i = 0;
        if (!in_scratch) {
            while (i < n) {
                const v = data[i];
                const bucket = ((v >> shift) & 0xFF) as usize;
                scratch[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = true;
        } else {
            while (i < n) {
                const v = scratch[i];
                const bucket = ((v >> shift) & 0xFF) as usize;
                data[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = false;
        }
        pass += 1;
    }

    if (in_scratch) {
        let i: usize = 0;
        while (i < n) {
            data[i] = scratch[i];
            i += 1;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 4. sortI64 (sign-bit flipped 8-pass LSD)
// ---------------------------------------------------------------------------

/// Sorts signed `i64` slice `data` in ascending order using `scratch` (handles full `i64::MIN..i64::MAX`).
pub fn sortI64(data: []i64, scratch: []i64) -> bool {
    const n = data.len;
    if (scratch.len < n) {
        return false;
    }
    if (n <= 1) {
        return true;
    }

    let counts: [256]usize = undefined;
    let in_scratch = false;
    let pass: u64 = 0;

    while (pass < 8) {
        const shift = pass * 8;
        let b: usize = 0;
        while (b < 256) {
            counts[b] = 0;
            b += 1;
        }

        let i: usize = 0;
        while (i < n) {
            const v = if (in_scratch) scratch[i] else data[i];
            const bucket = ((i64ToSortable(v) >> shift) & 0xFF) as usize;
            counts[bucket] += 1;
            i += 1;
        }

        const first_v = if (in_scratch) scratch[0] else data[0];
        const first_b = ((i64ToSortable(first_v) >> shift) & 0xFF) as usize;
        if (counts[first_b] == n) {
            pass += 1;
            continue;
        }

        let sum: usize = 0;
        b = 0;
        while (b < 256) {
            const c = counts[b];
            counts[b] = sum;
            sum += c;
            b += 1;
        }

        i = 0;
        if (!in_scratch) {
            while (i < n) {
                const v = data[i];
                const bucket = ((i64ToSortable(v) >> shift) & 0xFF) as usize;
                scratch[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = true;
        } else {
            while (i < n) {
                const v = scratch[i];
                const bucket = ((i64ToSortable(v) >> shift) & 0xFF) as usize;
                data[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = false;
        }
        pass += 1;
    }

    if (in_scratch) {
        let i: usize = 0;
        while (i < n) {
            data[i] = scratch[i];
            i += 1;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 5. sortF64 (IEEE-754 order-preserving 8-pass LSD)
// ---------------------------------------------------------------------------

/// Sorts IEEE-754 `f64` slice `data` in ascending total order (negative, -0.0, +0.0, positive)
/// using `scratch`.
pub fn sortF64(data: []f64, scratch: []f64) -> bool {
    const n = data.len;
    if (scratch.len < n) {
        return false;
    }
    if (n <= 1) {
        return true;
    }

    let counts: [256]usize = undefined;
    let in_scratch = false;
    let pass: u64 = 0;

    while (pass < 8) {
        const shift = pass * 8;
        let b: usize = 0;
        while (b < 256) {
            counts[b] = 0;
            b += 1;
        }

        let i: usize = 0;
        while (i < n) {
            const v = if (in_scratch) scratch[i] else data[i];
            const bucket = ((f64ToSortable(v) >> shift) & 0xFF) as usize;
            counts[bucket] += 1;
            i += 1;
        }

        const first_v = if (in_scratch) scratch[0] else data[0];
        const first_b = ((f64ToSortable(first_v) >> shift) & 0xFF) as usize;
        if (counts[first_b] == n) {
            pass += 1;
            continue;
        }

        let sum: usize = 0;
        b = 0;
        while (b < 256) {
            const c = counts[b];
            counts[b] = sum;
            sum += c;
            b += 1;
        }

        i = 0;
        if (!in_scratch) {
            while (i < n) {
                const v = data[i];
                const bucket = ((f64ToSortable(v) >> shift) & 0xFF) as usize;
                scratch[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = true;
        } else {
            while (i < n) {
                const v = scratch[i];
                const bucket = ((f64ToSortable(v) >> shift) & 0xFF) as usize;
                data[counts[bucket]] = v;
                counts[bucket] += 1;
                i += 1;
            }
            in_scratch = false;
        }
        pass += 1;
    }

    if (in_scratch) {
        let i: usize = 0;
        while (i < n) {
            data[i] = scratch[i];
            i += 1;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 6. Sorted Predicates
// ---------------------------------------------------------------------------

pub fn isSortedI64(data: []const i64) -> bool {
    let i: usize = 1;
    while (i < data.len) {
        if (data[i] < data[i - 1]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isSortedU64(data: []const u64) -> bool {
    let i: usize = 1;
    while (i < data.len) {
        if (data[i] < data[i - 1]) {
            return false;
        }
        i += 1;
    }
    return true;
}

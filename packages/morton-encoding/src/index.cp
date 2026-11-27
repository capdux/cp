// @cap/morton — Zero-allocation 2D/3D Morton (Z-Order) & 2D Hilbert curve codec for CP
// Clean-room MIT implementation inspired by Rust `morton-encoding`.

// 2D magic-bit masks (assembled from 32-bit halves to stay overflow-safe)
const M2_16: u64 = ((0x0000FFFF as u64) << 32) | (0x0000FFFF as u64);
const M2_8: u64  = ((0x00FF00FF as u64) << 32) | (0x00FF00FF as u64);
const M2_4: u64  = ((0x0F0F0F0F as u64) << 32) | (0x0F0F0F0F as u64);
const M2_2: u64  = ((0x33333333 as u64) << 32) | (0x33333333 as u64);
const M2_1: u64  = ((0x55555555 as u64) << 32) | (0x55555555 as u64);

// 3D magic-bit masks (21-bit -> 63-bit)
const M3_32: u64 = ((0x001F0000 as u64) << 32) | (0x0000FFFF as u64);
const M3_16: u64 = ((0x001F0000 as u64) << 32) | (0xFF0000FF as u64);
const M3_8: u64  = ((0x100F00F0 as u64) << 32) | (0x0F00F00F as u64);
const M3_4: u64  = ((0x10C30C30 as u64) << 32) | (0xC30C30C3 as u64);
const M3_2: u64  = ((0x12492492 as u64) << 32) | (0x49249249 as u64);

pub struct Point2D {
    pub x: u32,
    pub y: u32,
}

pub struct Point3D {
    pub x: u32,
    pub y: u32,
    pub z: u32,
}

// ---------------------------------------------------------------------------
// 1. 2D Morton (Z-Order Curve) — 32-bit x 32-bit <-> 64-bit
// ---------------------------------------------------------------------------

fn splitBy1(v: u32) -> u64 {
    let x = v as u64;
    x = (x | (x << 16)) & M2_16;
    x = (x | (x << 8)) & M2_8;
    x = (x | (x << 4)) & M2_4;
    x = (x | (x << 2)) & M2_2;
    x = (x | (x << 1)) & M2_1;
    return x;
}

fn compactBy1(v: u64) -> u32 {
    let x = v & M2_1;
    x = (x | (x >> 1)) & M2_2;
    x = (x | (x >> 2)) & M2_4;
    x = (x | (x >> 4)) & M2_8;
    x = (x | (x >> 8)) & M2_16;
    x = (x | (x >> 16)) & 0xFFFFFFFF;
    return x as u32;
}

/// Interleaves two 32-bit coordinates `(x, y)` into a 64-bit Morton (Z-order) code.
pub fn encode2D(x: u32, y: u32) -> u64 {
    return splitBy1(x) | (splitBy1(y) << 1);
}

/// Deinterleaves a 64-bit Morton code back into `Point2D{ x, y }`.
pub fn decode2D(code: u64) -> Point2D {
    return Point2D{
        x: compactBy1(code),
        y: compactBy1(code >> 1),
    };
}

// ---------------------------------------------------------------------------
// 2. 3D Morton (Z-Order Curve) — 21-bit x 21-bit x 21-bit <-> 63-bit
// ---------------------------------------------------------------------------

fn splitBy2(v: u32) -> u64 {
    let x = (v & 0x1FFFFF) as u64;
    x = (x | (x << 32)) & M3_32;
    x = (x | (x << 16)) & M3_16;
    x = (x | (x << 8)) & M3_8;
    x = (x | (x << 4)) & M3_4;
    x = (x | (x << 2)) & M3_2;
    return x;
}

fn compactBy2(v: u64) -> u32 {
    let x = v & M3_2;
    x = (x | (x >> 2)) & M3_4;
    x = (x | (x >> 4)) & M3_8;
    x = (x | (x >> 8)) & M3_16;
    x = (x | (x >> 16)) & M3_32;
    x = (x | (x >> 32)) & 0x1FFFFF;
    return x as u32;
}

/// Interleaves three 21-bit coordinates `(x, y, z)` (masked to `0x1FFFFF`) into a 63-bit Morton code.
pub fn encode3D(x: u32, y: u32, z: u32) -> u64 {
    return splitBy2(x) | (splitBy2(y) << 1) | (splitBy2(z) << 2);
}

/// Deinterleaves a 63-bit Morton code back into `Point3D{ x, y, z }`.
pub fn decode3D(code: u64) -> Point3D {
    return Point3D{
        x: compactBy2(code),
        y: compactBy2(code >> 1),
        z: compactBy2(code >> 2),
    };
}

// ---------------------------------------------------------------------------
// 3. 2D Hilbert Space-Filling Curve (`order` in 1..=16)
// ---------------------------------------------------------------------------

fn clampOrder(order: u32) -> u32 {
    if (order < 1) return 1;
    if (order > 16) return 16;
    return order;
}

fn hilbertRot(n: u32, x_in: u32, y_in: u32, rx: u32, ry: u32) -> Point2D {
    let x = x_in;
    let y = y_in;
    if (ry == 0) {
        if (rx == 1) {
            x = (n - 1) - x;
            y = (n - 1) - y;
        }
        const tmp = x;
        x = y;
        y = tmp;
    }
    return Point2D{ x: x, y: y };
}

/// Maps 2D grid coordinates `(x, y)` on a `2^order x 2^order` grid (`order` in `1..=16`)
/// to their 1D Hilbert curve index.
pub fn hilbert2DEncode(order: u32, x_in: u32, y_in: u32) -> u64 {
    const ord = clampOrder(order);
    const n: u32 = (1 as u32) << ord;
    const mask = n - 1;
    let x = x_in & mask;
    let y = y_in & mask;
    let d: u64 = 0;
    let s = n >> 1;

    while (s > 0) {
        const rx: u32 = if ((x & s) != 0) 1 as u32 else 0 as u32;
        const ry: u32 = if ((y & s) != 0) 1 as u32 else 0 as u32;
        d += (s as u64) * (s as u64) * (((3 * rx) ^ ry) as u64);
        const pt = hilbertRot(n, x, y, rx, ry);
        x = pt.x;
        y = pt.y;
        s >>= 1;
    }
    return d;
}

/// Maps a 1D Hilbert curve index `d` back to 2D coordinates on a `2^order x 2^order` grid (`order` in `1..=16`).
pub fn hilbert2DDecode(order: u32, d_in: u64) -> Point2D {
    const ord = clampOrder(order);
    const n: u32 = (1 as u32) << ord;
    let t = d_in;
    let x: u32 = 0;
    let y: u32 = 0;
    let s: u32 = 1;

    while (s < n) {
        const rx: u32 = ((t >> 1) & 1) as u32;
        const ry: u32 = ((t ^ (rx as u64)) & 1) as u32;
        const pt = hilbertRot(s, x, y, rx, ry);
        x = pt.x + s * rx;
        y = pt.y + s * ry;
        t >>= 2;
        s <<= 1;
    }
    return Point2D{ x: x, y: y };
}

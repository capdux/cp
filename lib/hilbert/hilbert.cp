// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc 2D Hilbert Space-Filling Curve & 2D/3D Morton Z-Order Spatial Index)

/// Minimum supported 2D Hilbert curve order (2x2 grid, 2-bit index).
pub const HILBERT_MIN_ORDER: u8 = 1;

/// Maximum supported 2D Hilbert curve order (2^32 x 2^32 grid, full 64-bit index).
pub const HILBERT_MAX_ORDER: u8 = 32;

/// Maximum 21-bit coordinate value for 3D Morton Z-Order encoding (`(1 << 21) - 1`).
pub const MORTON3D_MAX_COORD: u32 = 0x1FFFFF;

/// 2D integer grid coordinate.
pub struct Point2D {
    x: u32,
    y: u32,
}

/// 3D integer grid coordinate.
pub struct Point3D {
    x: u32,
    y: u32,
    z: u32,
}

/// Returns the maximum valid 64-bit Hilbert index for a given `order` (`1..32`),
/// i.e. `2^(2 * order) - 1`. Returns `null` if `order` is out of range.
pub fn hilbert2dMaxIndex(order: u8): u64? {
    if (order < HILBERT_MIN_ORDER || order > HILBERT_MAX_ORDER) {
        return null;
    }
    if (order == 32) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    const shift: u6 = (order * 2) as u6;
    return ((1 as u64) << shift) - 1;
}

/// Encodes a 2D coordinate `(x, y)` on a `2^order x 2^order` grid into its
/// 64-bit Hilbert curve index (`0 .. 2^(2*order) - 1`).
/// Returns `null` if `order` is not in `1..32` or if `x`/`y` exceeds `2^order - 1`.
pub fn hilbert2dEncode(order: u8, x: u32, y: u32): u64? {
    if (order < HILBERT_MIN_ORDER || order > HILBERT_MAX_ORDER) {
        return null;
    }
    if (order < 32) {
        const limit: u64 = (1 as u64) << (order as u6);
        if ((x as u64) >= limit || (y as u64) >= limit) {
            return null;
        }
    }

    let px: u64 = x as u64;
    let py: u64 = y as u64;
    let d: u64 = 0;
    let s: u64 = (1 as u64) << ((order - 1) as u6);

    while (s > 0) {
        const rx: u64 = if ((px & s) != 0) 1 else 0;
        const ry: u64 = if ((py & s) != 0) 1 else 0;
        d += s * s * ((3 * rx) ^ ry);

        // Mask to sub-quadrant [0, s - 1] before rotating to avoid underflow at any order.
        const mask = s - 1;
        px &= mask;
        py &= mask;
        if (ry == 0) {
            if (rx == 1) {
                px = mask - px;
                py = mask - py;
            }
            const tmp = px;
            px = py;
            py = tmp;
        }

        s >>= 1;
    }

    return d;
}

/// Decodes a 64-bit Hilbert curve `index` back to its 2D grid coordinate `(x, y)`
/// for a given `order` (`1..32`).
/// Returns `null` if `order` is not in `1..32` or if `index` exceeds `2^(2*order) - 1`.
pub fn hilbert2dDecode(order: u8, index: u64): Point2D? {
    const max_idx = hilbert2dMaxIndex(order) ?? return null;
    if (index > max_idx) {
        return null;
    }

    let t: u64 = index;
    let px: u64 = 0;
    let py: u64 = 0;
    let s: u64 = 1;
    let step: u8 = 0;

    while (step < order) {
        const rx: u64 = (t >> 1) & 1;
        const ry: u64 = (t ^ rx) & 1;

        if (ry == 0) {
            if (rx == 1) {
                const mask = s - 1;
                px = mask - px;
                py = mask - py;
            }
            const tmp = px;
            px = py;
            py = tmp;
        }

        px += s * rx;
        py += s * ry;
        t >>= 2;
        s <<= 1;
        step += 1;
    }

    return Point2D { x: px as u32, y: py as u32 };
}

fn part1By1(v: u32): u64 {
    let x: u64 = v as u64;
    x = (x | (x << 16)) & 0x0000FFFF0000FFFF;
    x = (x | (x << 8)) & 0x00FF00FF00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F0F0F0F0F;
    x = (x | (x << 2)) & 0x3333333333333333;
    x = (x | (x << 1)) & 0x5555555555555555;
    return x;
}

fn compact1By1(v: u64): u32 {
    let x: u64 = v & 0x5555555555555555;
    x = (x | (x >> 1)) & 0x3333333333333333;
    x = (x | (x >> 2)) & 0x0F0F0F0F0F0F0F0F;
    x = (x | (x >> 4)) & 0x00FF00FF00FF00FF;
    x = (x | (x >> 8)) & 0x0000FFFF0000FFFF;
    x = (x | (x >> 16)) & 0x00000000FFFFFFFF;
    return x as u32;
}

fn part1By2(v: u32): u64 {
    let x: u64 = (v as u64) & 0x1FFFFF;
    x = (x | (x << 32)) & 0x001F00000000FFFF;
    x = (x | (x << 16)) & 0x001F0000FF0000FF;
    x = (x | (x << 8)) & 0x100F00F00F00F00F;
    x = (x | (x << 4)) & 0x10C30C30C30C30C3;
    x = (x | (x << 2)) & 0x1249249249249249;
    return x;
}

fn compact1By2(v: u64): u32 {
    let x: u64 = v & 0x1249249249249249;
    x = (x | (x >> 2)) & 0x10C30C30C30C30C3;
    x = (x | (x >> 4)) & 0x100F00F00F00F00F;
    x = (x | (x >> 8)) & 0x001F0000FF0000FF;
    x = (x | (x >> 16)) & 0x001F00000000FFFF;
    x = (x | (x >> 32)) & 0x00000000001FFFFF;
    return x as u32;
}

/// Encodes a 2D 32-bit coordinate `(x, y)` into a 64-bit Morton (Z-Order) code
/// using branchless O(1) SWAR bit interleaving (bit `2i` = `x[i]`, bit `2i+1` = `y[i]`).
pub fn morton2dEncode(x: u32, y: u32): u64 {
    return part1By1(x) | (part1By1(y) << 1);
}

/// Decodes a 64-bit Morton (Z-Order) code back into its 2D 32-bit coordinate.
pub fn morton2dDecode(z: u64): Point2D {
    return Point2D {
        x: compact1By1(z),
        y: compact1By1(z >> 1),
    };
}

/// Encodes a 3D 21-bit coordinate `(x, y, z)` (`0..0x1FFFFF`) into a 63-bit Morton (Z-Order) code
/// using branchless O(1) SWAR bit interleaving (bit `3i` = `x[i]`, `3i+1` = `y[i]`, `3i+2` = `z[i]`).
/// Returns `null` if any coordinate exceeds `MORTON3D_MAX_COORD (0x1FFFFF)`.
pub fn morton3dEncode(x: u32, y: u32, z: u32): u64? {
    if (x > MORTON3D_MAX_COORD || y > MORTON3D_MAX_COORD || z > MORTON3D_MAX_COORD) {
        return null;
    }
    return part1By2(x) | (part1By2(y) << 1) | (part1By2(z) << 2);
}

/// Decodes a 64-bit 3D Morton (Z-Order) code back into its 3D 21-bit coordinate.
pub fn morton3dDecode(code: u64): Point3D {
    return Point3D {
        x: compact1By2(code),
        y: compact1By2(code >> 1),
        z: compact1By2(code >> 2),
    };
}

/// Computes the overflow-safe L1 (Manhattan) distance between two 2D points.
pub fn manhattanDist2d(a: Point2D, b: Point2D): u64 {
    const dx: u64 = if (a.x >= b.x) (a.x - b.x) as u64 else (b.x - a.x) as u64;
    const dy: u64 = if (a.y >= b.y) (a.y - b.y) as u64 else (b.y - a.y) as u64;
    return dx + dy;
}

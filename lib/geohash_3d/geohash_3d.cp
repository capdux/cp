// Source: cp/lib/geohash_3d/geohash_3d.cp (Pure CP 3D Morton Z-Order Code & Haversine Spatial Engine)
const std = @import("std");

pub const MortonError = error{
    CoordinateOutOfRange,
};

pub const Point3D = struct {
    x: u21,
    y: u21,
    z: u21,
};

fn expandBits21(v: u21): u64 {
    let x: u64 = @intCast(v);
    x = (x | (x << 32)) & 0x001f00000000ffff;
    x = (x | (x << 16)) & 0x001f0000ff0000ff;
    x = (x | (x << 8)) & 0x100f00f00f00f00f;
    x = (x | (x << 4)) & 0x10c30c30c30c30c3;
    x = (x | (x << 2)) & 0x1249249249249249;
    return x;
}

fn compactBits21(m: u64): u21 {
    let x = m & 0x1249249249249249;
    x = (x | (x >> 2)) & 0x10c30c30c30c30c3;
    x = (x | (x >> 4)) & 0x100f00f00f00f00f;
    x = (x | (x >> 8)) & 0x001f0000ff0000ff;
    x = (x | (x >> 16)) & 0x001f00000000ffff;
    x = (x | (x >> 32)) & 0x1fffff;
    return @intCast(x);
}

/// Encode 21-bit (x, y, z) integer coordinates into a 63-bit Morton Z-order code
pub fn mortonEncode3D(x: u21, y: u21, z: u21): u64 {
    return expandBits21(x) | (expandBits21(y) << 1) | (expandBits21(z) << 2);
}

/// Decode a 63-bit Morton Z-order code back into (x, y, z)
pub fn mortonDecode3D(code: u64): Point3D {
    return Point3D{
        .x = compactBits21(code),
        .y = compactBits21(code >> 1),
        .z = compactBits21(code >> 2),
    };
}

/// Quantize normalized 3D coordinates in [0.0, 1.0] to 63-bit Morton Z-order code
pub fn quantizeEncode3D(nx: f64, ny: f64, nz: f64): MortonError!u64 {
    if (nx < 0.0 or nx > 1.0 or ny < 0.0 or ny > 1.0 or nz < 0.0 or nz > 1.0) {
        return MortonError.CoordinateOutOfRange;
    }
    const max_q: f64 = 2097151.0; // (1 << 21) - 1
    const qx: u21 = @intFromFloat(@round(nx * max_q));
    const qy: u21 = @intFromFloat(@round(ny * max_q));
    const qz: u21 = @intFromFloat(@round(nz * max_q));
    return mortonEncode3D(qx, qy, qz);
}

/// Great-circle Haversine distance in meters between two WGS84 (lat, lon) degrees
pub fn haversineMeters(lat1_deg: f64, lon1_deg: f64, lat2_deg: f64, lon2_deg: f64): f64 {
    const earth_radius_m: f64 = 6371000.0;
    const deg_to_rad: f64 = std.math.pi / 180.0;
    const lat1 = lat1_deg * deg_to_rad;
    const lat2 = lat2_deg * deg_to_rad;
    const d_lat = (lat2_deg - lat1_deg) * deg_to_rad;
    const d_lon = (lon2_deg - lon1_deg) * deg_to_rad;

    const sin_dlat = @sin(d_lat * 0.5);
    const sin_dlon = @sin(d_lon * 0.5);
    const a = sin_dlat * sin_dlat + @cos(lat1) * @cos(lat2) * sin_dlon * sin_dlon;
    const clamped = if (a > 1.0) 1.0 else a;
    const c = 2.0 * std.math.atan2(@sqrt(clamped), @sqrt(1.0 - clamped));
    return earth_radius_m * c;
}

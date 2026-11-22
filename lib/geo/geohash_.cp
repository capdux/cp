// Reference Only: Functional overlap with lib/geo/geohash.cp | Kept for comparison/analysis
// Source: cp/lib/geohash/geohash.cp (Pure CP Geohash Spatial Grid Encoding, Neighbors & Haversine Distance)
// Zero-C-dependency Base32 Geohash encoder/decoder (precision 1..12),
// 8-direction neighbor lookup with antimeridian (+-180 deg) wrap-around and pole clamping,
// 64-bit integer Z-order Morton encoding, and WGS-84 Haversine distance.

const std = @import("std");

const BASE32_ALPHABET: string = "0123456789bcdefghjkmnpqrstuvwxyz";

// Standard Niemeyer Geohash neighbor and border lookup tables indexed by parity (0 = even len, 1 = odd len).
const NEIGHBOR_N_EVEN: string = "p0r21436x8zb9dcf5h7kjnmqesgutwvy";
const NEIGHBOR_N_ODD: string = "bc01fg45238967deuvhjyznpkmstqrwx";
const NEIGHBOR_S_EVEN: string = "14365h7k9dcfesgujnmqp0r2twvyx8zb";
const NEIGHBOR_S_ODD: string = "238967debc01fg45kmstqrwxuvhjyznp";
const NEIGHBOR_E_EVEN: string = "bc01fg45238967deuvhjyznpkmstqrwx";
const NEIGHBOR_E_ODD: string = "p0r21436x8zb9dcf5h7kjnmqesgutwvy";
const NEIGHBOR_W_EVEN: string = "238967debc01fg45kmstqrwxuvhjyznp";
const NEIGHBOR_W_ODD: string = "14365h7k9dcfesgujnmqp0r2twvyx8zb";

const BORDER_N_EVEN: string = "prxz";
const BORDER_N_ODD: string = "bcfguvyz";
const BORDER_S_EVEN: string = "028b";
const BORDER_S_ODD: string = "0145hjnp";
const BORDER_E_EVEN: string = "bcfguvyz";
const BORDER_E_ODD: string = "prxz";
const BORDER_W_EVEN: string = "0145hjnp";
const BORDER_W_ODD: string = "028b";

pub const EARTH_RADIUS_METERS: f64 = 6371000.0;
pub const MAX_PRECISION: u8 = 12;

/// Error set for all `cap:geohash` spatial grid operations.
pub err GeohashError {
    InvalidCoordinate,
    InvalidPrecision,
    InvalidGeohashChar,
    EmptyGeohash,
    OutOfMemory,
}

/// Geographic coordinate in decimal degrees (latitude in [-90, 90], longitude in [-180, 180]).
pub struct LatLon {
    pub lat: f64 = 0.0,
    pub lon: f64 = 0.0,

    /// Constructs a validated `LatLon`, returning `GeohashError.InvalidCoordinate` if out of range or non-finite.
    pub fn init(lat: f64, lon: f64): LatLon ! GeohashError {
        if (!isValidCoord(lat, lon)) return GeohashError.InvalidCoordinate;
        return LatLon { lat: lat, lon: lon };
    }
}

/// Decoded Geohash bounding box with center coordinate and half-cell error margins.
pub struct BoundingBox {
    pub min_lat: f64 = 0.0,
    pub max_lat: f64 = 0.0,
    pub min_lon: f64 = 0.0,
    pub max_lon: f64 = 0.0,
    pub center_lat: f64 = 0.0,
    pub center_lon: f64 = 0.0,
    pub lat_err: f64 = 0.0,
    pub lon_err: f64 = 0.0,

    /// Returns the center coordinate of this bounding box as a `LatLon`.
    pub fn center(): LatLon {
        return LatLon { lat: _.center_lat, lon: _.center_lon };
    }

    /// Returns true if `(lat, lon)` lies within `[min_lat, max_lat] x [min_lon, max_lon]`.
    pub fn contains(lat: f64, lon: f64): bool {
        return lat >= _.min_lat && lat <= _.max_lat && lon >= _.min_lon && lon <= _.max_lon;
    }
}

/// 8-way compass direction for Geohash neighbor lookups.
pub enum Direction {
    North,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
}

enum CardinalDir {
    North,
    South,
    East,
    West,
}

/// Zero-allocation stack-allocated Base32 Geohash code (up to 12 characters).
pub struct GeohashCode {
    pub buf: [12]u8 = undefined,
    pub len: u8 = 0,

    /// Returns a read-only slice view of the encoded Geohash characters (requires mutable binding `let`).
    pub fn slice(*_): []const u8 {
        return _.buf[0..(_.len as usize)];
    }

    /// Copies the encoded Geohash characters into `dst` and returns the written slice.
    pub fn copyTo(dst: []u8): []const u8 {
        const n = _.len as usize;
        const m = if (dst.len < n) dst.len else n;
        if (m > 0) @memcpy(dst[0..m], _.buf[0..m]);
        return dst[0..m];
    }

    /// Compares two `GeohashCode` values for byte-for-byte equality.
    pub fn eql(other: GeohashCode): bool {
        if (_.len != other.len) return false;
        const n = _.len as usize;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (_.buf[i] != other.buf[i]) return false;
        }
        return true;
    }

    /// Compares this `GeohashCode` against a raw string slice.
    pub fn eqlStr(s: []const u8): bool {
        if ((_.len as usize) != s.len) return false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (_.buf[i] != s[i]) return false;
        }
        return true;
    }
}

/// Zero-allocation container holding all 8 surrounding Geohash neighbors (`n, ne, e, se, s, sw, w, nw`).
pub struct Neighbors {
    pub n: GeohashCode = GeohashCode{},
    pub ne: GeohashCode = GeohashCode{},
    pub e: GeohashCode = GeohashCode{},
    pub se: GeohashCode = GeohashCode{},
    pub s: GeohashCode = GeohashCode{},
    pub sw: GeohashCode = GeohashCode{},
    pub w: GeohashCode = GeohashCode{},
    pub nw: GeohashCode = GeohashCode{},

    /// Returns all 8 neighbor slices in clockwise order starting from North:
    /// `[n, ne, e, se, s, sw, w, nw]` (requires mutable binding `let`).
    pub fn all(*_): [8][]const u8 {
        return [
            _.n.slice(),
            _.ne.slice(),
            _.e.slice(),
            _.se.slice(),
            _.s.slice(),
            _.sw.slice(),
            _.w.slice(),
            _.nw.slice(),
        ];
    }
}

fn isValidCoord(lat: f64, lon: f64): bool {
    if (!std.math.isFinite(lat) || !std.math.isFinite(lon)) return false;
    return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

fn base32Index(ch: u8): usize? {
    let i: usize = 0;
    while (i < BASE32_ALPHABET.len) : (i += 1) {
        if (BASE32_ALPHABET[i] == ch) return i;
    }
    return null;
}

fn lookupIndexIn(table: string, ch: u8): usize? {
    let i: usize = 0;
    while (i < table.len) : (i += 1) {
        if (table[i] == ch) return i;
    }
    return null;
}

fn containsChar(set: string, ch: u8): bool {
    return lookupIndexIn(set, ch) != null;
}

fn validateHash(hash: []const u8): void ! GeohashError {
    if (hash.len == 0) return GeohashError.EmptyGeohash;
    for (ch of hash) {
        if (base32Index(ch) == null) return GeohashError.InvalidGeohashChar;
    }
}

fn neighborTable(dir: CardinalDir, is_odd: bool): string {
    return switch (dir) {
        .North => if (is_odd) NEIGHBOR_N_ODD else NEIGHBOR_N_EVEN,
        .South => if (is_odd) NEIGHBOR_S_ODD else NEIGHBOR_S_EVEN,
        .East => if (is_odd) NEIGHBOR_E_ODD else NEIGHBOR_E_EVEN,
        .West => if (is_odd) NEIGHBOR_W_ODD else NEIGHBOR_W_EVEN,
    };
}

fn borderTable(dir: CardinalDir, is_odd: bool): string {
    return switch (dir) {
        .North => if (is_odd) BORDER_N_ODD else BORDER_N_EVEN,
        .South => if (is_odd) BORDER_S_ODD else BORDER_S_EVEN,
        .East => if (is_odd) BORDER_E_ODD else BORDER_E_EVEN,
        .West => if (is_odd) BORDER_W_ODD else BORDER_W_EVEN,
    };
}

fn writeAdjacentInPlace(dst: []u8, src: []const u8, dir: CardinalDir): void ! GeohashError {
    if (src.len == 0) return GeohashError.EmptyGeohash;
    const last_idx = src.len - 1;
    const last_ch = src[last_idx];
    const is_odd = (src.len % 2) == 1;

    const border = borderTable(dir, is_odd);
    const n_table = neighborTable(dir, is_odd);

    if (containsChar(border, last_ch)) {
        if (src.len == 1) {
            // At the root cell boundary:
            // - East/West wraps seamlessly across the +-180 deg antimeridian.
            // - North/South clamps at the +-90 deg pole instead of wrapping to the opposite pole.
            if (dir == .North || dir == .South) {
                dst[0] = last_ch;
                return;
            }
        } else {
            writeAdjacentInPlace(dst[0..last_idx], src[0..last_idx], dir)?;
        }
    } else {
        if (last_idx > 0) {
            @memcpy(dst[0..last_idx], src[0..last_idx]);
        }
    }

    const pos_opt = lookupIndexIn(n_table, last_ch);
    if (pos_opt == null) return GeohashError.InvalidGeohashChar;
    dst[last_idx] = BASE32_ALPHABET[pos_opt.?];
}

fn adjacentCode(src: []const u8, dir: CardinalDir): GeohashCode ! GeohashError {
    if (src.len == 0) return GeohashError.EmptyGeohash;
    if (src.len > (MAX_PRECISION as usize)) return GeohashError.InvalidPrecision;
    let code = GeohashCode { len: src.len as u8 };
    writeAdjacentInPlace(code.buf[0..src.len], src, dir)?;
    return code;
}

/// Encodes `(lat, lon)` into a zero-allocation `GeohashCode` of length `precision` (`1..12`).
pub fn encode(lat: f64, lon: f64, precision: u8): GeohashCode ! GeohashError {
    if (!isValidCoord(lat, lon)) return GeohashError.InvalidCoordinate;
    if (precision < 1 || precision > MAX_PRECISION) return GeohashError.InvalidPrecision;

    let code = GeohashCode { len: precision };
    const prec = precision as usize;

    let min_lat: f64 = -90.0;
    let max_lat: f64 = 90.0;
    let min_lon: f64 = -180.0;
    let max_lon: f64 = 180.0;

    let is_lon = true;
    let bit_idx: u5 = 0;
    let ch_idx: usize = 0;
    let out_idx: usize = 0;

    while (out_idx < prec) {
        if (is_lon) {
            const mid = (min_lon + max_lon) * 0.5;
            if (lon >= mid) {
                ch_idx = (ch_idx << 1) | 1;
                min_lon = mid;
            } else {
                ch_idx = ch_idx << 1;
                max_lon = mid;
            }
        } else {
            const mid = (min_lat + max_lat) * 0.5;
            if (lat >= mid) {
                ch_idx = (ch_idx << 1) | 1;
                min_lat = mid;
            } else {
                ch_idx = ch_idx << 1;
                max_lat = mid;
            }
        }

        is_lon = !is_lon;
        bit_idx += 1;

        if (bit_idx == 5) {
            code.buf[out_idx] = BASE32_ALPHABET[ch_idx];
            out_idx += 1;
            bit_idx = 0;
            ch_idx = 0;
        }
    }

    return code;
}

/// Encodes `(lat, lon)` into an owned heap-allocated Base32 Geohash slice (`allocator.free(s)`).
pub fn encodeAlloc(allocator: std.mem.Allocator, lat: f64, lon: f64, precision: u8): []u8 ! GeohashError {
    const code = encode(lat, lon, precision)?;
    const n = code.len as usize;
    let out = allocator.alloc(u8, n) catch return GeohashError.OutOfMemory;
    @memcpy(out, code.buf[0..n]);
    return out;
}

/// Decodes a Base32 Geohash string into its full `BoundingBox` (bounds, center, and error margins).
pub fn decodeBounds(hash: []const u8): BoundingBox ! GeohashError {
    if (hash.len == 0) return GeohashError.EmptyGeohash;

    let min_lat: f64 = -90.0;
    let max_lat: f64 = 90.0;
    let min_lon: f64 = -180.0;
    let max_lon: f64 = 180.0;
    let is_lon = true;

    for (ch of hash) {
        const val_opt = base32Index(ch);
        if (val_opt == null) return GeohashError.InvalidGeohashChar;
        const val = val_opt.?;
        let shift: u3 = 4;
        while (true) {
            const bit = (val >> shift) & 1;
            if (is_lon) {
                const mid = (min_lon + max_lon) * 0.5;
                if (bit == 1) {
                    min_lon = mid;
                } else {
                    max_lon = mid;
                }
            } else {
                const mid = (min_lat + max_lat) * 0.5;
                if (bit == 1) {
                    min_lat = mid;
                } else {
                    max_lat = mid;
                }
            }
            is_lon = !is_lon;
            if (shift == 0) break;
            shift -= 1;
        }
    }

    return BoundingBox {
        min_lat: min_lat,
        max_lat: max_lat,
        min_lon: min_lon,
        max_lon: max_lon,
        center_lat: (min_lat + max_lat) * 0.5,
        center_lon: (min_lon + max_lon) * 0.5,
        lat_err: (max_lat - min_lat) * 0.5,
        lon_err: (max_lon - min_lon) * 0.5,
    };
}

/// Decodes a Base32 Geohash string into its center `LatLon` coordinate.
pub fn decode(hash: []const u8): LatLon ! GeohashError {
    const bounds = decodeBounds(hash)?;
    return bounds.center();
}

/// Computes the adjacent Geohash cell in `dir` at the same precision.
/// Handles `+-180 deg` antimeridian wrap-around and `+-90 deg` pole clamping.
pub fn neighbor(hash: []const u8, dir: Direction): GeohashCode ! GeohashError {
    validateHash(hash)?;
    if (hash.len > (MAX_PRECISION as usize)) return GeohashError.InvalidPrecision;

    return switch (dir) {
        .North => adjacentCode(hash, .North),
        .South => adjacentCode(hash, .South),
        .East => adjacentCode(hash, .East),
        .West => adjacentCode(hash, .West),
        .NorthEast => {
            let n = adjacentCode(hash, .North)?;
            return adjacentCode(n.slice(), .East);
        },
        .NorthWest => {
            let n = adjacentCode(hash, .North)?;
            return adjacentCode(n.slice(), .West);
        },
        .SouthEast => {
            let s = adjacentCode(hash, .South)?;
            return adjacentCode(s.slice(), .East);
        },
        .SouthWest => {
            let s = adjacentCode(hash, .South)?;
            return adjacentCode(s.slice(), .West);
        },
    };
}

/// Computes all 8 neighboring Geohash cells around `hash` on the stack with zero heap allocations.
pub fn neighbors(hash: []const u8): Neighbors ! GeohashError {
    validateHash(hash)?;
    if (hash.len > (MAX_PRECISION as usize)) return GeohashError.InvalidPrecision;

    let n = adjacentCode(hash, .North)?;
    let s = adjacentCode(hash, .South)?;
    const e = adjacentCode(hash, .East)?;
    const w = adjacentCode(hash, .West)?;
    const ne = adjacentCode(n.slice(), .East)?;
    const nw = adjacentCode(n.slice(), .West)?;
    const se = adjacentCode(s.slice(), .East)?;
    const sw = adjacentCode(s.slice(), .West)?;

    return Neighbors {
        n: n,
        ne: ne,
        e: e,
        se: se,
        s: s,
        sw: sw,
        w: w,
        nw: nw,
    };
}

/// Encodes `(lat, lon)` into a 64-bit integer Z-order Morton Geohash of `bit_depth` (`1..63`).
pub fn encodeInt(lat: f64, lon: f64, bit_depth: u6): u64 ! GeohashError {
    if (!isValidCoord(lat, lon)) return GeohashError.InvalidCoordinate;
    if (bit_depth == 0) return GeohashError.InvalidPrecision;

    let min_lat: f64 = -90.0;
    let max_lat: f64 = 90.0;
    let min_lon: f64 = -180.0;
    let max_lon: f64 = 180.0;

    let bits: u64 = 0;
    let is_lon = true;
    let step: u6 = 0;

    while (step < bit_depth) : (step += 1) {
        bits = bits << 1;
        if (is_lon) {
            const mid = (min_lon + max_lon) * 0.5;
            if (lon >= mid) {
                bits |= 1;
                min_lon = mid;
            } else {
                max_lon = mid;
            }
        } else {
            const mid = (min_lat + max_lat) * 0.5;
            if (lat >= mid) {
                bits |= 1;
                min_lat = mid;
            } else {
                max_lat = mid;
            }
        }
        is_lon = !is_lon;
    }

    return bits;
}

/// Decodes a 64-bit integer Z-order Morton Geohash of `bit_depth` (`1..63`) into its `BoundingBox`.
pub fn decodeInt(bits: u64, bit_depth: u6): BoundingBox ! GeohashError {
    if (bit_depth == 0) return GeohashError.InvalidPrecision;

    let min_lat: f64 = -90.0;
    let max_lat: f64 = 90.0;
    let min_lon: f64 = -180.0;
    let max_lon: f64 = 180.0;
    let is_lon = true;

    let shift: u6 = bit_depth - 1;
    while (true) {
        const bit = (bits >> shift) & 1;
        if (is_lon) {
            const mid = (min_lon + max_lon) * 0.5;
            if (bit == 1) {
                min_lon = mid;
            } else {
                max_lon = mid;
            }
        } else {
            const mid = (min_lat + max_lat) * 0.5;
            if (bit == 1) {
                min_lat = mid;
            } else {
                max_lat = mid;
            }
        }
        is_lon = !is_lon;
        if (shift == 0) break;
        shift -= 1;
    }

    return BoundingBox {
        min_lat: min_lat,
        max_lat: max_lat,
        min_lon: min_lon,
        max_lon: max_lon,
        center_lat: (min_lat + max_lat) * 0.5,
        center_lon: (min_lon + max_lon) * 0.5,
        lat_err: (max_lat - min_lat) * 0.5,
        lon_err: (max_lon - min_lon) * 0.5,
    };
}

/// Computes the great-circle distance in meters between two coordinates using the Haversine formula
/// with WGS-84 mean Earth radius (`6371000.0` m).
pub fn haversineMeters(a: LatLon, b: LatLon): f64 {
    const deg_to_rad: f64 = std.math.pi / 180.0;
    const lat1 = a.lat * deg_to_rad;
    const lat2 = b.lat * deg_to_rad;
    const dlat = (b.lat - a.lat) * deg_to_rad;
    const dlon = (b.lon - a.lon) * deg_to_rad;

    const sin_dlat = std.math.sin(dlat * 0.5);
    const sin_dlon = std.math.sin(dlon * 0.5);
    const h = sin_dlat * sin_dlat + std.math.cos(lat1) * std.math.cos(lat2) * sin_dlon * sin_dlon;
    const clamped_h = if (h > 1.0) 1.0 else if (h < 0.0) 0.0 else h;
    const c = 2.0 * std.math.atan2(std.math.sqrt(clamped_h), std.math.sqrt(1.0 - clamped_h));
    return EARTH_RADIUS_METERS * c;
}

/// Recommends a Base32 Geohash precision (`1..12`) suitable for a search radius in meters.
pub fn precisionForRadiusMeters(radius_m: f64): u8 {
    if (!std.math.isFinite(radius_m)) return 1;
    if (radius_m >= 2500000.0) return 1;
    if (radius_m >= 625000.0) return 2;
    if (radius_m >= 156000.0) return 3;
    if (radius_m >= 19500.0) return 4;
    if (radius_m >= 4890.0) return 5;
    if (radius_m >= 610.0) return 6;
    if (radius_m >= 153.0) return 7;
    if (radius_m >= 19.1) return 8;
    if (radius_m >= 4.77) return 9;
    if (radius_m >= 0.596) return 10;
    if (radius_m >= 0.149) return 11;
    return 12;
}

// packages/@cap/geohash-pure/src/index.cp
// Zero-allocation Geohash Base32 spatial encoder, decoder & bounding box engine.
// MIT clean-room pure CP implementation.

/// Decoded latitude/longitude bounding box for a Geohash cell.
pub struct GeoBounds {
    pub min_lat: f64,
    pub max_lat: f64,
    pub min_lon: f64,
    pub max_lon: f64,
    pub valid: bool,
}

/// Decoded Geohash center coordinate and half-cell error margins.
pub struct GeoPoint {
    pub lat: f64,
    pub lon: f64,
    pub lat_err: f64,
    pub lon_err: f64,
    pub valid: bool,
}

fn ghEncodeChar(v: u8): u8 {
    const alpha = "0123456789bcdefghjkmnpqrstuvwxyz";
    return alpha[(v & (0x1F as u8)) as usize];
}

/// Decode a Geohash Base32 character (case-insensitive, rejecting `a`, `i`, `l`, `o`).
/// Returns `0..31`, or `0xFF` if invalid.
fn ghDecodeChar(c: u8): u8 {
    let ch = c;
    if (ch >= ('A' as u8) && ch <= ('Z' as u8)) {
        ch = ch + (32 as u8);
    }
    const alpha = "0123456789bcdefghjkmnpqrstuvwxyz";
    let i: usize = 0;
    while (i < 32) {
        if (alpha[i] == ch) {
            return i as u8;
        }
        i += 1;
    }
    return 0xFF as u8;
}

/// Validate whether `hash` is a non-empty legal Geohash Base32 string.
pub fn isValidGeohash(hash: string): bool {
    if (hash.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < hash.len) {
        if (ghDecodeChar(hash[i]) == (0xFF as u8)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Encode `(lat, lon)` into a Geohash string of length `precision` in `out`.
/// Returns `precision` on success, or `0` if coordinates are out of range, `precision == 0`, or `out.len < precision`.
pub fn encode(lat: f64, lon: f64, precision: usize, out: []u8): usize {
    if (lat != lat || lon != lon) {
        return 0;
    }
    if (lat < (-90.0 as f64) || lat > (90.0 as f64) || lon < (-180.0 as f64) || lon > (180.0 as f64)) {
        return 0;
    }
    if (precision == 0 || out.len < precision) {
        return 0;
    }

    let minLat: f64 = -90.0;
    let maxLat: f64 = 90.0;
    let minLon: f64 = -180.0;
    let maxLon: f64 = 180.0;

    let isEven = true;
    let bit: usize = 0;
    let ch: u8 = 0 as u8;
    let pos: usize = 0;

    while (pos < precision) {
        if (isEven) {
            const mid: f64 = (minLon + maxLon) * (0.5 as f64);
            if (lon >= mid) {
                ch = (ch << (1 as u3)) | (1 as u8);
                minLon = mid;
            } else {
                ch = ch << (1 as u3);
                maxLon = mid;
            }
        } else {
            const mid: f64 = (minLat + maxLat) * (0.5 as f64);
            if (lat >= mid) {
                ch = (ch << (1 as u3)) | (1 as u8);
                minLat = mid;
            } else {
                ch = ch << (1 as u3);
                maxLat = mid;
            }
        }

        isEven = !isEven;
        bit += 1;
        if (bit == 5) {
            out[pos] = ghEncodeChar(ch);
            pos += 1;
            bit = 0;
            ch = 0 as u8;
        }
    }

    return pos;
}

/// Decode a Geohash string into its enclosing latitude/longitude `GeoBounds`.
pub fn decodeBounds(hash: string): GeoBounds {
    let invalid = GeoBounds{
        min_lat: 0.0 as f64,
        max_lat: 0.0 as f64,
        min_lon: 0.0 as f64,
        max_lon: 0.0 as f64,
        valid: false,
    };

    if (hash.len == 0) {
        return invalid;
    }

    let minLat: f64 = -90.0;
    let maxLat: f64 = 90.0;
    let minLon: f64 = -180.0;
    let maxLon: f64 = 180.0;
    let isEven = true;

    let i: usize = 0;
    while (i < hash.len) {
        const v = ghDecodeChar(hash[i]);
        if (v == (0xFF as u8)) {
            return invalid;
        }
        let bitIdx: usize = 5;
        while (bitIdx > 0) {
            bitIdx -= 1;
            const mask: u8 = (1 as u8) << (bitIdx as u3);
            const isSet = (v & mask) != (0 as u8);
            if (isEven) {
                const mid: f64 = (minLon + maxLon) * (0.5 as f64);
                if (isSet) {
                    minLon = mid;
                } else {
                    maxLon = mid;
                }
            } else {
                const mid: f64 = (minLat + maxLat) * (0.5 as f64);
                if (isSet) {
                    minLat = mid;
                } else {
                    maxLat = mid;
                }
            }
            isEven = !isEven;
        }
        i += 1;
    }

    return GeoBounds{
        min_lat: minLat,
        max_lat: maxLat,
        min_lon: minLon,
        max_lon: maxLon,
        valid: true,
    };
}

/// Decode a Geohash string into its center `GeoPoint` and half-cell error margins.
pub fn decode(hash: string): GeoPoint {
    const b = decodeBounds(hash);
    if (!b.valid) {
        return GeoPoint{
            lat: 0.0 as f64,
            lon: 0.0 as f64,
            lat_err: 0.0 as f64,
            lon_err: 0.0 as f64,
            valid: false,
        };
    }
    return GeoPoint{
        lat: (b.min_lat + b.max_lat) * (0.5 as f64),
        lon: (b.min_lon + b.max_lon) * (0.5 as f64),
        lat_err: (b.max_lat - b.min_lat) * (0.5 as f64),
        lon_err: (b.max_lon - b.min_lon) * (0.5 as f64),
        valid: true,
    };
}

export {
    GeoBounds,
    GeoPoint,
    encode,
    decodeBounds,
    decode,
    isValidGeohash,
};

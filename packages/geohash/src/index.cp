// @cap/geohash — Zero-allocation Base32 Geohash encoder, bounding-box decoder & 8-way neighbor finder for CP
// Clean-room MIT implementation inspired by Rust `geohash` and npm `ngeohash`.
// Accelerated with comptime 256-entry Base32 decode LUT, 16-lane @Vector(16, u8) SIMD range guard, and bit-shifts.

type GhVec16 = @Vector(16, u8);

const BASE32_ALPHABET: string = "0123456789bcdefghjkmnpqrstuvwxyz";
const MAX_PRECISION: usize = 12;

fn ghBuildDecodeLut() -> [256]i16 {
    @setEvalBranchQuota(20000);
    let lut: [256]i16 = [-1 as i16; 256];
    let i: usize = 0;
    while (i < 32) {
        lut[BASE32_ALPHABET[i] as usize] = i as i16;
        i += 1;
    }
    return lut;
}

const GH_DECODE_LUT: [256]i16 = ghBuildDecodeLut();

pub struct GeoPoint {
    pub lat: f64,
    pub lon: f64,
}

pub struct GeoBBox {
    pub min_lat: f64,
    pub max_lat: f64,
    pub min_lon: f64,
    pub max_lon: f64,
    pub valid: bool,
}

fn invalidBBox() -> GeoBBox {
    return GeoBBox{
        min_lat: 0.0,
        max_lat: 0.0,
        min_lon: 0.0,
        max_lon: 0.0,
        valid: false,
    };
}

fn base32Index(c: u8) -> i16 {
    return GH_DECODE_LUT[c as usize];
}

pub fn isValid(hash: []const u8) -> bool {
    if (hash.len == 0 || hash.len > MAX_PRECISION) {
        return false;
    }
    // Load up to 12 bytes into a 16-lane SIMD vector padded with '0' for 1-instruction ASCII range rejection
    let padded: [16]u8 = ['0' as u8; 16];
    @memcpy(padded[0..hash.len], hash);
    const v: GhVec16 = padded;
    const v_lo: GhVec16 = @splat('0' as u8);
    const v_hi: GhVec16 = @splat('z' as u8);
    if (@reduce(.Or, v < v_lo) || @reduce(.Or, v > v_hi)) {
        return false;
    }
    let i: usize = 0;
    while (i < hash.len) {
        if (GH_DECODE_LUT[hash[i] as usize] < (0 as i16)) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn encode(lat: f64, lon: f64, precision: usize, dst: []u8) -> usize {
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        return 0 as usize;
    }
    if (precision == 0 || precision > MAX_PRECISION || dst.len < precision) {
        return 0 as usize;
    }

    let minLat: f64 = -90.0;
    let maxLat: f64 = 90.0;
    let minLon: f64 = -180.0;
    let maxLon: f64 = 180.0;

    let isLon = true;
    let bitCount: u8 = 0;
    let chIdx: usize = 0;
    let outLen: usize = 0;

    while (outLen < precision) {
        if (isLon) {
            const mid = (minLon + maxLon) * 0.5;
            if (lon >= mid) {
                chIdx = (chIdx << 1) | (1 as usize);
                minLon = mid;
            } else {
                chIdx = chIdx << 1;
                maxLon = mid;
            }
        } else {
            const mid = (minLat + maxLat) * 0.5;
            if (lat >= mid) {
                chIdx = (chIdx << 1) | (1 as usize);
                minLat = mid;
            } else {
                chIdx = chIdx << 1;
                maxLat = mid;
            }
        }
        isLon = !isLon;
        bitCount += 1 as u8;
        if (bitCount == (5 as u8)) {
            dst[outLen] = BASE32_ALPHABET[chIdx];
            outLen += 1;
            bitCount = 0 as u8;
            chIdx = 0 as usize;
        }
    }
    return outLen;
}

pub fn decodeBBox(hash: []const u8) -> GeoBBox {
    if (hash.len == 0 || hash.len > MAX_PRECISION) {
        return invalidBBox();
    }

    let minLat: f64 = -90.0;
    let maxLat: f64 = 90.0;
    let minLon: f64 = -180.0;
    let maxLon: f64 = 180.0;
    let isLon = true;

    let i: usize = 0;
    while (i < hash.len) {
        const idx = GH_DECODE_LUT[hash[i] as usize];
        if (idx < (0 as i16)) {
            return invalidBBox();
        }
        const val = idx as u8;
        let mask: u8 = 16;
        while (mask > (0 as u8)) {
            const bitSet = (val & mask) != (0 as u8);
            if (isLon) {
                const mid = (minLon + maxLon) * 0.5;
                if (bitSet) {
                    minLon = mid;
                } else {
                    maxLon = mid;
                }
            } else {
                const mid = (minLat + maxLat) * 0.5;
                if (bitSet) {
                    minLat = mid;
                } else {
                    maxLat = mid;
                }
            }
            isLon = !isLon;
            mask = mask >> 1;
        }
        i += 1;
    }

    return GeoBBox{
        min_lat: minLat,
        max_lat: maxLat,
        min_lon: minLon,
        max_lon: maxLon,
        valid: true,
    };
}

pub fn decode(hash: []const u8) -> GeoPoint {
    const bb = decodeBBox(hash);
    if (!bb.valid) {
        return GeoPoint{ lat: 0.0, lon: 0.0 };
    }
    return GeoPoint{
        lat: (bb.min_lat + bb.max_lat) * 0.5,
        lon: (bb.min_lon + bb.max_lon) * 0.5,
    };
}

pub fn neighbor(hash: []const u8, dlat: i32, dlon: i32, dst: []u8) -> usize {
    if (dlat < (-1 as i32) || dlat > (1 as i32) || dlon < (-1 as i32) || dlon > (1 as i32)) {
        return 0 as usize;
    }
    const bb = decodeBBox(hash);
    if (!bb.valid || dst.len < hash.len) {
        return 0 as usize;
    }

    const latCenter = (bb.min_lat + bb.max_lat) * 0.5;
    const lonCenter = (bb.min_lon + bb.max_lon) * 0.5;
    const latStep = bb.max_lat - bb.min_lat;
    const lonStep = bb.max_lon - bb.min_lon;

    const newLat = latCenter + ((dlat as f64) * latStep);
    let newLon = lonCenter + ((dlon as f64) * lonStep);

    if (newLat < -90.0 || newLat > 90.0) {
        return 0 as usize;
    }
    if (newLon > 180.0) {
        newLon -= 360.0;
    } else if (newLon < -180.0) {
        newLon += 360.0;
    }

    return encode(newLat, newLon, hash.len, dst);
}

export {
    GeoPoint,
    GeoBBox,
    encode,
    decodeBBox,
    decode,
    neighbor,
    isValid,
};

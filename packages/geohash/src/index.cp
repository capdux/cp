// @cap/geohash — Zero-allocation Base32 Geohash encoder, bounding-box decoder & 8-way neighbor finder for CP
// Clean-room MIT implementation inspired by Rust `geohash` and npm `ngeohash`.

const BASE32_ALPHABET: string = "0123456789bcdefghjkmnpqrstuvwxyz";
const MAX_PRECISION: usize = 12;

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
    let i: usize = 0;
    while (i < 32) {
        if (BASE32_ALPHABET[i] == c) {
            return i as i16;
        }
        i += 1;
    }
    return -1 as i16;
}

pub fn isValid(hash: []const u8) -> bool {
    if (hash.len == 0 || hash.len > MAX_PRECISION) {
        return false;
    }
    let i: usize = 0;
    while (i < hash.len) {
        if (base32Index(hash[i]) < (0 as i16)) {
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
                chIdx = (chIdx * (2 as usize)) + (1 as usize);
                minLon = mid;
            } else {
                chIdx = chIdx * (2 as usize);
                maxLon = mid;
            }
        } else {
            const mid = (minLat + maxLat) * 0.5;
            if (lat >= mid) {
                chIdx = (chIdx * (2 as usize)) + (1 as usize);
                minLat = mid;
            } else {
                chIdx = chIdx * (2 as usize);
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
        const idx = base32Index(hash[i]);
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
            mask = mask / (2 as u8);
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

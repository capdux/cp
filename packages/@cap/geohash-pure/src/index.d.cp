// Type definitions for CP

export class GeoBounds {
    min_lat: f64,
    max_lat: f64,
    min_lon: f64,
    max_lon: f64,
    valid: bool,
}

export class GeoPoint {
    lat: f64,
    lon: f64,
    lat_err: f64,
    lon_err: f64,
    valid: bool,
}

export fn ghEncodeChar(v: u8): u8;
export fn ghDecodeChar(c: u8): u8;
export fn isValidGeohash(hash: string): bool;
export fn encode(lat: f64, lon: f64, precision: usize, out: []u8): usize;
export fn decodeBounds(hash: string): GeoBounds;
export fn decode(hash: string): GeoPoint;

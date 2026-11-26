// Type definitions for CP

export class GeoPoint {
    lat: f64,
    lon: f64,
}

export class GeoBBox {
    min_lat: f64,
    max_lat: f64,
    min_lon: f64,
    max_lon: f64,
    valid: bool,
}

export fn invalidBBox(): GeoBBox;
export fn base32Index(c: u8): i16;
export fn isValid(hash: &[]u8): bool;
export fn encode(lat: f64, lon: f64, precision: usize, dst: []u8): usize;
export fn decodeBBox(hash: &[]u8): GeoBBox;
export fn decode(hash: &[]u8): GeoPoint;
export fn neighbor(hash: &[]u8, dlat: i32, dlon: i32, dst: []u8): usize;

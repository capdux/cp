// Type definitions for CP

export class LatLng {
    lat: f64,
    lng: f64,
}

export fn precisionScale(precision: u32): f64;
export fn roundToI64(v: f64): i64;
export fn encodeSigned(val: i64, out: []u8): usize;
export fn decodeSigned(data: string, start: usize, out_val: &mut i64): usize;
export fn encodePoints(points: &[]LatLng, precision: u32, out: []u8): usize;
export fn decodePoints(encoded: string, precision: u32, out: []LatLng): usize;
export fn countPoints(encoded: string): usize;
export fn isValid(encoded: string): bool;

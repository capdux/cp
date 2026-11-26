// Type definitions for CP

export class LatLng {
    lat_deg: f64,
    lng_deg: f64,
    fn fromDegrees(lat: f64, lng: f64): LatLng;
    fn isValid(): bool;
    fn latRad(): f64;
    fn lngRad(): f64;
}

export fn s2TrailingZeros(v: u64): u8;
export fn s2MaskAbove(lsb_val: u64): u64;
export class S2CellId {
    id: u64,
    fn fromFace(face_id: u8): S2CellId?;
    fn fromFacePosLevel(face_id: u8, pos: u64, lvl: u8): S2CellId?;
    fn face(): u8;
    fn lsb(): u64;
    fn isValid(): bool;
    fn level(): u8;
    fn isLeaf(): bool;
    fn parent(target_lvl: u8): S2CellId?;
    fn child(quadrant: u8): S2CellId?;
    fn rangeMin(): u64;
    fn rangeMax(): u64;
    fn contains(other: S2CellId): bool;
}

export fn s2Sin(x_in: f64): f64;
export fn s2Cos(x_in: f64): f64;
export fn s2Sqrt(x: f64): f64;
export fn s2Asin(x: f64): f64;
export fn haversineMeters(a: LatLng, b: LatLng): f64;

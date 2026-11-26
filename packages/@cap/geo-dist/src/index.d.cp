// Type definitions for CP

export class Coord {
    lat: f64,
    lon: f64,
}

export class Bounds {
    min_lat: f64,
    max_lat: f64,
    min_lon: f64,
    max_lon: f64,
}

export fn toRad(deg: f64): f64;
export fn toDeg(rad: f64): f64;
export fn normalizeLon(lon_in: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn sinF64(x_in: f64): f64;
export fn cosF64(x: f64): f64;
export fn atanF64(x_in: f64): f64;
export fn atan2F64(y: f64, x: f64): f64;
export fn asinF64(x: f64): f64;
export fn haversineMeters(a: Coord, b: Coord): f64;
export fn initialBearing(a: Coord, b: Coord): f64;
export fn destinationPoint(origin: Coord, bearing_deg: f64, distance_m: f64): Coord;
export fn midpoint(a: Coord, b: Coord): Coord;
export fn boundingBox(center: Coord, radius_m: f64): Bounds;
export fn pointInPolygon(pt: Coord, poly: &[]Coord): bool;

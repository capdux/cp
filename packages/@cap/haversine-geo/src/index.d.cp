// Type definitions for CP

export class LatLon {
    lat: f64,
    lon: f64,
}

export fn hgAbs(x: f64): f64;
export fn hgClamp(x: f64, lo: f64, hi: f64): f64;
export fn hgSqrt(x: f64): f64;
export fn hgWrapPi(rad: f64): f64;
export fn hgSin(rad: f64): f64;
export fn hgCos(rad: f64): f64;
export fn hgAtan(z: f64): f64;
export fn hgAtan2(y: f64, x: f64): f64;
export fn hgAsin(x: f64): f64;
export fn hgDegToRad(deg: f64): f64;
export fn hgRadToDeg(rad: f64): f64;
export fn hgNormLon(lon: f64): f64;
export fn isValidCoord(p: LatLon): bool;
export fn distanceMeters(a: LatLon, b: LatLon): f64;
export fn distanceKm(a: LatLon, b: LatLon): f64;
export fn distanceMiles(a: LatLon, b: LatLon): f64;
export fn initialBearing(a: LatLon, b: LatLon): f64;
export fn midpoint(a: LatLon, b: LatLon): LatLon;
export fn destinationPoint(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon;

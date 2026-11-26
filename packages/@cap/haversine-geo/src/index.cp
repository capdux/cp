// packages/@cap/haversine-geo/src/index.cp
// Zero-allocation WGS-84 spherical Haversine distance, bearing, midpoint & destination.
// MIT clean-room pure CP implementation (zero external dependencies).

pub const EARTH_RADIUS_KM: f64 = 6371.0088;
pub const EARTH_RADIUS_M: f64 = 6371008.8;

/// Geographic coordinate in decimal degrees (`lat` in `[-90, 90]`, `lon` in `[-180, 180]`).
pub struct LatLon {
    pub lat: f64,
    pub lon: f64,
}

fn hgAbs(x: f64): f64 {
    if (x < (0.0 as f64)) { return -x; }
    return x;
}

fn hgClamp(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

fn hgSqrt(x: f64): f64 {
    if (x <= (0.0 as f64)) {
        return 0.0 as f64;
    }
    let m: f64 = x;
    let factor: f64 = 1.0;
    while (m >= (4.0 as f64)) {
        m = m * (0.25 as f64);
        factor = factor * (2.0 as f64);
    }
    while (m < (0.25 as f64)) {
        m = m * (4.0 as f64);
        factor = factor * (0.5 as f64);
    }
    let g: f64 = (0.5 as f64) * (m + (1.0 as f64));
    let i: usize = 0;
    while (i < 12) {
        g = (0.5 as f64) * (g + (m / g));
        i += 1;
    }
    return g * factor;
}

fn hgWrapPi(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const twoPi: f64 = 6.28318530717958647692;
    let a: f64 = rad;
    if (a > pi || a < -pi) {
        const k: f64 = (((a + pi) / twoPi) as i64) as f64;
        a = a - k * twoPi;
        while (a > pi) { a = a - twoPi; }
        while (a < -pi) { a = a + twoPi; }
    }
    return a;
}

fn hgSin(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const halfPi: f64 = 1.57079632679489661923;
    let x: f64 = hgWrapPi(rad);
    if (x > halfPi) {
        x = pi - x;
    } else if (x < -halfPi) {
        x = -pi - x;
    }
    const x2: f64 = x * x;
    let term: f64 = x;
    let sum: f64 = x;
    let n: usize = 1;
    while (n <= 10) {
        const d1: f64 = (2 * n) as f64;
        const d2: f64 = (2 * n + 1) as f64;
        term = -term * x2 / (d1 * d2);
        sum = sum + term;
        n += 1;
    }
    return sum;
}

fn hgCos(rad: f64): f64 {
    const halfPi: f64 = 1.57079632679489661923;
    return hgSin(rad + halfPi);
}

fn hgAtan(z: f64): f64 {
    const halfPi: f64 = 1.57079632679489661923;
    const sixthPi: f64 = 0.52359877559829887308;
    const sqrt3: f64 = 1.73205080756887729353;
    const tan15: f64 = 0.26794919243112270647;

    let neg = false;
    let x: f64 = z;
    if (x < (0.0 as f64)) {
        neg = true;
        x = -x;
    }

    let inv = false;
    if (x > (1.0 as f64)) {
        inv = true;
        x = (1.0 as f64) / x;
    }

    let shifted = false;
    if (x > tan15) {
        shifted = true;
        x = ((x * sqrt3) - (1.0 as f64)) / (sqrt3 + x);
    }

    // Now |x| <= tan(15°) ≈ 0.268: 11-term Taylor series reaches ~1e-15 accuracy.
    const x2: f64 = x * x;
    let powX: f64 = x;
    let sum: f64 = x;
    let sign: f64 = -1.0;
    let k: usize = 1;
    while (k <= 11) {
        powX = powX * x2;
        sum = sum + (sign * powX / ((2 * k + 1) as f64));
        sign = -sign;
        k += 1;
    }

    if (shifted) {
        sum = sum + sixthPi;
    }
    if (inv) {
        sum = halfPi - sum;
    }
    if (neg) {
        sum = -sum;
    }
    return sum;
}

fn hgAtan2(y: f64, x: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    const halfPi: f64 = 1.57079632679489661923;
    if (x > (0.0 as f64)) {
        return hgAtan(y / x);
    }
    if (x < (0.0 as f64)) {
        if (y >= (0.0 as f64)) {
            return hgAtan(y / x) + pi;
        }
        return hgAtan(y / x) - pi;
    }
    if (y > (0.0 as f64)) { return halfPi; }
    if (y < (0.0 as f64)) { return -halfPi; }
    return 0.0 as f64;
}

fn hgAsin(x: f64): f64 {
    const c: f64 = hgClamp(x, -1.0 as f64, 1.0 as f64);
    const rem: f64 = (1.0 as f64) - (c * c);
    return hgAtan2(c, hgSqrt(rem));
}

fn hgDegToRad(deg: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    return deg * (pi / (180.0 as f64));
}

fn hgRadToDeg(rad: f64): f64 {
    const pi: f64 = 3.14159265358979323846;
    return rad * ((180.0 as f64) / pi);
}

fn hgNormLon(lon: f64): f64 {
    let cur: f64 = lon;
    while (cur > (180.0 as f64)) { cur = cur - (360.0 as f64); }
    while (cur < (-180.0 as f64)) { cur = cur + (360.0 as f64); }
    return cur;
}

/// Validate WGS-84 geographic coordinates (`-90 <= lat <= 90`, `-180 <= lon <= 180`, finite).
pub fn isValidCoord(p: LatLon): bool {
    if (p.lat != p.lat || p.lon != p.lon) {
        return false;
    }
    return p.lat >= (-90.0 as f64) && p.lat <= (90.0 as f64) && p.lon >= (-180.0 as f64) && p.lon <= (180.0 as f64);
}

/// Compute the spherical Haversine great-circle distance between `a` and `b` in meters.
pub fn distanceMeters(a: LatLon, b: LatLon): f64 {
    const lat1: f64 = hgDegToRad(a.lat);
    const lat2: f64 = hgDegToRad(b.lat);
    const dLat: f64 = hgDegToRad(b.lat - a.lat);
    const dLon: f64 = hgDegToRad(b.lon - a.lon);

    const sLat: f64 = hgSin(dLat * (0.5 as f64));
    const sLon: f64 = hgSin(dLon * (0.5 as f64));
    const h: f64 = hgClamp((sLat * sLat) + (hgCos(lat1) * hgCos(lat2) * sLon * sLon), 0.0 as f64, 1.0 as f64);
    const c: f64 = (2.0 as f64) * hgAtan2(hgSqrt(h), hgSqrt((1.0 as f64) - h));
    return EARTH_RADIUS_M * c;
}

/// Compute the spherical Haversine great-circle distance between `a` and `b` in kilometers.
pub fn distanceKm(a: LatLon, b: LatLon): f64 {
    return distanceMeters(a, b) / (1000.0 as f64);
}

/// Compute the spherical Haversine great-circle distance between `a` and `b` in statute miles.
pub fn distanceMiles(a: LatLon, b: LatLon): f64 {
    return distanceMeters(a, b) / (1609.344 as f64);
}

/// Compute the initial forward azimuth (bearing) from `a` to `b` in `[0.0, 360.0)` degrees clockwise from north.
pub fn initialBearing(a: LatLon, b: LatLon): f64 {
    const lat1: f64 = hgDegToRad(a.lat);
    const lat2: f64 = hgDegToRad(b.lat);
    const dLon: f64 = hgDegToRad(b.lon - a.lon);

    const y: f64 = hgSin(dLon) * hgCos(lat2);
    const x: f64 = (hgCos(lat1) * hgSin(lat2)) - (hgSin(lat1) * hgCos(lat2) * hgCos(dLon));
    let deg: f64 = hgRadToDeg(hgAtan2(y, x));
    while (deg < (0.0 as f64)) { deg = deg + (360.0 as f64); }
    while (deg >= (360.0 as f64)) { deg = deg - (360.0 as f64); }
    return deg;
}

/// Compute the spherical great-circle midpoint between `a` and `b`.
pub fn midpoint(a: LatLon, b: LatLon): LatLon {
    const lat1: f64 = hgDegToRad(a.lat);
    const lon1: f64 = hgDegToRad(a.lon);
    const lat2: f64 = hgDegToRad(b.lat);
    const dLon: f64 = hgDegToRad(b.lon - a.lon);

    const bx: f64 = hgCos(lat2) * hgCos(dLon);
    const by: f64 = hgCos(lat2) * hgSin(dLon);
    const sumX: f64 = hgCos(lat1) + bx;

    const lat3: f64 = hgAtan2(hgSin(lat1) + hgSin(lat2), hgSqrt((sumX * sumX) + (by * by)));
    const lon3: f64 = lon1 + hgAtan2(by, sumX);

    return LatLon{
        lat: hgRadToDeg(lat3),
        lon: hgNormLon(hgRadToDeg(lon3)),
    };
}

/// Compute the destination coordinate reached by traveling `distance_m` meters from `start`
/// along initial bearing `bearing_deg` degrees clockwise from north.
pub fn destinationPoint(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon {
    const delta: f64 = distance_m / EARTH_RADIUS_M;
    const theta: f64 = hgDegToRad(bearing_deg);
    const lat1: f64 = hgDegToRad(start.lat);
    const lon1: f64 = hgDegToRad(start.lon);

    const sinLat1: f64 = hgSin(lat1);
    const cosLat1: f64 = hgCos(lat1);
    const sinDelta: f64 = hgSin(delta);
    const cosDelta: f64 = hgCos(delta);

    const sinLat2: f64 = (sinLat1 * cosDelta) + (cosLat1 * sinDelta * hgCos(theta));
    const lat2: f64 = hgAsin(sinLat2);
    const y: f64 = hgSin(theta) * sinDelta * cosLat1;
    const x: f64 = cosDelta - (sinLat1 * sinLat2);
    const lon2: f64 = lon1 + hgAtan2(y, x);

    return LatLon{
        lat: hgRadToDeg(lat2),
        lon: hgNormLon(hgRadToDeg(lon2)),
    };
}

export {
    EARTH_RADIUS_KM,
    EARTH_RADIUS_M,
    LatLon,
    distanceMeters,
    distanceKm,
    distanceMiles,
    initialBearing,
    midpoint,
    destinationPoint,
    isValidCoord,
};

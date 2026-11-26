// @cap/geo-dist — Zero-allocation WGS-84 spherical geometry engine for CP
// Clean-room MIT implementation inspired by Rust `geo` / `haversine` and npm `geolib`.

const EARTH_RADIUS_M: f64 = 6371008.8;
const PI: f64 = 3.14159265358979323846;
const HALF_PI: f64 = 1.57079632679489661923;
const TWO_PI: f64 = 6.28318530717958647692;
const SQRT3: f64 = 1.73205080756887729352;
const TAN_PI_12: f64 = 0.26794919243112270647;
const PI_6: f64 = 0.52359877559829887307;

pub struct Coord {
    pub lat: f64,
    pub lon: f64,
}

pub struct Bounds {
    pub min_lat: f64,
    pub max_lat: f64,
    pub min_lon: f64,
    pub max_lon: f64,
}

fn toRad(deg: f64) -> f64 {
    return deg * (PI / 180.0);
}

fn toDeg(rad: f64) -> f64 {
    return rad * (180.0 / PI);
}

fn normalizeLon(lon_in: f64) -> f64 {
    let lon = lon_in;
    while (lon > 180.0) {
        lon -= 360.0;
    }
    while (lon < -180.0) {
        lon += 360.0;
    }
    return lon;
}

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 64) {
        const next = 0.5 * (g + (x / g));
        let diff = next - g;
        if (diff < 0.0) {
            diff = -diff;
        }
        g = next;
        if (diff <= 0.000000000000001 * g) {
            break;
        }
        i += 1;
    }
    return g;
}

fn sinF64(x_in: f64) -> f64 {
    let x = x_in;
    // Range reduction to [-PI, PI]
    const turns = ((x / TWO_PI) as i64) as f64;
    x -= turns * TWO_PI;
    while (x > PI) {
        x -= TWO_PI;
    }
    while (x < -PI) {
        x += TWO_PI;
    }
    // Fold to [-PI/2, PI/2]
    if (x > HALF_PI) {
        x = PI - x;
    } else if (x < -HALF_PI) {
        x = -PI - x;
    }
    // Taylor series up to x^21 / 21!
    const x2 = x * x;
    let term = x;
    let sum = x;
    let k: usize = 1;
    while (k <= 10) {
        const d1 = ((2 * k) as f64);
        const d2 = ((2 * k + 1) as f64);
        term = (-term * x2) / (d1 * d2);
        sum += term;
        k += 1;
    }
    return sum;
}

fn cosF64(x: f64) -> f64 {
    return sinF64(x + HALF_PI);
}

fn atanF64(x_in: f64) -> f64 {
    let neg = false;
    let x = x_in;
    if (x < 0.0) {
        neg = true;
        x = -x;
    }
    let invert = false;
    if (x > 1.0) {
        invert = true;
        x = 1.0 / x;
    }
    let shifted = false;
    if (x > TAN_PI_12) {
        shifted = true;
        x = ((x * SQRT3) - 1.0) / (x + SQRT3);
    }
    const x2 = x * x;
    let term = x;
    let sum = x;
    let k: usize = 1;
    while (k <= 12) {
        term = -term * x2;
        sum += term / (((2 * k) + 1) as f64);
        k += 1;
    }
    if (shifted) {
        sum += PI_6;
    }
    if (invert) {
        sum = HALF_PI - sum;
    }
    if (neg) {
        sum = -sum;
    }
    return sum;
}

fn atan2F64(y: f64, x: f64) -> f64 {
    if (x > 0.0) {
        return atanF64(y / x);
    }
    if (x < 0.0) {
        if (y >= 0.0) {
            return atanF64(y / x) + PI;
        }
        return atanF64(y / x) - PI;
    }
    if (y > 0.0) {
        return HALF_PI;
    }
    if (y < 0.0) {
        return -HALF_PI;
    }
    return 0.0;
}

fn asinF64(x: f64) -> f64 {
    if (x >= 1.0) {
        return HALF_PI;
    }
    if (x <= -1.0) {
        return -HALF_PI;
    }
    return atan2F64(x, sqrtF64(1.0 - (x * x)));
}

pub fn haversineMeters(a: Coord, b: Coord) -> f64 {
    const lat1 = toRad(a.lat);
    const lat2 = toRad(b.lat);
    const dLat = toRad(b.lat - a.lat);
    const dLon = toRad(b.lon - a.lon);

    const sLat = sinF64(dLat * 0.5);
    const sLon = sinF64(dLon * 0.5);
    let h = (sLat * sLat) + (cosF64(lat1) * cosF64(lat2) * sLon * sLon);
    if (h < 0.0) {
        h = 0.0;
    }
    if (h > 1.0) {
        h = 1.0;
    }
    const c = 2.0 * atan2F64(sqrtF64(h), sqrtF64(1.0 - h));
    return EARTH_RADIUS_M * c;
}

pub fn initialBearing(a: Coord, b: Coord) -> f64 {
    const lat1 = toRad(a.lat);
    const lat2 = toRad(b.lat);
    const dLon = toRad(b.lon - a.lon);

    const y = sinF64(dLon) * cosF64(lat2);
    const x = (cosF64(lat1) * sinF64(lat2)) - (sinF64(lat1) * cosF64(lat2) * cosF64(dLon));
    let deg = toDeg(atan2F64(y, x));
    while (deg < 0.0) {
        deg += 360.0;
    }
    while (deg >= 360.0) {
        deg -= 360.0;
    }
    return deg;
}

pub fn destinationPoint(origin: Coord, bearing_deg: f64, distance_m: f64) -> Coord {
    const ang = distance_m / EARTH_RADIUS_M;
    const brng = toRad(bearing_deg);
    const lat1 = toRad(origin.lat);
    const lon1 = toRad(origin.lon);

    const sinLat1 = sinF64(lat1);
    const cosLat1 = cosF64(lat1);
    const sinAng = sinF64(ang);
    const cosAng = cosF64(ang);

    const sinLat2 = (sinLat1 * cosAng) + (cosLat1 * sinAng * cosF64(brng));
    const lat2 = asinF64(sinLat2);
    const lon2 = lon1 + atan2F64(sinF64(brng) * sinAng * cosLat1, cosAng - (sinLat1 * sinLat2));

    return Coord{
        lat: toDeg(lat2),
        lon: normalizeLon(toDeg(lon2)),
    };
}

pub fn midpoint(a: Coord, b: Coord) -> Coord {
    const lat1 = toRad(a.lat);
    const lon1 = toRad(a.lon);
    const lat2 = toRad(b.lat);
    const dLon = toRad(b.lon - a.lon);

    const cosLat1 = cosF64(lat1);
    const cosLat2 = cosF64(lat2);
    const bx = cosLat2 * cosF64(dLon);
    const by = cosLat2 * sinF64(dLon);

    const cx = cosLat1 + bx;
    const midLat = atan2F64(sinF64(lat1) + sinF64(lat2), sqrtF64((cx * cx) + (by * by)));
    const midLon = lon1 + atan2F64(by, cx);

    return Coord{
        lat: toDeg(midLat),
        lon: normalizeLon(toDeg(midLon)),
    };
}

pub fn boundingBox(center: Coord, radius_m: f64) -> Bounds {
    let r = radius_m;
    if (r < 0.0) {
        r = 0.0;
    }
    const radDist = r / EARTH_RADIUS_M;
    const deltaLat = toDeg(radDist);
    let minLat = center.lat - deltaLat;
    let maxLat = center.lat + deltaLat;

    if (minLat <= -90.0 || maxLat >= 90.0) {
        if (minLat < -90.0) {
            minLat = -90.0;
        }
        if (maxLat > 90.0) {
            maxLat = 90.0;
        }
        return Bounds{
            min_lat: minLat,
            max_lat: maxLat,
            min_lon: -180.0,
            max_lon: 180.0,
        };
    }

    const cosLat = cosF64(toRad(center.lat));
    let ratio: f64 = 1.0;
    if (cosLat > 0.000000000001) {
        ratio = sinF64(radDist) / cosLat;
    }
    if (ratio >= 1.0) {
        return Bounds{
            min_lat: minLat,
            max_lat: maxLat,
            min_lon: -180.0,
            max_lon: 180.0,
        };
    }
    const deltaLon = toDeg(asinF64(ratio));
    let minLon = center.lon - deltaLon;
    let maxLon = center.lon + deltaLon;
    if (minLon < -180.0) {
        minLon = -180.0;
    }
    if (maxLon > 180.0) {
        maxLon = 180.0;
    }

    return Bounds{
        min_lat: minLat,
        max_lat: maxLat,
        min_lon: minLon,
        max_lon: maxLon,
    };
}

pub fn pointInPolygon(pt: Coord, poly: []const Coord) -> bool {
    if (poly.len < 3) {
        return false;
    }
    let inside = false;
    let j = poly.len - (1 as usize);
    let i: usize = 0;
    while (i < poly.len) {
        const yi = poly[i].lat;
        const xi = poly[i].lon;
        const yj = poly[j].lat;
        const xj = poly[j].lon;

        const crossesLat = (yi > pt.lat) != (yj > pt.lat);
        if (crossesLat) {
            const intersectLon = (((xj - xi) * (pt.lat - yi)) / (yj - yi)) + xi;
            if (pt.lon < intersectLon) {
                inside = !inside;
            }
        }
        j = i;
        i += 1;
    }
    return inside;
}

export {
    Coord,
    Bounds,
    haversineMeters,
    initialBearing,
    destinationPoint,
    midpoint,
    boundingBox,
    pointInPolygon,
};

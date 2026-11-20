// Source: cp/lib/geo/coord.cp (Geodesic Haversine, Bearing, BBox & Point-in-Polygon)
// Pure CP geodesic coordinates, WGS-84 spherical Haversine distance, bearing, bounding box,
// and ray-casting point-in-polygon geofence predicates.

const std = @import("std");

/// Mean Earth radius in meters (IUGG / WGS-84 arithmetic mean radius).
pub const EARTH_RADIUS_M: f64 = 6371008.8;

const DEG_TO_RAD: f64 = std.math.pi / 180.0;
const RAD_TO_DEG: f64 = 180.0 / std.math.pi;
const BOUNDARY_EPS: f64 = 0.000000000001;

/// Error set shared across `cap:geo` spatial and geohash operations.
pub err GeoError {
    InvalidCoordinate,
    InvalidGeohash,
    InvalidPrecision,
    OutOfMemory,
}

fn isFiniteF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn clampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

fn wrapLngDeg(lng: f64): f64 {
    if (!isFiniteF64(lng)) return 0.0;
    if (lng >= -180.0 && lng <= 180.0) return lng;
    let wrapped = @mod(lng + 180.0, 360.0);
    if (wrapped < 0.0) wrapped += 360.0;
    return wrapped - 180.0;
}

fn pointOnSegment(px: f64, py: f64, ax: f64, ay: f64, bx: f64, by: f64): bool {
    const min_x = if (ax < bx) ax else bx;
    const max_x = if (ax > bx) ax else bx;
    const min_y = if (ay < by) ay else by;
    const max_y = if (ay > by) ay else by;

    if (px < min_x - BOUNDARY_EPS || px > max_x + BOUNDARY_EPS) return false;
    if (py < min_y - BOUNDARY_EPS || py > max_y + BOUNDARY_EPS) return false;

    const dx = bx - ax;
    const dy = by - ay;
    const len_sq = dx * dx + dy * dy;
    if (len_sq <= BOUNDARY_EPS * BOUNDARY_EPS) {
        return @abs(px - ax) <= BOUNDARY_EPS && @abs(py - ay) <= BOUNDARY_EPS;
    }

    const cross = (px - ax) * dy - (py - ay) * dx;
    return (cross * cross) <= (BOUNDARY_EPS * BOUNDARY_EPS) * len_sq;
}

/// Geographic coordinate in decimal degrees (`lat` in `[-90, 90]`, `lng` in `[-180, 180]`).
pub struct LatLng {
    pub lat: f64 = 0.0;
    pub lng: f64 = 0.0;

    /// Constructs and validates a `LatLng` coordinate, rejecting NaN, Inf, and out-of-range values.
    pub static fn init(lat: f64, lng: f64): LatLng ! GeoError {
        if (!isFiniteF64(lat) || !isFiniteF64(lng)) {
            return GeoError.InvalidCoordinate;
        }
        if (lat < -90.0 || lat > 90.0 || lng < -180.0 || lng > 180.0) {
            return GeoError.InvalidCoordinate;
        }
        return LatLng { lat: lat, lng: lng };
    }

    /// Returns `true` if `lat` and `lng` are finite and within valid WGS-84 bounds.
    pub fn isValid(): bool {
        if (!isFiniteF64(_.lat) || !isFiniteF64(_.lng)) return false;
        return _.lat >= -90.0 && _.lat <= 90.0 && _.lng >= -180.0 && _.lng <= 180.0;
    }

    /// Returns `true` if both coordinates agree within `eps_deg` decimal degrees.
    pub fn approxEql(other: LatLng, eps_deg: f64 = 0.000001): bool {
        return @abs(_.lat - other.lat) <= eps_deg && @abs(_.lng - other.lng) <= eps_deg;
    }
}

/// Axis-aligned geographic bounding box in decimal degrees.
pub struct BBox {
    pub min_lat: f64 = 0.0;
    pub max_lat: f64 = 0.0;
    pub min_lng: f64 = 0.0;
    pub max_lng: f64 = 0.0;

    /// Constructs and validates a bounding box with `min_lat <= max_lat` and `min_lng <= max_lng`.
    pub static fn init(min_lat: f64, max_lat: f64, min_lng: f64, max_lng: f64): BBox ! GeoError {
        if (!isFiniteF64(min_lat) || !isFiniteF64(max_lat) || !isFiniteF64(min_lng) || !isFiniteF64(max_lng)) {
            return GeoError.InvalidCoordinate;
        }
        if (min_lat < -90.0 || max_lat > 90.0 || min_lng < -180.0 || max_lng > 180.0) {
            return GeoError.InvalidCoordinate;
        }
        if (min_lat > max_lat || min_lng > max_lng) {
            return GeoError.InvalidCoordinate;
        }
        return BBox {
            min_lat: min_lat,
            max_lat: max_lat,
            min_lng: min_lng,
            max_lng: max_lng,
        };
    }

    /// Constructs a degenerate zero-area bounding box around a single coordinate.
    pub static fn fromPoint(pt: LatLng): BBox ! GeoError {
        if (!pt.isValid()) return GeoError.InvalidCoordinate;
        return BBox {
            min_lat: pt.lat,
            max_lat: pt.lat,
            min_lng: pt.lng,
            max_lng: pt.lng,
        };
    }

    /// Returns `true` if `pt` lies inside or on the boundary of this bounding box.
    pub fn contains(pt: LatLng): bool {
        return pt.lat >= _.min_lat && pt.lat <= _.max_lat && pt.lng >= _.min_lng && pt.lng <= _.max_lng;
    }

    /// Returns `true` if this bounding box overlaps or touches `other`.
    pub fn intersects(other: BBox): bool {
        return _.min_lat <= other.max_lat &&
            _.max_lat >= other.min_lat &&
            _.min_lng <= other.max_lng &&
            _.max_lng >= other.min_lng;
    }

    /// Expands this bounding box in place to enclose `pt`.
    pub fn expand(*_, pt: LatLng) {
        if (!pt.isValid()) return;
        if (pt.lat < _.min_lat) _.min_lat = pt.lat;
        if (pt.lat > _.max_lat) _.max_lat = pt.lat;
        if (pt.lng < _.min_lng) _.min_lng = pt.lng;
        if (pt.lng > _.max_lng) _.max_lng = pt.lng;
    }

    /// Returns the geometric center point of this bounding box.
    pub fn center(): LatLng {
        return LatLng {
            lat: (_.min_lat + _.max_lat) * 0.5,
            lng: (_.min_lng + _.max_lng) * 0.5,
        };
    }
}

/// Geodesic spherical distance, bearing, destination projection, and polygon containment utilities.
pub static struct Geo {
    /// Computes great-circle distance in meters using the Haversine formula (`R = 6371008.8m`).
    pub static fn haversineMeters(a: LatLng, b: LatLng): f64 {
        const lat1 = a.lat * DEG_TO_RAD;
        const lat2 = b.lat * DEG_TO_RAD;
        const d_lat = (b.lat - a.lat) * DEG_TO_RAD;
        const d_lng = (b.lng - a.lng) * DEG_TO_RAD;

        const sin_dlat = @sin(d_lat * 0.5);
        const sin_dlng = @sin(d_lng * 0.5);

        const h_raw = sin_dlat * sin_dlat + @cos(lat1) * @cos(lat2) * sin_dlng * sin_dlng;
        const h = clampF64(h_raw, 0.0, 1.0);
        const c = 2.0 * std.math.atan2(@sqrt(h), @sqrt(1.0 - h));
        return EARTH_RADIUS_M * c;
    }

    /// Computes initial forward azimuth (bearing) from `a` to `b` in degrees within `[0.0, 360.0)`.
    pub static fn initialBearingDeg(a: LatLng, b: LatLng): f64 {
        if (a.lat == b.lat && a.lng == b.lng) return 0.0;

        const lat1 = a.lat * DEG_TO_RAD;
        const lat2 = b.lat * DEG_TO_RAD;
        const d_lng = (b.lng - a.lng) * DEG_TO_RAD;

        const y = @sin(d_lng) * @cos(lat2);
        const x = @cos(lat1) * @sin(lat2) - @sin(lat1) * @cos(lat2) * @cos(d_lng);
        const theta = std.math.atan2(y, x);
        let deg = @mod(theta * RAD_TO_DEG + 360.0, 360.0);
        if (deg >= 360.0) deg = 0.0;
        return deg;
    }

    /// Projects a destination coordinate from `origin` along `bearing_deg` by `dist_m` meters.
    pub static fn destinationPoint(origin: LatLng, bearing_deg: f64, dist_m: f64): LatLng {
        if (dist_m == 0.0) return origin;

        const lat1 = origin.lat * DEG_TO_RAD;
        const lng1 = origin.lng * DEG_TO_RAD;
        const theta = bearing_deg * DEG_TO_RAD;
        const delta = dist_m / EARTH_RADIUS_M;

        const sin_lat1 = @sin(lat1);
        const cos_lat1 = @cos(lat1);
        const sin_delta = @sin(delta);
        const cos_delta = @cos(delta);

        const sin_lat2 = clampF64(
            sin_lat1 * cos_delta + cos_lat1 * sin_delta * @cos(theta),
            -1.0,
            1.0,
        );
        const lat2 = std.math.asin(sin_lat2);
        const y = @sin(theta) * sin_delta * cos_lat1;
        const x = cos_delta - sin_lat1 * sin_lat2;
        const lng2 = lng1 + std.math.atan2(y, x);

        return LatLng {
            lat: clampF64(lat2 * RAD_TO_DEG, -90.0, 90.0),
            lng: wrapLngDeg(lng2 * RAD_TO_DEG),
        };
    }

    /// Determines whether `pt` lies inside or on the boundary of polygon `ring` via ray-casting.
    /// Supports both open (`ring[0] != ring[n-1]`) and closed (`ring[0] == ring[n-1]`) vertex rings.
    pub static fn pointInPolygon(pt: LatLng, ring: []const LatLng): bool {
        if (ring.len < 3) return false;

        let inside = false;
        let j = ring.len - 1;
        let i: usize = 0;
        while (i < ring.len) : (i += 1) {
            const xi = ring[i].lng;
            const yi = ring[i].lat;
            const xj = ring[j].lng;
            const yj = ring[j].lat;

            if (pointOnSegment(pt.lng, pt.lat, xi, yi, xj, yj)) {
                return true;
            }

            const crosses_y = (yi > pt.lat) != (yj > pt.lat);
            if (crosses_y) {
                const intersect_x = (xj - xi) * (pt.lat - yi) / (yj - yi) + xi;
                if (pt.lng < intersect_x) {
                    inside = !inside;
                }
            }
            j = i;
        }
        return inside;
    }
}

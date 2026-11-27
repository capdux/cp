// @cap/polyline — Zero-heap Google Encoded Polyline Algorithm Format
// Clean-room MIT implementation for CP (compatible with @mapbox/polyline & Rust polyline).

pub struct LatLng {
    pub lat: f64,
    pub lng: f64,
}

fn precisionScale(precision: u32): f64 {
    let scale: f64 = 1.0;
    let i: u32 = 0;
    let p: u32 = precision;
    if (p > 9) {
        p = 9;
    }
    while (i < p) {
        scale = scale * 10.0;
        i += 1;
    }
    return scale;
}

fn roundToI64(v: f64): i64 {
    if (v >= 0.0) {
        return (v + 0.5) as i64;
    }
    return (v - 0.5) as i64;
}

/// Encode a single signed integer delta using Google Polyline variable-length 5-bit chunks.
/// Returns the number of bytes written to `out`, or 0 if `out` is too small.
pub fn encodeSigned(val: i64, out: []u8): usize {
    let shifted: u64 = 0;
    if (val < 0) {
        let neg_abs: u64 = (0 - val) as u64;
        shifted = (neg_abs << 1) - 1;
    } else {
        shifted = (val as u64) << 1;
    }

    let pos: usize = 0;
    while (shifted >= 0x20) {
        if (pos >= out.len) {
            return 0;
        }
        let chunk: u8 = (((shifted & 0x1f) | 0x20) + 63) as u8;
        out[pos] = chunk;
        pos += 1;
        shifted = shifted >> 5;
    }
    if (pos >= out.len) {
        return 0;
    }
    out[pos] = ((shifted + 63) as u8);
    pos += 1;
    return pos;
}

/// Decode one signed integer delta starting at `data[start..]`.
/// Writes decoded integer into `out_val` and returns bytes consumed (0 on malformed/truncated input).
pub fn decodeSigned(data: string, start: usize, out_val: *i64): usize {
    if (start >= data.len) {
        return 0;
    }
    let result: u64 = 0;
    let shift: u6 = 0;
    let i: usize = start;

    while (i < data.len) {
        let raw: u8 = data[i];
        if (raw < 63 || raw > 126) {
            return 0;
        }
        let b: u64 = (raw - 63) as u64;
        i += 1;
        result = result | ((b & 0x1f) << shift);
        if ((b & 0x20) == 0) {
            if ((result & 1) != 0) {
                let mag: u64 = (result >> 1) + 1;
                out_val.* = 0 - (mag as i64);
            } else {
                out_val.* = (result >> 1) as i64;
            }
            return i - start;
        }
        if (shift >= 55) {
            return 0;
        }
        shift += 5;
    }
    return 0;
}

/// Encode a slice of `LatLng` points into Google Encoded Polyline format.
/// Use `precision = 5` (Google Maps / Mapbox default) or `precision = 6` (OSRM / Valhalla).
/// Returns bytes written to `out` (0 on empty input or insufficient buffer).
pub fn encodePoints(points: []const LatLng, precision: u32, out: []u8): usize {
    if (points.len == 0) {
        return 0;
    }
    let scale: f64 = precisionScale(precision);
    let prev_lat: i64 = 0;
    let prev_lng: i64 = 0;
    let pos: usize = 0;
    let i: usize = 0;

    while (i < points.len) {
        let cur_lat: i64 = roundToI64(points[i].lat * scale);
        let cur_lng: i64 = roundToI64(points[i].lng * scale);
        let dlat: i64 = cur_lat - prev_lat;
        let dlng: i64 = cur_lng - prev_lng;

        let n1: usize = encodeSigned(dlat, out[pos..]);
        if (n1 == 0) {
            return 0;
        }
        pos += n1;

        let n2: usize = encodeSigned(dlng, out[pos..]);
        if (n2 == 0) {
            return 0;
        }
        pos += n2;

        prev_lat = cur_lat;
        prev_lng = cur_lng;
        i += 1;
    }
    return pos;
}

/// Decode a Google Encoded Polyline string into `out`.
/// Returns number of `LatLng` points decoded.
pub fn decodePoints(encoded: string, precision: u32, out: []LatLng): usize {
    let scale: f64 = precisionScale(precision);
    let pos: usize = 0;
    let count: usize = 0;
    let cur_lat: i64 = 0;
    let cur_lng: i64 = 0;

    while (pos < encoded.len && count < out.len) {
        let dlat: i64 = 0;
        let c1: usize = decodeSigned(encoded, pos, &dlat);
        if (c1 == 0) {
            return count;
        }
        pos += c1;

        let dlng: i64 = 0;
        let c2: usize = decodeSigned(encoded, pos, &dlng);
        if (c2 == 0) {
            return count;
        }
        pos += c2;

        cur_lat += dlat;
        cur_lng += dlng;
        out[count] = LatLng {
            lat: (cur_lat as f64) / scale,
            lng: (cur_lng as f64) / scale,
        };
        count += 1;
    }
    return count;
}

/// Count how many valid coordinate pairs are contained in an encoded polyline string.
pub fn countPoints(encoded: string): usize {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < encoded.len) {
        let tmp: i64 = 0;
        let c1: usize = decodeSigned(encoded, pos, &tmp);
        if (c1 == 0) {
            return 0;
        }
        pos += c1;
        let c2: usize = decodeSigned(encoded, pos, &tmp);
        if (c2 == 0) {
            return 0;
        }
        pos += c2;
        count += 1;
    }
    return count;
}

/// Validate that an encoded polyline string contains well-formed complete coordinate pairs.
pub fn isValid(encoded: string): bool {
    if (encoded.len == 0) {
        return true;
    }
    let pos: usize = 0;
    while (pos < encoded.len) {
        let tmp: i64 = 0;
        let c1: usize = decodeSigned(encoded, pos, &tmp);
        if (c1 == 0) {
            return false;
        }
        pos += c1;
        let c2: usize = decodeSigned(encoded, pos, &tmp);
        if (c2 == 0) {
            return false;
        }
        pos += c2;
    }
    return true;
}

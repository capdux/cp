// packages/@cap/geo-uri/src/index.cp
// Zero-allocation RFC 5870 geo: URI parser, validator & serializer.
// MIT clean-room pure CP implementation.

/// Parsed RFC 5870 `geo:` URI components.
pub struct GeoUri {
    pub lat: f64,
    pub lng: f64,
    pub has_alt: bool,
    pub alt: f64,
    pub has_uncertainty: bool,
    pub uncertainty: f64,
    pub crs: string,
    pub valid: bool,
}

fn guToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn guEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (guToLower(a[i]) != guToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parse an RFC 5870 decimal number (`["-"] 1*DIGIT ["." 1*DIGIT]`).
fn guParseF64(s: []const u8, out: *f64): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == ('-' as u8)) {
        neg = true;
        i = 1;
    }
    if (i >= s.len) {
        return false;
    }

    let intPart: f64 = 0.0;
    let intDigits: usize = 0;
    while (i < s.len && s[i] >= ('0' as u8) && s[i] <= ('9' as u8)) {
        intPart = intPart * (10.0 as f64) + ((s[i] - ('0' as u8)) as f64);
        intDigits += 1;
        i += 1;
    }
    if (intDigits == 0) {
        return false;
    }

    let fracPart: f64 = 0.0;
    if (i < s.len) {
        if (s[i] != ('.' as u8)) {
            return false;
        }
        i += 1;
        let fracDigits: usize = 0;
        let scale: f64 = 0.1;
        while (i < s.len) {
            const c = s[i];
            if (c < ('0' as u8) || c > ('9' as u8)) {
                return false;
            }
            fracPart = fracPart + ((c - ('0' as u8)) as f64) * scale;
            scale = scale * (0.1 as f64);
            fracDigits += 1;
            i += 1;
        }
        if (fracDigits == 0) {
            return false;
        }
    }

    let val: f64 = intPart + fracPart;
    if (neg) {
        val = -val;
    }
    out.* = val;
    return true;
}

fn guWriteRaw(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn guWriteU64(out: []u8, pos: *usize, val: u64): bool {
    if (val == (0 as u64)) {
        return guWriteRaw(out, pos, "0");
    }
    let rev: [24]u8 = undefined;
    let n: usize = 0;
    let cur = val;
    while (cur > (0 as u64)) {
        rev[n] = ('0' as u8) + ((cur % (10 as u64)) as u8);
        n += 1;
        cur = cur / (10 as u64);
    }
    if (pos.* + n > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.*] = rev[n - 1 - i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

/// Format a finite `f64` with up to 6 fractional digits (trimming trailing zeros).
fn guWriteF64(out: []u8, pos: *usize, val: f64): bool {
    if (val != val) {
        return false;
    }
    let neg = false;
    let absVal: f64 = val;
    if (absVal < (0.0 as f64)) {
        neg = true;
        absVal = -absVal;
    }
    if (absVal > (1000000000.0 as f64)) {
        return false;
    }

    const scaled: u64 = ((absVal * (1000000.0 as f64)) + (0.5 as f64)) as u64;
    if (neg && scaled > (0 as u64)) {
        if (!guWriteRaw(out, pos, "-")) {
            return false;
        }
    }

    const intPart: u64 = scaled / (1000000 as u64);
    const fracPart: u64 = scaled % (1000000 as u64);

    if (!guWriteU64(out, pos, intPart)) {
        return false;
    }

    if (fracPart > (0 as u64)) {
        if (!guWriteRaw(out, pos, ".")) {
            return false;
        }
        let fbuf: [6]u8 = undefined;
        let rem = fracPart;
        let d: usize = 6;
        while (d > 0) {
            d -= 1;
            fbuf[d] = ('0' as u8) + ((rem % (10 as u64)) as u8);
            rem = rem / (10 as u64);
        }
        let end: usize = 6;
        while (end > 0 && fbuf[end - 1] == ('0' as u8)) {
            end -= 1;
        }
        if (!guWriteRaw(out, pos, fbuf[0..end])) {
            return false;
        }
    }

    return true;
}

/// Validate WGS-84 latitude (`-90.0..=90.0`) and longitude (`-180.0..=180.0`).
pub fn isValidLatLng(lat: f64, lng: f64): bool {
    if (lat != lat || lng != lng) {
        return false;
    }
    if (lat < (-90.0 as f64) || lat > (90.0 as f64)) {
        return false;
    }
    if (lng < (-180.0 as f64) || lng > (180.0 as f64)) {
        return false;
    }
    return true;
}

/// Parse an RFC 5870 `geo:` URI (e.g. `geo:37.786971,-122.399677,15.5;crs=wgs84;u=35`).
/// Defaults `crs` to `"wgs84"`. Enforces `-90 <= lat <= 90`, `-180 <= lng <= 180`, and `u >= 0`.
pub fn parseGeoUri(uri: string): GeoUri {
    let res = GeoUri{
        lat: 0.0 as f64,
        lng: 0.0 as f64,
        has_alt: false,
        alt: 0.0 as f64,
        has_uncertainty: false,
        uncertainty: 0.0 as f64,
        crs: "wgs84",
        valid: false,
    };

    if (uri.len < 7 || !guEqIgnoreCase(uri[0..4], "geo:")) {
        return res;
    }

    const body = uri[4..uri.len];

    // Split coordinates from optional ';params'.
    let semi = body.len;
    let si: usize = 0;
    while (si < body.len) {
        if (body[si] == (';' as u8)) {
            semi = si;
            break;
        }
        si += 1;
    }

    const coords = body[0..semi];
    if (coords.len == 0) {
        return res;
    }

    // Split coords into 2 or 3 comma-separated fields: lat,lng[,alt].
    let c1 = coords.len;
    let c2 = coords.len;
    let ci: usize = 0;
    while (ci < coords.len) {
        if (coords[ci] == (',' as u8)) {
            if (c1 == coords.len) {
                c1 = ci;
            } else if (c2 == coords.len) {
                c2 = ci;
            } else {
                // More than 3 coordinate components is invalid.
                return res;
            }
        }
        ci += 1;
    }

    if (c1 == coords.len) {
        return res;
    }

    let latVal: f64 = 0.0;
    let lngVal: f64 = 0.0;
    if (!guParseF64(coords[0..c1], &latVal)) {
        return res;
    }

    if (c2 == coords.len) {
        if (!guParseF64(coords[(c1 + 1)..coords.len], &lngVal)) {
            return res;
        }
    } else {
        if (!guParseF64(coords[(c1 + 1)..c2], &lngVal)) {
            return res;
        }
        let altVal: f64 = 0.0;
        if (!guParseF64(coords[(c2 + 1)..coords.len], &altVal)) {
            return res;
        }
        res.has_alt = true;
        res.alt = altVal;
    }

    if (!isValidLatLng(latVal, lngVal)) {
        return res;
    }
    res.lat = latVal;
    res.lng = lngVal;

    // Parse optional ';'-delimited parameters (`;crs=...`, `;u=...`).
    let pos = semi;
    while (pos < body.len) {
        pos += 1; // skip ';'
        let nextSemi = pos;
        while (nextSemi < body.len && body[nextSemi] != (';' as u8)) {
            nextSemi += 1;
        }
        const seg = body[pos..nextSemi];
        pos = nextSemi;

        if (seg.len == 0) {
            return res;
        }

        let eq: usize = 0;
        while (eq < seg.len && seg[eq] != ('=' as u8)) {
            eq += 1;
        }
        if (eq == 0 || eq + 1 >= seg.len) {
            return res;
        }

        const key = seg[0..eq];
        const val = seg[(eq + 1)..seg.len];

        if (guEqIgnoreCase(key, "crs")) {
            res.crs = val;
        } else if (guEqIgnoreCase(key, "u")) {
            let uVal: f64 = 0.0;
            if (!guParseF64(val, &uVal) || uVal < (0.0 as f64)) {
                return res;
            }
            res.uncertainty = uVal;
            res.has_uncertainty = true;
        }
    }

    res.valid = true;
    return res;
}

/// Format coordinates into a standard RFC 5870 `geo:lat,lng[,alt][;u=...]` URI in `out`.
/// Returns bytes written, or `0` on out-of-range coordinates, negative uncertainty, or insufficient `out` buffer.
pub fn formatGeoUri(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize {
    if (!isValidLatLng(lat, lng)) {
        return 0;
    }
    if (has_alt && alt != alt) {
        return 0;
    }
    if (has_u && (u != u || u < (0.0 as f64))) {
        return 0;
    }

    let pos: usize = 0;
    if (!guWriteRaw(out, &pos, "geo:")) {
        return 0;
    }
    if (!guWriteF64(out, &pos, lat)) {
        return 0;
    }
    if (!guWriteRaw(out, &pos, ",")) {
        return 0;
    }
    if (!guWriteF64(out, &pos, lng)) {
        return 0;
    }
    if (has_alt) {
        if (!guWriteRaw(out, &pos, ",")) {
            return 0;
        }
        if (!guWriteF64(out, &pos, alt)) {
            return 0;
        }
    }
    if (has_u) {
        if (!guWriteRaw(out, &pos, ";u=")) {
            return 0;
        }
        if (!guWriteF64(out, &pos, u)) {
            return 0;
        }
    }
    return pos;
}

export {
    GeoUri,
    parseGeoUri,
    formatGeoUri,
    isValidLatLng,
};

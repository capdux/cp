// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Google Open Location Code / Plus Codes Geocoder)

pub const OLC_ALPHABET: []const u8 = "23456789CFGHJMPQRVWX";
pub const OLC_SEPARATOR: u8 = '+';
pub const OLC_SEPARATOR_POS: usize = 8;
pub const OLC_PADDING: u8 = '0';
pub const OLC_DEFAULT_LEN: usize = 10;
pub const OLC_MAX_LEN: usize = 15;

const FINAL_LAT_PRECISION: u64 = 25000000; // 8000 * 5^5
const FINAL_LNG_PRECISION: u64 = 8192000;  // 8000 * 4^5
const MAX_LAT_INT: u64 = (180 as u64) * FINAL_LAT_PRECISION;
const MAX_LNG_INT: u64 = (360 as u64) * FINAL_LNG_PRECISION;

/// Decoded bounding box of a Google Open Location Code (Plus Code).
pub struct OlcArea {
    lat_lo: f64,
    lng_lo: f64,
    lat_hi: f64,
    lng_hi: f64,
    code_len: usize,

    /// Returns the center latitude of the decoded cell (clamped to `<= 90.0`).
    fn latCenter(): f64 {
        const c: f64 = _.lat_lo + (_.lat_hi - _.lat_lo) * 0.5;
        if (c > 90.0) {
            return 90.0;
        }
        return c;
    }

    /// Returns the center longitude of the decoded cell (clamped to `<= 180.0`).
    fn lngCenter(): f64 {
        const c: f64 = _.lng_lo + (_.lng_hi - _.lng_lo) * 0.5;
        if (c > 180.0) {
            return 180.0;
        }
        return c;
    }
}

fn toUpperAscii(ch: u8): u8 {
    if (ch >= 'a' && ch <= 'z') {
        return ch - ('a' as u8) + ('A' as u8);
    }
    return ch;
}

fn decodeBase20(ch: u8): u8? {
    const u: u8 = toUpperAscii(ch);
    let i: usize = 0;
    while (i < 20) {
        if (OLC_ALPHABET[i] == u) {
            return i as u8;
        }
        i += 1;
    }
    return null;
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && (v > -1.0e308) && (v < 1.0e308);
}

fn clipLatitude(lat: f64): f64 {
    if (lat < -90.0) {
        return -90.0;
    }
    if (lat > 90.0) {
        return 90.0;
    }
    return lat;
}

fn normalizeLongitude(lng: f64): f64 {
    let x: f64 = lng;
    while (x < -180.0) {
        x += 360.0;
    }
    while (x >= 180.0) {
        x -= 360.0;
    }
    return x;
}

fn computeLatPrecision(code_len: usize): f64 {
    if (code_len <= 2) {
        return 20.0;
    }
    if (code_len <= 4) {
        return 1.0;
    }
    if (code_len <= 6) {
        return 0.05;
    }
    if (code_len <= 8) {
        return 0.0025;
    }
    let prec: f64 = 0.000125;
    let extra: usize = 10;
    while (extra < code_len) {
        prec /= 5.0;
        extra += 1;
    }
    return prec;
}

fn findSeparator(code: []const u8): usize? {
    let found: usize = code.len;
    let i: usize = 0;
    while (i < code.len) {
        if (code[i] == OLC_SEPARATOR) {
            if (found != code.len) {
                return null; // Multiple '+' separators are invalid.
            }
            found = i;
        }
        i += 1;
    }
    if (found == code.len) {
        return null;
    }
    return found;
}

/// Validates whether `code` conforms to the Google Open Location Code (Plus Codes) specification.
/// Accepts both uppercase and lowercase Base-20 characters.
pub fn isValidOlc(code: []const u8): bool {
    if (code.len < 2) {
        return false;
    }

    let sep_pos: usize = 0;
    if (sp := findSeparator(code)) {
        sep_pos = sp;
    } else {
        return false;
    }

    // '+' must appear at an even index <= 8.
    if (sep_pos > OLC_SEPARATOR_POS || (sep_pos & 1) != 0) {
        return false;
    }

    // Cannot have exactly 1 character after '+'.
    const after_sep: usize = code.len - sep_pos - 1;
    if (after_sep == 1) {
        return false;
    }

    // Check padding ('0') rules.
    let first_pad: usize = code.len;
    let i: usize = 0;
    while (i < code.len) {
        if (code[i] == OLC_PADDING) {
            if (first_pad == code.len) {
                first_pad = i;
            }
        }
        i += 1;
    }

    if (first_pad != code.len) {
        // Short codes cannot contain padding; padded codes must end immediately with '+' at index 8.
        if (sep_pos < OLC_SEPARATOR_POS || after_sep > 0) {
            return false;
        }
        // First '0' must be at an even index >= 2 (i.e. 2, 4, or 6).
        if (first_pad < 2 || (first_pad & 1) != 0 || first_pad >= sep_pos) {
            return false;
        }
        // All characters from first_pad up to sep_pos - 1 must be '0'.
        let p: usize = first_pad;
        while (p < sep_pos) {
            if (code[p] != OLC_PADDING) {
                return false;
            }
            p += 1;
        }
    }

    // Count effective Base-20 digits and validate every non-'+' / non-'0' character.
    let eff_len: usize = 0;
    let k: usize = 0;
    while (k < code.len) {
        const ch: u8 = code[k];
        if (ch != OLC_SEPARATOR && ch != OLC_PADDING) {
            if (_ := decodeBase20(ch)) {
                eff_len += 1;
            } else {
                return false;
            }
        }
        k += 1;
    }

    if (eff_len < 2 || eff_len > OLC_MAX_LEN) {
        return false;
    }
    return true;
}

/// Returns true if `code` is a valid shortened Open Location Code (`+` at even position < 8).
pub fn isShortOlc(code: []const u8): bool {
    if (!isValidOlc(code)) {
        return false;
    }
    if (sp := findSeparator(code)) {
        return sp < OLC_SEPARATOR_POS;
    }
    return false;
}

/// Returns true if `code` is a valid full Open Location Code (`+` at index 8 and in-range first pair).
pub fn isFullOlc(code: []const u8): bool {
    if (!isValidOlc(code)) {
        return false;
    }
    if (sp := findSeparator(code)) {
        if (sp != OLC_SEPARATOR_POS) {
            return false;
        }
    } else {
        return false;
    }

    // First char (latitude, step 20 deg) must be < 9 (0..8 covers [-90, 90]).
    // Second char (longitude, step 20 deg) must be < 18 (0..17 covers [-180, 180]).
    if (lat0 := decodeBase20(code[0])) {
        if (lat0 >= 9) {
            return false;
        }
    } else {
        return false;
    }
    if (lng0 := decodeBase20(code[1])) {
        if (lng0 >= 18) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

/// Encodes `(lat, lng)` into a Google Open Location Code of precision `code_len` (2, 4, 6, 8, or 10..15).
/// Automatically clips latitude to `[-90, 90]` and normalizes longitude to `[-180, 180)`.
/// Returns bytes written to `out`, or `null` on invalid `code_len`, non-finite inputs, or short buffer.
pub fn olcEncode(lat: f64, lng: f64, code_len: usize, out: *[]u8): usize? {
    if (!isFiniteF64(lat) || !isFiniteF64(lng)) {
        return null;
    }
    if (code_len < 2 || code_len > OLC_MAX_LEN) {
        return null;
    }
    if (code_len < 10 && (code_len & 1) != 0) {
        return null;
    }

    let out_len: usize = code_len + 1;
    if (code_len < OLC_SEPARATOR_POS) {
        out_len = OLC_SEPARATOR_POS + 1; // 8 chars (with '0' padding) + '+'
    }
    if (out.*.len < out_len) {
        return null;
    }

    let c_lat: f64 = clipLatitude(lat);
    const c_lng: f64 = normalizeLongitude(lng);
    if (c_lat == 90.0) {
        c_lat -= computeLatPrecision(code_len);
    }

    const lat_scaled: f64 = (c_lat + 90.0) * (FINAL_LAT_PRECISION as f64) + 1.0e-12;
    const lng_scaled: f64 = (c_lng + 180.0) * (FINAL_LNG_PRECISION as f64) + 1.0e-12;

    let lat_val: u64 = 0;
    if (lat_scaled > 0.0) {
        lat_val = lat_scaled as u64;
    }
    if (lat_val >= MAX_LAT_INT) {
        lat_val = MAX_LAT_INT - 1;
    }

    let lng_val: u64 = 0;
    if (lng_scaled > 0.0) {
        lng_val = lng_scaled as u64;
    }
    if (lng_val >= MAX_LNG_INT) {
        lng_val = MAX_LNG_INT - 1;
    }

    // Build all 15 Base-20 digits in a fixed stack buffer (indices 0..14).
    let digits: [15]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

    // 5 grid refinement digits (indices 14 down to 10): 5 lat rows x 4 lng cols.
    let g: usize = 0;
    while (g < 5) {
        const row: usize = (lat_val % 5) as usize;
        const col: usize = (lng_val % 4) as usize;
        digits[14 - g] = OLC_ALPHABET[row * 4 + col];
        lat_val /= 5;
        lng_val /= 4;
        g += 1;
    }

    // 5 Base-20 pairs (indices 9 down to 0).
    let p: usize = 0;
    while (p < 5) {
        const pair_base: usize = ((4 as usize) - p) * 2;
        digits[pair_base + 1] = OLC_ALPHABET[(lng_val % 20) as usize];
        digits[pair_base] = OLC_ALPHABET[(lat_val % 20) as usize];
        lat_val /= 20;
        lng_val /= 20;
        p += 1;
    }

    // Emit prefix (up to 8 chars, padded with '0' if code_len < 8).
    let i: usize = 0;
    while (i < OLC_SEPARATOR_POS) {
        if (i < code_len) {
            out.*[i] = digits[i];
        } else {
            out.*[i] = OLC_PADDING;
        }
        i += 1;
    }

    out.*[OLC_SEPARATOR_POS] = OLC_SEPARATOR;

    // Emit suffix after '+' if code_len > 8.
    let s: usize = OLC_SEPARATOR_POS;
    while (s < code_len) {
        out.*[s + 1] = digits[s];
        s += 1;
    }

    return out_len;
}

/// Decodes a valid full Open Location Code (including padded 2..8 and refined 10..15 codes) into an `OlcArea`.
pub fn olcDecode(full_code: []const u8): OlcArea? {
    if (!isFullOlc(full_code)) {
        return null;
    }

    // Extract effective Base-20 digit values (skipping '+' and '0').
    let vals: [15]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let eff_len: usize = 0;
    let i: usize = 0;
    while (i < full_code.len) {
        const ch: u8 = full_code[i];
        if (ch != OLC_SEPARATOR && ch != OLC_PADDING) {
            if (v := decodeBase20(ch)) {
                vals[eff_len] = v;
                eff_len += 1;
            } else {
                return null;
            }
        }
        i += 1;
    }

    // Accumulate exact integer grid coordinates to avoid floating-point drift.
    let lat_lo_int: u64 = 0;
    let lng_lo_int: u64 = 0;
    let lat_place: u64 = (20 as u64) * FINAL_LAT_PRECISION; // 20 degrees in lat units
    let lng_place: u64 = (20 as u64) * FINAL_LNG_PRECISION; // 20 degrees in lng units

    let pairs: usize = eff_len;
    if (pairs > 10) {
        pairs = 10;
    }
    let pi: usize = 0;
    while (pi < pairs) {
        lat_lo_int += (vals[pi] as u64) * lat_place;
        lng_lo_int += (vals[pi + 1] as u64) * lng_place;
        pi += 2;
        if (pi < pairs) {
            lat_place /= 20;
            lng_place /= 20;
        }
    }

    // Apply 4x5 grid refinements for positions 10..eff_len-1.
    let gi: usize = 10;
    while (gi < eff_len) {
        lat_place /= 5;
        lng_place /= 4;
        const d: u64 = vals[gi] as u64;
        const row: u64 = d / 4;
        const col: u64 = d % 4;
        lat_lo_int += row * lat_place;
        lng_lo_int += col * lng_place;
        gi += 1;
    }

    const lat_lo: f64 = ((lat_lo_int as f64) / (FINAL_LAT_PRECISION as f64)) - 90.0;
    const lng_lo: f64 = ((lng_lo_int as f64) / (FINAL_LNG_PRECISION as f64)) - 180.0;
    const lat_hi: f64 = (((lat_lo_int + lat_place) as f64) / (FINAL_LAT_PRECISION as f64)) - 90.0;
    const lng_hi: f64 = (((lng_lo_int + lng_place) as f64) / (FINAL_LNG_PRECISION as f64)) - 180.0;

    return OlcArea {
        lat_lo: lat_lo,
        lng_lo: lng_lo,
        lat_hi: lat_hi,
        lng_hi: lng_hi,
        code_len: eff_len,
    };
}

fn absF64(x: f64): f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

/// Shortens a full non-padded Open Location Code relative to `(ref_lat, ref_lng)` by stripping
/// up to 8, 6, or 4 leading characters when safely within `0.3 * resolution`.
pub fn olcShorten(full_code: []const u8, ref_lat: f64, ref_lng: f64, out: *[]u8): usize? {
    if (!isFiniteF64(ref_lat) || !isFiniteF64(ref_lng) || !isFullOlc(full_code)) {
        return null;
    }
    // Padded codes cannot be shortened per Google OLC spec.
    let k: usize = 0;
    while (k < full_code.len) {
        if (full_code[k] == OLC_PADDING) {
            return null;
        }
        k += 1;
    }

    if (area := olcDecode(full_code)) {
        if (area.code_len < 6) {
            return null;
        }
        const c_lat: f64 = clipLatitude(ref_lat);
        const c_lng: f64 = normalizeLongitude(ref_lng);
        const d_lat: f64 = absF64(area.latCenter() - c_lat);
        const d_lng: f64 = absF64(area.lngCenter() - c_lng);
        let range: f64 = d_lat;
        if (d_lng > range) {
            range = d_lng;
        }

        // Try trimming 8, 6, or 4 leading characters (thresholds = 0.3 * pair resolution).
        let trim: usize = 0;
        if (range < 0.0025 * 0.3) {
            trim = 8;
        } else if (range < 0.05 * 0.3) {
            trim = 6;
        } else if (range < 1.0 * 0.3) {
            trim = 4;
        }

        const out_len: usize = full_code.len - trim;
        if (out.*.len < out_len) {
            return null;
        }
        let i: usize = 0;
        while (i < out_len) {
            out.*[i] = toUpperAscii(full_code[trim + i]);
            i += 1;
        }
        return out_len;
    }
    return null;
}

/// Recovers a full Open Location Code from `short_code` using reference coordinates `(ref_lat, ref_lng)`,
/// applying half-resolution nearest-quadrant adjustment. If `short_code` is already full, writes it normalized.
pub fn olcRecoverNearest(short_code: []const u8, ref_lat: f64, ref_lng: f64, out: *[]u8): usize? {
    if (!isFiniteF64(ref_lat) || !isFiniteF64(ref_lng)) {
        return null;
    }

    if (isFullOlc(short_code)) {
        if (out.*.len < short_code.len) {
            return null;
        }
        let i: usize = 0;
        while (i < short_code.len) {
            out.*[i] = toUpperAscii(short_code[i]);
            i += 1;
        }
        return short_code.len;
    }

    if (!isShortOlc(short_code)) {
        return null;
    }

    let sep_pos: usize = 0;
    if (sp := findSeparator(short_code)) {
        sep_pos = sp;
    } else {
        return null;
    }

    const c_lat: f64 = clipLatitude(ref_lat);
    const c_lng: f64 = normalizeLongitude(ref_lng);
    const pad_len: usize = OLC_SEPARATOR_POS - sep_pos; // 2, 4, 6, or 8

    let ref_arr: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let ref_slice: []u8 = ref_arr[0..];
    if (_ := olcEncode(c_lat, c_lng, OLC_DEFAULT_LEN, *ref_slice)) {
        // Combine prefix from reference code with short_code.
        let cand_arr: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        const cand_len: usize = pad_len + short_code.len;
        if (cand_len > 16) {
            return null;
        }
        let i: usize = 0;
        while (i < pad_len) {
            cand_arr[i] = ref_arr[i];
            i += 1;
        }
        let j: usize = 0;
        while (j < short_code.len) {
            cand_arr[pad_len + j] = toUpperAscii(short_code[j]);
            j += 1;
        }

        if (area := olcDecode(cand_arr[0..cand_len])) {
            let resolution: f64 = 20.0;
            if (pad_len == 4) {
                resolution = 1.0;
            } else if (pad_len == 6) {
                resolution = 0.05;
            } else if (pad_len == 8) {
                resolution = 0.0025;
            }
            const half_res: f64 = resolution * 0.5;

            let lat: f64 = area.latCenter();
            let lng: f64 = area.lngCenter();

            if (c_lat + half_res < lat && lat - resolution >= -90.0) {
                lat -= resolution;
            } else if (c_lat - half_res > lat && lat + resolution <= 90.0) {
                lat += resolution;
            }

            if (c_lng + half_res < lng) {
                lng -= resolution;
            } else if (c_lng - half_res > lng) {
                lng += resolution;
            }

            return olcEncode(lat, lng, area.code_len, out);
        }
    }
    return null;
}

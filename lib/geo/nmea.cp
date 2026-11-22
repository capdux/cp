// Origin: Pure-CP Stdlib (Zero-Alloc NMEA 0183 GNSS Sentence & Stream Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation NMEA 0183 parser: XOR checksum verification, generic sentence
// and empty-preserving field iteration, ddmm.mmmm coordinate conversion, GGA/RMC/VTG decoders,
// and noisy serial stream line iterator.

const std = @import("std");

/// Parsed generic NMEA 0183 sentence with zero-copy slices into the original buffer.
pub struct NmeaSentence {
    start_char: u8 = '$',
    talker: []const u8 = "",
    sentence_type: []const u8 = "",
    fields_slice: []const u8 = "",
    checksum: u8 = 0,

    /// Returns a zero-allocation field iterator over `fields_slice` (preserves empty fields).
    fn fields(): NmeaFieldIter {
        return NmeaFieldIter.init(_.fields_slice);
    }
}

/// Decoded NMEA GGA (Global Positioning System Fix Data) sentence.
pub struct NmeaGga {
    talker: []const u8 = "",
    utc_time: []const u8 = "",
    lat: f64 = 0.0,
    lon: f64 = 0.0,
    has_fix: bool = false,
    fix_quality: u8 = 0,
    satellites: u8 = 0,
    hdop: f64 = 0.0,
    altitude_m: f64 = 0.0,
}

/// Decoded NMEA RMC (Recommended Minimum Specific GNSS Data) sentence.
pub struct NmeaRmc {
    talker: []const u8 = "",
    utc_time: []const u8 = "",
    is_active: bool = false,
    lat: f64 = 0.0,
    lon: f64 = 0.0,
    speed_knots: f64 = 0.0,
    course_deg: f64 = 0.0,
    date_ddmmyy: []const u8 = "",

    /// Converts ground speed from knots to km/h (`knots * 1.852`).
    fn speedKmh(): f64 {
        return _.speed_knots * 1.852;
    }

    /// Converts ground speed from knots to m/s (`knots * 0.5144444`).
    fn speedMs(): f64 {
        return _.speed_knots * 0.5144444;
    }
}

/// Decoded NMEA VTG (Course Over Ground and Ground Speed) sentence.
pub struct NmeaVtg {
    talker: []const u8 = "",
    true_track_deg: f64 = 0.0,
    mag_track_deg: f64 = 0.0,
    speed_knots: f64 = 0.0,
    speed_kmh: f64 = 0.0,
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn hexDigit(b: u8): u8? {
    if (b >= '0' && b <= '9') {
        return b - '0';
    }
    if (b >= 'A' && b <= 'F') {
        return b - 'A' + 10;
    }
    if (b >= 'a' && b <= 'f') {
        return b - 'a' + 10;
    }
    return null;
}

fn parseHexByte(hi: u8, lo: u8): u8? {
    const h = hexDigit(hi) ?? return null;
    const l = hexDigit(lo) ?? return null;
    return (h << 4) | l;
}

fn trimLineEnding(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\n' || line[end - 1] == '\r')) {
        end -= 1;
    }
    return line[0..end];
}

fn isUpperAlphaNum(b: u8): bool {
    return (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9');
}

fn parseU8Strict(s: []const u8): u8? {
    if (s.len == 0 || s.len > 3) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < '0' || b > '9') {
            return null;
        }
        acc = acc * 10 + ((b - '0') as u32);
        if (acc > 255) {
            return null;
        }
        i += 1;
    }
    return acc as u8;
}

fn parseU32Digits(s: []const u8): u32? {
    if (s.len == 0 || s.len > 9) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < '0' || b > '9') {
            return null;
        }
        acc = acc * 10 + ((b - '0') as u32);
        i += 1;
    }
    return acc;
}

fn parseOptionalF64(s: []const u8): f64? {
    if (s.len == 0) {
        return 0.0;
    }
    const val = std.fmt.parseFloat(f64, s) catch return null;
    if (!std.math.isFinite(val)) {
        return null;
    }
    return val;
}

// ---------------------------------------------------------------------------
// 1. XOR Checksum & Sentence Verification
// ---------------------------------------------------------------------------

/// Computes the NMEA 0183 XOR checksum across all bytes in `body` (between `$`/`!` and `*`).
pub fn nmeaChecksum(body: []const u8): u8 {
    let cs: u8 = 0;
    let i: usize = 0;
    while (i < body.len) {
        cs ^= body[i];
        i += 1;
    }
    return cs;
}

/// Strictly verifies an NMEA 0183 sentence line:
/// - Starts with `$` or `!`
/// - Contains `*HH` two-hex-digit checksum at the end (ignoring trailing `\r`/`\n`)
/// - Contains no stray `$`, `!`, `*`, `\r`, or `\n` inside the checksummed body
/// - Computed XOR checksum matches `HH`
pub fn verifyNmeaSentence(line: []const u8): bool {
    const trimmed = trimLineEnding(line);
    if (trimmed.len < 5) {
        return false;
    }
    const start = trimmed[0];
    if (start != '$' && start != '!') {
        return false;
    }
    const star_idx = trimmed.len - 3;
    if (trimmed[star_idx] != '*') {
        return false;
    }
    const expected = parseHexByte(trimmed[star_idx + 1], trimmed[star_idx + 2]) ?? return false;
    const body = trimmed[1..star_idx];
    let i: usize = 0;
    while (i < body.len) {
        const b = body[i];
        if (b == '$' || b == '!' || b == '*' || b == '\r' || b == '\n') {
            return false;
        }
        i += 1;
    }
    return nmeaChecksum(body) == expected;
}

// ---------------------------------------------------------------------------
// 2. Generic Sentence Parser & Empty-Preserving Field Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation comma-delimited NMEA field iterator that preserves empty fields `""`.
pub struct NmeaFieldIter {
    src: []const u8 = "",
    pos: usize = 0,
    has_more: bool = false,

    fn init(fields_slice: []const u8): NmeaFieldIter {
        return NmeaFieldIter {
            src: fields_slice,
            pos: 0,
            has_more: true,
        };
    }

    fn next(*_): ([]const u8)? {
        if (!_.has_more) {
            return null;
        }
        let i = _.pos;
        while (i < _.src.len && _.src[i] != ',') {
            i += 1;
        }
        const field = _.src[_.pos..i];
        if (i < _.src.len && _.src[i] == ',') {
            _.pos = i + 1;
            _.has_more = true;
        } else {
            _.pos = _.src.len;
            _.has_more = false;
        }
        return field;
    }
}

/// Verifies and parses a generic NMEA 0183 sentence line into zero-copy slices.
pub fn parseNmeaSentence(line: []const u8): NmeaSentence? {
    if (!verifyNmeaSentence(line)) {
        return null;
    }
    const trimmed = trimLineEnding(line);
    const star_idx = trimmed.len - 3;
    const cs = parseHexByte(trimmed[star_idx + 1], trimmed[star_idx + 2]) ?? return null;
    const body = trimmed[1..star_idx];

    let comma_idx: usize = 0;
    while (comma_idx < body.len && body[comma_idx] != ',') {
        comma_idx += 1;
    }
    const addr = body[0..comma_idx];
    if (addr.len < 5) {
        return null;
    }
    let j: usize = 0;
    while (j < addr.len) {
        if (!isUpperAlphaNum(addr[j])) {
            return null;
        }
        j += 1;
    }

    const fields_slice = if (comma_idx < body.len) body[comma_idx + 1 ..] else "";
    return NmeaSentence {
        start_char: trimmed[0],
        talker: addr[0..2],
        sentence_type: addr[2..5],
        fields_slice: fields_slice,
        checksum: cs,
    };
}

// ---------------------------------------------------------------------------
// 3. Coordinate Conversion (ddmm.mmmm / dddmm.mmmm -> Decimal Degrees)
// ---------------------------------------------------------------------------

/// Converts an NMEA `ddmm.mmmm` (latitude) or `dddmm.mmmm` (longitude) string plus
/// hemisphere indicator (`"N"`, `"S"`, `"E"`, `"W"`) into signed decimal degrees
/// (`deg + min / 60.0`). Returns `null` on invalid format, hemisphere, or range.
pub fn parseNmeaCoord(dm_str: []const u8, hemi: []const u8): f64? {
    if (dm_str.len < 3 || hemi.len != 1) {
        return null;
    }
    const h = hemi[0];
    const is_lat = (h == 'N' || h == 'S');
    const is_lon = (h == 'E' || h == 'W');
    if (!is_lat && !is_lon) {
        return null;
    }

    // Validate digits & at most one decimal point
    let dot_pos: usize = dm_str.len;
    let dot_count: usize = 0;
    let i: usize = 0;
    while (i < dm_str.len) {
        const b = dm_str[i];
        if (b == '.') {
            dot_count += 1;
            if (dot_count > 1) {
                return null;
            }
            dot_pos = i;
        } else if (b < '0' || b > '9') {
            return null;
        }
        i += 1;
    }

    // Must have at least 1 degree digit + 2 minute digits before decimal point
    if (dot_pos < 3) {
        return null;
    }
    if (is_lat && dot_pos > 4) {
        return null;
    }
    if (is_lon && dot_pos > 5) {
        return null;
    }

    const deg_str = dm_str[0 .. dot_pos - 2];
    const min_str = dm_str[dot_pos - 2 ..];

    const deg_u = parseU32Digits(deg_str) ?? return null;
    const minutes = std.fmt.parseFloat(f64, min_str) catch return null;
    if (!std.math.isFinite(minutes) || minutes < 0.0 || minutes >= 60.0) {
        return null;
    }

    const deg_val = (deg_u as f64) + minutes / 60.0;
    if (is_lat && deg_val > 90.0) {
        return null;
    }
    if (is_lon && deg_val > 180.0) {
        return null;
    }

    if (h == 'S' || h == 'W') {
        if (deg_val == 0.0) {
            return 0.0;
        }
        return -deg_val;
    }
    return deg_val;
}

// ---------------------------------------------------------------------------
// 4. Structured Decoders: GGA, RMC, VTG
// ---------------------------------------------------------------------------

/// Parses an NMEA 0183 GGA sentence (`$GPGGA`, `$GNGGA`, `$BDGGA`, etc.).
pub fn parseNmeaGga(line: []const u8): NmeaGga? {
    const sent = parseNmeaSentence(line) ?? return null;
    if (!std.mem.eql(u8, sent.sentence_type, "GGA")) {
        return null;
    }

    let it = sent.fields();
    const utc = it.next() ?? return null;
    const lat_s = it.next() ?? return null;
    const ns_s = it.next() ?? return null;
    const lon_s = it.next() ?? return null;
    const ew_s = it.next() ?? return null;
    const qual_s = it.next() ?? return null;
    const sat_s = it.next() ?? return null;
    const hdop_s = it.next() ?? return null;
    const alt_s = it.next() ?? return null;

    const qual: u8 = if (qual_s.len == 0) 0 else (parseU8Strict(qual_s) ?? return null);
    const sats: u8 = if (sat_s.len == 0) 0 else (parseU8Strict(sat_s) ?? return null);
    const hdop = parseOptionalF64(hdop_s) ?? return null;
    const alt_m = parseOptionalF64(alt_s) ?? return null;

    let lat: f64 = 0.0;
    let lon: f64 = 0.0;
    const has_coords = lat_s.len > 0 || ns_s.len > 0 || lon_s.len > 0 || ew_s.len > 0;
    if (has_coords) {
        lat = parseNmeaCoord(lat_s, ns_s) ?? return null;
        lon = parseNmeaCoord(lon_s, ew_s) ?? return null;
    }

    return NmeaGga {
        talker: sent.talker,
        utc_time: utc,
        lat: lat,
        lon: lon,
        has_fix: qual > 0,
        fix_quality: qual,
        satellites: sats,
        hdop: hdop,
        altitude_m: alt_m,
    };
}

/// Parses an NMEA 0183 RMC sentence (`$GPRMC`, `$GNRMC`, `$BDRMC`, etc.).
pub fn parseNmeaRmc(line: []const u8): NmeaRmc? {
    const sent = parseNmeaSentence(line) ?? return null;
    if (!std.mem.eql(u8, sent.sentence_type, "RMC")) {
        return null;
    }

    let it = sent.fields();
    const utc = it.next() ?? return null;
    const status = it.next() ?? return null;
    const lat_s = it.next() ?? return null;
    const ns_s = it.next() ?? return null;
    const lon_s = it.next() ?? return null;
    const ew_s = it.next() ?? return null;
    const spd_s = it.next() ?? return null;
    const cog_s = it.next() ?? return null;
    const date_s = it.next() ?? return null;

    let is_active = false;
    if (std.mem.eql(u8, status, "A")) {
        is_active = true;
    } else if (std.mem.eql(u8, status, "V")) {
        is_active = false;
    } else {
        return null;
    }

    let lat: f64 = 0.0;
    let lon: f64 = 0.0;
    const has_coords = lat_s.len > 0 || ns_s.len > 0 || lon_s.len > 0 || ew_s.len > 0;
    if (has_coords) {
        lat = parseNmeaCoord(lat_s, ns_s) ?? return null;
        lon = parseNmeaCoord(lon_s, ew_s) ?? return null;
    }

    const speed_knots = parseOptionalF64(spd_s) ?? return null;
    const course_deg = parseOptionalF64(cog_s) ?? return null;

    return NmeaRmc {
        talker: sent.talker,
        utc_time: utc,
        is_active: is_active,
        lat: lat,
        lon: lon,
        speed_knots: speed_knots,
        course_deg: course_deg,
        date_ddmmyy: date_s,
    };
}

/// Parses an NMEA 0183 VTG sentence (`$GPVTG`, `$GNVTG`, etc.).
pub fn parseNmeaVtg(line: []const u8): NmeaVtg? {
    const sent = parseNmeaSentence(line) ?? return null;
    if (!std.mem.eql(u8, sent.sentence_type, "VTG")) {
        return null;
    }

    let it = sent.fields();
    const cogt_s = it.next() ?? return null;
    const t_ind = it.next() ?? return null;
    const cogm_s = it.next() ?? return null;
    const m_ind = it.next() ?? return null;
    const sogn_s = it.next() ?? return null;
    const n_ind = it.next() ?? return null;
    const sogk_s = it.next() ?? return null;
    * = t_ind;
    * = m_ind;
    * = n_ind;

    const true_track = parseOptionalF64(cogt_s) ?? return null;
    const mag_track = parseOptionalF64(cogm_s) ?? return null;
    const knots = parseOptionalF64(sogn_s) ?? return null;
    const kmh = parseOptionalF64(sogk_s) ?? return null;

    return NmeaVtg {
        talker: sent.talker,
        true_track_deg: true_track,
        mag_track_deg: mag_track,
        speed_knots: knots,
        speed_kmh: kmh,
    };
}

// ---------------------------------------------------------------------------
// 5. Multi-Line Serial / Log Stream Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over all valid NMEA 0183 sentences in a multi-line
/// serial or log buffer, automatically filtering out noise lines and invalid checksums.
pub struct NmeaLineIter {
    src: []const u8 = "",
    pos: usize = 0,

    fn init(buf: []const u8): NmeaLineIter {
        return NmeaLineIter {
            src: buf,
            pos: 0,
        };
    }

    fn next(*_): NmeaSentence? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            let i = line_start;
            while (i < _.src.len && _.src[i] != '\n') {
                i += 1;
            }
            const line = _.src[line_start..i];
            _.pos = if (i < _.src.len) i + 1 else _.src.len;

            if (sent := parseNmeaSentence(line)) {
                return sent;
            }
        }
        return null;
    }
}

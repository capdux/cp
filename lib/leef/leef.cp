// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IBM QRadar LEEF 1.0/2.0 Parser)
// Pure CP zero-heap-allocation IBM QRadar Log Event Extended Format (LEEF 1.0 & 2.0) library:
// optional Syslog prefix extraction, LEEF 1.0 (5-field, Tab) & LEEF 2.0 (6-field, custom single-char
// or hex 0xHH / xHH delimiter), zero-copy key=value attribute iteration, and header formatting.

/// Standard LEEF header prefix marker.
pub const LEEF_PREFIX: []const u8 = "LEEF:";

/// Default LEEF attribute delimiter (`0x09` horizontal tab).
pub const LEEF_DEFAULT_DELIM: u8 = '\t';

/// Zero-copy decoded IBM QRadar LEEF 1.0 / 2.0 event.
pub struct LeefEvent {
    syslog_prefix: []const u8 = "",
    version_major: u8 = 1,
    version_minor: u8 = 0,
    vendor: []const u8 = "",
    product: []const u8 = "",
    product_version: []const u8 = "",
    event_id: []const u8 = "",
    delimiter: u8 = LEEF_DEFAULT_DELIM,
    attributes_slice: []const u8 = "",

    /// Returns a zero-allocation iterator over `key=value` attributes in this event.
    fn attrs(): LeefAttrIter {
        return LeefAttrIter.init(_.attributes_slice, _.delimiter);
    }

    /// Finds the value of the first attribute matching `key` (exact match).
    fn findAttr(key: []const u8): ([]const u8)? {
        return findLeefAttr(_, key);
    }
}

/// Zero-copy `key=value` attribute pair from a LEEF event payload.
pub struct LeefAttr {
    key: []const u8 = "",
    value: []const u8 = "",
}

// ---------------------------------------------------------------------------
// Internal ASCII, Hex & Delimiter Helpers
// ---------------------------------------------------------------------------

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripLineEnding(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[0..end];
}

fn trimRightSpaces(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[0..end];
}

fn hexNibble(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return null;
}

fn parseHexByte(hi: u8, lo: u8): u8? {
    if (h := hexNibble(hi)) {
        if (l := hexNibble(lo)) {
            return (h << 4) | l;
        }
    }
    return null;
}

/// Parses a LEEF 2.0 custom delimiter field:
/// - `""` -> `LEEF_DEFAULT_DELIM` (`'\t'`)
/// - 1-byte literal (e.g. `"^"`, `","`, `"\t"`)
/// - `"0xHH"` / `"0XHH"` or `"xHH"` / `"XHH"` hexadecimal byte
fn parseLeef2Delimiter(field: []const u8): u8? {
    if (field.len == 0) {
        return LEEF_DEFAULT_DELIM;
    }
    if (field.len == 1) {
        return field[0];
    }
    if (field.len == 4 && field[0] == '0' && (field[1] == 'x' || field[1] == 'X')) {
        return parseHexByte(field[2], field[3]);
    }
    if (field.len == 3 && (field[0] == 'x' || field[0] == 'X')) {
        return parseHexByte(field[1], field[2]);
    }
    return null;
}

/// Locates `"LEEF:"` either at index 0 or immediately preceded by whitespace.
fn findLeefStart(line: []const u8): usize? {
    if (line.len < 5) {
        return null;
    }
    let i: usize = 0;
    while (i + 5 <= line.len) {
        if (line[i] == 'L' &&
            line[i + 1] == 'E' &&
            line[i + 2] == 'E' &&
            line[i + 3] == 'F' &&
            line[i + 4] == ':')
        {
            if (i == 0 || line[i - 1] == ' ' || line[i - 1] == '\t') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn findByte(s: []const u8, start: usize, target: u8): usize? {
    let i = start;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

// ---------------------------------------------------------------------------
// Public Detection & Parsing API
// ---------------------------------------------------------------------------

/// Checks whether `line` contains a valid `"LEEF:1.0|"` or `"LEEF:2.0|"` header
/// (with optional Syslog prefix).
pub fn isLeef(line: []const u8): bool {
    const clean = stripLineEnding(line);
    if (pos := findLeefStart(clean)) {
        const rest = clean[pos + 5..];
        if (rest.len >= 4 && rest[1] == '.' && rest[2] == '0' && rest[3] == '|') {
            return rest[0] == '1' || rest[0] == '2';
        }
    }
    return false;
}

/// Parses a single LEEF 1.0 or LEEF 2.0 log line (with optional Syslog prefix).
/// Returns `null` if the version, pipe count, or LEEF 2.0 custom delimiter is invalid.
pub fn parseLeefLine(line: []const u8): LeefEvent? {
    const clean = stripLineEnding(line);
    let leef_pos: usize = 0;
    if (p := findLeefStart(clean)) {
        leef_pos = p;
    } else {
        return null;
    }

    const syslog_prefix = trimRightSpaces(clean[0..leef_pos]);
    const rest = clean[leef_pos + 5..];

    // Parse first 5 pipe delimiters: version|vendor|product|product_ver|event_id|
    let pipes: [5]usize = [0; 5];
    let cur: usize = 0;
    let p_idx: usize = 0;
    while (p_idx < 5) {
        if (p := findByte(rest, cur, '|')) {
            pipes[p_idx] = p;
            cur = p + 1;
            p_idx += 1;
        } else {
            return null;
        }
    }

    const ver_str = rest[0..pipes[0]];
    let major: u8 = 0;
    let minor: u8 = 0;
    if (bytesEqual(ver_str, "1.0")) {
        major = 1;
        minor = 0;
    } else if (bytesEqual(ver_str, "2.0")) {
        major = 2;
        minor = 0;
    } else {
        return null;
    }

    const vendor = rest[pipes[0] + 1..pipes[1]];
    const product = rest[pipes[1] + 1..pipes[2]];
    const product_ver = rest[pipes[2] + 1..pipes[3]];
    const event_id = rest[pipes[3] + 1..pipes[4]];

    let delim: u8 = LEEF_DEFAULT_DELIM;
    let attrs_start = pipes[4] + 1;

    if (major == 2) {
        // LEEF 2.0 requires a 6th pipe delimiter for the custom delimiter field
        if (p6 := findByte(rest, attrs_start, '|')) {
            const delim_field = rest[attrs_start..p6];
            if (d := parseLeef2Delimiter(delim_field)) {
                delim = d;
            } else {
                return null;
            }
            attrs_start = p6 + 1;
        } else {
            return null;
        }
    }

    return LeefEvent {
        syslog_prefix: syslog_prefix,
        version_major: major,
        version_minor: minor,
        vendor: vendor,
        product: product,
        product_version: product_ver,
        event_id: event_id,
        delimiter: delim,
        attributes_slice: rest[attrs_start..],
    };
}

// ---------------------------------------------------------------------------
// Zero-Alloc Attribute Iterator & Lookup
// ---------------------------------------------------------------------------

/// Zero-allocation iterator splitting `attributes_slice` by `delimiter` into `key=value` pairs.
/// Splits each attribute at the first `'='` so embedded `'='` inside values are preserved.
pub struct LeefAttrIter {
    attrs: []const u8 = "",
    delim: u8 = LEEF_DEFAULT_DELIM,
    pos: usize = 0,

    pub fn init(attributes_slice: []const u8, delimiter: u8): LeefAttrIter {
        return LeefAttrIter {
            attrs: attributes_slice,
            delim: delimiter,
            pos: 0,
        };
    }

    fn next(*_): LeefAttr? {
        while (_.pos < _.attrs.len) {
            const seg_start = _.pos;
            let seg_end = _.attrs.len;
            if (d_pos := findByte(_.attrs, _.pos, _.delim)) {
                seg_end = d_pos;
                _.pos = d_pos + 1;
            } else {
                _.pos = _.attrs.len;
            }

            const seg = _.attrs[seg_start..seg_end];
            if (seg.len == 0) {
                continue;
            }
            if (eq := findByte(seg, 0, '=')) {
                if (eq > 0) {
                    return LeefAttr {
                        key: seg[0..eq],
                        value: seg[eq + 1..],
                    };
                }
            }
        }
        return null;
    }
}

/// Finds the value for `key` (exact match) in `event.attributes_slice`.
pub fn findLeefAttr(event: LeefEvent, key: []const u8): ([]const u8)? {
    let it = LeefAttrIter.init(event.attributes_slice, event.delimiter);
    while (attr := it.next()) {
        if (bytesEqual(attr.key, key)) {
            return attr.value;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// Zero-Alloc Header Formatter
// ---------------------------------------------------------------------------

fn writeRaw(out: []u8, pos: usize, s: []const u8): usize? {
    if (pos + s.len > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return pos + s.len;
}

fn hexDigitUpper(n: u8): u8 {
    if (n < 10) {
        return ('0' as u8) + n;
    }
    return ('A' as u8) + (n - 10);
}

/// Formats a LEEF 1.0 or LEEF 2.0 header into `out`:
/// - `version_major == 1`: `LEEF:1.0|<vendor>|<product>|<product_ver>|<event_id>|`
/// - `version_major == 2`: `LEEF:2.0|<vendor>|<product>|<product_ver>|<event_id>|<delim>|`
///   (uses `0xHH` hex notation automatically for `'|'` and non-printable bytes such as `'\t'`).
/// Returns total bytes written, or `null` if `version_major` is unsupported or `out` is too small.
pub fn formatLeefHeader(
    version_major: u8,
    vendor: []const u8,
    product: []const u8,
    product_ver: []const u8,
    event_id: []const u8,
    delimiter: u8,
    out: []u8,
): usize? {
    if (version_major != 1 && version_major != 2) {
        return null;
    }
    let pos: usize = 0;
    const prefix = if (version_major == 1) "LEEF:1.0|" else "LEEF:2.0|";
    if (p := writeRaw(out, pos, prefix)) { pos = p; } else { return null; }

    const fields: [4][]const u8 = [vendor, product, product_ver, event_id];
    let i: usize = 0;
    while (i < 4) {
        if (p := writeRaw(out, pos, fields[i])) { pos = p; } else { return null; }
        if (pos >= out.len) { return null; }
        out[pos] = '|';
        pos += 1;
        i += 1;
    }

    if (version_major == 2) {
        if (delimiter == '|' || delimiter < 0x20 || delimiter >= 0x7F) {
            if (pos + 5 > out.len) { return null; }
            out[pos] = '0';
            out[pos + 1] = 'x';
            out[pos + 2] = hexDigitUpper((delimiter >> 4) & 0x0F);
            out[pos + 3] = hexDigitUpper(delimiter & 0x0F);
            out[pos + 4] = '|';
            pos += 5;
        } else {
            if (pos + 2 > out.len) { return null; }
            out[pos] = delimiter;
            out[pos + 1] = '|';
            pos += 2;
        }
    }

    return pos;
}

// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ISO 28500 WARC 1.0/1.1 Parser)
// Pure CP zero-heap-allocation ISO 28500 Web ARChive (.warc) 1.0 & 1.1 streaming parser:
// zero-copy header field iterator, exact Content-Length binary payload slicing,
// trailing CRLFCRLF / LFLF separator validation, and single-pass corpus summarization.

/// ISO 28500 WARC 1.0 version line prefix.
pub const WARC_VERSION_1_0: []const u8 = "WARC/1.0";

/// ISO 28500 WARC 1.1 version line prefix.
pub const WARC_VERSION_1_1: []const u8 = "WARC/1.1";

/// Standard ISO 28500 `WARC-Type` record classifications.
pub enum WarcRecordType: u8 {
    warcinfo = 0,
    response = 1,
    resource = 2,
    request = 3,
    metadata = 4,
    revisit = 5,
    conversion = 6,
    continuation = 7,
    unknown = 255,
}

/// Parses a `WARC-Type` field value (case-insensitive) into `WarcRecordType`.
pub fn warcRecordTypeFromSlice(s: []const u8): WarcRecordType {
    if (bytesEqualIgnoreCase(s, "warcinfo")) {
        return WarcRecordType.warcinfo;
    }
    if (bytesEqualIgnoreCase(s, "response")) {
        return WarcRecordType.response;
    }
    if (bytesEqualIgnoreCase(s, "resource")) {
        return WarcRecordType.resource;
    }
    if (bytesEqualIgnoreCase(s, "request")) {
        return WarcRecordType.request;
    }
    if (bytesEqualIgnoreCase(s, "metadata")) {
        return WarcRecordType.metadata;
    }
    if (bytesEqualIgnoreCase(s, "revisit")) {
        return WarcRecordType.revisit;
    }
    if (bytesEqualIgnoreCase(s, "conversion")) {
        return WarcRecordType.conversion;
    }
    if (bytesEqualIgnoreCase(s, "continuation")) {
        return WarcRecordType.continuation;
    }
    return WarcRecordType.unknown;
}

/// Zero-copy `Name: Value` header field inside a WARC record header block.
pub struct WarcField {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Zero-copy decoded ISO 28500 WARC record.
pub struct WarcRecord {
    version: []const u8 = "",
    record_type: WarcRecordType = WarcRecordType.unknown,
    record_id: []const u8 = "",
    date: []const u8 = "",
    target_uri: []const u8 = "",
    content_type: []const u8 = "",
    payload_digest: []const u8 = "",
    content_length: usize = 0,
    headers_slice: []const u8 = "",
    payload: []const u8 = "",

    /// Case-insensitive lookup of any header field in `headers_slice`.
    fn findHeader(name: []const u8): ([]const u8)? {
        return findWarcHeader(_.headers_slice, name);
    }
}

/// Aggregate statistics from a single-pass scan of a `.warc` stream.
pub struct WarcStats {
    record_count: u32 = 0,
    response_count: u32 = 0,
    request_count: u32 = 0,
    warcinfo_count: u32 = 0,
    conversion_count: u32 = 0,
    total_payload_bytes: u64 = 0,
}

// ---------------------------------------------------------------------------
// Internal ASCII & Line Parsing Helpers
// ---------------------------------------------------------------------------

fn toLowerAscii(ch: u8): u8 {
    if (ch >= 'A' && ch <= 'Z') {
        return ch + 32;
    }
    return ch;
}

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

fn bytesEqualIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimAsciiWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// Parses an unsigned decimal integer into `usize` with overflow protection.
fn parseDecimalUsize(s: []const u8): usize? {
    if (s.len == 0) {
        return null;
    }
    const max_div10: usize = 1844674407370955161; // floor(u64::MAX / 10)
    let acc: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') {
            return null;
        }
        const d = (ch - '0') as usize;
        if (acc > max_div10 || (acc == max_div10 && d > 5)) {
            return null;
        }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

struct VersionLine {
    version: []const u8,
    next_pos: usize,
}

fn parseVersionLine(data: []const u8, start: usize): VersionLine? {
    if (start + 9 > data.len) {
        return null;
    }
    const prefix = data[start..start + 8];
    if (!bytesEqual(prefix, WARC_VERSION_1_0) && !bytesEqual(prefix, WARC_VERSION_1_1)) {
        return null;
    }
    if (start + 10 <= data.len && data[start + 8] == '\r' && data[start + 9] == '\n') {
        return VersionLine { version: prefix, next_pos: start + 10 };
    }
    if (data[start + 8] == '\n') {
        return VersionLine { version: prefix, next_pos: start + 9 };
    }
    return null;
}

// ---------------------------------------------------------------------------
// Public Header & Detection API
// ---------------------------------------------------------------------------

/// Validates that `data` begins with `"WARC/1.0\r\n"` or `"WARC/1.1\r\n"` (also tolerating `\n`).
pub fn isWarc(data: []const u8): bool {
    if (v := parseVersionLine(data, 0)) {
        * = v;
        return true;
    }
    return false;
}

/// Zero-allocation line-by-line `Name: Value` iterator over a WARC record header block.
pub struct WarcFieldIter {
    data: []const u8 = "",
    pos: usize = 0,

    pub fn init(headers_slice: []const u8): WarcFieldIter {
        return WarcFieldIter {
            data: headers_slice,
            pos: 0,
        };
    }

    fn next(*_): WarcField? {
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                _.pos += 1;
            }
            let line_end = _.pos;
            if (_.pos < _.data.len && _.data[_.pos] == '\n') {
                _.pos += 1;
            }
            if (line_end > line_start && _.data[line_end - 1] == '\r') {
                line_end -= 1;
            }
            const line = _.data[line_start..line_end];
            if (line.len == 0) {
                continue;
            }

            let colon: usize = 0;
            let found = false;
            while (colon < line.len) {
                if (line[colon] == ':') {
                    found = true;
                    break;
                }
                colon += 1;
            }
            if (!found) {
                continue;
            }
            return WarcField {
                name: trimAsciiWs(line[0..colon]),
                value: trimAsciiWs(line[colon + 1..]),
            };
        }
        return null;
    }
}

/// Finds the first header value matching `name` (case-insensitive) inside `headers_slice`.
pub fn findWarcHeader(headers_slice: []const u8, name: []const u8): ([]const u8)? {
    let it = WarcFieldIter.init(headers_slice);
    while (f := it.next()) {
        if (bytesEqualIgnoreCase(f.name, name)) {
            return f.value;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// Streaming Record Iterator & Single-Pass Summary
// ---------------------------------------------------------------------------

/// Zero-allocation streaming iterator over concatenated ISO 28500 WARC records.
pub struct WarcIter {
    data: []const u8 = "",
    pos: usize = 0,
    had_error: bool = false,

    pub fn init(data: []const u8): WarcIter {
        return WarcIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    /// Parses and returns the next `WarcRecord`, advancing strictly by `Content-Length`
    /// plus the ISO 28500 trailing `\r\n\r\n` (or `\n\n`) record separator.
    fn next(*_): WarcRecord? {
        if (_.had_error || _.pos >= _.data.len) {
            return null;
        }

        let version_str: []const u8 = "";
        let cur = _.pos;
        if (vl := parseVersionLine(_.data, cur)) {
            version_str = vl.version;
            cur = vl.next_pos;
        } else {
            _.had_error = true;
            return null;
        }

        // Scan header block until terminating blank line (\r\n or \n)
        const headers_start = cur;
        let headers_end = cur;
        let body_start: usize = 0;
        let found_end = false;

        while (cur < _.data.len) {
            const line_start = cur;
            while (cur < _.data.len && _.data[cur] != '\n') {
                cur += 1;
            }
            if (cur >= _.data.len) {
                break;
            }
            let line_end = cur;
            cur += 1; // consume '\n'
            if (line_end > line_start && _.data[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (line_end == line_start) {
                headers_end = line_start;
                body_start = cur;
                found_end = true;
                break;
            }
        }

        if (!found_end) {
            _.had_error = true;
            return null;
        }

        const headers_slice = _.data[headers_start..headers_end];
        let rec = WarcRecord {
            version: version_str,
            headers_slice: headers_slice,
        };

        let has_content_length = false;
        let fit = WarcFieldIter.init(headers_slice);
        while (f := fit.next()) {
            if (bytesEqualIgnoreCase(f.name, "WARC-Type")) {
                rec.record_type = warcRecordTypeFromSlice(f.value);
            } else if (bytesEqualIgnoreCase(f.name, "WARC-Record-ID")) {
                rec.record_id = f.value;
            } else if (bytesEqualIgnoreCase(f.name, "WARC-Date")) {
                rec.date = f.value;
            } else if (bytesEqualIgnoreCase(f.name, "WARC-Target-URI")) {
                rec.target_uri = f.value;
            } else if (bytesEqualIgnoreCase(f.name, "Content-Type")) {
                rec.content_type = f.value;
            } else if (bytesEqualIgnoreCase(f.name, "WARC-Payload-Digest")) {
                rec.payload_digest = f.value;
            } else if (bytesEqualIgnoreCase(f.name, "Content-Length")) {
                if (cl := parseDecimalUsize(f.value)) {
                    rec.content_length = cl;
                    has_content_length = true;
                } else {
                    _.had_error = true;
                    return null;
                }
            }
        }

        if (!has_content_length) {
            _.had_error = true;
            return null;
        }

        if (rec.content_length > _.data.len - body_start) {
            _.had_error = true;
            return null;
        }

        const body_end = body_start + rec.content_length;
        rec.payload = _.data[body_start..body_end];

        // ISO 28500 trailing double-CRLF (\r\n\r\n) record separator (tolerating \n\n or clean EOF)
        if (body_end + 4 <= _.data.len &&
            _.data[body_end] == '\r' &&
            _.data[body_end + 1] == '\n' &&
            _.data[body_end + 2] == '\r' &&
            _.data[body_end + 3] == '\n')
        {
            _.pos = body_end + 4;
        } else if (body_end + 2 <= _.data.len &&
            _.data[body_end] == '\n' &&
            _.data[body_end + 1] == '\n')
        {
            _.pos = body_end + 2;
        } else if (body_end == _.data.len) {
            _.pos = body_end;
        } else {
            _.had_error = true;
            return null;
        }

        return rec;
    }
}

/// Scans a WARC stream in a single zero-allocation pass and aggregates record/payload stats.
/// Returns `null` if invalid or if any record is malformed/truncated.
pub fn summarizeWarc(data: []const u8): WarcStats? {
    if (!isWarc(data)) {
        return null;
    }
    let it = WarcIter.init(data);
    let stats = WarcStats {};

    while (rec := it.next()) {
        stats.record_count += 1;
        stats.total_payload_bytes += rec.content_length as u64;
        if (rec.record_type == WarcRecordType.response) {
            stats.response_count += 1;
        } else if (rec.record_type == WarcRecordType.request) {
            stats.request_count += 1;
        } else if (rec.record_type == WarcRecordType.warcinfo) {
            stats.warcinfo_count += 1;
        } else if (rec.record_type == WarcRecordType.conversion) {
            stats.conversion_count += 1;
        }
    }

    if (it.had_error || stats.record_count == 0) {
        return null;
    }
    return stats;
}

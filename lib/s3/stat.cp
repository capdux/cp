// Source: Absorbed from /data/dev/bun/src/runtime/webcore/S3Stat.cp | Synced: 2026-11-22T16:45:51Z
// Pure-CP zero-allocation S3 object metadata (`S3Stat`) & HEAD response header parser.
// Includes RFC 7231 IMF-fixdate / ISO-8601 Last-Modified timestamp parsing, ETag normalization,
// multipart ETag detection, and version-id extraction.

pub const S3_STAT_MAX_ETAG_LEN: usize = 128;
pub const S3_STAT_MAX_TYPE_LEN: usize = 128;
pub const S3_STAT_MAX_DATE_LEN: usize = 64;
pub const S3_STAT_MAX_VERSION_LEN: usize = 128;

pub err S3StatError {
    FieldTooLong,
    InvalidContentLength,
    InvalidLastModified,
    MalformedHeaders,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn copyFixed(dst: []u8, src: []const u8): usize! {
    if (src.len > dst.len) return S3StatError.FieldTooLong;
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
    return src.len;
}

fn parseUintDec(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const digit: u64 = @as(u64, c - '0');
        if (val > (@as(u64, 18446744073709551615) - digit) / 10) return null;
        val = val * 10 + digit;
        i += 1;
    }
    return val;
}

fn parseFixedDigits(s: []const u8, start: usize, count: usize): i64? {
    if (start + count > s.len) return null;
    let val: i64 = 0;
    let i: usize = 0;
    while (i < count) {
        const c = s[start + i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(i64, c - '0');
        i += 1;
    }
    return val;
}

fn parseMonth3(s: []const u8): i64? {
    if (s.len != 3) return null;
    if (eqlIgnoreCaseAscii(s, "Jan")) return 1;
    if (eqlIgnoreCaseAscii(s, "Feb")) return 2;
    if (eqlIgnoreCaseAscii(s, "Mar")) return 3;
    if (eqlIgnoreCaseAscii(s, "Apr")) return 4;
    if (eqlIgnoreCaseAscii(s, "May")) return 5;
    if (eqlIgnoreCaseAscii(s, "Jun")) return 6;
    if (eqlIgnoreCaseAscii(s, "Jul")) return 7;
    if (eqlIgnoreCaseAscii(s, "Aug")) return 8;
    if (eqlIgnoreCaseAscii(s, "Sep")) return 9;
    if (eqlIgnoreCaseAscii(s, "Oct")) return 10;
    if (eqlIgnoreCaseAscii(s, "Nov")) return 11;
    if (eqlIgnoreCaseAscii(s, "Dec")) return 12;
    return null;
}

/// Howard Hinnant days-from-civil algorithm: returns days since 1970-01-01.
fn daysFromCivil(year: i64, month: i64, day: i64): i64 {
    const y: i64 = if (month <= 2) year - 1 else year;
    const era: i64 = if (y >= 0) @divTrunc(y, 400) else @divTrunc(y - 399, 400);
    const yoe: i64 = y - era * 400;
    const mp: i64 = month + (if (month > 2) @as(i64, -3) else @as(i64, 9));
    const doy: i64 = @divTrunc(153 * mp + 2, 5) + day - 1;
    const doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

fn isValidYmdHms(year: i64, month: i64, day: i64, hour: i64, min: i64, sec: i64): bool {
    if (year < 1970 || year > 9999) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour < 0 || hour > 23) return false;
    if (min < 0 || min > 59) return false;
    if (sec < 0 || sec > 60) return false;
    return true;
}

/// Parses an S3 `Last-Modified` header in either RFC 7231 IMF-fixdate format
/// (`"Tue, 07 Jan 2025 00:19:10 GMT"`) or ISO-8601 UTC format
/// (`"2025-01-07T00:19:10Z"` / `"2025-01-07T00:19:10.123Z"`) into UTC epoch milliseconds.
pub fn parseHttpDateEpochMs(raw: []const u8): i64? {
    const s = trimAscii(raw);
    if (s.len == 0) return null;

    // Format 1: ISO-8601 "YYYY-MM-DDTHH:MM:SS(.mmm)?Z"
    if (s.len >= 20 && s[4] == '-' && s[7] == '-' && (s[10] == 'T' || s[10] == 't') && s[13] == ':' && s[16] == ':') {
        const year = parseFixedDigits(s, 0, 4) ?? return null;
        const month = parseFixedDigits(s, 5, 2) ?? return null;
        const day = parseFixedDigits(s, 8, 2) ?? return null;
        const hour = parseFixedDigits(s, 11, 2) ?? return null;
        const min = parseFixedDigits(s, 14, 2) ?? return null;
        const sec = parseFixedDigits(s, 17, 2) ?? return null;
        if (!isValidYmdHms(year, month, day, hour, min, sec)) return null;

        let ms: i64 = 0;
        let idx: usize = 19;
        if (idx < s.len && s[idx] == '.') {
            idx += 1;
            let mult: i64 = 100;
            let digits: usize = 0;
            while (idx < s.len && s[idx] >= '0' && s[idx] <= '9') {
                if (mult > 0) {
                    ms += @as(i64, s[idx] - '0') * mult;
                    mult = @divTrunc(mult, 10);
                }
                digits += 1;
                idx += 1;
            }
            if (digits == 0) return null;
        }
        if (idx != s.len - 1 || (s[idx] != 'Z' && s[idx] != 'z')) return null;

        const days = daysFromCivil(year, month, day);
        const total_sec = days * 86400 + hour * 3600 + min * 60 + sec;
        return total_sec * 1000 + ms;
    }

    // Format 2: RFC 7231 IMF-fixdate "Tue, 07 Jan 2025 00:19:10 GMT" (29 bytes)
    if (s.len == 29 && s[3] == ',' && s[4] == ' ' && s[7] == ' ' && s[11] == ' ' && s[16] == ' ' && s[19] == ':' && s[22] == ':' && s[25] == ' ') {
        if (!eqlIgnoreCaseAscii(s[26..29], "GMT")) return null;
        const day = parseFixedDigits(s, 5, 2) ?? return null;
        const month = parseMonth3(s[8..11]) ?? return null;
        const year = parseFixedDigits(s, 12, 4) ?? return null;
        const hour = parseFixedDigits(s, 17, 2) ?? return null;
        const min = parseFixedDigits(s, 20, 2) ?? return null;
        const sec = parseFixedDigits(s, 23, 2) ?? return null;
        if (!isValidYmdHms(year, month, day, hour, min, sec)) return null;

        const days = daysFromCivil(year, month, day);
        const total_sec = days * 86400 + hour * 3600 + min * 60 + sec;
        return total_sec * 1000;
    }

    return null;
}

/// Zero-allocation S3 object metadata aligned with `Bun.S3Client.stat()` / `S3File.stat()`.
pub struct S3Stat {
    size: u64 = 0,
    last_modified_ms: i64 = 0,
    has_last_modified: bool = false,
    etag_buf: [S3_STAT_MAX_ETAG_LEN]u8 = [0; S3_STAT_MAX_ETAG_LEN],
    etag_len: usize = 0,
    content_type_buf: [S3_STAT_MAX_TYPE_LEN]u8 = [0; S3_STAT_MAX_TYPE_LEN],
    content_type_len: usize = 0,
    last_modified_buf: [S3_STAT_MAX_DATE_LEN]u8 = [0; S3_STAT_MAX_DATE_LEN],
    last_modified_len: usize = 0,
    version_id_buf: [S3_STAT_MAX_VERSION_LEN]u8 = [0; S3_STAT_MAX_VERSION_LEN],
    version_id_len: usize = 0,

    /// Constructs `S3Stat` directly from metadata fields (mirrors Bun `S3Stat.init`).
    static fn init(
        size: u64,
        etag_in: []const u8,
        content_type_in: []const u8,
        last_modified_in: []const u8,
    ): S3Stat! {
        let st = S3Stat { size: size };
        st.etag_len = copyFixed(st.etag_buf[0..], trimAscii(etag_in))?;
        st.content_type_len = copyFixed(st.content_type_buf[0..], trimAscii(content_type_in))?;
        const trimmed_date = trimAscii(last_modified_in);
        if (trimmed_date.len > 0) {
            const epoch_ms = parseHttpDateEpochMs(trimmed_date) ?? return S3StatError.InvalidLastModified;
            st.last_modified_len = copyFixed(st.last_modified_buf[0..], trimmed_date)?;
            st.last_modified_ms = epoch_ms;
            st.has_last_modified = true;
        }
        return st;
    }

    /// Parses raw HTTP HEAD response headers (`\r\n` or `\n` delimited) into `S3Stat`.
    static fn parseHeadResponse(raw_headers: []const u8): S3Stat! {
        let st = S3Stat {};
        let pos: usize = 0;
        while (pos < raw_headers.len) {
            let line_end = pos;
            while (line_end < raw_headers.len && raw_headers[line_end] != '\n') {
                line_end += 1;
            }
            const raw_line = trimAscii(raw_headers[pos..line_end]);
            pos = if (line_end < raw_headers.len) line_end + 1 else raw_headers.len;

            if (raw_line.len == 0) continue;
            // Skip HTTP status line if present (e.g. "HTTP/1.1 200 OK")
            if (raw_line.len >= 5 && eqlIgnoreCaseAscii(raw_line[0..5], "HTTP/")) continue;

            let colon: usize = raw_line.len;
            let k: usize = 0;
            while (k < raw_line.len) {
                if (raw_line[k] == ':') {
                    colon = k;
                    break;
                }
                k += 1;
            }
            if (colon == raw_line.len) continue;

            const name = trimAscii(raw_line[0..colon]);
            const val = trimAscii(raw_line[colon + 1..]);

            if (eqlIgnoreCaseAscii(name, "content-length")) {
                st.size = parseUintDec(val) ?? return S3StatError.InvalidContentLength;
            } else if (eqlIgnoreCaseAscii(name, "etag")) {
                st.etag_len = copyFixed(st.etag_buf[0..], val)?;
            } else if (eqlIgnoreCaseAscii(name, "content-type")) {
                st.content_type_len = copyFixed(st.content_type_buf[0..], val)?;
            } else if (eqlIgnoreCaseAscii(name, "last-modified")) {
                const epoch_ms = parseHttpDateEpochMs(val) ?? return S3StatError.InvalidLastModified;
                st.last_modified_len = copyFixed(st.last_modified_buf[0..], val)?;
                st.last_modified_ms = epoch_ms;
                st.has_last_modified = true;
            } else if (eqlIgnoreCaseAscii(name, "x-amz-version-id")) {
                st.version_id_len = copyFixed(st.version_id_buf[0..], val)?;
            }
        }
        return st;
    }

    fn getSize(): u64 {
        return _.size;
    }

    fn etag(): []const u8 {
        return _.etag_buf[0.._.etag_len];
    }

    fn isWeakEtag(): bool {
        const e = _.etag();
        return e.len >= 2 && e[0] == 'W' && e[1] == '/';
    }

    /// Returns ETag with optional `W/` weak prefix and surrounding quotes stripped.
    fn unquotedEtag(): []const u8 {
        let e = _.etag();
        if (e.len >= 2 && e[0] == 'W' && e[1] == '/') {
            e = e[2..];
        }
        if (e.len >= 2 && e[0] == '"' && e[e.len - 1] == '"') {
            return e[1..e.len - 1];
        }
        return e;
    }

    /// S3 multipart uploads produce ETags formatted as `<md5>-<part_count>`.
    fn isMultipartEtag(): bool {
        const u = _.unquotedEtag();
        let i: usize = 0;
        while (i < u.len) {
            if (u[i] == '-') return i > 0 && i + 1 < u.len;
            i += 1;
        }
        return false;
    }

    /// Compares normalized (unquoted) ETags for cache validation.
    fn matchesEtag(other: []const u8): bool {
        const a = _.unquotedEtag();
        let b = trimAscii(other);
        if (b.len >= 2 && b[0] == 'W' && b[1] == '/') b = b[2..];
        if (b.len >= 2 && b[0] == '"' && b[b.len - 1] == '"') b = b[1..b.len - 1];
        if (a.len != b.len) return false;
        let i: usize = 0;
        while (i < a.len) {
            if (a[i] != b[i]) return false;
            i += 1;
        }
        return true;
    }

    fn contentType(): []const u8 {
        return _.content_type_buf[0.._.content_type_len];
    }

    fn lastModified(): []const u8 {
        return _.last_modified_buf[0.._.last_modified_len];
    }

    fn lastModifiedEpochMs(): i64? {
        if (!_.has_last_modified) return null;
        return _.last_modified_ms;
    }

    fn versionId(): []const u8 {
        return _.version_id_buf[0.._.version_id_len];
    }

    fn hasVersionId(): bool {
        return _.version_id_len > 0;
    }
}

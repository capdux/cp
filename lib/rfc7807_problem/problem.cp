// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:17:21Z (RFC 9457 / RFC 7807 HTTP Problem Details)
// Pure-CP zero-allocation RFC 9457 / RFC 7807 `application/problem+json` builder and serializer.

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn hexLower(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'a') + (v - 10);
}

fn writeJsonString(out: []u8, pos: *usize, s: []const u8): bool {
    if (!writeByte(out, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            if (!writeByte(out, pos, '\\') || !writeByte(out, pos, c)) return false;
        } else if (c == '\n') {
            if (!writeSlice(out, pos, "\\n")) return false;
        } else if (c == '\r') {
            if (!writeSlice(out, pos, "\\r")) return false;
        } else if (c == '\t') {
            if (!writeSlice(out, pos, "\\t")) return false;
        } else if (c < 0x20) {
            if (pos.* + 6 > out.len) return false;
            out[pos.*] = '\\';
            out[pos.* + 1] = 'u';
            out[pos.* + 2] = '0';
            out[pos.* + 3] = '0';
            out[pos.* + 4] = hexLower((c >> 4) & 0x0F);
            out[pos.* + 5] = hexLower(c & 0x0F);
            pos.* += 6;
        } else {
            if (!writeByte(out, pos, c)) return false;
        }
        i += 1;
    }
    return writeByte(out, pos, '"');
}

fn writeU16(out: []u8, pos: *usize, val: u16): bool {
    if (val == 0) return writeByte(out, pos, '0');
    let tmp: [8]u8 = [0; 8];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = @as(u8, '0') + @as(u8, v % 10);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

export const PROBLEM_JSON_CONTENT_TYPE = "application/problem+json";

/// Standard HTTP status reason phrase for common 4xx/5xx error statuses.
export fn defaultTitleForStatus(status: u16): []const u8 {
    if (status == 400) return "Bad Request";
    if (status == 401) return "Unauthorized";
    if (status == 403) return "Forbidden";
    if (status == 404) return "Not Found";
    if (status == 405) return "Method Not Allowed";
    if (status == 409) return "Conflict";
    if (status == 412) return "Precondition Failed";
    if (status == 413) return "Content Too Large";
    if (status == 415) return "Unsupported Media Type";
    if (status == 422) return "Unprocessable Content";
    if (status == 429) return "Too Many Requests";
    if (status == 500) return "Internal Server Error";
    if (status == 502) return "Bad Gateway";
    if (status == 503) return "Service Unavailable";
    if (status == 504) return "Gateway Timeout";
    return "HTTP Error";
}

export struct ProblemDetails {
    type_uri: []const u8,
    title: []const u8,
    status: u16?,
    detail: ([]const u8)?,
    instance: ([]const u8)?,

    /// Create a ProblemDetails with `type: "about:blank"` and default title for `status`.
    pub fn fromStatus(status: u16, detail: ([]const u8)?): ProblemDetails {
        return ProblemDetails {
            type_uri: "about:blank",
            title: defaultTitleForStatus(status),
            status: status,
            detail: detail,
            instance: null,
        };
    }

    pub fn custom(
        type_uri: []const u8,
        title: []const u8,
        status: u16?,
        detail: ([]const u8)?,
        instance: ([]const u8)?,
    ): ProblemDetails {
        return ProblemDetails {
            type_uri: if (type_uri.len == 0) "about:blank" else type_uri,
            title: title,
            status: status,
            detail: detail,
            instance: instance,
        };
    }

    /// Serialize into RFC 9457 / RFC 7807 `application/problem+json` payload in `out`.
    pub fn toJson(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (!writeSlice(out, &pos, "{\"type\":")) return null;
        if (!writeJsonString(out, &pos, _.type_uri)) return null;
        if (!writeSlice(out, &pos, ",\"title\":")) return null;
        if (!writeJsonString(out, &pos, _.title)) return null;
        if (st := _.status) {
            if (!writeSlice(out, &pos, ",\"status\":")) return null;
            if (!writeU16(out, &pos, st)) return null;
        }
        if (det := _.detail) {
            if (!writeSlice(out, &pos, ",\"detail\":")) return null;
            if (!writeJsonString(out, &pos, det)) return null;
        }
        if (inst := _.instance) {
            if (!writeSlice(out, &pos, ",\"instance\":")) return null;
            if (!writeJsonString(out, &pos, inst)) return null;
        }
        if (!writeByte(out, &pos, '}')) return null;
        return out[0..pos];
    }
}

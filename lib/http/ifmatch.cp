// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T20:23:41Z (RFC 9110 §13.1 Conditional Requests Evaluator)
// Pure-CP zero-allocation RFC 9110 §13.1/§13.2.2 conditional request evaluator (If-Match, If-None-Match, If-Modified-Since, If-Unmodified-Since).

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

export struct EntityTag {
    opaque_tag: []const u8, // inner content without quotes
    is_weak: bool,

    /// Parse an RFC 9110 entity-tag (`"abc"` or `W/"abc"`).
    pub fn parse(raw: []const u8): EntityTag? {
        const s = trimWs(raw);
        let is_weak = false;
        let rest = s;
        if (rest.len >= 2 && rest[0] == 'W' && rest[1] == '/') {
            is_weak = true;
            rest = rest[2..];
        }
        if (rest.len < 2 || rest[0] != '"' || rest[rest.len - 1] != '"') return null;
        return EntityTag {
            opaque_tag: rest[1..rest.len - 1],
            is_weak: is_weak,
        };
    }

    /// RFC 9110 §8.8.3.2 strong comparison: both must be strong and opaque tags byte-equal.
    pub fn strongEquals(other: EntityTag): bool {
        if (_.is_weak || other.is_weak) return false;
        return strEq(_.opaque_tag, other.opaque_tag);
    }

    /// RFC 9110 §8.8.3.2 weak comparison: opaque tags byte-equal regardless of weakness.
    pub fn weakEquals(other: EntityTag): bool {
        return strEq(_.opaque_tag, other.opaque_tag);
    }
}

/// Evaluate whether `header` (comma-separated entity-tags or `*`) matches `current_etag`.
/// `strong_mode == true` for `If-Match`; `strong_mode == false` for `If-None-Match`.
export fn etagListMatches(header: []const u8, current_etag: ([]const u8)?, strong_mode: bool): bool {
    const trimmed = trimWs(header);
    if (trimmed.len == 0) return false;
    if (trimmed.len == 1 && trimmed[0] == '*') {
        return current_etag != null;
    }
    const cur_raw = current_etag ?? return false;
    const cur = EntityTag.parse(cur_raw) ?? return false;

    let pos: usize = 0;
    while (pos < trimmed.len) {
        const start = pos;
        let in_quote = false;
        while (pos < trimmed.len) {
            const c = trimmed[pos];
            if (c == '"') {
                in_quote = !in_quote;
            } else if (c == ',' && !in_quote) {
                break;
            }
            pos += 1;
        }
        const seg = trimWs(trimmed[start..pos]);
        if (pos < trimmed.len && trimmed[pos] == ',') pos += 1;
        if (seg.len == 0) continue;
        if (seg.len == 1 && seg[0] == '*') return true;
        if (cand := EntityTag.parse(seg)) {
            if (strong_mode) {
                if (cur.strongEquals(cand)) return true;
            } else {
                if (cur.weakEquals(cand)) return true;
            }
        }
    }
    return false;
}

export enum PreconditionResult {
    proceed,
    not_modified_304,
    precondition_failed_412,
}

export struct ConditionalRequest {
    is_get_or_head: bool,
    if_match: ([]const u8)?,
    if_unmodified_since_sec: i64?,
    if_none_match: ([]const u8)?,
    if_modified_since_sec: i64?,

    pub fn empty(is_get_or_head: bool): ConditionalRequest {
        return ConditionalRequest {
            is_get_or_head: is_get_or_head,
            if_match: null,
            if_unmodified_since_sec: null,
            if_none_match: null,
            if_modified_since_sec: null,
        };
    }
}

/// Evaluate RFC 9110 §13.2.2 precedence order for HTTP conditional requests:
/// 1. If-Match -> 412 on mismatch (strong comparison)
/// 2. If-Unmodified-Since (only if If-Match absent) -> 412 if resource modified after timestamp
/// 3. If-None-Match -> 304 for GET/HEAD or 412 for other methods on match (weak comparison)
/// 4. If-Modified-Since (only if If-None-Match absent and GET/HEAD) -> 304 if resource <= timestamp
export fn evaluatePreconditions(
    req: ConditionalRequest,
    current_etag: ([]const u8)?,
    last_modified_sec: i64?,
): PreconditionResult {
    // Step 1: If-Match
    if (im := req.if_match) {
        if (!etagListMatches(im, current_etag, true)) {
            return PreconditionResult.precondition_failed_412;
        }
    } else if (ius := req.if_unmodified_since_sec) {
        // Step 2: If-Unmodified-Since
        if (lm := last_modified_sec) {
            if (lm > ius) {
                return PreconditionResult.precondition_failed_412;
            }
        }
    }

    // Step 3: If-None-Match
    if (inm := req.if_none_match) {
        if (etagListMatches(inm, current_etag, false)) {
            if (req.is_get_or_head) {
                return PreconditionResult.not_modified_304;
            }
            return PreconditionResult.precondition_failed_412;
        }
    } else if (req.is_get_or_head) {
        // Step 4: If-Modified-Since
        if (ims := req.if_modified_since_sec) {
            if (lm := last_modified_sec) {
                if (lm <= ims) {
                    return PreconditionResult.not_modified_304;
                }
            }
        }
    }

    return PreconditionResult.proceed;
}

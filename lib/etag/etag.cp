// Source: Absorbed from /data/dev/bun/src/http_types/ETag.cp
// Pure-CP zero-allocation RFC 9110 / RFC 7232 HTTP ETag parser, strong/weak
// comparison, If-None-Match / If-Match evaluation, and content ETag generator.

pub struct ParsedETag {
    /// Unquoted opaque-tag slice.
    tag: []const u8,
    /// True if prefixed with `W/`.
    is_weak: bool,

    /// Parse a single entity-tag (`"abc"` or `W/"abc"`).
    /// Returns null if malformed (missing quotes or invalid prefix).
    pub static fn parse(input: []const u8): ParsedETag? {
        let s = trimOws(input);
        if (s.len < 2) return null;

        let is_weak = false;
        if (s.len >= 2 && s[0] == 'W' && s[1] == '/') {
            is_weak = true;
            s = trimOws(s[2..]);
        }

        if (s.len < 2 || s[0] != '"' || s[s.len - 1] != '"') return null;
        const inner = s[1 .. s.len - 1];
        // Validate no unquoted control chars or inner '"' per RFC 9110 §8.8.3
        let i: usize = 0;
        while (i < inner.len) : (i += 1) {
            const c = inner[i];
            if (c == '"' || c < 0x21 || c == 0x7F) return null;
        }
        return ParsedETag{ tag: inner, is_weak: is_weak };
    }

    /// RFC 9110 §8.8.3.2 Weak Comparison: true iff opaque-tags match character-by-character.
    pub fn weakMatch(other: ParsedETag): bool {
        return strEq(_.tag, other.tag);
    }

    /// RFC 9110 §8.8.3.2 Strong Comparison: true iff neither is weak and opaque-tags match.
    pub fn strongMatch(other: ParsedETag): bool {
        if (_.is_weak || other.is_weak) return false;
        return strEq(_.tag, other.tag);
    }

    /// Format back to HTTP header syntax (`"tag"` or `W/"tag"`) into `out`.
    pub fn toHeader(out: []u8): ([]const u8)? {
        const extra: usize = if (_.is_weak) 4 else 2;
        if (_.tag.len + extra > out.len) return null;
        let p: usize = 0;
        if (_.is_weak) {
            out[0] = 'W';
            out[1] = '/';
            p = 2;
        }
        out[p] = '"';
        p += 1;
        let i: usize = 0;
        while (i < _.tag.len) : (i += 1) {
            out[p] = _.tag[i];
            p += 1;
        }
        out[p] = '"';
        p += 1;
        return out[0..p];
    }
}

/// Evaluate HTTP `If-None-Match` header against `etag` using RFC 9110 weak comparison.
/// Returns true if the header matches (e.g. server should reply `304 Not Modified`).
pub fn ifNoneMatch(etag: []const u8, header_val: []const u8): bool {
    const trimmed = trimOws(header_val);
    if (trimmed.len == 0) return false;
    if (strEq(trimmed, "*")) return true;

    const ours = ParsedETag.parse(etag) ?? return false;
    return matchHeaderList(ours, trimmed, false);
}

/// Evaluate HTTP `If-Match` header against `etag` using RFC 9110 strong comparison.
/// Returns true if the precondition succeeds.
pub fn ifMatch(etag: []const u8, header_val: []const u8): bool {
    const trimmed = trimOws(header_val);
    if (trimmed.len == 0) return false;
    if (strEq(trimmed, "*")) return true;

    const ours = ParsedETag.parse(etag) ?? return false;
    return matchHeaderList(ours, trimmed, true);
}

/// Generate a strong content ETag (`"<16-hex-fnv1a64>"`) into `out` (needs >= 18 bytes).
pub fn generateStrong(content: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 18) return null;
    const h = fnv1a64(content);
    out[0] = '"';
    writeHexU64(h, out[1..17]);
    out[17] = '"';
    return out[0..18];
}

/// Generate a weak content ETag (`W/"<16-hex-fnv1a64>"`) into `out` (needs >= 20 bytes).
pub fn generateWeak(content: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 20) return null;
    const h = fnv1a64(content);
    out[0] = 'W';
    out[1] = '/';
    out[2] = '"';
    writeHexU64(h, out[3..19]);
    out[19] = '"';
    return out[0..20];
}

// ---------- Helpers ----------

fn matchHeaderList(ours: ParsedETag, list: []const u8, strong: bool): bool {
    let pos: usize = 0;
    while (pos < list.len) {
        const start = pos;
        let in_quote = false;
        while (pos < list.len) : (pos += 1) {
            if (list[pos] == '"') {
                in_quote = !in_quote;
            } else if (list[pos] == ',' && !in_quote) {
                break;
            }
        }
        const item = list[start..pos];
        if (parsed := ParsedETag.parse(item)) {
            if (strong) {
                if (ours.strongMatch(parsed)) return true;
            } else {
                if (ours.weakMatch(parsed)) return true;
            }
        }
        if (pos < list.len && list[pos] == ',') pos += 1;
    }
    return false;
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h ^= @as(u64, data[i]);
        h = h *% 0x100000001b3;
    }
    return h;
}

fn writeHexU64(v: u64, dst: []u8) {
    const hex = "0123456789abcdef";
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const shift: u6 = @intCast((15 - i) * 4);
        dst[i] = hex[@as(usize, @intCast((v >> shift) & 0xF))];
    }
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

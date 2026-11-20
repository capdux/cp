// Source: cp/lib/http_cache/cache_policy.cp (RFC 9111 HTTP Cache Policy, EntityTag & Vary Evaluator)
// Pure CP zero-C-dependency HTTP caching semantics: Cache-Control, strong/weak ETag, conditional requests, freshness, and Vary keys.

const std = @import("std");

/// Errors returned during Cache-Control, ETag, or Vary header parsing and cache key construction.
pub err HttpCacheError {
    InvalidETag,
    InvalidCacheControl,
    InvalidHeaderValue,
    OutOfMemory,
}

/// Freshness evaluation outcome for a cached HTTP response under RFC 9111.
pub enum FreshnessStatus {
    Fresh,
    Stale,
    Uncacheable,
}

/// HTTP request header name/value pair used for Vary cache key construction.
pub struct CacheHeader {
    name: []const u8;
    value: []const u8;
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isOws(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerChar(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerChar(a[i]) != toLowerChar(b[i])) return false;
    }
    return true;
}

fn isTokenChar(c: u8): bool {
    if (c >= '0' && c <= '9') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    return switch (c) {
        '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' => true,
        else => false,
    };
}

fn isValidToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isTokenChar(s[i])) return false;
    }
    return true;
}

fn isValidEtagChar(c: u8): bool {
    // RFC 9110 Section 8.8.3: etagc = %x21 / %x23-7E / obs-text (%x80-FF)
    if (c == 0x21) return true;
    if (c >= 0x23 && c <= 0x7E) return true;
    if (c >= 0x80) return true;
    return false;
}

fn parseU64Strict(s: []const u8): u64 ! HttpCacheError {
    if (s.len == 0) return HttpCacheError.InvalidCacheControl;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return HttpCacheError.InvalidCacheControl;
        const digit = u64(c - '0');
        if (val > (std.math.maxInt(u64) - digit) / 10) {
            return HttpCacheError.InvalidCacheControl;
        }
        val = val * 10 + digit;
    }
    return val;
}

fn unquoteDirectiveValue(raw_val: []const u8, scratch: *[256]u8): ([]const u8) ! HttpCacheError {
    if (raw_val.len == 0) return HttpCacheError.InvalidCacheControl;
    if (raw_val[0] != '"') {
        if (!isValidToken(raw_val)) return HttpCacheError.InvalidCacheControl;
        return raw_val;
    }
    if (raw_val.len < 2 || raw_val[raw_val.len - 1] != '"') {
        return HttpCacheError.InvalidCacheControl;
    }
    const inner = raw_val[1..(raw_val.len - 1)];
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < inner.len) : (i += 1) {
        const c = inner[i];
        if (c == '\\') {
            if (i + 1 >= inner.len) return HttpCacheError.InvalidCacheControl;
            i += 1;
            const esc = inner[i];
            if (esc < 0x20 && esc != '\t') return HttpCacheError.InvalidCacheControl;
            if (out_len >= scratch.len) return HttpCacheError.InvalidCacheControl;
            scratch[out_len] = esc;
            out_len += 1;
        } else {
            if (c == '"' || (c < 0x20 && c != '\t') || c == 0x7F) {
                return HttpCacheError.InvalidCacheControl;
            }
            if (out_len >= scratch.len) return HttpCacheError.InvalidCacheControl;
            scratch[out_len] = c;
            out_len += 1;
        }
    }
    return scratch[0..out_len];
}

fn appendCommaDirective(
    list: *std.ArrayListUnmanaged(u8),
    alloc: std.mem.Allocator,
    has_prev: *bool,
    text: []const u8,
) ! HttpCacheError {
    if (has_prev.*) {
        list.appendSlice(alloc, ", ") catch return HttpCacheError.OutOfMemory;
    }
    list.appendSlice(alloc, text) catch return HttpCacheError.OutOfMemory;
    has_prev.* = true;
}

fn appendCommaDirectiveU64(
    list: *std.ArrayListUnmanaged(u8),
    alloc: std.mem.Allocator,
    has_prev: *bool,
    name: []const u8,
    val: u64,
) ! HttpCacheError {
    let buf: [64]u8 = undefined;
    const formatted = std.fmt.bufPrint(buf[0..], "{s}={d}", .{ name, val }) catch return HttpCacheError.OutOfMemory;
    appendCommaDirective(list, alloc, has_prev, formatted)?;
}

/// Parsed RFC 9111 `Cache-Control` directive set for HTTP requests and responses.
pub struct CacheControl {
    no_store: bool = false;
    no_cache: bool = false;
    public_flag: bool = false;
    private_flag: bool = false;
    must_revalidate: bool = false;
    immutable: bool = false;
    max_age: u64? = null;
    s_maxage: u64? = null;
    stale_while_revalidate: u64? = null;
    min_fresh: u64? = null;
    max_stale: u64? = null;

    /// Parses a `Cache-Control` header value into a `CacheControl` struct.
    static fn parse(header_val: []const u8): CacheControl ! HttpCacheError {
        let cc = CacheControl{};
        let i: usize = 0;
        let scratch: [256]u8 = undefined;

        while (i < header_val.len) {
            let seg_start = i;
            let in_quotes = false;
            while (i < header_val.len) : (i += 1) {
                const c = header_val[i];
                if (in_quotes) {
                    if (c == '\\') {
                        if (i + 1 >= header_val.len) return HttpCacheError.InvalidCacheControl;
                        i += 1;
                    } else if (c == '"') {
                        in_quotes = false;
                    }
                } else {
                    if (c == '"') {
                        in_quotes = true;
                    } else if (c == ',') {
                        break;
                    }
                }
            }
            if (in_quotes) return HttpCacheError.InvalidCacheControl;

            const seg = trimOws(header_val[seg_start..i]);
            if (i < header_val.len && header_val[i] == ',') {
                i += 1;
            }
            if (seg.len == 0) continue;

            let eq_idx: usize? = null;
            let k: usize = 0;
            while (k < seg.len) : (k += 1) {
                if (seg[k] == '=') {
                    eq_idx = k;
                    break;
                }
            }

            if (idx := eq_idx) {
                const name = seg[0..idx];
                const raw_val = seg[(idx + 1)..];
                if (!isValidToken(name)) return HttpCacheError.InvalidCacheControl;
                const val = unquoteDirectiveValue(raw_val, *scratch)?;

                if (asciiEqlIgnoreCase(name, "max-age")) {
                    cc.max_age = parseU64Strict(val)?;
                } else if (asciiEqlIgnoreCase(name, "s-maxage")) {
                    cc.s_maxage = parseU64Strict(val)?;
                } else if (asciiEqlIgnoreCase(name, "stale-while-revalidate")) {
                    cc.stale_while_revalidate = parseU64Strict(val)?;
                } else if (asciiEqlIgnoreCase(name, "min-fresh")) {
                    cc.min_fresh = parseU64Strict(val)?;
                } else if (asciiEqlIgnoreCase(name, "max-stale")) {
                    cc.max_stale = parseU64Strict(val)?;
                } else if (asciiEqlIgnoreCase(name, "no-cache")) {
                    if (val.len == 0) return HttpCacheError.InvalidCacheControl;
                    cc.no_cache = true;
                } else if (asciiEqlIgnoreCase(name, "private")) {
                    if (val.len == 0) return HttpCacheError.InvalidCacheControl;
                    cc.private_flag = true;
                } else if (
                    asciiEqlIgnoreCase(name, "no-store")
                    || asciiEqlIgnoreCase(name, "public")
                    || asciiEqlIgnoreCase(name, "must-revalidate")
                    || asciiEqlIgnoreCase(name, "immutable")
                ) {
                    return HttpCacheError.InvalidCacheControl;
                }
            } else {
                if (!isValidToken(seg)) return HttpCacheError.InvalidCacheControl;
                if (asciiEqlIgnoreCase(seg, "no-store")) {
                    cc.no_store = true;
                } else if (asciiEqlIgnoreCase(seg, "no-cache")) {
                    cc.no_cache = true;
                } else if (asciiEqlIgnoreCase(seg, "public")) {
                    cc.public_flag = true;
                } else if (asciiEqlIgnoreCase(seg, "private")) {
                    cc.private_flag = true;
                } else if (asciiEqlIgnoreCase(seg, "must-revalidate")) {
                    cc.must_revalidate = true;
                } else if (asciiEqlIgnoreCase(seg, "immutable")) {
                    cc.immutable = true;
                } else if (asciiEqlIgnoreCase(seg, "max-stale")) {
                    cc.max_stale = std.math.maxInt(u64);
                } else if (
                    asciiEqlIgnoreCase(seg, "max-age")
                    || asciiEqlIgnoreCase(seg, "s-maxage")
                    || asciiEqlIgnoreCase(seg, "stale-while-revalidate")
                    || asciiEqlIgnoreCase(seg, "min-fresh")
                ) {
                    return HttpCacheError.InvalidCacheControl;
                }
            }
        }

        return cc;
    }

    /// Serializes this `CacheControl` into a normalized `Cache-Control` header string.
    fn toHeader(alloc: std.mem.Allocator): ([]u8) ! HttpCacheError {
        let list: std.ArrayListUnmanaged(u8) = .empty;
        errdefer list.deinit(alloc);
        let has_prev = false;

        if (_.public_flag) appendCommaDirective(*list, alloc, *has_prev, "public")?;
        if (_.private_flag) appendCommaDirective(*list, alloc, *has_prev, "private")?;
        if (_.no_store) appendCommaDirective(*list, alloc, *has_prev, "no-store")?;
        if (_.no_cache) appendCommaDirective(*list, alloc, *has_prev, "no-cache")?;
        if (_.must_revalidate) appendCommaDirective(*list, alloc, *has_prev, "must-revalidate")?;
        if (_.immutable) appendCommaDirective(*list, alloc, *has_prev, "immutable")?;
        if (v := _.max_age) appendCommaDirectiveU64(*list, alloc, *has_prev, "max-age", v)?;
        if (v := _.s_maxage) appendCommaDirectiveU64(*list, alloc, *has_prev, "s-maxage", v)?;
        if (v := _.stale_while_revalidate) appendCommaDirectiveU64(*list, alloc, *has_prev, "stale-while-revalidate", v)?;
        if (v := _.min_fresh) appendCommaDirectiveU64(*list, alloc, *has_prev, "min-fresh", v)?;
        if (v := _.max_stale) {
            if (v == std.math.maxInt(u64)) {
                appendCommaDirective(*list, alloc, *has_prev, "max-stale")?;
            } else {
                appendCommaDirectiveU64(*list, alloc, *has_prev, "max-stale", v)?;
            }
        }

        return list.toOwnedSlice(alloc) catch return HttpCacheError.OutOfMemory;
    }
}

/// RFC 9110 Section 8.8.3 Entity Tag (`"opaque"` or `W/"opaque"`).
pub struct EntityTag {
    weak: bool = false;
    opaque_tag: []const u8;

    /// Parses a raw ETag string (`"abc"` or `W/"abc"`), rejecting unquoted or invalid characters.
    static fn parse(raw: []const u8): EntityTag ! HttpCacheError {
        const trimmed = trimOws(raw);
        if (trimmed.len < 2) return HttpCacheError.InvalidETag;

        let is_weak = false;
        let rest = trimmed;
        if (trimmed.len >= 2 && trimmed[0] == 'W' && trimmed[1] == '/') {
            is_weak = true;
            rest = trimmed[2..];
        }

        if (rest.len < 2 || rest[0] != '"' || rest[rest.len - 1] != '"') {
            return HttpCacheError.InvalidETag;
        }

        const inner = rest[1..(rest.len - 1)];
        let i: usize = 0;
        while (i < inner.len) : (i += 1) {
            if (!isValidEtagChar(inner[i])) return HttpCacheError.InvalidETag;
        }

        return EntityTag{
            weak: is_weak,
            opaque_tag: inner,
        };
    }

    /// RFC 9110 strong comparison: both tags must be strong and have identical opaque strings.
    static fn strongEquals(a: EntityTag, b: EntityTag): bool {
        if (a.weak || b.weak) return false;
        return std.mem.eql(u8, a.opaque_tag, b.opaque_tag);
    }

    /// RFC 9110 weak comparison: compares opaque strings regardless of weakness indicator.
    static fn weakEquals(a: EntityTag, b: EntityTag): bool {
        return std.mem.eql(u8, a.opaque_tag, b.opaque_tag);
    }

    /// Evaluates an `If-None-Match` header against `current_etag` using weak comparison (for 304/412).
    static fn matchesIfNoneMatch(header_val: []const u8, current_etag: EntityTag): bool ! HttpCacheError {
        return matchEtagList(header_val, current_etag, false);
    }

    /// Evaluates an `If-Match` header against `current_etag` using strong comparison (for 412).
    static fn matchesIfMatch(header_val: []const u8, current_etag: EntityTag): bool ! HttpCacheError {
        return matchEtagList(header_val, current_etag, true);
    }
}

fn matchEtagList(header_val: []const u8, current_etag: EntityTag, require_strong: bool): bool ! HttpCacheError {
    const trimmed_all = trimOws(header_val);
    if (trimmed_all.len == 0) return false;
    if (std.mem.eql(u8, trimmed_all, "*")) return true;

    let i: usize = 0;
    let matched = false;
    while (i < header_val.len) {
        const seg_start = i;
        let in_quotes = false;
        while (i < header_val.len) : (i += 1) {
            const c = header_val[i];
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                break;
            }
        }
        if (in_quotes) return HttpCacheError.InvalidETag;

        const item = trimOws(header_val[seg_start..i]);
        if (i < header_val.len && header_val[i] == ',') {
            i += 1;
        }
        if (item.len == 0) continue;

        const parsed = EntityTag.parse(item)?;
        if (require_strong) {
            if (EntityTag.strongEquals(parsed, current_etag)) {
                matched = true;
            }
        } else {
            if (EntityTag.weakEquals(parsed, current_etag)) {
                matched = true;
            }
        }
    }

    return matched;
}

fn freeStringList(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged([]u8)) {
    for (item of list.items) {
        alloc.free(item);
    }
    list.deinit(alloc);
}

fn asciiLessThan(a: []const u8, b: []const u8): bool {
    return std.mem.order(u8, a, b) == .lt;
}

/// RFC 9111 HTTP cache freshness evaluator and Vary cache key builder.
pub static class HttpCacheEvaluator {
    /// Evaluates response freshness considering `no-store`, `private`, `no-cache`, `s-maxage`, `max-age`, `Expires - Date`, `min-fresh`, and `max-stale`.
    pub fn evaluateResponse(
        req_cc: CacheControl,
        resp_cc: CacheControl,
        is_shared_cache: bool,
        age_sec: u64,
        expires_minus_date_sec: i64?,
    ): FreshnessStatus {
        if (resp_cc.no_store || req_cc.no_store) {
            return .Uncacheable;
        }
        if (is_shared_cache && resp_cc.private_flag) {
            return .Uncacheable;
        }
        if (resp_cc.no_cache || req_cc.no_cache) {
            return .Stale;
        }

        let lifetime_opt: u64? = null;
        if (is_shared_cache) {
            if (sm := resp_cc.s_maxage) {
                lifetime_opt = sm;
            }
        }
        if (lifetime_opt == null) {
            if (ma := resp_cc.max_age) {
                lifetime_opt = ma;
            }
        }
        if (lifetime_opt == null) {
            if (exp := expires_minus_date_sec) {
                if (exp <= 0) {
                    lifetime_opt = 0;
                } else {
                    lifetime_opt = u64(exp);
                }
            }
        }

        const lifetime = lifetime_opt ?? return .Stale;

        if (age_sec >= lifetime) {
            if (ms := req_cc.max_stale) {
                if (!resp_cc.must_revalidate && !(is_shared_cache && resp_cc.s_maxage != null)) {
                    const over = age_sec - lifetime;
                    if (ms == std.math.maxInt(u64) || over < ms) {
                        return .Fresh;
                    }
                }
            }
            return .Stale;
        }

        const remaining = lifetime - age_sec;
        if (mf := req_cc.min_fresh) {
            if (remaining < mf) {
                return .Stale;
            }
        }

        return .Fresh;
    }

    /// RFC 9110 `If-Modified-Since` check: returns `true` when resource was modified after `if_modified_since_sec`.
    pub fn isModifiedSince(last_modified_sec: i64, if_modified_since_sec: i64): bool {
        return last_modified_sec > if_modified_since_sec;
    }

    /// Builds a deterministic cache key from method, URL, `Vary` header, and request headers.
    /// Returns `null` if `Vary` contains `*` (response cannot be shared/reused across requests).
    pub fn buildVaryKey(
        alloc: std.mem.Allocator,
        method: []const u8,
        url: []const u8,
        vary_header: []const u8?,
        req_headers: []const CacheHeader,
    ): ([]u8)? ! HttpCacheError {
        let vary_names: std.ArrayListUnmanaged([]u8) = .empty;
        defer freeStringList(alloc, *vary_names);

        if (vh := vary_header) {
            let start: usize = 0;
            let i: usize = 0;
            while (i <= vh.len) : (i += 1) {
                if (i == vh.len || vh[i] == ',') {
                    const part = trimOws(vh[start..i]);
                    start = i + 1;
                    if (part.len == 0) continue;
                    if (std.mem.eql(u8, part, "*")) {
                        return null;
                    }
                    if (!isValidToken(part)) {
                        return HttpCacheError.InvalidHeaderValue;
                    }

                    let dup = false;
                    for (existing of vary_names.items) {
                        if (asciiEqlIgnoreCase(existing, part)) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) {
                        let lower_name = alloc.alloc(u8, part.len) catch return HttpCacheError.OutOfMemory;
                        let k: usize = 0;
                        while (k < part.len) : (k += 1) {
                            lower_name[k] = toLowerChar(part[k]);
                        }
                        vary_names.append(alloc, lower_name) catch {
                            alloc.free(lower_name);
                            return HttpCacheError.OutOfMemory;
                        };
                    }
                }
            }
        }

        // Sort vary header names lexicographically for deterministic key order
        if (vary_names.items.len > 1) {
            let a: usize = 1;
            while (a < vary_names.items.len) : (a += 1) {
                const key = vary_names.items[a];
                let b = a;
                while (b > 0 && asciiLessThan(key, vary_names.items[b - 1])) : (b -= 1) {
                    vary_names.items[b] = vary_names.items[b - 1];
                }
                vary_names.items[b] = key;
            }
        }

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, method) catch return HttpCacheError.OutOfMemory;
        out.append(alloc, '\n') catch return HttpCacheError.OutOfMemory;
        out.appendSlice(alloc, url) catch return HttpCacheError.OutOfMemory;
        out.append(alloc, '\n') catch return HttpCacheError.OutOfMemory;

        for (hname of vary_names.items) {
            out.appendSlice(alloc, hname) catch return HttpCacheError.OutOfMemory;
            out.append(alloc, ':') catch return HttpCacheError.OutOfMemory;
            let first_val = true;
            for (hdr of req_headers) {
                if (asciiEqlIgnoreCase(hdr.name, hname)) {
                    if (!first_val) {
                        out.appendSlice(alloc, ", ") catch return HttpCacheError.OutOfMemory;
                    }
                    out.appendSlice(alloc, trimOws(hdr.value)) catch return HttpCacheError.OutOfMemory;
                    first_val = false;
                }
            }
            out.append(alloc, '\n') catch return HttpCacheError.OutOfMemory;
        }

        const owned = out.toOwnedSlice(alloc) catch return HttpCacheError.OutOfMemory;
        return owned;
    }
}

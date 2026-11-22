// Origin: Pure-CP Stdlib (RFC 9111 HTTP Cache Freshness, Cache-Control Parser & SWR/SIE Evaluator) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation HTTP caching policy engine:
//   - Case-insensitive RFC 9111 Cache-Control parser (quoted values, extension skipping, overflow/negative guard)
//   - RFC 9111 §4.2 freshness lifetime (s-maxage > max-age > Expires-Date > 10% Last-Modified heuristic) & current age
//   - Shared vs private storability checks (no-store, private, Authorization)
//   - Lookup verdict engine: fresh_hit | stale_while_revalidate | stale_if_error | revalidate_required | miss_bypass

const std = @import("std");

/// RFC 9111 §1.2.2 recommended maximum representable delta-seconds (2^31).
pub const CACHE_MAX_DELTA_SECONDS: u64 = 2147483648;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Cache deployment scope controlling `s-maxage`, `private`, `proxy-revalidate`, and `Authorization`.
pub enum CacheScope: u8 {
    private_cache = 0,
    shared_cache = 1,
}

/// Cache lookup decision returned by `evaluateCacheLookup`.
pub enum CacheLookupVerdict: u8 {
    fresh_hit = 0,
    stale_while_revalidate = 1,
    stale_if_error = 2,
    revalidate_required = 3,
    miss_bypass = 4,
}

/// Parsed RFC 9111 + RFC 5861 `Cache-Control` directive set.
pub struct CacheControl {
    no_store: bool = false;
    no_cache: bool = false;
    is_private: bool = false;
    is_public: bool = false;
    must_revalidate: bool = false;
    proxy_revalidate: bool = false;
    immutable: bool = false;
    only_if_cached: bool = false;
    max_age: u64? = null;
    s_maxage: u64? = null;
    min_fresh: u64? = null;
    has_max_stale: bool = false;
    max_stale: u64? = null;
    stale_while_revalidate: u64? = null;
    stale_if_error: u64? = null;

    static fn empty(): CacheControl {
        return CacheControl{};
    }
}

/// Metadata of a cached HTTP response used for storability, freshness, and age calculations.
pub struct StoredResponseMeta {
    status_code: u16 = 200;
    cache_control: CacheControl = CacheControl.empty();
    date_sec: i64? = null;
    expires_sec: i64? = null;
    last_modified_sec: i64? = null;
    age_header_sec: u64 = 0;
    request_time_sec: i64 = 0;
    response_time_sec: i64 = 0;
    had_authorization: bool = false;
    has_validator: bool = false;
}

/// Request-side lookup context for `evaluateCacheLookup`.
pub struct CacheLookupRequest {
    scope: CacheScope = CacheScope.shared_cache;
    req_cc: CacheControl = CacheControl.empty();
    now_sec: i64 = 0;
    upstream_error: bool = false;
}

/// Full lookup verdict and freshness/age breakdown.
pub struct CacheLookupDecision {
    verdict: CacheLookupVerdict = CacheLookupVerdict.miss_bypass;
    freshness_lifetime_sec: u64 = 0;
    current_age_sec: u64 = 0;
    remaining_fresh_sec: u64 = 0;
    stale_for_sec: u64 = 0;
}

// ============================================================================
// Internal ASCII / Numeric Helpers
// ============================================================================

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isTokenChar(c: u8): bool {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' ||
        c == '*' || c == '+' || c == '-' || c == '.' || c == '^' || c == '_' ||
        c == '`' || c == '|' || c == '~';
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_v: u64 = std.math.maxInt(u64);
    if (a > max_v - b) {
        return max_v;
    }
    return a + b;
}

fn diffPosI64ToU64(later: i64, earlier: i64): u64 {
    if (later <= earlier) {
        return 0;
    }
    // Safe even across negative/positive i64 boundaries via i128
    const diff: i128 = (later as i128) - (earlier as i128);
    if (diff <= 0) return 0;
    const max_u64: i128 = std.math.maxInt(u64) as i128;
    if (diff >= max_u64) return std.math.maxInt(u64);
    return diff as u64;
}

/// Parses a non-negative ASCII decimal delta-seconds value with strict overflow & negative rejection.
fn parseDeltaSeconds(raw: []const u8): u64? {
    if (raw.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    const max_v: u64 = std.math.maxInt(u64);
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        const c: u8 = raw[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = (c - '0') as u64;
        if (acc > (max_v - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
    }
    return acc;
}

fn isHeuristicallyCacheableStatus(code: u16): bool {
    return code == 200 || code == 203 || code == 204 || code == 206 ||
        code == 300 || code == 301 || code == 308 ||
        code == 404 || code == 405 || code == 410 || code == 414 || code == 501;
}

// ============================================================================
// 1. RFC 9111 Case-Insensitive Cache-Control Parser
// ============================================================================

/// Parses a `Cache-Control` header value into a `CacheControl` struct.
/// Supports case-insensitive directive names, optional quoted values (`max-age="60"`),
/// `max-stale` with or without delta-seconds, and safe skipping of unknown extension directives.
/// Returns `null` on malformed syntax (unterminated quote, control/CRLF bytes, negative or
/// overflowing delta-seconds, or missing value on directives requiring delta-seconds).
pub fn parseCacheControl(header: string): CacheControl? {
    const bytes: []const u8 = header;
    let cc = CacheControl.empty();

    // Reject CR/LF/NUL injection early
    let chk: usize = 0;
    while (chk < bytes.len) : (chk += 1) {
        const b: u8 = bytes[chk];
        if (b == '\r' || b == '\n' || b == 0) {
            return null;
        }
    }

    let pos: usize = 0;
    while (pos < bytes.len) {
        // Skip leading whitespace and empty comma elements
        while (pos < bytes.len && (isOws(bytes[pos]) || bytes[pos] == ',')) : (pos += 1) {}
        if (pos >= bytes.len) break;

        // Parse directive name token
        const name_start: usize = pos;
        while (pos < bytes.len && isTokenChar(bytes[pos])) : (pos += 1) {}
        const name_end: usize = pos;
        if (name_end == name_start) {
            return null;
        }
        const name: []const u8 = bytes[name_start..name_end];

        // Skip optional whitespace before '=' or ','
        while (pos < bytes.len && isOws(bytes[pos])) : (pos += 1) {}

        let has_value: bool = false;
        let val_start: usize = 0;
        let val_end: usize = 0;
        let was_quoted: bool = false;

        if (pos < bytes.len && bytes[pos] == '=') {
            has_value = true;
            pos += 1;
            while (pos < bytes.len && isOws(bytes[pos])) : (pos += 1) {}
            if (pos >= bytes.len) {
                return null;
            }
            if (bytes[pos] == '"') {
                was_quoted = true;
                pos += 1;
                val_start = pos;
                let closed: bool = false;
                while (pos < bytes.len) : (pos += 1) {
                    if (bytes[pos] == '\\') {
                        pos += 1;
                        if (pos >= bytes.len) {
                            return null;
                        }
                        continue;
                    }
                    if (bytes[pos] == '"') {
                        val_end = pos;
                        pos += 1;
                        closed = true;
                        break;
                    }
                }
                if (!closed) {
                    return null;
                }
            } else {
                val_start = pos;
                while (pos < bytes.len && bytes[pos] != ',' && !isOws(bytes[pos])) : (pos += 1) {
                    if (bytes[pos] == '"') {
                        return null;
                    }
                }
                val_end = pos;
                if (val_end == val_start) {
                    return null;
                }
            }
            // After value, only OWS then ',' or end-of-header is valid
            while (pos < bytes.len && isOws(bytes[pos])) : (pos += 1) {}
            if (pos < bytes.len && bytes[pos] != ',') {
                return null;
            }
        } else if (pos < bytes.len && bytes[pos] != ',') {
            return null;
        }

        const val_slice: []const u8 = if (has_value) bytes[val_start..val_end] else bytes[0..0];
        * = was_quoted;

        // Match known directives (case-insensitive)
        if (eqIgnoreCase(name, "no-store")) {
            cc.no_store = true;
        } else if (eqIgnoreCase(name, "no-cache")) {
            cc.no_cache = true;
        } else if (eqIgnoreCase(name, "private")) {
            cc.is_private = true;
        } else if (eqIgnoreCase(name, "public")) {
            cc.is_public = true;
        } else if (eqIgnoreCase(name, "must-revalidate")) {
            cc.must_revalidate = true;
        } else if (eqIgnoreCase(name, "proxy-revalidate")) {
            cc.proxy_revalidate = true;
        } else if (eqIgnoreCase(name, "immutable")) {
            cc.immutable = true;
        } else if (eqIgnoreCase(name, "only-if-cached")) {
            cc.only_if_cached = true;
        } else if (eqIgnoreCase(name, "max-age")) {
            if (!has_value) return null;
            const secs: u64 = parseDeltaSeconds(val_slice) ?? return null;
            cc.max_age = secs;
        } else if (eqIgnoreCase(name, "s-maxage")) {
            if (!has_value) return null;
            const secs: u64 = parseDeltaSeconds(val_slice) ?? return null;
            cc.s_maxage = secs;
        } else if (eqIgnoreCase(name, "min-fresh")) {
            if (!has_value) return null;
            const secs: u64 = parseDeltaSeconds(val_slice) ?? return null;
            cc.min_fresh = secs;
        } else if (eqIgnoreCase(name, "max-stale")) {
            cc.has_max_stale = true;
            if (has_value) {
                const secs: u64 = parseDeltaSeconds(val_slice) ?? return null;
                cc.max_stale = secs;
            } else {
                cc.max_stale = null;
            }
        } else if (eqIgnoreCase(name, "stale-while-revalidate")) {
            if (!has_value) return null;
            const secs: u64 = parseDeltaSeconds(val_slice) ?? return null;
            cc.stale_while_revalidate = secs;
        } else if (eqIgnoreCase(name, "stale-if-error")) {
            if (!has_value) return null;
            const secs: u64 = parseDeltaSeconds(val_slice) ?? return null;
            cc.stale_if_error = secs;
        }
        // Unknown extension directives are safely skipped per RFC 9111 §5.2.3

        if (pos < bytes.len && bytes[pos] == ',') {
            pos += 1;
        }
    }

    return cc;
}

// ============================================================================
// 2. RFC 9111 §3 Storability & §4.2 Freshness / Age Calculations
// ============================================================================

/// Determines whether a response is storable in a cache of the given `scope` (RFC 9111 §3 & §3.5).
/// - Rejects `no-store`.
/// - Shared caches reject `private`, and reject `Authorization` responses unless `must-revalidate`,
///   `public`, or `s-maxage` is explicitly present.
/// - Non-default status codes require explicit freshness/public directives to be storable.
pub fn isResponseStorable(resp: StoredResponseMeta, scope: CacheScope): bool {
    const cc = resp.cache_control;
    if (cc.no_store) {
        return false;
    }
    if (scope == CacheScope.shared_cache) {
        if (cc.is_private) {
            return false;
        }
        if (resp.had_authorization) {
            const auth_allowed: bool = cc.must_revalidate || cc.is_public || (cc.s_maxage != null);
            if (!auth_allowed) {
                return false;
            }
        }
    }

    if (isHeuristicallyCacheableStatus(resp.status_code)) {
        return true;
    }
    // Other status codes require explicit caching permission / freshness lifetime
    const has_explicit_freshness: bool = cc.is_public ||
        (cc.max_age != null) ||
        (scope == CacheScope.shared_cache && cc.s_maxage != null) ||
        (resp.expires_sec != null);
    return has_explicit_freshness;
}

/// Computes RFC 9111 §4.2.1 & §4.2.2 freshness lifetime in seconds.
/// Precedence:
///   1. `s-maxage` (shared caches only; ignored by private caches)
///   2. `max-age`
///   3. `Expires - Date` (returns `0` if `Date >= Expires`)
///   4. Heuristic: 10% of `(Date - Last-Modified)` for heuristically cacheable status codes
///   5. `0` otherwise
pub fn computeFreshnessLifetime(resp: StoredResponseMeta, scope: CacheScope): u64 {
    const cc = resp.cache_control;
    if (scope == CacheScope.shared_cache) {
        if (s_max := cc.s_maxage) {
            return s_max;
        }
    }
    if (m_age := cc.max_age) {
        return m_age;
    }

    const ref_date: i64 = resp.date_sec ?? resp.response_time_sec;
    if (exp := resp.expires_sec) {
        return diffPosI64ToU64(exp, ref_date);
    }

    if (isHeuristicallyCacheableStatus(resp.status_code)) {
        if (lm := resp.last_modified_sec) {
            return diffPosI64ToU64(ref_date, lm) / 10;
        }
    }

    return 0;
}

/// Computes RFC 9111 §4.2.3 current age in seconds at `now_sec` with saturating arithmetic.
pub fn computeCurrentAge(resp: StoredResponseMeta, now_sec: i64): u64 {
    let apparent_age: u64 = 0;
    if (date_v := resp.date_sec) {
        apparent_age = diffPosI64ToU64(resp.response_time_sec, date_v);
    }
    const response_delay: u64 = diffPosI64ToU64(resp.response_time_sec, resp.request_time_sec);
    const corrected_age_value: u64 = saturatingAddU64(resp.age_header_sec, response_delay);
    const corrected_initial_age: u64 = if (apparent_age > corrected_age_value) apparent_age else corrected_age_value;
    const resident_time: u64 = diffPosI64ToU64(now_sec, resp.response_time_sec);
    return saturatingAddU64(corrected_initial_age, resident_time);
}

// ============================================================================
// 3. RFC 9111 + RFC 5861 Cache Lookup Verdict Engine
// ============================================================================

/// Evaluates whether a stored response can be served fresh, served stale via
/// `stale-while-revalidate` / `stale-if-error` / `max-stale`, must be revalidated, or bypassed.
pub fn evaluateCacheLookup(resp: StoredResponseMeta, req: CacheLookupRequest): CacheLookupDecision {
    const lifetime: u64 = computeFreshnessLifetime(resp, req.scope);
    const age: u64 = computeCurrentAge(resp, req.now_sec);
    const remaining_fresh: u64 = if (lifetime > age) (lifetime - age) else 0;
    const stale_for: u64 = if (age > lifetime) (age - lifetime) else 0;

    if (req.req_cc.no_store || !isResponseStorable(resp, req.scope)) {
        return CacheLookupDecision{
            verdict: CacheLookupVerdict.miss_bypass,
            freshness_lifetime_sec: lifetime,
            current_age_sec: age,
            remaining_fresh_sec: remaining_fresh,
            stale_for_sec: stale_for,
        };
    }

    // Explicit no-cache on request or response mandates origin revalidation
    if (req.req_cc.no_cache || resp.cache_control.no_cache) {
        return CacheLookupDecision{
            verdict: CacheLookupVerdict.revalidate_required,
            freshness_lifetime_sec: lifetime,
            current_age_sec: age,
            remaining_fresh_sec: remaining_fresh,
            stale_for_sec: stale_for,
        };
    }

    // Check freshness against response lifetime and request constraints (max-age, min-fresh)
    let is_fresh: bool = lifetime > age;
    if (is_fresh) {
        if (req_max_age := req.req_cc.max_age) {
            if (age >= req_max_age) {
                is_fresh = false;
            }
        }
    }
    if (is_fresh) {
        if (req_min_fresh := req.req_cc.min_fresh) {
            if (remaining_fresh < req_min_fresh) {
                is_fresh = false;
            }
        }
    }

    if (is_fresh) {
        return CacheLookupDecision{
            verdict: CacheLookupVerdict.fresh_hit,
            freshness_lifetime_sec: lifetime,
            current_age_sec: age,
            remaining_fresh_sec: remaining_fresh,
            stale_for_sec: stale_for,
        };
    }

    // Response is stale (or rejected by request max-age/min-fresh).
    // 1. Check RFC 5861 stale-if-error when upstream_error is active
    if (req.upstream_error) {
        let sie_window: u64? = resp.cache_control.stale_if_error;
        if (sie_window == null) {
            sie_window = req.req_cc.stale_if_error;
        }
        if (win := sie_window) {
            if (stale_for <= win) {
                return CacheLookupDecision{
                    verdict: CacheLookupVerdict.stale_if_error,
                    freshness_lifetime_sec: lifetime,
                    current_age_sec: age,
                    remaining_fresh_sec: remaining_fresh,
                    stale_for_sec: stale_for,
                };
            }
        }
    }

    const forbid_stale: bool = resp.cache_control.must_revalidate ||
        (req.scope == CacheScope.shared_cache && resp.cache_control.proxy_revalidate);

    if (!forbid_stale) {
        // 2. Check RFC 5861 stale-while-revalidate (only when request does not demand min-fresh or tighter max-age)
        let req_tightened: bool = req.req_cc.min_fresh != null;
        if (rm := req.req_cc.max_age) {
            if (age >= rm) {
                req_tightened = true;
            }
        }
        if (!req_tightened) {
            if (swr := resp.cache_control.stale_while_revalidate) {
                if (stale_for <= swr) {
                    return CacheLookupDecision{
                        verdict: CacheLookupVerdict.stale_while_revalidate,
                        freshness_lifetime_sec: lifetime,
                        current_age_sec: age,
                        remaining_fresh_sec: remaining_fresh,
                        stale_for_sec: stale_for,
                    };
                }
            }
        }

        // 3. Check client-side max-stale allowance (RFC 9111 §5.2.1.2)
        if (req.req_cc.has_max_stale && req.req_cc.min_fresh == null) {
            if (max_st := req.req_cc.max_stale) {
                if (stale_for <= max_st) {
                    return CacheLookupDecision{
                        verdict: CacheLookupVerdict.fresh_hit,
                        freshness_lifetime_sec: lifetime,
                        current_age_sec: age,
                        remaining_fresh_sec: remaining_fresh,
                        stale_for_sec: stale_for,
                    };
                }
            } else {
                // max-stale with no value accepts any staleness
                return CacheLookupDecision{
                    verdict: CacheLookupVerdict.fresh_hit,
                    freshness_lifetime_sec: lifetime,
                    current_age_sec: age,
                    remaining_fresh_sec: remaining_fresh,
                    stale_for_sec: stale_for,
                };
            }
        }
    }

    return CacheLookupDecision{
        verdict: CacheLookupVerdict.revalidate_required,
        freshness_lifetime_sec: lifetime,
        current_age_sec: age,
        remaining_fresh_sec: remaining_fresh,
        stale_for_sec: stale_for,
    };
}

// Origin: Pure-CP Stdlib (Zero-Alloc IETF RateLimit / RateLimit-Policy / Retry-After Codec & Retry Planner) | Created: 2026-11-23
// Reference-hardened against kornelski/rusty-http-cache-semantics (BSD-2-Clause) & RFC 9111/9110
// Pure CP zero-heap-allocation HTTP rate-limit header codec & client backoff planner:
//   - IETF `RateLimit` (both `limit=100, remaining=42, reset=15` and SF `"default";r=42;t=15` forms)
//   - IETF `RateLimit-Policy` (`100;w=60` and SF `"default";q=100;w=60;c=10;pk=...` forms)
//   - RFC 9110 §10.2.3 `Retry-After` (delta-seconds & IMF-fixdate HTTP-date bidirectional codec)
//   - Legacy `X-RateLimit-Limit` / `X-RateLimit-Remaining` / `X-RateLimit-Reset` parser & formatter
//   - Unified client retry planner (`planRateLimitRetry`) with strict precedence & `max_delay_ms` clamp
//   - 16-lane SIMD parameter matching & delimiter scan + integer-overflow & CRLF/control-character guards

const std = @import("std");
import {
    simdWebSemAsciiLowerEql,
    simdWebSemFindDelim,
} from "./cachePolicy.cp";

// ============================================================================
// 1. Constants & Data Models
// ============================================================================

/// Maximum byte length for an inline structured policy name.
pub const RATE_HDR_MAX_NAME_LEN: usize = 48;

/// Maximum byte length for an inline partition key (`pk=...`).
pub const RATE_HDR_MAX_PK_LEN: usize = 64;

const RATE_HDR_EPOCH_THRESHOLD: u64 = 1_000_000_000;
const RATE_HDR_DEFAULT_BASE_BACKOFF_MS: u64 = 500;
const RATE_HDR_MAX_U64: u64 = ~@as(u64, 0);

/// Parsed or buildable IETF `RateLimit` header state.
pub struct RateLimitHeader {
    has_policy_name: bool = false;
    policy_name_buf: [RATE_HDR_MAX_NAME_LEN]u8 = [0; RATE_HDR_MAX_NAME_LEN];
    policy_name_len: usize = 0;

    has_limit: bool = false;
    limit: u64 = 0;

    has_remaining: bool = false;
    remaining: u64 = 0;

    has_reset: bool = false;
    reset_s: u64 = 0;

    pub fn policyNameSlice(): []const u8 {
        return policy_name_buf[0..policy_name_len];
    }

    pub fn setPolicyName(*_, name: []const u8): bool {
        if (name.len == 0 || name.len > RATE_HDR_MAX_NAME_LEN) return false;
        if (!isSafeTokenOrName(name)) return false;
        let i: usize = 0;
        while (i < name.len) : (i += 1) {
            policy_name_buf[i] = name[i];
        }
        policy_name_len = name.len;
        has_policy_name = true;
        return true;
    }
}

/// Parsed or buildable IETF `RateLimit-Policy` header state.
pub struct RateLimitPolicyHeader {
    has_policy_name: bool = false;
    policy_name_buf: [RATE_HDR_MAX_NAME_LEN]u8 = [0; RATE_HDR_MAX_NAME_LEN];
    policy_name_len: usize = 0;

    quota: u64 = 0;
    window_s: u64 = 0;

    has_concurrency: bool = false;
    concurrency: u32 = 0;

    has_partition_key: bool = false;
    partition_key_buf: [RATE_HDR_MAX_PK_LEN]u8 = [0; RATE_HDR_MAX_PK_LEN];
    partition_key_len: usize = 0;

    pub fn policyNameSlice(): []const u8 {
        return policy_name_buf[0..policy_name_len];
    }

    pub fn partitionKeySlice(): []const u8 {
        return partition_key_buf[0..partition_key_len];
    }

    pub fn setPolicyName(*_, name: []const u8): bool {
        if (name.len == 0 || name.len > RATE_HDR_MAX_NAME_LEN) return false;
        if (!isSafeTokenOrName(name)) return false;
        let i: usize = 0;
        while (i < name.len) : (i += 1) {
            policy_name_buf[i] = name[i];
        }
        policy_name_len = name.len;
        has_policy_name = true;
        return true;
    }

    pub fn setPartitionKey(*_, pk: []const u8): bool {
        if (pk.len == 0 || pk.len > RATE_HDR_MAX_PK_LEN) return false;
        if (!isSafeHeaderParamValue(pk)) return false;
        let i: usize = 0;
        while (i < pk.len) : (i += 1) {
            partition_key_buf[i] = pk[i];
        }
        partition_key_len = pk.len;
        has_partition_key = true;
        return true;
    }
}

/// Discriminator for RFC 9110 `Retry-After` header representation.
pub enum RetryAfterKind {
    delta_seconds,
    http_date,
}

/// Parsed RFC 9110 `Retry-After` value (delta-seconds or IMF-fixdate epoch seconds).
pub struct RetryAfterValue {
    kind: RetryAfterKind = RetryAfterKind.delta_seconds;
    delta_seconds: u64 = 0;
    http_date_epoch_s: i64 = 0;
}

/// Source that determined the planned retry delay.
pub enum RetryDelaySource {
    retry_after_delta,
    retry_after_date,
    rate_limit_reset,
    exponential_backoff,
}

/// Aggregated rate-limit response header metadata fed into `planRateLimitRetry`.
pub struct RateLimitHeaderInfo {
    has_retry_after: bool = false;
    retry_after: RetryAfterValue = RetryAfterValue{};

    has_rate_limit: bool = false;
    rate_limit: RateLimitHeader = RateLimitHeader{};

    has_policy: bool = false;
    policy: RateLimitPolicyHeader = RateLimitPolicyHeader{};

    /// Base delay in milliseconds used for exponential backoff fallback (0 -> default 500ms).
    base_backoff_ms: u64 = RATE_HDR_DEFAULT_BASE_BACKOFF_MS;
}

/// Output of `planRateLimitRetry`.
pub struct RetryPlan {
    delay_ms: u64 = 0;
    source: RetryDelaySource = RetryDelaySource.exponential_backoff;
    clamped: bool = false;
    retry_immediately: bool = false;
}

// ============================================================================
// 2. Internal Byte, Integer & Buffer Helpers
// ============================================================================

fn hasCrlfOrControl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c == 0x0D || c == 0x0A || c == 0x00 || c == 0x7F) return true;
        if (c < 0x20 && c != 0x09) return true;
    }
    return false;
}

fn isSafeTokenOrName(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c <= 0x20 || c >= 0x7F || c == 0x22 || c == 0x5C || c == 0x3B || c == 0x2C) {
            return false;
        }
    }
    return true;
}

fn isSafeHeaderParamValue(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c <= 0x20 || c >= 0x7F || c == 0x3B || c == 0x2C) {
            return false;
        }
    }
    return true;
}

fn rhAsciiEqIgnoreCase(a: []const u8, b: []const u8): bool {
    return simdWebSemAsciiLowerEql(a, b);
}

fn rhBytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn rhTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == 0x20 || s[start] == 0x09)) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == 0x20 || s[end - 1] == 0x09)) : (end -= 1) {}
    return s[start..end];
}

fn rhFindByteDelim(s: []const u8, start: usize, delim: u8): usize {
    let i: usize = start;
    while (i < s.len) {
        const d: usize = simdWebSemFindDelim(s, i);
        if (d >= s.len) return s.len;
        if (s[d] == delim) return d;
        i = d + 1;
    }
    return s.len;
}

fn parseStrictU64(s: []const u8): u64? {
    if (s.len == 0 || s.len > 20) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c < 0x30 || c > 0x39) return null;
        const digit: u64 = (c - 0x30) as u64;
        if (acc > (RATE_HDR_MAX_U64 - digit) / 10) return null;
        acc = acc * 10 + digit;
    }
    return acc;
}

fn rhAppendRaw(out: *[]u8, pos: usize, s: []const u8): usize? {
    if (pos + s.len > out.len) return null;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos + i] = s[i];
    }
    return pos + s.len;
}

fn rhAppendU64(out: *[]u8, pos: usize, val_in: u64): usize? {
    if (val_in == 0) return rhAppendRaw(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val_in;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (pos + n > out.len) return null;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = tmp[n - 1 - i];
    }
    return pos + n;
}

fn rhAppendFixed2(out: *[]u8, pos: usize, val: u32): usize? {
    if (pos + 2 > out.len) return null;
    out[pos] = (0x30 as u8) + (((val / 10) % 10) as u8);
    out[pos + 1] = (0x30 as u8) + ((val % 10) as u8);
    return pos + 2;
}

fn rhAppendFixed4(out: *[]u8, pos: usize, val: u32): usize? {
    if (pos + 4 > out.len) return null;
    out[pos] = (0x30 as u8) + (((val / 1000) % 10) as u8);
    out[pos + 1] = (0x30 as u8) + (((val / 100) % 10) as u8);
    out[pos + 2] = (0x30 as u8) + (((val / 10) % 10) as u8);
    out[pos + 3] = (0x30 as u8) + ((val % 10) as u8);
    return pos + 4;
}

// ============================================================================
// 3. IETF `RateLimit` & `RateLimit-Policy` Codec
// ============================================================================

fn parseRateLimitKvParam(hdr: *RateLimitHeader, seg: []const u8): bool {
    const trimmed = rhTrimOws(seg);
    if (trimmed.len == 0) return false;

    let eq_idx: usize = trimmed.len;
    let i: usize = 0;
    while (i < trimmed.len) : (i += 1) {
        if (trimmed[i] == 0x3D) { // '='
            eq_idx = i;
            break;
        }
    }
    if (eq_idx == 0 || eq_idx >= trimmed.len - 1) return false;

    const key = rhTrimOws(trimmed[0..eq_idx]);
    const val_str = rhTrimOws(trimmed[eq_idx + 1 ..]);
    const val = parseStrictU64(val_str) ?? return false;

    if (rhAsciiEqIgnoreCase(key, "limit") || rhAsciiEqIgnoreCase(key, "q")) {
        hdr.has_limit = true;
        hdr.limit = val;
        return true;
    }
    if (rhAsciiEqIgnoreCase(key, "remaining") || rhAsciiEqIgnoreCase(key, "r")) {
        hdr.has_remaining = true;
        hdr.remaining = val;
        return true;
    }
    if (rhAsciiEqIgnoreCase(key, "reset") || rhAsciiEqIgnoreCase(key, "t")) {
        hdr.has_reset = true;
        hdr.reset_s = val;
        return true;
    }
    // Ignore unknown numeric extension parameters only if key is valid token.
    return isSafeTokenOrName(key);
}

/// Parses an IETF `RateLimit` header value in either format:
///   - Legacy/draft key-value form: `limit=100, remaining=42, reset=15`
///   - Structured-field quoted policy form: `"default";r=42;t=15` (with optional `;q=100`)
/// Rejects CRLF/control characters, malformed syntax, and overflowing/negative numbers.
pub fn parseRateLimitHeader(raw: []const u8): RateLimitHeader? {
    if (hasCrlfOrControl(raw)) return null;
    const s = rhTrimOws(raw);
    if (s.len == 0) return null;

    let res = RateLimitHeader{};

    // Structured-field quoted policy form: `"name";r=42;t=15`
    if (s[0] == 0x22) {
        let close_q: usize = s.len;
        let i: usize = 1;
        while (i < s.len) : (i += 1) {
            if (s[i] == 0x22) {
                close_q = i;
                break;
            }
        }
        if (close_q <= 1 || close_q >= s.len) return null;
        if (!res.setPolicyName(s[1..close_q])) return null;

        let pos: usize = close_q + 1;
        while (pos < s.len && (s[pos] == 0x20 || s[pos] == 0x09)) : (pos += 1) {}
        if (pos >= s.len || s[pos] != 0x3B) return null; // must have at least one ';param'
        pos += 1;

        while (pos <= s.len) {
            const next_semi: usize = rhFindByteDelim(s, pos, 0x3B);
            if (!parseRateLimitKvParam(*res, s[pos..next_semi])) return null;
            if (next_semi >= s.len) break;
            pos = next_semi + 1;
        }
        if (!res.has_limit && !res.has_remaining && !res.has_reset) return null;
        return res;
    }

    // Comma-separated key=value form: `limit=100, remaining=42, reset=15`
    let pos: usize = 0;
    while (pos <= s.len) {
        const next_comma: usize = rhFindByteDelim(s, pos, 0x2C);
        if (!parseRateLimitKvParam(*res, s[pos..next_comma])) return null;
        if (next_comma >= s.len) break;
        pos = next_comma + 1;
    }
    if (!res.has_limit && !res.has_remaining && !res.has_reset) return null;
    return res;
}

/// Formats a `RateLimit` header value into `out_buf`.
///   - `structured_sf == false`: `limit=100, remaining=42, reset=15` (omits unset fields)
///   - `structured_sf == true`: `"default";r=42;t=15` (with optional `;q=100`)
/// Returns bytes written, or `null` if no fields are set, policy name is unsafe, or `out_buf` is too small.
pub fn formatRateLimitHeader(
    hdr: RateLimitHeader,
    structured_sf: bool,
    out_buf: *[]u8,
): usize? {
    if (!hdr.has_limit && !hdr.has_remaining && !hdr.has_reset) return null;
    let pos: usize = 0;

    if (structured_sf) {
        const pname = if (hdr.has_policy_name) hdr.policyNameSlice() else "default";
        if (!isSafeTokenOrName(pname)) return null;
        pos = rhAppendRaw(out_buf, pos, "\"") ?? return null;
        pos = rhAppendRaw(out_buf, pos, pname) ?? return null;
        pos = rhAppendRaw(out_buf, pos, "\"") ?? return null;
        if (hdr.has_remaining) {
            pos = rhAppendRaw(out_buf, pos, ";r=") ?? return null;
            pos = rhAppendU64(out_buf, pos, hdr.remaining) ?? return null;
        }
        if (hdr.has_reset) {
            pos = rhAppendRaw(out_buf, pos, ";t=") ?? return null;
            pos = rhAppendU64(out_buf, pos, hdr.reset_s) ?? return null;
        }
        if (hdr.has_limit) {
            pos = rhAppendRaw(out_buf, pos, ";q=") ?? return null;
            pos = rhAppendU64(out_buf, pos, hdr.limit) ?? return null;
        }
        return pos;
    }

    let first: bool = true;
    if (hdr.has_limit) {
        pos = rhAppendRaw(out_buf, pos, "limit=") ?? return null;
        pos = rhAppendU64(out_buf, pos, hdr.limit) ?? return null;
        first = false;
    }
    if (hdr.has_remaining) {
        if (!first) {
            pos = rhAppendRaw(out_buf, pos, ", ") ?? return null;
        }
        pos = rhAppendRaw(out_buf, pos, "remaining=") ?? return null;
        pos = rhAppendU64(out_buf, pos, hdr.remaining) ?? return null;
        first = false;
    }
    if (hdr.has_reset) {
        if (!first) {
            pos = rhAppendRaw(out_buf, pos, ", ") ?? return null;
        }
        pos = rhAppendRaw(out_buf, pos, "reset=") ?? return null;
        pos = rhAppendU64(out_buf, pos, hdr.reset_s) ?? return null;
    }
    return pos;
}

fn parsePolicyParam(pol: *RateLimitPolicyHeader, has_q: *bool, has_w: *bool, seg: []const u8): bool {
    const trimmed = rhTrimOws(seg);
    if (trimmed.len == 0) return false;

    let eq_idx: usize = trimmed.len;
    let i: usize = 0;
    while (i < trimmed.len) : (i += 1) {
        if (trimmed[i] == 0x3D) {
            eq_idx = i;
            break;
        }
    }
    if (eq_idx == 0 || eq_idx >= trimmed.len - 1) return false;

    const key = rhTrimOws(trimmed[0..eq_idx]);
    const val = rhTrimOws(trimmed[eq_idx + 1 ..]);

    if (rhAsciiEqIgnoreCase(key, "w") || rhAsciiEqIgnoreCase(key, "window")) {
        const w = parseStrictU64(val) ?? return false;
        if (w == 0) return false;
        pol.window_s = w;
        has_w.* = true;
        return true;
    }
    if (rhAsciiEqIgnoreCase(key, "q") || rhAsciiEqIgnoreCase(key, "quota")) {
        const q = parseStrictU64(val) ?? return false;
        pol.quota = q;
        has_q.* = true;
        return true;
    }
    if (rhAsciiEqIgnoreCase(key, "c") || rhAsciiEqIgnoreCase(key, "concurrency")) {
        const c = parseStrictU64(val) ?? return false;
        if (c > 0xFFFFFFFF) return false;
        pol.has_concurrency = true;
        pol.concurrency = c as u32;
        return true;
    }
    if (rhAsciiEqIgnoreCase(key, "pk") || rhAsciiEqIgnoreCase(key, "partition")) {
        return pol.setPartitionKey(val);
    }
    // Allow unknown extension parameters (e.g., `comment="burst"` or `qu="requests"`)
    return isSafeTokenOrName(key) && val.len > 0;
}

/// Parses an IETF `RateLimit-Policy` header in either format:
///   - Classic quota form: `100;w=60` (with optional `;c=10;pk=:abc:`)
///   - Structured-field quoted policy form: `"default";q=100;w=60;c=10;pk=:abc:`
/// Requires valid quota and positive window (`w >= 1`). Rejects CRLF/control bytes.
pub fn parseRateLimitPolicyHeader(raw: []const u8): RateLimitPolicyHeader? {
    if (hasCrlfOrControl(raw)) return null;
    const s = rhTrimOws(raw);
    if (s.len == 0) return null;

    let pol = RateLimitPolicyHeader{};
    let has_q: bool = false;
    let has_w: bool = false;
    let pos: usize = 0;

    if (s[0] == 0x22) {
        let close_q: usize = s.len;
        let i: usize = 1;
        while (i < s.len) : (i += 1) {
            if (s[i] == 0x22) {
                close_q = i;
                break;
            }
        }
        if (close_q <= 1 || close_q >= s.len) return null;
        if (!pol.setPolicyName(s[1..close_q])) return null;
        pos = close_q + 1;
        while (pos < s.len && (s[pos] == 0x20 || s[pos] == 0x09)) : (pos += 1) {}
        if (pos >= s.len || s[pos] != 0x3B) return null;
        pos += 1;
    } else {
        // Leading quota integer before first ';'
        let semi_idx: usize = 0;
        while (semi_idx < s.len && s[semi_idx] != 0x3B) : (semi_idx += 1) {}
        if (semi_idx >= s.len) return null;
        const q_str = rhTrimOws(s[0..semi_idx]);
        const q = parseStrictU64(q_str) ?? return null;
        pol.quota = q;
        has_q = true;
        pos = semi_idx + 1;
    }

    while (pos <= s.len) {
        let next_semi: usize = pos;
        while (next_semi < s.len && s[next_semi] != 0x3B) : (next_semi += 1) {}
        if (!parsePolicyParam(*pol, *has_q, *has_w, s[pos..next_semi])) return null;
        if (next_semi >= s.len) break;
        pos = next_semi + 1;
    }

    if (!has_q || !has_w || pol.window_s == 0) return null;
    return pol;
}

/// Formats an IETF `RateLimit-Policy` header into `out_buf`.
///   - `structured_sf == false`: `100;w=60` (plus optional `;c=...`, `;pk=...`)
///   - `structured_sf == true`: `"default";q=100;w=60` (plus optional `;c=...`, `;pk=...`)
pub fn formatRateLimitPolicyHeader(
    pol: RateLimitPolicyHeader,
    structured_sf: bool,
    out_buf: *[]u8,
): usize? {
    if (pol.window_s == 0) return null;
    let pos: usize = 0;

    if (structured_sf) {
        const pname = if (pol.has_policy_name) pol.policyNameSlice() else "default";
        if (!isSafeTokenOrName(pname)) return null;
        pos = rhAppendRaw(out_buf, pos, "\"") ?? return null;
        pos = rhAppendRaw(out_buf, pos, pname) ?? return null;
        pos = rhAppendRaw(out_buf, pos, "\";q=") ?? return null;
        pos = rhAppendU64(out_buf, pos, pol.quota) ?? return null;
    } else {
        pos = rhAppendU64(out_buf, pos, pol.quota) ?? return null;
    }

    pos = rhAppendRaw(out_buf, pos, ";w=") ?? return null;
    pos = rhAppendU64(out_buf, pos, pol.window_s) ?? return null;

    if (pol.has_concurrency) {
        pos = rhAppendRaw(out_buf, pos, ";c=") ?? return null;
        pos = rhAppendU64(out_buf, pos, pol.concurrency as u64) ?? return null;
    }
    if (pol.has_partition_key) {
        const pk = pol.partitionKeySlice();
        if (!isSafeHeaderParamValue(pk)) return null;
        pos = rhAppendRaw(out_buf, pos, ";pk=") ?? return null;
        pos = rhAppendRaw(out_buf, pos, pk) ?? return null;
    }
    return pos;
}

// ============================================================================
// 4. RFC 9110 `Retry-After` (Delta-Seconds & IMF-fixdate) Codec
// ============================================================================

fn parseWeekday3(s: []const u8): u8? {
    if (rhBytesEq(s, "Sun")) return 0;
    if (rhBytesEq(s, "Mon")) return 1;
    if (rhBytesEq(s, "Tue")) return 2;
    if (rhBytesEq(s, "Wed")) return 3;
    if (rhBytesEq(s, "Thu")) return 4;
    if (rhBytesEq(s, "Fri")) return 5;
    if (rhBytesEq(s, "Sat")) return 6;
    return null;
}

fn formatWeekday3(wday: u8): []const u8 {
    if (wday == 0) return "Sun";
    if (wday == 1) return "Mon";
    if (wday == 2) return "Tue";
    if (wday == 3) return "Wed";
    if (wday == 4) return "Thu";
    if (wday == 5) return "Fri";
    return "Sat";
}

fn parseMonth3(s: []const u8): u32? {
    if (rhBytesEq(s, "Jan")) return 1;
    if (rhBytesEq(s, "Feb")) return 2;
    if (rhBytesEq(s, "Mar")) return 3;
    if (rhBytesEq(s, "Apr")) return 4;
    if (rhBytesEq(s, "May")) return 5;
    if (rhBytesEq(s, "Jun")) return 6;
    if (rhBytesEq(s, "Jul")) return 7;
    if (rhBytesEq(s, "Aug")) return 8;
    if (rhBytesEq(s, "Sep")) return 9;
    if (rhBytesEq(s, "Oct")) return 10;
    if (rhBytesEq(s, "Nov")) return 11;
    if (rhBytesEq(s, "Dec")) return 12;
    return null;
}

fn formatMonth3(m: u32): []const u8 {
    if (m == 1) return "Jan";
    if (m == 2) return "Feb";
    if (m == 3) return "Mar";
    if (m == 4) return "Apr";
    if (m == 5) return "May";
    if (m == 6) return "Jun";
    if (m == 7) return "Jul";
    if (m == 8) return "Aug";
    if (m == 9) return "Sep";
    if (m == 10) return "Oct";
    if (m == 11) return "Nov";
    return "Dec";
}

fn isLeapYear(y: i64): bool {
    return (@rem(y, 4) == 0 && @rem(y, 100) != 0) || (@rem(y, 400) == 0);
}

fn daysInMonth(y: i64, m: u32): u32 {
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    if (m == 2) return if (isLeapYear(y)) 29 else 28;
    return 31;
}

// Howard Hinnant civil-from-days / days-from-civil (1970-01-01 == 0)
fn daysFromCivil(y_in: i64, m: u32, d: u32): i64 {
    const y: i64 = if (m <= 2) y_in - 1 else y_in;
    const era: i64 = @divFloor(y, 400);
    const yoe: i64 = y - era * 400;
    const mp: i64 = if (m > 2) (m as i64) - 3 else (m as i64) + 9;
    const doy: i64 = @divTrunc(153 * mp + 2, 5) + (d as i64) - 1;
    const doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

fn parseFixedDigits(s: []const u8): u32? {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (c < 0x30 || c > 0x39) return null;
        acc = acc * 10 + ((c - 0x30) as u32);
    }
    return acc;
}

/// Parses RFC 9110 §5.6.7 IMF-fixdate: `Sun, 06 Nov 1994 08:49:37 GMT` (29 bytes).
fn parseImfFixdate(s: []const u8): i64? {
    if (s.len != 29) return null;
    if (s[3] != 0x2C || s[4] != 0x20 || s[7] != 0x20 || s[11] != 0x20 || s[16] != 0x20) return null;
    if (s[19] != 0x3A || s[22] != 0x3A || s[25] != 0x20) return null;
    if (!rhBytesEq(s[26..29], "GMT")) return null;

    const wday = parseWeekday3(s[0..3]) ?? return null;
    const day = parseFixedDigits(s[5..7]) ?? return null;
    const month = parseMonth3(s[8..11]) ?? return null;
    const year = parseFixedDigits(s[12..16]) ?? return null;
    const hour = parseFixedDigits(s[17..19]) ?? return null;
    const minute = parseFixedDigits(s[20..22]) ?? return null;
    const second = parseFixedDigits(s[23..25]) ?? return null;

    if (year < 1970 || day < 1 || day > daysInMonth(year as i64, month)) return null;
    if (hour > 23 || minute > 59 || second > 59) return null;

    const days = daysFromCivil(year as i64, month, day);
    // Validate weekday consistency (1970-01-01 was Thursday = 4).
    const expected_wday: u8 = (@mod(days + 4, 7)) as u8;
    if (wday != expected_wday) return null;

    return days * 86400 + (hour as i64) * 3600 + (minute as i64) * 60 + (second as i64);
}

/// Parses an RFC 9110 `Retry-After` header value (either non-negative delta-seconds or IMF-fixdate).
/// Rejects CRLF/control bytes, negative values, malformed dates, and overflowing integers.
pub fn parseRetryAfter(raw: []const u8): RetryAfterValue? {
    if (hasCrlfOrControl(raw)) return null;
    const s = rhTrimOws(raw);
    if (s.len == 0) return null;

    if (delta := parseStrictU64(s)) {
        return RetryAfterValue{
            kind: RetryAfterKind.delta_seconds,
            delta_seconds: delta,
            http_date_epoch_s: 0,
        };
    }

    if (epoch_s := parseImfFixdate(s)) {
        return RetryAfterValue{
            kind: RetryAfterKind.http_date,
            delta_seconds: 0,
            http_date_epoch_s: epoch_s,
        };
    }

    return null;
}

/// Formats a delta-seconds `Retry-After` header value into `out_buf`.
pub fn formatRetryAfterDelta(seconds: u64, out_buf: *[]u8): usize? {
    return rhAppendU64(out_buf, 0, seconds);
}

/// Formats a Unix epoch timestamp (`>= 0`, year `1970..=9999`) as an RFC 9110 IMF-fixdate
/// (`Wed, 21 Oct 2015 07:28:00 GMT`, 29 bytes) into `out_buf`.
pub fn formatRetryAfterHttpDate(epoch_s: i64, out_buf: *[]u8): usize? {
    if (epoch_s < 0 || out_buf.len < 29) return null;

    const days: i64 = @divFloor(epoch_s, 86400);
    const rem_s: i64 = epoch_s - days * 86400;
    const hour: u32 = (@divTrunc(rem_s, 3600)) as u32;
    const minute: u32 = (@divTrunc(@rem(rem_s, 3600), 60)) as u32;
    const second: u32 = (@rem(rem_s, 60)) as u32;

    // Howard Hinnant civil-from-days
    const z: i64 = days + 719468;
    const era: i64 = @divFloor(z, 146097);
    const doe: i64 = z - era * 146097;
    const yoe: i64 = @divTrunc(doe - @divTrunc(doe, 1460) + @divTrunc(doe, 36524) - @divTrunc(doe, 146096), 365);
    const y0: i64 = yoe + era * 400;
    const doy: i64 = doe - (365 * yoe + @divTrunc(yoe, 4) - @divTrunc(yoe, 100));
    const mp: i64 = @divTrunc(5 * doy + 2, 153);
    const day: u32 = (doy - @divTrunc(153 * mp + 2, 5) + 1) as u32;
    const month: u32 = (if (mp < 10) mp + 3 else mp - 9) as u32;
    const year_i: i64 = if (month <= 2) y0 + 1 else y0;
    if (year_i < 1970 || year_i > 9999) return null;

    const wday: u8 = (@mod(days + 4, 7)) as u8;
    let pos: usize = 0;
    pos = rhAppendRaw(out_buf, pos, formatWeekday3(wday)) ?? return null;
    pos = rhAppendRaw(out_buf, pos, ", ") ?? return null;
    pos = rhAppendFixed2(out_buf, pos, day) ?? return null;
    pos = rhAppendRaw(out_buf, pos, " ") ?? return null;
    pos = rhAppendRaw(out_buf, pos, formatMonth3(month)) ?? return null;
    pos = rhAppendRaw(out_buf, pos, " ") ?? return null;
    pos = rhAppendFixed4(out_buf, pos, year_i as u32) ?? return null;
    pos = rhAppendRaw(out_buf, pos, " ") ?? return null;
    pos = rhAppendFixed2(out_buf, pos, hour) ?? return null;
    pos = rhAppendRaw(out_buf, pos, ":") ?? return null;
    pos = rhAppendFixed2(out_buf, pos, minute) ?? return null;
    pos = rhAppendRaw(out_buf, pos, ":") ?? return null;
    pos = rhAppendFixed2(out_buf, pos, second) ?? return null;
    pos = rhAppendRaw(out_buf, pos, " GMT") ?? return null;
    return pos;
}

// ============================================================================
// 5. Legacy `X-RateLimit-*` Header Parser & Formatter
// ============================================================================

/// Parses legacy `X-RateLimit-Limit`, `X-RateLimit-Remaining`, and `X-RateLimit-Reset` headers.
/// Any empty header slice is treated as absent. If `X-RateLimit-Reset` is a Unix epoch timestamp
/// (`>= 1_000_000_000`), it is converted to delta seconds relative to `now_epoch_s` (clamped at 0).
/// Returns `null` if all headers are absent or any present header is malformed / contains CRLF.
pub fn parseLegacyRateLimitHeaders(
    limit_hdr: []const u8,
    remaining_hdr: []const u8,
    reset_hdr: []const u8,
    now_epoch_s: i64,
): RateLimitHeader? {
    if (hasCrlfOrControl(limit_hdr) || hasCrlfOrControl(remaining_hdr) || hasCrlfOrControl(reset_hdr)) {
        return null;
    }

    let res = RateLimitHeader{};
    const l_s = rhTrimOws(limit_hdr);
    const r_s = rhTrimOws(remaining_hdr);
    const t_s = rhTrimOws(reset_hdr);

    if (l_s.len > 0) {
        const v = parseStrictU64(l_s) ?? return null;
        res.has_limit = true;
        res.limit = v;
    }
    if (r_s.len > 0) {
        const v = parseStrictU64(r_s) ?? return null;
        res.has_remaining = true;
        res.remaining = v;
    }
    if (t_s.len > 0) {
        const raw_reset = parseStrictU64(t_s) ?? return null;
        res.has_reset = true;
        if (raw_reset >= RATE_HDR_EPOCH_THRESHOLD) {
            const now_u: u64 = if (now_epoch_s > 0) (now_epoch_s as u64) else 0;
            res.reset_s = if (raw_reset > now_u) raw_reset - now_u else 0;
        } else {
            res.reset_s = raw_reset;
        }
    }

    if (!res.has_limit && !res.has_remaining && !res.has_reset) return null;
    return res;
}

/// Formats legacy `X-RateLimit-Limit`, `X-RateLimit-Remaining`, and `X-RateLimit-Reset` headers
/// (CRLF-delimited lines) into `out_buf`. When `reset_as_epoch` is `true`, `X-RateLimit-Reset`
/// is emitted as `now_epoch_s + reset_s`.
pub fn formatLegacyRateLimitHeaders(
    hdr: RateLimitHeader,
    reset_as_epoch: bool,
    now_epoch_s: i64,
    out_buf: *[]u8,
): usize? {
    if (!hdr.has_limit && !hdr.has_remaining && !hdr.has_reset) return null;
    let pos: usize = 0;

    if (hdr.has_limit) {
        pos = rhAppendRaw(out_buf, pos, "X-RateLimit-Limit: ") ?? return null;
        pos = rhAppendU64(out_buf, pos, hdr.limit) ?? return null;
        pos = rhAppendRaw(out_buf, pos, "\r\n") ?? return null;
    }
    if (hdr.has_remaining) {
        pos = rhAppendRaw(out_buf, pos, "X-RateLimit-Remaining: ") ?? return null;
        pos = rhAppendU64(out_buf, pos, hdr.remaining) ?? return null;
        pos = rhAppendRaw(out_buf, pos, "\r\n") ?? return null;
    }
    if (hdr.has_reset) {
        pos = rhAppendRaw(out_buf, pos, "X-RateLimit-Reset: ") ?? return null;
        let reset_out: u64 = hdr.reset_s;
        if (reset_as_epoch && now_epoch_s > 0) {
            const now_u: u64 = now_epoch_s as u64;
            if (reset_out > RATE_HDR_MAX_U64 - now_u) {
                reset_out = RATE_HDR_MAX_U64;
            } else {
                reset_out = reset_out + now_u;
            }
        }
        pos = rhAppendU64(out_buf, pos, reset_out) ?? return null;
        pos = rhAppendRaw(out_buf, pos, "\r\n") ?? return null;
    }
    return pos;
}

// ============================================================================
// 6. Unified Client Retry / Backoff Planner
// ============================================================================

fn satSecToMs(sec: u64): u64 {
    if (sec > RATE_HDR_MAX_U64 / 1000) return RATE_HDR_MAX_U64;
    return sec * 1000;
}

/// Computes the client retry delay from parsed HTTP rate-limit response headers:
///   1. Respects `Retry-After` first (delta-seconds or IMF-fixdate minus `now_epoch_s`, i128 overflow-safe).
///   2. Falls back to `RateLimit` `reset_s` when `Retry-After` is absent.
///   3. Falls back to overflow-safe exponential backoff (`base_backoff_ms * 2^attempt`) when both are absent.
///   4. Clamps the computed delay to `max_delay_ms` (when `max_delay_ms > 0`).
pub fn planRateLimitRetry(
    info: RateLimitHeaderInfo,
    attempt: u32,
    now_epoch_s: i64,
    max_delay_ms: u64,
): RetryPlan {
    let delay_ms: u64 = 0;
    let source: RetryDelaySource = RetryDelaySource.exponential_backoff;

    if (info.has_retry_after) {
        if (info.retry_after.kind == RetryAfterKind.delta_seconds) {
            delay_ms = satSecToMs(info.retry_after.delta_seconds);
            source = RetryDelaySource.retry_after_delta;
        } else {
            const diff_128: i128 = (info.retry_after.http_date_epoch_s as i128) - (now_epoch_s as i128);
            const max_u64_128: i128 = RATE_HDR_MAX_U64 as i128;
            const clamped_s: u64 = if (diff_128 <= 0) 0 else if (diff_128 >= max_u64_128) RATE_HDR_MAX_U64 else (diff_128 as u64);
            delay_ms = satSecToMs(clamped_s);
            source = RetryDelaySource.retry_after_date;
        }
    } else if (info.has_rate_limit && info.rate_limit.has_reset) {
        delay_ms = satSecToMs(info.rate_limit.reset_s);
        source = RetryDelaySource.rate_limit_reset;
    } else {
        const base: u64 = if (info.base_backoff_ms > 0) info.base_backoff_ms else RATE_HDR_DEFAULT_BASE_BACKOFF_MS;
        const shift: u6 = if (attempt > 20) (20 as u6) else (attempt as u6);
        const mult: u64 = (1 as u64) << shift;
        if (base > RATE_HDR_MAX_U64 / mult) {
            delay_ms = RATE_HDR_MAX_U64;
        } else {
            delay_ms = base * mult;
        }
        source = RetryDelaySource.exponential_backoff;
    }

    let clamped: bool = false;
    if (max_delay_ms > 0 && delay_ms > max_delay_ms) {
        delay_ms = max_delay_ms;
        clamped = true;
    }

    return RetryPlan{
        delay_ms: delay_ms,
        source: source,
        clamped: clamped,
        retry_immediately: delay_ms == 0,
    };
}

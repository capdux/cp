// Origin: Pure-CP Stdlib (RFC 6797 HTTP Strict-Transport-Security Parser, Store & HTTPS Upgrade Engine) | Created: 2026-11-23
// Reference-hardened against actix/actix-web (MIT OR Apache-2.0) & RFC 6797
// 100% pure CP zero-heap-allocation RFC 6797 HSTS policy engine:
//   - Strict `parseHstsHeader` / `formatHstsHeader`: 16-lane SIMD case-insensitive directives, quoted `max-age`,
//     duplicate-directive rejection, invalid/negative/overflowing `max-age` rejection, and CRLF/CTL injection guard
//   - RFC 6797 §8.1.1 host eligibility (`isHstsEligibleHost`): rejects IPv4/IPv6 literals, empty/malformed
//     hosts, and normalizes lowercase + strips trailing FQDN dot
//   - `HstsStore`: dynamic & immutable preload domain registry (`<= 64` domains), exact-match-first
//     with strict dot-boundary parent `includeSubDomains` walk, `max-age=0` dynamic revocation, and TTL eviction

const std = @import("std");
import { simdWebAuthAsciiLowerEql } from "./csrfGuard.cp";

// ============================================================================
// 1. Constants, Error Set & Data Models
// ============================================================================

/// Fixed maximum number of HSTS domain entries stored inline in `HstsStore`.
pub const HSTS_MAX_DOMAINS: usize = 64;

/// Maximum inline byte length for a normalized HSTS domain name.
pub const HSTS_HOST_MAX_LEN: usize = 128;

/// Minimum `max-age` (1 year = 31,536,000 seconds) required for HSTS preload eligibility.
pub const HSTS_PRELOAD_MIN_MAX_AGE: u64 = 31_536_000;

/// Error set for RFC 6797 HSTS header parsing, formatting, and domain store operations.
pub err HstsError {
    EmptyHeader,
    MissingMaxAge,
    DuplicateDirective,
    InvalidMaxAge,
    CrlfRejected,
    InvalidHost,
    IpLiteralDisallowed,
    StoreFull,
    BufferTooSmall,
}

/// Parsed RFC 6797 `Strict-Transport-Security` header policy.
pub struct HstsPolicy {
    max_age_sec: u64 = 0;
    include_subdomains: bool = false;
    preload: bool = false;

    /// Returns `true` iff this policy meets HSTS preload submission requirements:
    /// `max_age_sec >= 31_536_000 && include_subdomains && preload`.
    fn isPreloadEligible(): bool {
        return max_age_sec >= HSTS_PRELOAD_MIN_MAX_AGE && include_subdomains && preload;
    }
}

/// Single stored HSTS host policy entry inside `HstsStore`.
pub struct HstsEntry {
    active: bool = false;
    host_buf: [HSTS_HOST_MAX_LEN]u8 = [0; HSTS_HOST_MAX_LEN];
    host_len: usize = 0;
    expires_at_sec: u64 = 0;
    include_subdomains: bool = false;
    is_preload: bool = false;

    /// Returns the normalized lowercase domain name slice.
    fn host(): []const u8 {
        return host_buf[0..host_len];
    }
}

/// Upgrade decision returned by `HstsStore.shouldUpgrade`.
pub struct HstsLookupResult {
    must_upgrade: bool = false;
    matched_exact: bool = false;
    matched_via_subdomain: bool = false;
    is_preload: bool = false;
}

/// Aggregate occupancy and enforcement telemetry for `HstsStore`.
pub struct HstsStoreStats {
    entry_count: usize = 0;
    preload_count: usize = 0;
    upgrades_enforced: u64 = 0;
    expired_evictions: u64 = 0;
}

// ============================================================================
// 2. Internal String, Host Normalization & Validation Helpers
// ============================================================================

fn hstsHasCtrlOrCrlf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if ((c < 0x20 && c != '\t') || c == 0x7F) return true;
    }
    return false;
}

fn hstsToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn hstsTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn hstsAsciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    return simdWebAuthAsciiLowerEql(a, b);
}

fn hstsIsAlphaNumAscii(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn hstsIsDigitAscii(c: u8): bool {
    return c >= '0' && c <= '9';
}

/// Validates and normalizes `raw_host` into `out_buf` (lowercase, trailing FQDN dot stripped).
/// Rejects IPv4/IPv6 literals with `HstsError.IpLiteralDisallowed` per RFC 6797 §8.1.1.
fn normalizeHstsHost(raw_host: []const u8, out_buf: *[HSTS_HOST_MAX_LEN]u8): usize ! HstsError {
    if (hstsHasCtrlOrCrlf(raw_host)) return HstsError.CrlfRejected;
    let trimmed: []const u8 = hstsTrimOws(raw_host);
    if (trimmed.len == 0) return HstsError.InvalidHost;

    // Strip a single trailing FQDN dot (e.g. "example.com.")
    if (trimmed[trimmed.len - 1] == '.') {
        trimmed = trimmed[0 .. trimmed.len - 1];
    }
    if (trimmed.len == 0 || trimmed.len > HSTS_HOST_MAX_LEN) return HstsError.InvalidHost;

    // Detect IPv6 literals (contain ':' or brackets)
    let check_i: usize = 0;
    while (check_i < trimmed.len) : (check_i += 1) {
        const c: u8 = trimmed[check_i];
        if (c == ':' || c == '[' || c == ']') return HstsError.IpLiteralDisallowed;
    }

    // Validate DNS labels and check if all labels / last label are purely numeric (IPv4 literal)
    let label_start: usize = 0;
    let last_label_all_digits: bool = true;
    let i: usize = 0;
    while (i <= trimmed.len) : (i += 1) {
        const at_end: bool = (i == trimmed.len);
        if (at_end || trimmed[i] == '.') {
            const label_len: usize = i - label_start;
            if (label_len == 0 || label_len > 63) return HstsError.InvalidHost;
            if (trimmed[label_start] == '-' || trimmed[i - 1] == '-') return HstsError.InvalidHost;
            let all_digits: bool = true;
            let k: usize = label_start;
            while (k < i) : (k += 1) {
                const ch: u8 = trimmed[k];
                if (!hstsIsAlphaNumAscii(ch) && ch != '-') return HstsError.InvalidHost;
                if (!hstsIsDigitAscii(ch)) all_digits = false;
                out_buf[k] = hstsToLowerAscii(ch);
            }
            if (!at_end) {
                out_buf[i] = '.';
            }
            last_label_all_digits = all_digits;
            label_start = i + 1;
        }
    }

    // Per RFC 3986 / RFC 6797 §8.1.1: purely numeric top-level label indicates IPv4 literal
    if (last_label_all_digits) return HstsError.IpLiteralDisallowed;
    return trimmed.len;
}

/// Returns `true` if `host` is a valid non-IP DNS hostname eligible for HSTS per RFC 6797 §8.1.1.
pub fn isHstsEligibleHost(host: []const u8): bool {
    let scratch: [HSTS_HOST_MAX_LEN]u8 = [0; HSTS_HOST_MAX_LEN];
    const ok = normalizeHstsHost(host, *scratch) catch {
        return false;
    };
    return ok > 0;
}

// ============================================================================
// 3. RFC 6797 Header Parser & Formatter
// ============================================================================

fn parseDecimalU64(s: []const u8): u64 ! HstsError {
    if (s.len == 0) return HstsError.InvalidMaxAge;
    let acc: u64 = 0;
    const max_u64: u64 = ~@as(u64, 0);
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!hstsIsDigitAscii(s[i])) return HstsError.InvalidMaxAge;
        const d: u64 = @as(u64, s[i] - '0');
        if (acc > (max_u64 - d) / 10) return HstsError.InvalidMaxAge;
        acc = acc * 10 + d;
    }
    return acc;
}

/// Parses an RFC 6797 `Strict-Transport-Security` header value into `HstsPolicy`.
///
/// - Case-insensitive directive names (`max-age`, `includeSubDomains`, `preload`) via 16-lane SIMD.
/// - Supports optional quoted `max-age="31536000"`.
/// - Rejects CRLF/control-byte injection, missing `max-age`, duplicate directives, and non-numeric `max-age`.
pub fn parseHstsHeader(header: []const u8): HstsPolicy ! HstsError {
    if (hstsHasCtrlOrCrlf(header)) return HstsError.CrlfRejected;
    const trimmed: []const u8 = hstsTrimOws(header);
    if (trimmed.len == 0) return HstsError.EmptyHeader;

    let pol = HstsPolicy{};
    let seen_max_age: bool = false;
    let seen_subdomains: bool = false;
    let seen_preload: bool = false;

    let pos: usize = 0;
    while (pos <= trimmed.len) {
        let next_semi: usize = pos;
        while (next_semi < trimmed.len && trimmed[next_semi] != ';') : (next_semi += 1) {}

        const seg: []const u8 = hstsTrimOws(trimmed[pos..next_semi]);
        if (seg.len > 0) {
            let eq_pos: usize = seg.len;
            let k: usize = 0;
            while (k < seg.len) : (k += 1) {
                if (seg[k] == '=') {
                    eq_pos = k;
                    break;
                }
            }

            const dir_name: []const u8 = hstsTrimOws(seg[0..eq_pos]);
            const has_val: bool = eq_pos < seg.len;
            let dir_val: []const u8 = if (has_val) hstsTrimOws(seg[eq_pos + 1 .. seg.len]) else seg[0..0];

            if (hstsAsciiEqlIgnoreCase(dir_name, "max-age")) {
                if (seen_max_age) return HstsError.DuplicateDirective;
                if (!has_val) return HstsError.InvalidMaxAge;
                if (dir_val.len >= 2 && dir_val[0] == '"' && dir_val[dir_val.len - 1] == '"') {
                    dir_val = dir_val[1 .. dir_val.len - 1];
                }
                pol.max_age_sec = parseDecimalU64(dir_val)?;
                seen_max_age = true;
            } else if (hstsAsciiEqlIgnoreCase(dir_name, "includeSubDomains")) {
                if (seen_subdomains) return HstsError.DuplicateDirective;
                if (has_val) return HstsError.InvalidMaxAge;
                pol.include_subdomains = true;
                seen_subdomains = true;
            } else if (hstsAsciiEqlIgnoreCase(dir_name, "preload")) {
                if (seen_preload) return HstsError.DuplicateDirective;
                if (has_val) return HstsError.InvalidMaxAge;
                pol.preload = true;
                seen_preload = true;
            }
        }

        if (next_semi == trimmed.len) break;
        pos = next_semi + 1;
    }

    if (!seen_max_age) return HstsError.MissingMaxAge;
    return pol;
}

fn writeLiteral(dst: *[]u8, pos: usize, lit: []const u8): usize ! HstsError {
    if (lit.len > dst.len - pos) return HstsError.BufferTooSmall;
    let i: usize = 0;
    while (i < lit.len) : (i += 1) {
        dst[pos + i] = lit[i];
    }
    return pos + lit.len;
}

fn writeU64Decimal(dst: *[]u8, pos: usize, val: u64): usize ! HstsError {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, '0') + @as(u8, @intCast(v % 10));
            v = v / 10;
            n += 1;
        }
    }
    if (n > dst.len - pos) return HstsError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[pos + i] = tmp[n - 1 - i];
    }
    return pos + n;
}

/// Formats `policy` into canonical `Strict-Transport-Security` header syntax inside `out_buf`.
pub fn formatHstsHeader(policy: HstsPolicy, out_buf: *[]u8): usize ! HstsError {
    let pos: usize = writeLiteral(out_buf, 0, "max-age=")?;
    pos = writeU64Decimal(out_buf, pos, policy.max_age_sec)?;
    if (policy.include_subdomains) {
        pos = writeLiteral(out_buf, pos, "; includeSubDomains")?;
    }
    if (policy.preload) {
        pos = writeLiteral(out_buf, pos, "; preload")?;
    }
    return pos;
}

// ============================================================================
// 4. Core Engine: `HstsStore`
// ============================================================================

/// Zero-heap-allocation RFC 6797 HSTS domain registry and HTTPS upgrade decision engine.
pub struct HstsStore {
    entries: [HSTS_MAX_DOMAINS]HstsEntry = [HstsEntry{}; HSTS_MAX_DOMAINS];
    upgrades_enforced: u64 = 0;
    expired_evictions: u64 = 0;

    /// Initializes an empty `HstsStore`.
    static fn init(): HstsStore {
        return HstsStore{};
    }

    fn findExactNormalized(norm_host: []const u8): usize? {
        let i: usize = 0;
        while (i < HSTS_MAX_DOMAINS) : (i += 1) {
            if (entries[i].active && simdWebAuthAsciiLowerEql(entries[i].host(), norm_host)) {
                return i;
            }
        }
        return null;
    }

    /// Registers an immutable preloaded HSTS domain (never expires and cannot be revoked by `max-age=0`).
    fn addPreload(*_, host: []const u8, include_subdomains: bool): void ! HstsError {
        let norm_buf: [HSTS_HOST_MAX_LEN]u8 = [0; HSTS_HOST_MAX_LEN];
        const norm_len: usize = normalizeHstsHost(host, *norm_buf)?;
        const norm_slice: []const u8 = norm_buf[0..norm_len];

        if (idx := findExactNormalized(norm_slice)) {
            entries[idx].is_preload = true;
            entries[idx].include_subdomains = include_subdomains;
            entries[idx].expires_at_sec = std.math.maxInt(u64);
            return;
        }

        let free_idx: usize = HSTS_MAX_DOMAINS;
        let i: usize = 0;
        while (i < HSTS_MAX_DOMAINS) : (i += 1) {
            if (!entries[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == HSTS_MAX_DOMAINS) return HstsError.StoreFull;

        let e = HstsEntry{
            active: true,
            host_len: norm_len,
            expires_at_sec: std.math.maxInt(u64),
            include_subdomains: include_subdomains,
            is_preload: true,
        };
        let k: usize = 0;
        while (k < norm_len) : (k += 1) {
            e.host_buf[k] = norm_buf[k];
        }
        entries[free_idx] = e;
    }

    /// Applies a received `Strict-Transport-Security` response header for `host` at `now_sec`.
    /// - `max-age=0` immediately removes a dynamic (non-preload) entry and returns `true` if removed.
    /// - `max-age > 0` inserts or refreshes the domain's TTL and `includeSubDomains` flag.
    fn applyHeader(
        *_,
        host: []const u8,
        header: []const u8,
        now_sec: u64,
    ): bool ! HstsError {
        let norm_buf: [HSTS_HOST_MAX_LEN]u8 = [0; HSTS_HOST_MAX_LEN];
        const norm_len: usize = normalizeHstsHost(host, *norm_buf)?;
        const norm_slice: []const u8 = norm_buf[0..norm_len];

        const pol: HstsPolicy = parseHstsHeader(header)?;

        if (pol.max_age_sec == 0) {
            if (idx := findExactNormalized(norm_slice)) {
                if (entries[idx].is_preload) {
                    // Preloaded entries cannot be cleared via dynamic max-age=0
                    return false;
                }
                entries[idx] = HstsEntry{};
                return true;
            }
            return false;
        }

        const max_u64: u64 = std.math.maxInt(u64);
        const exp_sec: u64 = if (pol.max_age_sec > max_u64 - now_sec) max_u64 else (now_sec + pol.max_age_sec);

        if (idx := findExactNormalized(norm_slice)) {
            if (!entries[idx].is_preload) {
                entries[idx].expires_at_sec = exp_sec;
                entries[idx].include_subdomains = pol.include_subdomains;
            }
            return true;
        }

        // Reclaim expired dynamic slots first if full
        let free_idx: usize = HSTS_MAX_DOMAINS;
        let i: usize = 0;
        while (i < HSTS_MAX_DOMAINS) : (i += 1) {
            if (!entries[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == HSTS_MAX_DOMAINS) {
            * = evictExpired(now_sec);
            let j: usize = 0;
            while (j < HSTS_MAX_DOMAINS) : (j += 1) {
                if (!entries[j].active) {
                    free_idx = j;
                    break;
                }
            }
        }
        if (free_idx == HSTS_MAX_DOMAINS) return HstsError.StoreFull;

        let e = HstsEntry{
            active: true,
            host_len: norm_len,
            expires_at_sec: exp_sec,
            include_subdomains: pol.include_subdomains,
            is_preload: false,
        };
        let k: usize = 0;
        while (k < norm_len) : (k += 1) {
            e.host_buf[k] = norm_buf[k];
        }
        entries[free_idx] = e;
        return true;
    }

    /// Evaluates whether an HTTP request to `host` at `now_sec` must be upgraded to HTTPS.
    /// Checks exact host match first, then walks parent superdomains for `include_subdomains == true`.
    /// Expired dynamic entries are ignored.
    fn shouldUpgrade(*_, host: []const u8, now_sec: u64): HstsLookupResult {
        let norm_buf: [HSTS_HOST_MAX_LEN]u8 = [0; HSTS_HOST_MAX_LEN];
        const norm_len: usize = normalizeHstsHost(host, *norm_buf) catch {
            return HstsLookupResult{};
        };
        const norm_slice: []const u8 = norm_buf[0..norm_len];

        // 1. Exact host match
        if (idx := findExactNormalized(norm_slice)) {
            if (entries[idx].is_preload || now_sec < entries[idx].expires_at_sec) {
                upgrades_enforced += 1;
                return HstsLookupResult{
                    must_upgrade: true,
                    matched_exact: true,
                    matched_via_subdomain: false,
                    is_preload: entries[idx].is_preload,
                };
            }
        }

        // 2. Superdomain walk (a.b.example.com -> b.example.com -> example.com)
        let dot_pos: usize = 0;
        while (dot_pos < norm_len) : (dot_pos += 1) {
            if (norm_buf[dot_pos] == '.') {
                const parent: []const u8 = norm_buf[dot_pos + 1 .. norm_len];
                if (parent.len == 0) break;
                if (pidx := findExactNormalized(parent)) {
                    if (entries[pidx].include_subdomains && (entries[pidx].is_preload || now_sec < entries[pidx].expires_at_sec)) {
                        upgrades_enforced += 1;
                        return HstsLookupResult{
                            must_upgrade: true,
                            matched_exact: false,
                            matched_via_subdomain: true,
                            is_preload: entries[pidx].is_preload,
                        };
                    }
                }
            }
        }

        return HstsLookupResult{};
    }

    /// Evicts all expired dynamic (non-preload) entries at `now_sec`. Returns number of entries evicted.
    fn evictExpired(*_, now_sec: u64): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < HSTS_MAX_DOMAINS) : (i += 1) {
            if (entries[i].active && !entries[i].is_preload && now_sec >= entries[i].expires_at_sec) {
                entries[i] = HstsEntry{};
                removed += 1;
            }
        }
        expired_evictions += @as(u64, removed);
        return removed;
    }

    /// Returns aggregate domain counts and upgrade/eviction telemetry.
    fn stats(): HstsStoreStats {
        let s = HstsStoreStats{
            upgrades_enforced: upgrades_enforced,
            expired_evictions: expired_evictions,
        };
        let i: usize = 0;
        while (i < HSTS_MAX_DOMAINS) : (i += 1) {
            if (entries[i].active) {
                s.entry_count += 1;
                if (entries[i].is_preload) {
                    s.preload_count += 1;
                }
            }
        }
        return s;
    }
}

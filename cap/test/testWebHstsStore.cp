// Origin: Pure-CP Stdlib Test Suite (lib/web/hstsStore.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 6797 HTTP Strict-Transport-Security (HSTS):
//   1. `parseHstsHeader` (case-insensitive & quoted `max-age`), `formatHstsHeader` round-trip & `isPreloadEligible`
//   2. Strict rejections: duplicate directives, missing/invalid `max-age`, empty header & CRLF/NUL
//   3. RFC 6797 §8.1.1 host eligibility: rejects IPv4/IPv6 literals, accepts & normalizes DNS/FQDN
//   4. `includeSubDomains` multi-level parent domain walk & exact-match precedence
//   5. `max-age=0` immediate dynamic revocation & immutable preload entry protection
//   6. TTL expiration, `evictExpired` reclamation & `HstsStoreStats` telemetry
//   7. `BufferTooSmall` guard & `cap.web` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    HSTS_MAX_DOMAINS,
    HSTS_HOST_MAX_LEN,
    HSTS_PRELOAD_MIN_MAX_AGE,
    HstsError,
    HstsPolicy,
    parseHstsHeader,
    formatHstsHeader,
    isHstsEligibleHost,
    HstsStore,
} from "cap:web";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("HSTS: parseHstsHeader (quoted & case-insensitive), format round-trip & isPreloadEligible") {
    const p1 = parseHstsHeader("MAX-AGE=\"31536000\"; IncludeSubDomains; PRELOAD") catch {
        expect_true(false);
        return;
    };
    expect_eq(p1.max_age_sec, HSTS_PRELOAD_MIN_MAX_AGE);
    expect_true(p1.include_subdomains);
    expect_true(p1.preload);
    expect_true(p1.isPreloadEligible());

    // Format back to canonical string
    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];
    const n = formatHstsHeader(p1, *buf_s) catch 0;
    expect_true(strEq(buf[0..n], "max-age=31536000; includeSubDomains; preload"));

    // Not preload-eligible if max-age < 31536000 or missing includeSubDomains
    const p_short = parseHstsHeader("max-age=86400; includeSubDomains; preload") catch {
        expect_true(false);
        return;
    };
    expect_true(!p_short.isPreloadEligible());
}

test("HSTS: rejects duplicate directives, missing/invalid max-age, empty & CRLF") {
    // 1. Empty header
    let got_empty: bool = false;
    * = parseHstsHeader("   ") catch |e| { got_empty = (e == HstsError.EmptyHeader); };
    expect_true(got_empty);

    // 2. Missing max-age
    let got_missing: bool = false;
    * = parseHstsHeader("includeSubDomains; preload") catch |e| { got_missing = (e == HstsError.MissingMaxAge); };
    expect_true(got_missing);

    // 3. Duplicate directive
    let got_dup: bool = false;
    * = parseHstsHeader("max-age=100; max-age=200") catch |e| { got_dup = (e == HstsError.DuplicateDirective); };
    expect_true(got_dup);

    let got_dup_sub: bool = false;
    * = parseHstsHeader("max-age=100; includeSubDomains; includesubdomains") catch |e| { got_dup_sub = (e == HstsError.DuplicateDirective); };
    expect_true(got_dup_sub);

    // 4. Non-numeric max-age
    let got_bad_age: bool = false;
    * = parseHstsHeader("max-age=-10") catch |e| { got_bad_age = (e == HstsError.InvalidMaxAge); };
    expect_true(got_bad_age);

    // 5. CRLF boundary guard
    let got_crlf: bool = false;
    * = parseHstsHeader("max-age=3600\r\nX-Extra: 1") catch |e| { got_crlf = (e == HstsError.CrlfRejected); };
    expect_true(got_crlf);
}

test("HSTS: RFC 6797 §8.1.1 rejects IPv4/IPv6 literals & normalizes DNS/FQDN") {
    expect_true(isHstsEligibleHost("example.com"));
    expect_true(isHstsEligibleHost("API.Example.COM."));
    expect_true(!isHstsEligibleHost("127.0.0.1"));
    expect_true(!isHstsEligibleHost("10.0.0.5"));
    expect_true(!isHstsEligibleHost("[::1]"));
    expect_true(!isHstsEligibleHost("2001:db8::1"));
    expect_true(!isHstsEligibleHost(""));
    expect_true(!isHstsEligibleHost("-bad.example.com"));

    let store = HstsStore.init();
    let got_ip_v4: bool = false;
    * = store.applyHeader("192.168.1.1", "max-age=3600", 1000) catch |e| {
        got_ip_v4 = (e == HstsError.IpLiteralDisallowed);
    };
    expect_true(got_ip_v4);

    let got_ip_v6: bool = false;
    * = store.applyHeader("[::1]", "max-age=3600", 1000) catch |e| {
        got_ip_v6 = (e == HstsError.IpLiteralDisallowed);
    };
    expect_true(got_ip_v6);
}

test("HSTS: includeSubDomains multi-level superdomain upgrade matching") {
    let store = HstsStore.init();
    const applied = store.applyHeader("Example.COM.", "max-age=3600; includeSubDomains", 1000) catch false;
    expect_true(applied);
    * = store.applyHeader("exactonly.org", "max-age=3600", 1000) catch false;

    // Exact match on example.com (case-insensitive + trailing dot)
    const r_exact = store.shouldUpgrade("example.com", 2000);
    expect_true(r_exact.must_upgrade);
    expect_true(r_exact.matched_exact);
    expect_true(!r_exact.matched_via_subdomain);

    // Multi-level subdomain match: a.b.example.com -> walks up to example.com
    const r_sub = store.shouldUpgrade("a.b.example.com", 2000);
    expect_true(r_sub.must_upgrade);
    expect_true(!r_sub.matched_exact);
    expect_true(r_sub.matched_via_subdomain);

    // exactonly.org has includeSubDomains=false -> sub.exactonly.org must NOT upgrade
    const r_no_sub = store.shouldUpgrade("sub.exactonly.org", 2000);
    expect_true(!r_no_sub.must_upgrade);
    expect_true(store.shouldUpgrade("exactonly.org", 2000).must_upgrade);
}

test("HSTS: max-age=0 dynamic revocation vs immutable preload domain") {
    let store = HstsStore.init();
    * = store.applyHeader("dyn.example.com", "max-age=3600", 1000) catch false;
    expect_true(store.shouldUpgrade("dyn.example.com", 1500).must_upgrade);

    // Dynamic max-age=0 immediately removes dyn.example.com
    const revoked = store.applyHeader("dyn.example.com", "max-age=0", 1600) catch false;
    expect_true(revoked);
    expect_true(!store.shouldUpgrade("dyn.example.com", 1600).must_upgrade);

    // Preloaded domain cannot be revoked by dynamic max-age=0 and never expires
    store.addPreload("bank.example", true) catch { expect_true(false); };
    const preload_revoke = store.applyHeader("bank.example", "max-age=0", 2000) catch true;
    expect_true(!preload_revoke);

    const r_pre = store.shouldUpgrade("pay.bank.example", 9_999_999_999);
    expect_true(r_pre.must_upgrade);
    expect_true(r_pre.matched_via_subdomain);
    expect_true(r_pre.is_preload);
}

test("HSTS: TTL expiration, evictExpired reclamation & stats telemetry") {
    let store = HstsStore.init();
    store.addPreload("pinned.org", true) catch { expect_true(false); };
    * = store.applyHeader("short.example", "max-age=100", 1000) catch false; // expires at 1100
    * = store.applyHeader("long.example", "max-age=5000", 1000) catch false; // expires at 6000

    expect_true(store.shouldUpgrade("short.example", 1099).must_upgrade);
    // At t=1100 short.example is expired and ignored
    expect_true(!store.shouldUpgrade("short.example", 1100).must_upgrade);
    expect_true(store.shouldUpgrade("long.example", 1100).must_upgrade);

    // Evict expired at t=1100 -> removes short.example only (keeps pinned.org and long.example)
    expect_eq(store.evictExpired(1100), @as(usize, 1));
    const st = store.stats();
    expect_eq(st.entry_count, @as(usize, 2));
    expect_eq(st.preload_count, @as(usize, 1));
    expect_eq(st.expired_evictions, @as(u64, 1));
    expect_eq(st.upgrades_enforced, @as(u64, 2));
}

test("HSTS: BufferTooSmall & cap.web facade parity") {
    const pol = HstsPolicy{ max_age_sec: 31536000, include_subdomains: true, preload: true };
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    let got_small: bool = false;
    * = formatHstsHeader(pol, *tiny_s) catch |e| {
        got_small = (e == HstsError.BufferTooSmall);
    };
    expect_true(got_small);

    // `cap.web` facade parity
    expect_eq(cap.web.HSTS_MAX_DOMAINS, @as(usize, 64));
    expect_eq(cap.web.HSTS_HOST_MAX_LEN, @as(usize, 128));
    expect_eq(cap.web.HSTS_PRELOAD_MIN_MAX_AGE, @as(u64, 31_536_000));
    expect_eq(HSTS_MAX_DOMAINS, @as(usize, 64));
    expect_eq(HSTS_HOST_MAX_LEN, @as(usize, 128));
}

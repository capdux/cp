// Source: cp/cap/test/testHttpCookieRustRefSuite.cp
// Reference-hardened against rwf2/cookie-rs (MIT/Apache-2.0) & RFC 6265 / RFC 6265bis
// Covers:
// 1. Multi-value Cookie request header parsing (CookieMap & CookieCodec.parseCookieHeader)
// 2. Full Set-Cookie attribute decode/encode roundtrip (Expires, Max-Age, Domain, Path, Secure, HttpOnly, SameSite, Partitioned)
// 3. RFC 6265bis __Secure- & __Host- prefix matrix, SameSite=None / Partitioned Secure guards, CRLF/control injection rejection
// 4. 16-lane SIMD vs scalar 0-discrepancy differential parity (control/name/value/delim/semicolon)
// 5. 1,500-case deterministic PRNG malformed Cookie & Set-Cookie fuzz suite (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CookieSameSite,
    CookieEntry,
    CookieMap,
    SameSite,
    HttpCookie,
    CookieValidationError,
    CookieCodec,
    HttpCookieJar,
    isValidCookiePrefix,
    validateSetCookieStrict,
    simdHasInvalidControl,
    simdIsValidCookieName,
    simdIsValidCookieValue,
    simdFindCookieDelim,
    simdIndexOfSemicolon,
} from "../../lib/http/cookie.cp";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarHasInvalidControl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if ((c < 0x20 && c != 0x09) || c == 0x7F) return true;
    }
    return false;
}

fn scalarIsCookieToken(c: u8): bool {
    if (c <= 0x20 || c >= 0x7F) return false;
    switch (c) {
        '(', ')', '<', '>', '@', ',', ';', ':', '\\', '"', '/', '[', ']', '?', '=', '{', '}' => return false,
        else => return true,
    }
}

fn scalarIsValidCookieName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        if (!scalarIsCookieToken(name[i])) return false;
    }
    return true;
}

fn scalarIsValidCookieValue(raw_value: []const u8): bool {
    let inner = raw_value;
    if (inner.len > 0 && (inner[0] == '"' || inner[inner.len - 1] == '"')) {
        if (inner.len < 2 || inner[0] != '"' || inner[inner.len - 1] != '"') {
            return false;
        }
        inner = inner[1 .. inner.len - 1];
    }
    let i: usize = 0;
    while (i < inner.len) : (i += 1) {
        const c = inner[i];
        if (c <= 0x20 || c >= 0x7F || c == '"' || c == ',' || c == ';' || c == '\\') {
            return false;
        }
    }
    return true;
}

fn scalarFindCookieDelim(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ';' || s[i] == '=') return i;
    }
    return null;
}

fn scalarIndexOfSemicolon(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ';') return i;
    }
    return null;
}

struct Rng {
    state: u64;

    fn nextU64(*_): u64 {
        let x = _.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn nextU8(*_): u8 {
        return ((_.nextU64() >> 32) & 0xFF) as u8;
    }

    fn nextRange(*_, bound: usize): usize {
        return ((_.nextU64() as usize) % bound);
    }
}

// --- Test 1: Multi-value Cookie Header & Set-Cookie Full Attribute Roundtrip ---

fn testCookieParsingAndRoundtrip(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Multi-value Cookie request header (CookieMap & CookieCodec.parseCookieHeader)
    const hdr = "session_id=\"tok_9f8a\"; theme=dark; empty=; orphan_no_eq; lang=zh-CN; ; a=1=2";
    const cmap = CookieMap.parse(hdr);
    expect_eq(cmap.count, 5 as usize);
    expect_eq(std.mem.eql(u8, cmap.get("session_id").?, "tok_9f8a"), true);
    expect_eq(std.mem.eql(u8, cmap.get("theme").?, "dark"), true);
    expect_eq(std.mem.eql(u8, cmap.get("empty").?, ""), true);
    expect_eq(std.mem.eql(u8, cmap.get("lang").?, "zh-CN"), true);
    expect_eq(std.mem.eql(u8, cmap.get("a").?, "1=2"), true);
    expect_eq(cmap.has("orphan_no_eq"), false);

    const pairs = try CookieCodec.parseCookieHeader(alloc, hdr);
    defer if (pairs.len > 0) alloc.free(pairs);
    expect_eq(pairs.len, 5 as usize);
    expect_eq(std.mem.eql(u8, pairs[0].name, "session_id"), true);
    expect_eq(std.mem.eql(u8, pairs[0].value, "tok_9f8a"), true);
    expect_eq(std.mem.eql(u8, pairs[4].value, "1=2"), true);

    // 2. Full Set-Cookie parse & format roundtrip (including Partitioned, Expires, Max-Age, SameSite)
    const raw_sc =
        "sid=abc123; Domain=.Example.COM; Path=/api; Expires=Wed, 21 Oct 2015 07:28:00 GMT; " ++
        "Max-Age=3600; Secure; HttpOnly; SameSite=Strict; Partitioned";
    const parsed = try CookieCodec.parseSetCookie(raw_sc);
    expect_eq(std.mem.eql(u8, parsed.name, "sid"), true);
    expect_eq(std.mem.eql(u8, parsed.value, "abc123"), true);
    expect_eq(std.mem.eql(u8, parsed.domain, "Example.COM"), true);
    expect_eq(parsed.host_only, false);
    expect_eq(std.mem.eql(u8, parsed.path, "/api"), true);
    expect_eq(parsed.expires_epoch_s.?, 1445412480 as i64);
    expect_eq(parsed.max_age.?, 3600 as i64);
    expect_eq(parsed.secure, true);
    expect_eq(parsed.http_only, true);
    expect_eq(parsed.same_site, SameSite.Strict);
    expect_eq(parsed.partitioned, true);
    expect_eq(try validateSetCookieStrict(parsed), true);

    const formatted = try CookieCodec.formatSetCookie(alloc, parsed);
    defer if (formatted.len > 0) alloc.free(formatted);
    const reparsed = try CookieCodec.parseSetCookie(formatted);
    expect_eq(std.mem.eql(u8, reparsed.name, parsed.name), true);
    expect_eq(std.mem.eql(u8, reparsed.value, parsed.value), true);
    expect_eq(reparsed.max_age.?, 3600 as i64);
    expect_eq(reparsed.expires_epoch_s.?, 1445412480 as i64);
    expect_eq(reparsed.same_site, SameSite.Strict);
    expect_eq(reparsed.partitioned, true);

    // 3. Max-Age edge cases (negative / zero / overflow clamping matching cookie-rs)
    const neg_c = try CookieCodec.parseSetCookie("k=v; Max-Age=-10");
    expect_eq(neg_c.max_age.? <= 0, true);
    const ovf_pos = try CookieCodec.parseSetCookie("k=v; Max-Age=99999999999999999999999999");
    expect_eq(ovf_pos.max_age.?, std.math.maxInt(i64));
    const ovf_neg = try CookieCodec.parseSetCookie("k=v; Max-Age=-99999999999999999999999999");
    expect_eq(ovf_neg.max_age.?, 0 as i64);
    const bad_ma = try CookieCodec.parseSetCookie("k=v; Max-Age=12abc");
    expect_eq(bad_ma.max_age == null, true);
}

// --- Test 2: RFC 6265bis Security Prefixes, SameSite=None & Injection Guards ---

fn testSecurityPrefixesAndInjectionGuards(): !void {
    const alloc = std.heap.page_allocator;

    // 1. __Secure- prefix matrix
    const ok_sec = try CookieCodec.parseSetCookie("__Secure-ID=123; Secure; Domain=example.com");
    expect_eq(isValidCookiePrefix(ok_sec), true);
    expect_eq(try validateSetCookieStrict(ok_sec), true);

    const bad_sec = try CookieCodec.parseSetCookie("__Secure-ID=123; Domain=example.com");
    expect_eq(isValidCookiePrefix(bad_sec), false);
    let sec_err = false;
    * = validateSetCookieStrict(bad_sec) catch |e| {
        sec_err = (e == CookieValidationError.InvalidSecurePrefix);
    };
    expect_eq(sec_err, true);

    // Case-insensitive prefix guard per RFC 6265bis
    const bad_sec_ci = try CookieCodec.parseSetCookie("__secure-ID=123");
    expect_eq(isValidCookiePrefix(bad_sec_ci), false);

    // 2. __Host- prefix matrix (requires Secure + no Domain + Path=/)
    const ok_host = try CookieCodec.parseSetCookie("__Host-SID=xyz; Secure; Path=/");
    expect_eq(isValidCookiePrefix(ok_host), true);
    expect_eq(try validateSetCookieStrict(ok_host), true);

    const host_no_sec = try CookieCodec.parseSetCookie("__Host-SID=xyz; Path=/");
    expect_eq(isValidCookiePrefix(host_no_sec), false);

    const host_with_dom = try CookieCodec.parseSetCookie("__Host-SID=xyz; Secure; Domain=example.com; Path=/");
    expect_eq(isValidCookiePrefix(host_with_dom), false);

    const host_bad_path = try CookieCodec.parseSetCookie("__Host-SID=xyz; Secure; Path=/app");
    expect_eq(isValidCookiePrefix(host_bad_path), false);
    let host_err = false;
    * = validateSetCookieStrict(host_bad_path) catch |e| {
        host_err = (e == CookieValidationError.InvalidHostPrefix);
    };
    expect_eq(host_err, true);

    // HttpCookieJar.setFromHeader must enforce prefixes while allowing valid __Host-
    let jar = HttpCookieJar.init(alloc);
    defer jar.deinit();
    try jar.setFromHeader("app.example.com", "__Host-SID=xyz; Secure; Path=/", 1000);
    expect_eq(jar.count(), 1 as usize);

    let jar_rej = false;
    jar.setFromHeader("app.example.com", "__Host-BAD=1; Secure; Domain=example.com; Path=/", 1000) catch {
        jar_rej = true;
    };
    expect_eq(jar_rej, true);

    // 3. SameSite=None & Partitioned require Secure
    const ss_none_bad = try CookieCodec.parseSetCookie("a=1; SameSite=None");
    let ss_err = false;
    * = validateSetCookieStrict(ss_none_bad) catch |e| {
        ss_err = (e == CookieValidationError.SameSiteNoneRequiresSecure);
    };
    expect_eq(ss_err, true);

    const part_bad = try CookieCodec.parseSetCookie("a=1; Partitioned");
    let part_err = false;
    * = validateSetCookieStrict(part_bad) catch |e| {
        part_err = (e == CookieValidationError.PartitionedRequiresSecure);
    };
    expect_eq(part_err, true);

    // 4. CRLF / control char / malformed token / unbalanced quote rejection
    const bad_headers: []const []const u8 = [
        "a=1\r\nSet-Cookie: evil=1",
        "a=1\nSet-Cookie: evil=1",
        "a=1\x00evil",
        "a\x07b=1",
        "bad name=1",
        "bad/name=1",
        "=noname",
        "noequals",
        "a=\"unterminated",
        "a=unopened\"",
        "a=val,comma",
        "a=val\\slash",
    ];
    for (bh of bad_headers) {
        let rejected = false;
        * = CookieCodec.parseSetCookie(bh) catch {
            rejected = true;
        };
        expect_eq(rejected, true);
    }
}

// --- Test 3: 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x3D91_A4F2_8C07_5E1B };
    let buf: [96]u8 = undefined;

    for (len of 0..97) {
        const len_u = len as usize;

        // 1. simdHasInvalidControl parity
        for (i of 0..len) buf[i] = 'a';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s1 = buf[0..len_u];
        expect_eq(simdHasInvalidControl(s1), scalarHasInvalidControl(s1));

        // 2. simdIsValidCookieName parity
        for (i of 0..len) buf[i] = 'k';
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s2 = buf[0..len_u];
        expect_eq(simdIsValidCookieName(s2), scalarIsValidCookieName(s2));

        // 3. simdIsValidCookieValue parity (both unquoted and quoted)
        for (i of 0..len) buf[i] = 'v';
        if (len_u >= 2 && (len_u % 3) == 0) {
            buf[0] = '"';
            buf[len_u - 1] = '"';
        }
        if (len_u > 0) buf[rng.nextRange(len_u)] = rng.nextU8();
        const s3 = buf[0..len_u];
        expect_eq(simdIsValidCookieValue(s3), scalarIsValidCookieValue(s3));

        // 4. simdFindCookieDelim & simdIndexOfSemicolon parity
        for (i of 0..len) buf[i] = 'x';
        if (len_u > 0) {
            const p = rng.nextRange(len_u);
            buf[p] = if ((len_u % 2) == 0) ';' else '=';
        }
        const s4 = buf[0..len_u];
        expect_eq(simdFindCookieDelim(s4), scalarFindCookieDelim(s4));
        expect_eq(simdIndexOfSemicolon(s4), scalarIndexOfSemicolon(s4));
    }
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed Cookie & Set-Cookie Fuzz Suite ---

fn fuzzCheckSetCookie(alloc: std.mem.Allocator, slice: []const u8) {
    const c = CookieCodec.parseSetCookie(slice) catch return;
    const pref_ok = isValidCookiePrefix(c);
    if (!pref_ok) {}
    * = validateSetCookieStrict(c) catch false;
    const fmt = CookieCodec.formatSetCookie(alloc, c) catch return;
    if (fmt.len > 0) alloc.free(fmt);
}

fn fuzzCheckCookieHeader(alloc: std.mem.Allocator, slice: []const u8) {
    const cmap = CookieMap.parse(slice);
    if (cmap.count > 64) return;
    const pairs = CookieCodec.parseCookieHeader(alloc, slice) catch return;
    if (pairs.len > 0) alloc.free(pairs);
}

fn testDeterministicCookieFuzz(): !void {
    const alloc = std.heap.page_allocator;
    let rng = Rng { state: 0x5E37_79B9_7F4A_7C15 };
    let buf: [192]u8 = undefined;
    const tmpl_sc = "__Host-sid=abc_123; Path=/; Max-Age=60; Secure; HttpOnly; SameSite=Lax; Partitioned";
    const tmpl_ch = "a=1; b=\"two\"; __Secure-c=3; d=4";

    let total: usize = 0;
    for (case_idx of 0..1500) {
        const strat = (case_idx as usize) % 3;
        let len: usize = 0;
        if (strat == 0) {
            len = rng.nextRange(160);
            for (i of 0..len) buf[i] = rng.nextU8();
        } else if (strat == 1) {
            len = (1 as usize) + rng.nextRange(tmpl_sc.len);
            @memcpy(buf[0..len], tmpl_sc[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            let m: usize = 0;
            while (m < muts) : (m += 1) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else {
            len = (1 as usize) + rng.nextRange(tmpl_ch.len);
            @memcpy(buf[0..len], tmpl_ch[0..len]);
            const muts = (1 as usize) + rng.nextRange(4);
            let m: usize = 0;
            while (m < muts) : (m += 1) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        }

        const slice = buf[0..len];
        fuzzCheckSetCookie(alloc, slice);
        fuzzCheckCookieHeader(alloc, slice);
        total += 1;
    }
    expect_eq(total, 1500 as usize);
}

fn main() {
    test("1. Multi-Value Cookie Header & Full Set-Cookie Attribute Roundtrip", () => {
        try testCookieParsingAndRoundtrip();
    });
    test("2. RFC 6265bis Security Prefixes, SameSite=None & Injection Guards", () => {
        try testSecurityPrefixesAndInjectionGuards();
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        try testSimdVsScalarParity();
    });
    test("4. 1,500-Case Deterministic PRNG Malformed Cookie & Set-Cookie Fuzz Suite", () => {
        try testDeterministicCookieFuzz();
    });
}

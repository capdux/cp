// Origin: Pure-CP Stdlib Test Suite (lib/obs/baggage.cp) | Created: 2026-11-23
// Production Deep Verification Suite for W3C Baggage Distributed Context & Redaction Engine:
//   1. Parse & format round-trip with OWS trimming, `;prop1=v1;prop2` metadata preservation & `%XX` percent-encoding
//   2. Duplicate key Last-Write-Wins deduplication & `BaggageContext` CRUD (`set`, `get`, `getProps`, `remove`, `clear`)
//   3. Cross-agent `mergeFrom` with prefix allowlist & read-only (`overwrite_existing = false`) protection
//   4. In-place PII/secret redaction (`redactKeys`, case-insensitive) preserving metadata properties
//   5. Security guards: CRLF injection rejection, >8192B header cap, malformed `%XX` escapes & invalid token keys
//   6. High-cardinality flood overflow (`dropped_overflow` / `capacity_full`) & `cap.obs` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    BAGGAGE_MAX_ENTRIES,
    BAGGAGE_MAX_HEADER_BYTES,
    BaggageOpStatus,
    BaggageContext,
    isValidBaggageKey,
    parseBaggageHeader,
    formatBaggageHeader,
} from "cap:obs";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("Baggage: parse/format round-trip with OWS, properties & percent-encoding") {
    let ctx = BaggageContext.init();
    const hdr = "  user_id = alice%20smith ; role=admin;readonly , region=ap-southeast-2 , msg=hello%2Cworld%3B! ";
    const rep = parseBaggageHeader(hdr, *ctx);

    expect_eq(rep.status, BaggageOpStatus.ok);
    expect_eq(rep.parsed_members, @as(usize, 3));
    expect_eq(rep.stored_entries, @as(usize, 3));
    expect_eq(rep.skipped_invalid, @as(usize, 0));

    if (v1 := ctx.get("user_id")) {
        expect_true(bytesEq(v1, "alice smith"));
    } else {
        expect_true(false);
    }
    if (p1 := ctx.getProps("user_id")) {
        expect_true(bytesEq(p1, "role=admin;readonly"));
    } else {
        expect_true(false);
    }
    if (v3 := ctx.get("msg")) {
        expect_true(bytesEq(v3, "hello,world;!"));
    } else {
        expect_true(false);
    }

    // Format back to W3C header and verify exact length matches estimatedHeaderBytes()
    let out: [512]u8 = [0; 512];
    let out_s: []u8 = out[0..];
    const n = formatBaggageHeader(ctx, *out_s) ?? 0;
    expect_eq(n, ctx.estimatedHeaderBytes());
    expect_true(containsSubstr(out[0..n], "user_id=alice%20smith;role=admin;readonly"));
    expect_true(containsSubstr(out[0..n], "msg=hello%2Cworld%3B!"));

    // Undersized format buffer returns null cleanly
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatBaggageHeader(ctx, *tiny_s) == null);
}

test("Baggage: duplicate key Last-Write-Wins & CRUD operations") {
    let ctx = BaggageContext.init();
    const rep = parseBaggageHeader("env=staging,tenant=t1,env=prod;v=2", *ctx);
    expect_eq(rep.status, BaggageOpStatus.ok);
    expect_eq(rep.parsed_members, @as(usize, 3));
    expect_eq(rep.stored_entries, @as(usize, 2));
    expect_eq(rep.overwritten_duplicates, @as(usize, 1));

    if (v := ctx.get("env")) {
        expect_true(bytesEq(v, "prod"));
    } else {
        expect_true(false);
    }
    if (p := ctx.getProps("env")) {
        expect_true(bytesEq(p, "v=2"));
    } else {
        expect_true(false);
    }

    expect_true(ctx.has("tenant"));
    expect_true(ctx.remove("tenant"));
    expect_true(!ctx.has("tenant"));
    expect_true(ctx.get("tenant") == null);
    expect_eq(ctx.len(), @as(usize, 1));

    ctx.clear();
    expect_eq(ctx.len(), @as(usize, 0));
    expect_eq(ctx.estimatedHeaderBytes(), @as(usize, 0));
}

test("Baggage: cross-agent mergeFrom with prefix allowlist & overwrite policy") {
    let local = BaggageContext.init();
    expect_eq(local.set("app.tenant", "local-auth"), BaggageOpStatus.ok);
    expect_eq(local.set("trace.hop", "1"), BaggageOpStatus.ok);

    let incoming = BaggageContext.init();
    expect_eq(incoming.set("app.tenant", "spoofed"), BaggageOpStatus.ok);
    expect_eq(incoming.setWithProps("app.user", "u42", "verified"), BaggageOpStatus.ok);
    expect_eq(incoming.set("internal.secret", "do-not-import"), BaggageOpStatus.ok);

    // 1. Read-only merge with prefix "app." -> imports app.user, protects local app.tenant, ignores internal.secret
    const m1 = local.mergeFrom(incoming, false, "app.");
    expect_eq(m1, @as(usize, 1));
    expect_true(!local.has("internal.secret"));
    if (vt := local.get("app.tenant")) {
        expect_true(bytesEq(vt, "local-auth")); // not overwritten
    } else {
        expect_true(false);
    }
    if (vu := local.get("app.user")) {
        expect_true(bytesEq(vu, "u42"));
    } else {
        expect_true(false);
    }
    if (pu := local.getProps("app.user")) {
        expect_true(bytesEq(pu, "verified"));
    } else {
        expect_true(false);
    }

    // 2. Overwrite merge with prefix "app." -> updates app.tenant too
    const m2 = local.mergeFrom(incoming, true, "app.");
    expect_eq(m2, @as(usize, 2));
    if (vt2 := local.get("app.tenant")) {
        expect_true(bytesEq(vt2, "spoofed"));
    } else {
        expect_true(false);
    }
}

test("Baggage: in-place redactKeys (case-insensitive) preserves properties") {
    let ctx = BaggageContext.init();
    expect_eq(ctx.setWithProps("Auth_Token", "Bearer-xyz-999", "scope=read"), BaggageOpStatus.ok);
    expect_eq(ctx.set("user.email", "ceo@example.com"), BaggageOpStatus.ok);
    expect_eq(ctx.set("region", "us-west-2"), BaggageOpStatus.ok);

    const sens: [2][]const u8 = ["auth_token", "USER.EMAIL"];
    const n_red = ctx.redactKeys(sens[0..], ""); // default "[REDACTED]"
    expect_eq(n_red, @as(usize, 2));

    if (vt := ctx.get("Auth_Token")) {
        expect_true(bytesEq(vt, "[REDACTED]"));
    } else {
        expect_true(false);
    }
    if (pt := ctx.getProps("Auth_Token")) {
        expect_true(bytesEq(pt, "scope=read")); // metadata preserved
    } else {
        expect_true(false);
    }
    if (vr := ctx.get("region")) {
        expect_true(bytesEq(vr, "us-west-2")); // untouched
    } else {
        expect_true(false);
    }
}

test("Baggage: CRLF injection, oversized header, invalid keys & malformed percent escapes") {
    let ctx = BaggageContext.init();

    // 1. CRLF injection rejected atomically
    const r_crlf = parseBaggageHeader("a=1\r\nX-Evil: 2", *ctx);
    expect_eq(r_crlf.status, BaggageOpStatus.crlf_rejected);
    expect_eq(ctx.len(), @as(usize, 0));

    // Direct set CRLF rejected
    expect_eq(ctx.set("k", "val\ninj"), BaggageOpStatus.crlf_rejected);
    expect_eq(ctx.set("bad key", "v"), BaggageOpStatus.invalid_key);
    expect_true(!isValidBaggageKey("bad=key"));
    expect_true(!isValidBaggageKey(""));
    expect_true(isValidBaggageKey("app.tenant-id_1"));

    // 2. Oversized header (> 8192 bytes) rejected atomically
    let huge: [BAGGAGE_MAX_HEADER_BYTES + 16]u8 = ['a'; BAGGAGE_MAX_HEADER_BYTES + 16];
    const r_huge = parseBaggageHeader(huge[0..], *ctx);
    expect_eq(r_huge.status, BaggageOpStatus.header_too_large);
    expect_eq(ctx.len(), @as(usize, 0));

    // 3. Mixed valid and malformed members (bad %ZZ, missing '=', invalid key) -> skips invalid, keeps valid
    const r_mix = parseBaggageHeader("ok=1,bad%ZZ=2,noequals,bad_val=%2G,good=yes", *ctx);
    expect_eq(r_mix.status, BaggageOpStatus.ok);
    expect_eq(r_mix.stored_entries, @as(usize, 2));
    expect_true(r_mix.skipped_invalid >= 2);
    expect_true(ctx.has("ok"));
    expect_true(ctx.has("good"));
}

test("Baggage: high-cardinality flood overflow guard & cap.obs facade parity") {
    let ctx = cap.obs.BaggageContext.init();

    // Build header with 36 distinct keys (> BAGGAGE_MAX_ENTRIES = 32)
    let hdr: [512]u8 = [0; 512];
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        if (i > 0) {
            hdr[pos] = ',';
            pos += 1;
        }
        hdr[pos] = 'k';
        hdr[pos + 1] = @as(u8, @intCast('0' + (i / 10)));
        hdr[pos + 2] = @as(u8, @intCast('0' + (i % 10)));
        hdr[pos + 3] = '=';
        hdr[pos + 4] = '1';
        pos += 5;
    }

    const rep = cap.obs.parseBaggageHeader(hdr[0..pos], *ctx);
    expect_eq(rep.status, cap.obs.BaggageOpStatus.capacity_full);
    expect_eq(rep.parsed_members, @as(usize, 36));
    expect_eq(rep.stored_entries, BAGGAGE_MAX_ENTRIES);
    expect_eq(rep.dropped_overflow, @as(usize, 4));
    expect_eq(ctx.len(), BAGGAGE_MAX_ENTRIES);
}

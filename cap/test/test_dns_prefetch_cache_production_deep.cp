import { test, expect, expect_eq } from "cap:test";
import {
    DNS_PREFETCH_DEFAULT_TTL_MS,
    DNS_PREFETCH_MAX_TTL_MS,
    DNS_PREFETCH_MAX_ENTRIES,
    DnsPrefetchError,
    parseDnsTtlEnvSeconds,
    DnsCacheStats,
    DnsLookupState,
    DnsLookupOutcomeKind,
    DnsLookupOutcome,
    DnsPrefetchEntry,
    DnsPrefetchCache,
} from "cap:dns";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("dns prefetch_cache: parseDnsTtlEnvSeconds parsing, clamping, and invalid guards", fn()! {
    expect_eq(DNS_PREFETCH_DEFAULT_TTL_MS, 30000);
    expect_eq(DNS_PREFETCH_MAX_TTL_MS, 86400000);
    expect_eq(DNS_PREFETCH_MAX_ENTRIES, 255);

    expect_eq(parseDnsTtlEnvSeconds("0")?, 0);
    expect_eq(parseDnsTtlEnvSeconds("5")?, 5000);
    expect_eq(parseDnsTtlEnvSeconds("30")?, 30000);
    expect_eq(parseDnsTtlEnvSeconds("999999")?, DNS_PREFETCH_MAX_TTL_MS);

    let errs: usize = 0;
    if (_ := parseDnsTtlEnvSeconds("")) {} else { errs += 1; }
    if (_ := parseDnsTtlEnvSeconds("-5")) {} else { errs += 1; }
    if (_ := parseDnsTtlEnvSeconds("30s")) {} else { errs += 1; }
    expect_eq(errs, 3);
});

test("dns prefetch_cache: in-flight lookup deduplication and getCacheStats accounting", fn()! {
    let cache = DnsPrefetchCache.init(30000);

    // 1st lookup: cache miss -> starts inflight
    const l1 = cache.lookup("bun.com", 443, 1000)?;
    expect(l1.kind == .MissStarted);
    expect_eq(l1.waiter_count, 1);

    // 2nd concurrent lookup (case-insensitive) -> coalesced inflight hit!
    const l2 = cache.lookup("BUN.COM", 443, 1005)?;
    expect(l2.kind == .HitInflight);
    expect_eq(l2.waiter_count, 2);

    // Complete DNS resolution
    expect(cache.complete("bun.com", 443, "172.67.161.226", 1020));

    // 3rd lookup -> completed cache hit
    const l3 = cache.lookup("Bun.Com", 443, 5000)?;
    expect(l3.kind == .HitCompleted);
    expect(strEql(l3.resolved_ip, "172.67.161.226"));

    const st = cache.stats();
    expect_eq(st.total_count, 3);
    expect_eq(st.cache_misses, 1);
    expect_eq(st.cache_hits_inflight, 1);
    expect_eq(st.cache_hits_completed, 1);
    expect_eq(st.size, 1);
    expect_eq(st.errors, 0);
});

test("dns prefetch_cache: prefetch idempotency, warm hit, and expiry re-prefetch", fn()! {
    let cache = DnsPrefetchCache.init(5000);

    expect(cache.prefetch("db.internal", 5432, 1000)?);
    expect(!cache.prefetch("DB.INTERNAL", 5432, 1005)?); // already inflight

    expect(cache.complete("db.internal", 5432, "10.0.1.9", 1010));
    expect(!cache.prefetch("db.internal", 5432, 2000)?); // still fresh

    // Lookup hits immediately without miss
    const hit = cache.lookup("db.internal", 5432, 3000)?;
    expect(hit.kind == .HitCompleted);
    expect(strEql(hit.resolved_ip, "10.0.1.9"));

    // After TTL (1010 + 5000 = 6010), prefetch restarts cleanly
    expect(cache.prefetch("db.internal", 5432, 6010)?);
});

test("dns prefetch_cache: TTL expiry, pruneExpired, and ttl=0 disabled caching", fn()! {
    let cache = DnsPrefetchCache.init(1000);
    * = cache.lookup("a.test", 80, 100)?;
    * = cache.complete("a.test", 80, "1.1.1.1", 100);
    * = cache.lookup("b.test", 80, 500)?;
    * = cache.complete("b.test", 80, "2.2.2.2", 500);
    expect_eq(cache.stats().size, 2);

    // At t=1200: a.test expired (1100), b.test still fresh (1500)
    const pruned = cache.pruneExpired(1200);
    expect_eq(pruned, 1);
    expect_eq(cache.stats().size, 1);

    // ttl_ms = 0 disables retention upon complete()
    let nocache = DnsPrefetchCache.init(0);
    const m = nocache.lookup("ephemeral.test", 443, 100)?;
    expect(m.kind == .MissStarted);
    expect(nocache.complete("ephemeral.test", 443, "9.9.9.9", 110));
    expect_eq(nocache.stats().size, 0);
});

test("dns prefetch_cache: onConnectionError and failLookup evict stale entries", fn()! {
    let cache = DnsPrefetchCache.init(30000);
    * = cache.lookup("edge.test", 443, 100)?;
    * = cache.complete("edge.test", 443, "203.0.113.5", 110);
    expect_eq(cache.stats().size, 1);

    // Connection failure evicts cached entry and increments errors
    expect(cache.onConnectionError("EDGE.TEST", 443));
    expect_eq(cache.stats().size, 0);
    expect_eq(cache.stats().errors, 1);

    // Next lookup must be a fresh miss
    const again = cache.lookup("edge.test", 443, 200)?;
    expect(again.kind == .MissStarted);

    // Failing an inflight lookup also clears slot & increments errors
    expect(cache.failLookup("edge.test", 443));
    expect_eq(cache.stats().size, 0);
    expect_eq(cache.stats().errors, 2);
});

test("dns prefetch_cache: EmptyHost, HostTooLong, and port isolation guards", fn()! {
    let cache = DnsPrefetchCache.init(30000);
    let errs: usize = 0;

    if (_ := cache.lookup("", 443, 0)) {} else { errs += 1; }
    if (_ := cache.prefetch("", 443, 0)) {} else { errs += 1; }

    let long_host: [254]u8 = ['a'; 254];
    if (_ := cache.lookup(long_host[0..], 443, 0)) {} else { errs += 1; }
    if (_ := cache.prefetch(long_host[0..], 443, 0)) {} else { errs += 1; }
    expect_eq(errs, 4);

    // Different ports on same host maintain separate slots
    * = cache.lookup("dual.test", 80, 10)?;
    * = cache.complete("dual.test", 80, "10.0.0.1", 10);
    const p443 = cache.lookup("dual.test", 443, 20)?;
    expect(p443.kind == .MissStarted);
    expect_eq(cache.stats().size, 2);
});

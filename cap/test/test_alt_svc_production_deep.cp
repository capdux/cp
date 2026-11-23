import { test, expect } from "cap:test";
import { DEFAULT_MAX_AGE, AltSvcHeader, AltSvcCache } from "cap:http";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("alt_svc: parse multiple alternatives, params, and clear", () => {
    const h = AltSvcHeader.parse("h3-29=\":443\"; ma=3600, h3=\"alt.example.com:8443\"; ma=600, h3=\":443\"; ma=2592000; persist=1");
    expect(!h.is_clear && h.len == 3);
    expect(strEq(h.entries[0].protocol, "h3-29") && h.entries[0].port == 443 && h.entries[0].max_age == 3600);
    expect(strEq(h.entries[1].host, "alt.example.com") && h.entries[1].port == 8443);

    const same_h3 = h.firstSameHostH3().?;
    expect(same_h3.port == 443 && same_h3.max_age == 2592000 && same_h3.persist);

    const def = AltSvcHeader.parse("h3=\":8443\"");
    expect(def.firstSameHostH3().?.max_age == DEFAULT_MAX_AGE);

    const clr = AltSvcHeader.parse("  CLEAR ");
    expect(clr.is_clear && clr.len == 0);
});

test("alt_svc: AltSvcCache record, TTL expiry, and clear eviction", () => {
    let cache = AltSvcCache.init();
    cache.record("example.com", 443, "h3=\":443\"; ma=120", 1000);
    expect(cache.lookup("example.com", 443, 1050).? == 443);
    expect(cache.lookup("example.com", 443, 1120) == null); // expired

    cache.record("api.cp.dev", 443, "h3=\":8443\"; ma=600", 2000);
    expect(cache.lookup("api.cp.dev", 443, 2100).? == 8443);
    cache.record("api.cp.dev", 443, "clear", 2150);
    expect(cache.lookup("api.cp.dev", 443, 2160) == null);
});

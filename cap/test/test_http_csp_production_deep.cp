import {
    CspDirectiveIter,
    findDirective,
    effectiveFetchDirective,
    appendCspDirective,
} from "cap:http_csp";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("CSP directive parsing, first-wins, default-src fallback, and nonce check", () => {
    const policy = "default-src 'self'; script-src 'self' 'nonce-AbC123' https://cdn.example.com; object-src 'none'; script-src 'unsafe-inline'; upgrade-insecure-requests";

    const def = findDirective(policy, "default-src").?;
    expect(def.allowsSelf());
    expect(!def.allowsNone());

    // First script-src wins; duplicate 'unsafe-inline' ignored per CSP3 §2.2.1
    const sc = findDirective(policy, "SCRIPT-SRC").?;
    expect(sc.allowsSelf());
    expect(!sc.allowsUnsafeInline());
    expect(sc.allowsNonce("AbC123"));
    expect(!sc.allowsNonce("abc123")); // nonces are case-sensitive
    expect(sc.hasToken("https://cdn.example.com"));

    const obj = findDirective(policy, "object-src").?;
    expect(obj.allowsNone());

    // img-src falls back to default-src
    const img = effectiveFetchDirective(policy, "img-src").?;
    expect(strEq(img.name, "default-src") && img.allowsSelf());

    // Valueless directive
    const upg = findDirective(policy, "upgrade-insecure-requests").?;
    expect(upg.value.len == 0);
});

test("appendCspDirective builds canonical policy header", () => {
    let out: [256]u8 = [0; 256];
    let pos: usize = 0;
    expect(appendCspDirective(out[0..], &pos, "Default-Src", "'self'"));
    expect(appendCspDirective(out[0..], &pos, "frame-ancestors", "'none'"));
    expect(appendCspDirective(out[0..], &pos, "upgrade-insecure-requests", ""));

    expect(strEq(out[0..pos], "default-src 'self'; frame-ancestors 'none'; upgrade-insecure-requests"));
});

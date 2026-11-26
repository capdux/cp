// cap/test/testCapSpiffeId.cp
// Comprehensive unit tests for @cap/spiffe-id

import { test, expect_eq } from "cap:test";
import {
    isValidTrustDomain,
    isValidPath,
    parseSpiffeId,
    belongsToTrustDomain,
    formatSpiffeId,
} from "../../packages/@cap/spiffe-id/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn main() {
    test("1. @cap/spiffe-id — Standard SPIFFE ID with workload path", () => {
        const id = parseSpiffeId("spiffe://prod.example.org/ns/default/sa/api-server");
        expect_eq(id.valid, true);
        expect_eq(sliceEq(id.trust_domain, "prod.example.org"), true);
        expect_eq(sliceEq(id.path, "/ns/default/sa/api-server"), true);
    });

    test("2. @cap/spiffe-id — Trust-domain-only SPIFFE ID without path", () => {
        const id = parseSpiffeId("spiffe://example.org");
        expect_eq(id.valid, true);
        expect_eq(sliceEq(id.trust_domain, "example.org"), true);
        expect_eq(id.path.len, 0 as usize);
        expect_eq(isValidPath(""), true);
    });

    test("3. @cap/spiffe-id — belongsToTrustDomain membership checks", () => {
        const id = parseSpiffeId("spiffe://mesh.prod.internal/workload/payments");
        expect_eq(belongsToTrustDomain(id, "mesh.prod.internal"), true);
        expect_eq(belongsToTrustDomain(id, "prod.internal"), false);
        expect_eq(belongsToTrustDomain(id, "MESH.PROD.INTERNAL"), false);

        const inv = parseSpiffeId("https://mesh.prod.internal/a");
        expect_eq(belongsToTrustDomain(inv, "mesh.prod.internal"), false);
    });

    test("4. @cap/spiffe-id — formatSpiffeId round-trip", () => {
        let buf: [128]u8 = undefined;
        const n = formatSpiffeId("prod.example.org", "/ns/core/sa/gateway", buf[0..128]);
        expect_eq(sliceEq(buf[0..n], "spiffe://prod.example.org/ns/core/sa/gateway"), true);

        const reparsed = parseSpiffeId(buf[0..n]);
        expect_eq(reparsed.valid, true);
        expect_eq(sliceEq(reparsed.trust_domain, "prod.example.org"), true);
        expect_eq(sliceEq(reparsed.path, "/ns/core/sa/gateway"), true);

        const nRoot = formatSpiffeId("example.org", "", buf[0..128]);
        expect_eq(sliceEq(buf[0..nRoot], "spiffe://example.org"), true);
    });

    test("5. @cap/spiffe-id — Reject dot-segment traversal, trailing slash & double slash", () => {
        expect_eq(parseSpiffeId("spiffe://example.org/a/../b").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/./a").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/..").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/a/").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/a//b").valid, false);
        // Dot inside a normal segment name (e.g. "v1.2" or "...a") is allowed per SPIFFE spec.
        expect_eq(parseSpiffeId("spiffe://example.org/v1.2/app").valid, true);
    });

    test("6. @cap/spiffe-id — Reject uppercase trust domain, port, userinfo, query, fragment & tiny buffer", () => {
        expect_eq(isValidTrustDomain("Example.org"), false);
        expect_eq(parseSpiffeId("spiffe://Example.org/app").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org:8443/app").valid, false);
        expect_eq(parseSpiffeId("spiffe://user@example.org/app").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/app?k=v").valid, false);
        expect_eq(parseSpiffeId("spiffe://example.org/app#frag").valid, false);
        expect_eq(parseSpiffeId("SPIFFE://example.org/app").valid, false);

        let tiny: [12]u8 = undefined;
        expect_eq(formatSpiffeId("example.org", "/app", tiny[0..12]), 0 as usize);
    });
}

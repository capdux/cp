// cap/test/testCapOciRef.cp
// Comprehensive unit tests for @cap/oci-distribution (OCI / Docker image reference parser)

import { test, expect_eq } from "cap:test";
import {
    ImageRef,
    isValidTag,
    isValidDigest,
    parseImageRef,
    formatNormalized,
} from "../../packages/oci-distribution/src/index.cp";

fn ociEq(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/oci-distribution — parseImageRef official library & custom registry", () => {
        const r1: ImageRef = parseImageRef("nginx:1.25-alpine");
        expect_eq(r1.valid, true);
        expect_eq(r1.registry, "");
        expect_eq(r1.repository, "nginx");
        expect_eq(r1.tag, "1.25-alpine");

        let out: [128]u8 = undefined;
        const n = formatNormalized(r1, out[0..128]);
        expect_eq(ociEq(out[0..n], "docker.io/library/nginx:1.25-alpine"), true);

        const r2: ImageRef = parseImageRef("ghcr.io/org/app:v2");
        expect_eq(r2.valid, true);
        expect_eq(r2.registry, "ghcr.io");
        expect_eq(r2.repository, "org/app");
        expect_eq(r2.tag, "v2");
    });

    test("2. @cap/oci-distribution — digest & tag validation", () => {
        expect_eq(isValidTag("v1.0.0"), true);
        expect_eq(isValidTag("-bad"), false);
        expect_eq(isValidDigest("sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"), true);
        expect_eq(isValidDigest("sha256:short"), false);
    });
}

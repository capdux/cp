// cap/test/testCapDockerRef.cp
// Comprehensive unit tests for @cap/docker-ref

import { test, expect_eq } from "cap:test";
import {
    ImageRef,
    isValidTag,
    isValidDigest,
    parseImageRef,
    formatNormalized,
} from "../../packages/@cap/docker-ref/src/index.cp";

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
    test("1. @cap/docker-ref — Official short names normalize to docker.io/library/...", () => {
        const r1 = parseImageRef("nginx");
        expect_eq(r1.valid, true);
        expect_eq(r1.registry.len, 0 as usize);
        expect_eq(sliceEq(r1.repository, "nginx"), true);
        expect_eq(r1.tag.len, 0 as usize);

        let b1: [128]u8 = undefined;
        const n1 = formatNormalized(r1, b1[0..128]);
        expect_eq(sliceEq(b1[0..n1], "docker.io/library/nginx:latest"), true);

        const r2 = parseImageRef("alpine:3.19");
        expect_eq(r2.valid, true);
        let b2: [128]u8 = undefined;
        const n2 = formatNormalized(r2, b2[0..128]);
        expect_eq(sliceEq(b2[0..n2], "docker.io/library/alpine:3.19"), true);
    });

    test("2. @cap/docker-ref — Private registry with port (localhost:5000/team/app:v2)", () => {
        const r = parseImageRef("localhost:5000/team/app:v2");
        expect_eq(r.valid, true);
        expect_eq(sliceEq(r.registry, "localhost:5000"), true);
        expect_eq(sliceEq(r.repository, "team/app"), true);
        expect_eq(sliceEq(r.tag, "v2"), true);
        expect_eq(r.digest.len, 0 as usize);

        let buf: [128]u8 = undefined;
        const n = formatNormalized(r, buf[0..128]);
        expect_eq(sliceEq(buf[0..n], "localhost:5000/team/app:v2"), true);

        // Registry with port and no tag must not treat ':5000' as tag.
        const rNoTag = parseImageRef("localhost:5000/team/app");
        expect_eq(rNoTag.valid, true);
        expect_eq(sliceEq(rNoTag.registry, "localhost:5000"), true);
        expect_eq(sliceEq(rNoTag.repository, "team/app"), true);
        expect_eq(rNoTag.tag.len, 0 as usize);
    });

    test("3. @cap/docker-ref — Digest-only & combined tag+digest references", () => {
        const dg = "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        const r1 = parseImageRef("ghcr.io/cap-lang/cpc@sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        expect_eq(r1.valid, true);
        expect_eq(sliceEq(r1.registry, "ghcr.io"), true);
        expect_eq(sliceEq(r1.repository, "cap-lang/cpc"), true);
        expect_eq(r1.tag.len, 0 as usize);
        expect_eq(sliceEq(r1.digest, dg), true);

        // Digest-only should NOT append :latest.
        let b1: [256]u8 = undefined;
        const n1 = formatNormalized(r1, b1[0..256]);
        expect_eq(sliceEq(b1[0..n1], "ghcr.io/cap-lang/cpc@sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"), true);

        const r2 = parseImageRef("ubuntu:24.04@sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        expect_eq(r2.valid, true);
        expect_eq(sliceEq(r2.tag, "24.04"), true);
        expect_eq(sliceEq(r2.digest, dg), true);
    });

    test("4. @cap/docker-ref — isValidTag & isValidDigest boundary checks", () => {
        expect_eq(isValidTag("v1.0.0-alpha_1"), true);
        expect_eq(isValidTag("_internal"), true);
        expect_eq(isValidTag(""), false);
        expect_eq(isValidTag("-leading-dash"), false);
        expect_eq(isValidTag(".leading-dot"), false);
        expect_eq(isValidTag("bad/tag"), false);

        expect_eq(isValidDigest("sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"), true);
        // Uppercase hex rejected per OCI digest spec.
        expect_eq(isValidDigest("sha256:0123456789ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef"), false);
        expect_eq(isValidDigest("sha256:short"), false);
        expect_eq(isValidDigest("md5:0123456789abcdef0123456789abcdef"), false);
    });

    test("5. @cap/docker-ref — Reject uppercase repo, bad digest & empty input", () => {
        expect_eq(parseImageRef("").valid, false);
        expect_eq(parseImageRef("MyRepo/app:v1").valid, false);
        expect_eq(parseImageRef("nginx@sha256:bad").valid, false);
        expect_eq(parseImageRef("nginx:").valid, false);
        expect_eq(parseImageRef("nginx/-bad").valid, false);
    });

    test("6. @cap/docker-ref — Small output buffer returns 0 safely", () => {
        const r = parseImageRef("nginx");
        let tiny: [10]u8 = undefined;
        expect_eq(formatNormalized(r, tiny[0..10]), 0 as usize);

        const inv = parseImageRef("");
        let okBuf: [64]u8 = undefined;
        expect_eq(formatNormalized(inv, okBuf[0..64]), 0 as usize);
    });
}

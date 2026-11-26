// cap/test/testCapOciRef.cp
// Comprehensive unit tests for @cap/oci-ref

import { test, expect_eq } from "cap:test";
import {
    OciRef,
    parseOciRef,
    formatCanonical,
    isPinned,
} from "../../packages/@cap/oci-ref/src/index.cp";

fn main() {
    test("1. @cap/oci-ref — bare Docker Hub official image", () => {
        const r: OciRef = parseOciRef("nginx");
        expect_eq(r.valid, true);
        expect_eq(r.registry, "docker.io");
        expect_eq(r.repository, "nginx");
        expect_eq(r.tag, "latest");
        expect_eq(r.is_official_library, true);
        expect_eq(isPinned(r), false);

        let buf: [256]u8 = undefined;
        expect_eq(formatCanonical(r, buf[0..256]), "docker.io/library/nginx:latest");
    });

    test("2. @cap/oci-ref — custom registry with multi-segment repo & tag", () => {
        const r: OciRef = parseOciRef("ghcr.io/capdux/caprun/agent:v2.1.0-alpine");
        expect_eq(r.valid, true);
        expect_eq(r.registry, "ghcr.io");
        expect_eq(r.repository, "capdux/caprun/agent");
        expect_eq(r.tag, "v2.1.0-alpine");
        expect_eq(r.is_official_library, false);
    });

    test("3. @cap/oci-ref — localhost registry with port and no tag", () => {
        const r: OciRef = parseOciRef("localhost:5000/myorg/worker");
        expect_eq(r.valid, true);
        expect_eq(r.registry, "localhost:5000");
        expect_eq(r.repository, "myorg/worker");
        expect_eq(r.tag, "latest");

        let buf: [256]u8 = undefined;
        expect_eq(formatCanonical(r, buf[0..256]), "localhost:5000/myorg/worker:latest");
    });

    test("4. @cap/oci-ref — pinned sha256 digest with and without tag", () => {
        const sha: string = "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        const r1: OciRef = parseOciRef("registry.capdux.org/core/runtime:1.0@sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        expect_eq(r1.valid, true);
        expect_eq(r1.registry, "registry.capdux.org");
        expect_eq(r1.repository, "core/runtime");
        expect_eq(r1.tag, "1.0");
        expect_eq(r1.digest, sha);
        expect_eq(isPinned(r1), true);

        const r2: OciRef = parseOciRef("alpine@sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        expect_eq(r2.valid, true);
        expect_eq(r2.tag, "");
        expect_eq(isPinned(r2), true);
    });

    test("5. @cap/oci-ref — Docker Hub user/repo shorthand", () => {
        const r: OciRef = parseOciRef("bitnami/redis:7.4");
        expect_eq(r.valid, true);
        expect_eq(r.registry, "docker.io");
        expect_eq(r.repository, "bitnami/redis");
        expect_eq(r.tag, "7.4");
        expect_eq(r.is_official_library, false);

        let buf: [256]u8 = undefined;
        expect_eq(formatCanonical(r, buf[0..256]), "docker.io/bitnami/redis:7.4");
    });

    test("6. @cap/oci-ref — invalid inputs rejected", () => {
        expect_eq(parseOciRef("").valid, false);
        expect_eq(parseOciRef("UPPERCASE/repo").valid, false);
        expect_eq(parseOciRef("nginx:-badtag").valid, false);
        expect_eq(parseOciRef("nginx@sha256:tooshort").valid, false);
        expect_eq(parseOciRef("a//b:latest").valid, false);
    });
}

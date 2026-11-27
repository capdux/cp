// cap/test/testCapCacheControl.cp
// Comprehensive unit tests for @cap/cache-control

import { test, expect_eq } from "cap:test";
import {
    CacheControl,
} from "../../packages/cache-control-parser/src/index.cp";

fn main() {
    test("1. @cap/cache-control — Classic CDN header parsing & shared vs private TTL", () => {
        const cc = CacheControl.parse("public, max-age=60, s-maxage=3600, stale-while-revalidate=30, immutable");
        expect_eq(cc.is_public, true);
        expect_eq(cc.immutable, true);
        expect_eq(cc.max_age, 60 as i64);
        expect_eq(cc.s_maxage, 3600 as i64);
        expect_eq(cc.stale_while_revalidate, 30 as i64);
        expect_eq(cc.stale_if_error, -1 as i64);
        expect_eq(cc.isCacheableShared(), true);
        expect_eq(cc.effectiveTtl(true), 3600 as i64);
        expect_eq(cc.effectiveTtl(false), 60 as i64);
    });

    test("2. @cap/cache-control — no-store, no-cache & private TTL semantics", () => {
        const noStore = CacheControl.parse("no-store, max-age=300");
        expect_eq(noStore.isCacheableShared(), false);
        expect_eq(noStore.effectiveTtl(true), 0 as i64);
        expect_eq(noStore.effectiveTtl(false), 0 as i64);

        const noCache = CacheControl.parse("public, no-cache, max-age=600");
        expect_eq(noCache.effectiveTtl(true), 0 as i64);
        expect_eq(noCache.effectiveTtl(false), 0 as i64);

        const privCc = CacheControl.parse("private, max-age=120");
        expect_eq(privCc.isCacheableShared(), false);
        expect_eq(privCc.effectiveTtl(true), 0 as i64);
        expect_eq(privCc.effectiveTtl(false), 120 as i64);
    });

    test("3. @cap/cache-control — Mixed case & quoted directive values", () => {
        const cc = CacheControl.parse("  PUBLIC, MAX-AGE=\"120\", Stale-If-Error=\"600\", Must-Revalidate, No-Transform  ");
        expect_eq(cc.is_public, true);
        expect_eq(cc.max_age, 120 as i64);
        expect_eq(cc.stale_if_error, 600 as i64);
        expect_eq(cc.must_revalidate, true);
        expect_eq(cc.no_transform, true);
        expect_eq(cc.effectiveTtl(true), 120 as i64);
    });

    test("4. @cap/cache-control — Unknown extension directives safely ignored", () => {
        const cc = CacheControl.parse("x-custom=foo, max-age=45, community=\"UCI, test\", proxy-revalidate");
        expect_eq(cc.max_age, 45 as i64);
        expect_eq(cc.isCacheableShared(), true);
        expect_eq(cc.effectiveTtl(false), 45 as i64);
    });

    test("5. @cap/cache-control — format -> parse full round-trip fidelity", () => {
        const orig = CacheControl.parse("public, must-revalidate, immutable, max-age=60, s-maxage=3600, stale-while-revalidate=30, stale-if-error=600");
        let buf: [256]u8 = undefined;
        const n = orig.format(buf[0..256]);
        expect_eq(n > (0 as i64), true);

        const reparsed = CacheControl.parse(buf[0..(n as usize)]);
        expect_eq(reparsed.is_public, orig.is_public);
        expect_eq(reparsed.must_revalidate, orig.must_revalidate);
        expect_eq(reparsed.immutable, orig.immutable);
        expect_eq(reparsed.max_age, orig.max_age);
        expect_eq(reparsed.s_maxage, orig.s_maxage);
        expect_eq(reparsed.stale_while_revalidate, orig.stale_while_revalidate);
        expect_eq(reparsed.stale_if_error, orig.stale_if_error);
    });

    test("6. @cap/cache-control — Empty header & insufficient buffer returns -1", () => {
        const empty = CacheControl.parse("");
        expect_eq(empty.isCacheableShared(), false);
        expect_eq(empty.effectiveTtl(true), -1 as i64);
        expect_eq(empty.effectiveTtl(false), -1 as i64);

        let buf: [64]u8 = undefined;
        expect_eq(empty.format(buf[0..64]), 0 as i64);

        const cc = CacheControl.parse("public, max-age=3600");
        let tiny: [5]u8 = undefined;
        expect_eq(cc.format(tiny[0..5]), -1 as i64);
    });
}

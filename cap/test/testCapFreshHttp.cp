import { test, expect_eq } from "cap:test";
import {
    isFresh,
    etagWeakMatch,
    etagStrongMatch,
    parseHttpDate,
    hasNoCacheDirective,
} from "../../packages/@cap/fresh-http/src/index.cp";

fn main() {
    test("1. @cap/fresh-http — etagWeakMatch & etagStrongMatch (RFC 7232 §2.3.2)", () => {
        expect_eq(etagWeakMatch("\"abc\"", "\"abc\""), true);
        expect_eq(etagWeakMatch("W/\"abc\"", "\"abc\""), true);
        expect_eq(etagWeakMatch("W/\"abc\"", "W/\"abc\""), true);
        expect_eq(etagWeakMatch("\"abc\"", "\"xyz\""), false);
        expect_eq(etagWeakMatch("unquoted", "unquoted"), false);

        expect_eq(etagStrongMatch("\"abc\"", "\"abc\""), true);
        expect_eq(etagStrongMatch("W/\"abc\"", "\"abc\""), false);
        expect_eq(etagStrongMatch("\"abc\"", "W/\"abc\""), false);
        expect_eq(etagStrongMatch("W/\"abc\"", "W/\"abc\""), false);
    });

    test("2. @cap/fresh-http — parseHttpDate IMF-fixdate parsing & calendar validation", () => {
        const ts1 = parseHttpDate("Wed, 21 Oct 2015 07:28:00 GMT");
        expect_eq(ts1 != null, true);
        expect_eq(ts1 ?? 0, 1445412480);

        const epoch = parseHttpDate("Thu, 01 Jan 1970 00:00:00 GMT");
        expect_eq(epoch != null, true);
        expect_eq(epoch ?? -1, 0);

        // Leap day valid in 2024, invalid in 2023
        expect_eq(parseHttpDate("Thu, 29 Feb 2024 12:00:00 GMT") != null, true);
        expect_eq(parseHttpDate("Wed, 29 Feb 2023 12:00:00 GMT") == null, true);

        // Malformed dates
        expect_eq(parseHttpDate("not-a-date") == null, true);
        expect_eq(parseHttpDate("Wed, 21 Oct 2015 25:00:00 GMT") == null, true);
        expect_eq(parseHttpDate("Wed, 21 Oct 2015 07:28:00 UTC") == null, true);
    });

    test("3. @cap/fresh-http — hasNoCacheDirective via @cap/unicase (case-insensitive & boundary-safe)", () => {
        expect_eq(hasNoCacheDirective("no-cache"), true);
        expect_eq(hasNoCacheDirective("public, NO-CACHE, max-age=0"), true);
        expect_eq(hasNoCacheDirective("No-Cache=\"Set-Cookie\""), true);
        expect_eq(hasNoCacheDirective("max-age=3600, must-revalidate"), false);
        expect_eq(hasNoCacheDirective("x-no-cache"), false);
        expect_eq(hasNoCacheDirective(""), false);
    });

    test("4. @cap/fresh-http — isFresh with If-None-Match (*, multi-ETag, weak match, quoted comma)", () => {
        // Wildcard matches any response
        expect_eq(isFresh("*", "", "", "\"any\"", ""), true);
        // Multi-ETag list with weak match
        expect_eq(isFresh("\"v1\", W/\"v2\", \"v3\"", "", "", "\"v2\"", ""), true);
        expect_eq(isFresh("\"v1\", \"v2\"", "", "", "\"v9\"", ""), false);
        // Comma inside quoted ETag is not split
        expect_eq(isFresh("\"a,b\", \"c\"", "", "", "\"a,b\"", ""), true);
        // Missing response ETag
        expect_eq(isFresh("\"v1\"", "", "", "", ""), false);
        // Unconditional request is never fresh
        expect_eq(isFresh("", "", "", "\"v1\"", "Wed, 21 Oct 2015 07:28:00 GMT"), false);
    });

    test("5. @cap/fresh-http — isFresh with If-Modified-Since comparison", () => {
        const t_old = "Wed, 21 Oct 2015 07:00:00 GMT";
        const t_mid = "Wed, 21 Oct 2015 07:28:00 GMT";
        const t_new = "Wed, 21 Oct 2015 08:00:00 GMT";

        // last_modified <= if_modified_since -> fresh
        expect_eq(isFresh("", t_mid, "", "", t_old), true);
        expect_eq(isFresh("", t_mid, "", "", t_mid), true);
        // last_modified > if_modified_since -> stale
        expect_eq(isFresh("", t_mid, "", "", t_new), false);
        // Invalid date -> stale
        expect_eq(isFresh("", "bad-date", "", "", t_old), false);
    });

    test("6. @cap/fresh-http — Combined If-None-Match + If-Modified-Since & Cache-Control: no-cache override", () => {
        const t_old = "Wed, 21 Oct 2015 07:00:00 GMT";
        const t_mid = "Wed, 21 Oct 2015 07:28:00 GMT";
        const t_new = "Wed, 21 Oct 2015 08:00:00 GMT";

        // Both satisfied -> fresh
        expect_eq(isFresh("W/\"v1\"", t_mid, "max-age=60", "\"v1\"", t_old), true);
        // ETag matches, but modified more recently -> stale
        expect_eq(isFresh("\"v1\"", t_mid, "", "\"v1\"", t_new), false);
        // Not modified, but ETag mismatches -> stale
        expect_eq(isFresh("\"v1\"", t_mid, "", "\"v2\"", t_old), false);
        // Cache-Control: no-cache overrides freshness even when both match
        expect_eq(isFresh("\"v1\"", t_mid, "no-cache", "\"v1\"", t_old), false);
    });
}

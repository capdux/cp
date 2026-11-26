import { test, expect_eq } from "cap:test";
import {
    RequestHeaders,
    ResponseHeaders,
    hasNoCache,
    etagWeakMatch,
    etagStrongMatch,
    parseHttpDate,
    isFresh,
} from "../../packages/@cap/fresh/src/index.cp";

fn main() {
    test("1. @cap/fresh — Unconditional requests & Cache-Control: no-cache invalidation", () => {
        const res = ResponseHeaders {
            etag: "\"abc123\"",
            last_modified: "Wed, 21 Oct 2015 07:28:00 GMT",
        };
        const no_cond = RequestHeaders {
            if_none_match: "",
            if_modified_since: "",
            cache_control: "",
        };
        expect_eq(isFresh(no_cond, res), false);

        expect_eq(hasNoCache("no-cache"), true);
        expect_eq(hasNoCache("public, NO-CACHE, max-age=0"), true);
        expect_eq(hasNoCache("x-no-cache, max-age=60"), false);

        const nocache_req = RequestHeaders {
            if_none_match: "\"abc123\"",
            if_modified_since: "",
            cache_control: "max-age=0, no-cache",
        };
        expect_eq(isFresh(nocache_req, res), false);
    });

    test("2. @cap/fresh — If-None-Match strong/weak ETag list & '*' wildcard", () => {
        const res = ResponseHeaders {
            etag: "W/\"v2-tag\"",
            last_modified: "",
        };

        const list_hit = RequestHeaders {
            if_none_match: "\"v1-tag\", \"v2-tag\", \"v3-tag\"",
            if_modified_since: "",
            cache_control: "",
        };
        expect_eq(isFresh(list_hit, res), true);

        const list_miss = RequestHeaders {
            if_none_match: "\"v1-tag\", \"v3-tag\"",
            if_modified_since: "",
            cache_control: "",
        };
        expect_eq(isFresh(list_miss, res), false);

        const star_req = RequestHeaders {
            if_none_match: "*",
            if_modified_since: "",
            cache_control: "",
        };
        expect_eq(isFresh(star_req, res), true);
    });

    test("3. @cap/fresh — parseHttpDate exact Unix conversion & leap-year validation", () => {
        // RFC 9110 canonical example: Wed, 21 Oct 2015 07:28:00 GMT == 1445412480
        expect_eq(parseHttpDate("Wed, 21 Oct 2015 07:28:00 GMT"), 1445412480 as i64);
        // Unix epoch start
        expect_eq(parseHttpDate("Thu, 01 Jan 1970 00:00:00 GMT"), 0 as i64);
        // Leap day 2024-02-29 valid; 2023-02-29 invalid
        expect_eq(parseHttpDate("Thu, 29 Feb 2024 00:00:00 GMT") > 0, true);
        expect_eq(parseHttpDate("Wed, 29 Feb 2023 00:00:00 GMT"), -1 as i64);
        // Malformed date
        expect_eq(parseHttpDate("not-a-date"), -1 as i64);
    });

    test("4. @cap/fresh — If-Modified-Since vs Last-Modified (equal / older / newer)", () => {
        const res = ResponseHeaders {
            etag: "",
            last_modified: "Wed, 21 Oct 2015 07:28:00 GMT",
        };

        // Exact equal -> fresh
        const eq_req = RequestHeaders {
            if_none_match: "",
            if_modified_since: "Wed, 21 Oct 2015 07:28:00 GMT",
            cache_control: "",
        };
        expect_eq(isFresh(eq_req, res), true);

        // Client date newer than Last-Modified -> fresh
        const newer_req = RequestHeaders {
            if_none_match: "",
            if_modified_since: "Wed, 21 Oct 2015 08:00:00 GMT",
            cache_control: "",
        };
        expect_eq(isFresh(newer_req, res), true);

        // Client date older than Last-Modified -> stale
        const older_req = RequestHeaders {
            if_none_match: "",
            if_modified_since: "Wed, 21 Oct 2015 07:27:59 GMT",
            cache_control: "",
        };
        expect_eq(isFresh(older_req, res), false);
    });

    test("5. @cap/fresh — Combined If-None-Match & If-Modified-Since must both pass", () => {
        const res = ResponseHeaders {
            etag: "\"etag-1\"",
            last_modified: "Wed, 21 Oct 2015 07:28:00 GMT",
        };

        // Both match -> fresh
        const both_ok = RequestHeaders {
            if_none_match: "\"etag-1\"",
            if_modified_since: "Wed, 21 Oct 2015 07:28:00 GMT",
            cache_control: "",
        };
        expect_eq(isFresh(both_ok, res), true);

        // ETag mismatch even when date matches -> stale
        const bad_etag = RequestHeaders {
            if_none_match: "\"etag-old\"",
            if_modified_since: "Wed, 21 Oct 2015 08:00:00 GMT",
            cache_control: "",
        };
        expect_eq(isFresh(bad_etag, res), false);

        // Date stale even when ETag matches -> stale
        const bad_date = RequestHeaders {
            if_none_match: "\"etag-1\"",
            if_modified_since: "Wed, 21 Oct 2015 06:00:00 GMT",
            cache_control: "",
        };
        expect_eq(isFresh(bad_date, res), false);
    });

    test("6. @cap/fresh — etagStrongMatch rejects weak W/ tags while etagWeakMatch accepts", () => {
        expect_eq(etagStrongMatch("\"abc\"", "\"abc\""), true);
        expect_eq(etagStrongMatch("W/\"abc\"", "\"abc\""), false);
        expect_eq(etagStrongMatch("\"abc\"", "W/\"abc\""), false);
        expect_eq(etagStrongMatch("W/\"abc\"", "W/\"abc\""), false);

        expect_eq(etagWeakMatch("W/\"abc\"", "\"abc\""), true);
        expect_eq(etagWeakMatch("W/\"abc\"", "W/\"abc\""), true);
        expect_eq(etagWeakMatch("invalid", "\"abc\""), false);
    });
}

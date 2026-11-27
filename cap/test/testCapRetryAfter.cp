import { test, expect_eq } from "cap:test";
import {
    parseRetryAfter,
    waitSeconds,
    parseHttpDate,
    formatDelaySeconds,
    KIND_DELAY_SECONDS,
    KIND_HTTP_DATE,
    KIND_INVALID,
} from "../../packages/retry-after/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("parse delay-seconds with whitespace trimming", () => {
        let r = parseRetryAfter("  120  ");
        expect_eq(r.valid, true);
        expect_eq(r.kind, KIND_DELAY_SECONDS);
        expect_eq(r.delay_seconds, 120);
        expect_eq(waitSeconds(r, 1700000000), 120);
    });

    test("parse RFC 9110 IMF-fixdate HTTP-date and compute waitSeconds", () => {
        let r = parseRetryAfter("Wed, 21 Oct 2015 07:28:00 GMT");
        expect_eq(r.valid, true);
        expect_eq(r.kind, KIND_HTTP_DATE);
        expect_eq(r.epoch_seconds, 1445412480);
        expect_eq(waitSeconds(r, 1445412400), 80);
        expect_eq(waitSeconds(r, 1445412500), 0);
    });

    test("leap year Feb 29 validation", () => {
        expect_eq(parseHttpDate("Thu, 29 Feb 2024 00:00:00 GMT") > 0, true);
        expect_eq(parseHttpDate("Fri, 29 Feb 2025 00:00:00 GMT"), -1);
    });

    test("invalid headers return valid == false and waitSeconds == -1", () => {
        let r1 = parseRetryAfter("");
        expect_eq(r1.valid, false);
        expect_eq(r1.kind, KIND_INVALID);
        expect_eq(waitSeconds(r1, 0), -1);

        expect_eq(parseRetryAfter("-10").valid, false);
        expect_eq(parseRetryAfter("abc").valid, false);
        expect_eq(parseRetryAfter("Wed, 21 Oct 2015 07:28:00 UTC").valid, false);
    });

    test("formatDelaySeconds handles zero, multi-digit, and undersized buffer", () => {
        let buf: [16]u8 = undefined;
        let n0 = formatDelaySeconds(0, buf[0..16]);
        expect_eq(eqBytes(buf[0..(n0 as usize)], "0"), true);
        let n1 = formatDelaySeconds(3600, buf[0..16]);
        expect_eq(eqBytes(buf[0..(n1 as usize)], "3600"), true);
        let tiny: [2]u8 = undefined;
        expect_eq(formatDelaySeconds(3600, tiny[0..2]), -1);
    });

    test("zero delay-seconds is valid", () => {
        let r = parseRetryAfter("0");
        expect_eq(r.valid, true);
        expect_eq(waitSeconds(r, 1000), 0);
    });
}

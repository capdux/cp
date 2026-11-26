// cap/test/testCapPrettyMs.cp
// Comprehensive unit tests for @cap/pretty-ms

import { test, expect_eq } from "cap:test";
import {
    PrettyMsOptions,
    parseMs,
    prettyMs,
    prettyMsWith,
} from "../../packages/@cap/pretty-ms/src/index.cp";

fn main() {
    test("1. @cap/pretty-ms — parseMs exact positive & negative decomposition", () => {
        const p = parseMs(1337000000);
        expect_eq(p.days, 15 as i64);
        expect_eq(p.hours, 11 as i64);
        expect_eq(p.minutes, 23 as i64);
        expect_eq(p.seconds, 20 as i64);
        expect_eq(p.milliseconds, 0 as i64);

        const neg = parseMs(-3723456);
        expect_eq(neg.days, 0 as i64);
        expect_eq(neg.hours, -1 as i64);
        expect_eq(neg.minutes, -2 as i64);
        expect_eq(neg.seconds, -3 as i64);
        expect_eq(neg.milliseconds, -456 as i64);
    });

    test("2. @cap/pretty-ms — default prettyMs formatting", () => {
        let buf: [128]u8 = undefined;
        expect_eq(prettyMs(1337000000, buf[0..128]), "15d 11h 23m 20s");
        expect_eq(prettyMs(1337, buf[0..128]), "1.3s");
        expect_eq(prettyMs(42, buf[0..128]), "42ms");
        expect_eq(prettyMs(0, buf[0..128]), "0ms");
        expect_eq(prettyMs(-1337, buf[0..128]), "-1.3s");
    });

    test("3. @cap/pretty-ms — verbose mode with singular & plural units", () => {
        let buf: [128]u8 = undefined;
        let opts = PrettyMsOptions.defaults();
        opts.verbose = true;

        // 1 day, 2 hours, 3 minutes, 4 seconds = 86400 + 7200 + 180 + 4 = 93784s
        expect_eq(prettyMsWith(93784000, opts, buf[0..128]), "1 day 2 hours 3 minutes 4 seconds");
        // Singular check: 1 millisecond & 1 second
        expect_eq(prettyMsWith(1, opts, buf[0..128]), "1 millisecond");
        expect_eq(prettyMsWith(1000, opts, buf[0..128]), "1 second");
        expect_eq(prettyMsWith(0, opts, buf[0..128]), "0 milliseconds");
    });

    test("4. @cap/pretty-ms — compact mode & unit_count cap", () => {
        let buf: [128]u8 = undefined;

        let c_opts = PrettyMsOptions.defaults();
        c_opts.compact = true;
        expect_eq(prettyMsWith(1337000000, c_opts, buf[0..128]), "15d");

        let u_opts = PrettyMsOptions.defaults();
        u_opts.unit_count = 2;
        expect_eq(prettyMsWith(1337000000, u_opts, buf[0..128]), "15d 11h");
    });

    test("5. @cap/pretty-ms — colon_notation timer/player style", () => {
        let buf: [128]u8 = undefined;
        let opts = PrettyMsOptions.defaults();
        opts.colon_notation = true;

        expect_eq(prettyMsWith(0, opts, buf[0..128]), "0:00");
        expect_eq(prettyMsWith(5000, opts, buf[0..128]), "0:05");
        expect_eq(prettyMsWith(65000, opts, buf[0..128]), "1:05");
        expect_eq(prettyMsWith(3723000, opts, buf[0..128]), "1:02:03");
        expect_eq(prettyMsWith(-5000, opts, buf[0..128]), "-0:05");
    });

    test("6. @cap/pretty-ms — separate_ms mode & negative durations", () => {
        let buf: [128]u8 = undefined;
        let opts = PrettyMsOptions.defaults();
        opts.separate_ms = true;

        expect_eq(prettyMsWith(1337, opts, buf[0..128]), "1s 337ms");
        expect_eq(prettyMsWith(-61005, opts, buf[0..128]), "-1m 1s 5ms");
        expect_eq(prettyMsWith(500, opts, buf[0..128]), "500ms");
    });
}

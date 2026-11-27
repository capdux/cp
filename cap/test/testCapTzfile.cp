import { test, expect_eq } from "cap:test";
import {
    TZIF_HEADER_LEN, TzifHeader, PosixTz, TzTransition,
    isTzifMagic, parseTzifHeader, parsePosixTz,
    lookupTransitionOffset, utcToLocalSeconds
} from "../../packages/tzfile/src/index.cp";

fn main() {
    test("1. @cap/tzfile — parses valid TZif v2 binary header", () => {
        let buf: [44]u8 = [0; 44];
        buf[0] = 84; buf[1] = 90; buf[2] = 105; buf[3] = 102; // "TZif"
        buf[4] = 50; // '2'
        buf[35] = 12; // timecnt = 12
        buf[39] = 3;  // typecnt = 3
        buf[43] = 16; // charcnt = 16
        expect_eq(isTzifMagic(buf[0..44]), true);
        let h = parseTzifHeader(buf[0..44]) ?? TzifHeader { version: 0, isutcnt: 0, isstdcnt: 0, leapcnt: 0, timecnt: 0, typecnt: 0, charcnt: 0 };
        expect_eq(h.version, 2);
        expect_eq(h.timecnt, 12);
        expect_eq(h.typecnt, 3);
        expect_eq(h.charcnt, 16);
    });

    test("2. @cap/tzfile — rejects invalid magic, bad version, or zero typecnt", () => {
        let bad: [44]u8 = [0; 44];
        expect_eq(parseTzifHeader(bad[0..44]) == null, true);
        bad[0] = 84; bad[1] = 90; bad[2] = 105; bad[3] = 102;
        bad[4] = 50;
        // typecnt == 0 invalid per RFC 8536
        expect_eq(parseTzifHeader(bad[0..44]) == null, true);
        expect_eq(TZIF_HEADER_LEN, 44);
    });

    test("3. @cap/tzfile — parsePosixTz parses EST5EDT with default +1h DST", () => {
        let tz = parsePosixTz("EST5EDT,M3.2.0,M11.1.0") ?? PosixTz { std_abbr: "", std_offset_secs: 0, has_dst: false, dst_abbr: "", dst_offset_secs: 0 };
        expect_eq(tz.std_abbr, "EST");
        expect_eq(tz.std_offset_secs, -18000);
        expect_eq(tz.has_dst, true);
        expect_eq(tz.dst_abbr, "EDT");
        expect_eq(tz.dst_offset_secs, -14400);
    });

    test("4. @cap/tzfile — parsePosixTz parses AEST-10AEDT & quoted <+0530>-5:30", () => {
        let au = parsePosixTz("AEST-10AEDT") ?? PosixTz { std_abbr: "", std_offset_secs: 0, has_dst: false, dst_abbr: "", dst_offset_secs: 0 };
        expect_eq(au.std_abbr, "AEST");
        expect_eq(au.std_offset_secs, 36000);
        expect_eq(au.dst_offset_secs, 39600);

        let ind = parsePosixTz("<+0530>-5:30") ?? PosixTz { std_abbr: "", std_offset_secs: 0, has_dst: false, dst_abbr: "", dst_offset_secs: 0 };
        expect_eq(ind.std_abbr, "+0530");
        expect_eq(ind.std_offset_secs, 19800);
        expect_eq(ind.has_dst, false);
    });

    test("5. @cap/tzfile — lookupTransitionOffset & utcToLocalSeconds", () => {
        let tr: [2]TzTransition = [
            TzTransition { at_unix: 1000, offset_secs: 36000, is_dst: false },
            TzTransition { at_unix: 5000, offset_secs: 39600, is_dst: true },
        ];
        expect_eq(lookupTransitionOffset(tr[0..2], 500, 0), 0);
        expect_eq(lookupTransitionOffset(tr[0..2], 2000, 0), 36000);
        expect_eq(lookupTransitionOffset(tr[0..2], 6000, 0), 39600);
        expect_eq(utcToLocalSeconds(10000, 39600), 49600);
    });

    test("6. @cap/tzfile — rejects malformed POSIX TZ strings", () => {
        expect_eq(parsePosixTz("") == null, true);
        expect_eq(parsePosixTz("AB5") == null, true);
        expect_eq(parsePosixTz("UTC99") == null, true);
    });
}

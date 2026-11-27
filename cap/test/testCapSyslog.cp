// cap/test/testCapSyslog.cp
// Comprehensive unit tests for @cap/syslog (RFC 5424 Syslog codec)

import { test, expect_eq } from "cap:test";
import {
    SEV_EMERG,
    SEV_ERR,
    SEV_INFO,
    SEV_DEBUG,
    FAC_KERN,
    FAC_DAEMON,
    FAC_AUTH,
    FAC_LOCAL0,
    SyslogMessage,
    encodePri,
    decodeFacility,
    decodeSeverity,
    parseRfc5424,
    formatRfc5424,
} from "../../packages/syslog/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/syslog — encodePri, decodeFacility & decodeSeverity", () => {
        const pri1: u8 = encodePri(FAC_LOCAL0, SEV_INFO);
        expect_eq(pri1, 134 as u8); // 16 * 8 + 6
        expect_eq(decodeFacility(pri1), FAC_LOCAL0);
        expect_eq(decodeSeverity(pri1), SEV_INFO);

        const pri2: u8 = encodePri(FAC_KERN, SEV_EMERG);
        expect_eq(pri2, 0 as u8);
        expect_eq(decodeFacility(pri2), FAC_KERN);
        expect_eq(decodeSeverity(pri2), SEV_EMERG);

        const pri3: u8 = encodePri(FAC_AUTH, SEV_ERR);
        expect_eq(pri3, 35 as u8);
        expect_eq(decodeFacility(pri3), FAC_AUTH);
        expect_eq(decodeSeverity(pri3), SEV_ERR);
    });

    test("2. @cap/syslog — parseRfc5424 standard message", () => {
        const raw = "<34>1 2026-11-26T12:00:00Z myhost.example su 1234 ID47 - su root failed";
        const m: SyslogMessage = parseRfc5424(raw);

        expect_eq(m.valid, true);
        expect_eq(m.facility, 4 as u8);
        expect_eq(m.severity, 2 as u8);
        expect_eq(m.version, 1 as u8);
        expect_eq(sliceEq(raw[m.ts_start..(m.ts_start + m.ts_len)], "2026-11-26T12:00:00Z"), true);
        expect_eq(sliceEq(raw[m.host_start..(m.host_start + m.host_len)], "myhost.example"), true);
        expect_eq(sliceEq(raw[m.app_start..(m.app_start + m.app_len)], "su"), true);
        expect_eq(sliceEq(raw[m.proc_start..(m.proc_start + m.proc_len)], "1234"), true);
        expect_eq(sliceEq(raw[m.msgid_start..(m.msgid_start + m.msgid_len)], "ID47"), true);
        expect_eq(sliceEq(raw[m.msg_start..(m.msg_start + m.msg_len)], "su root failed"), true);
    });

    test("3. @cap/syslog — parseRfc5424 with structured data [exampleSDID@32473 iut=\"3\"]", () => {
        const raw = "<165>1 2026-11-26T12:30:00Z web01 app 99 ID01 [exampleSDID@32473 iut=\"3\" msg=\"a]b\"] event fired";
        const m: SyslogMessage = parseRfc5424(raw);

        expect_eq(m.valid, true);
        expect_eq(m.facility, 20 as u8);
        expect_eq(m.severity, 5 as u8);
        expect_eq(sliceEq(raw[m.app_start..(m.app_start + m.app_len)], "app"), true);
        expect_eq(sliceEq(raw[m.msg_start..(m.msg_start + m.msg_len)], "event fired"), true);
    });

    test("4. @cap/syslog — NILVALUE '-' fields map to empty spans", () => {
        const raw = "<14>1 - - - - - - minimal";
        const m: SyslogMessage = parseRfc5424(raw);

        expect_eq(m.valid, true);
        expect_eq(m.ts_len, 0 as usize);
        expect_eq(m.host_len, 0 as usize);
        expect_eq(m.app_len, 0 as usize);
        expect_eq(m.proc_len, 0 as usize);
        expect_eq(m.msgid_len, 0 as usize);
        expect_eq(sliceEq(raw[m.msg_start..(m.msg_start + m.msg_len)], "minimal"), true);
    });

    test("5. @cap/syslog — formatRfc5424 -> parseRfc5424 round-trip", () => {
        let buf: [256]u8 = undefined;
        const n: i64 = formatRfc5424(
            FAC_DAEMON,
            SEV_DEBUG,
            "2026-11-26T12:10:00Z",
            "edge-01",
            "capd",
            "",
            "BOOT",
            "ready to serve",
            buf[0..256],
        );
        expect_eq(n > 0, true);

        const raw: []const u8 = buf[0..(n as usize)];
        const m: SyslogMessage = parseRfc5424(raw);
        expect_eq(m.valid, true);
        expect_eq(m.facility, FAC_DAEMON);
        expect_eq(m.severity, SEV_DEBUG);
        expect_eq(sliceEq(raw[m.host_start..(m.host_start + m.host_len)], "edge-01"), true);
        expect_eq(sliceEq(raw[m.app_start..(m.app_start + m.app_len)], "capd"), true);
        expect_eq(m.proc_len, 0 as usize);
        expect_eq(sliceEq(raw[m.msgid_start..(m.msgid_start + m.msgid_len)], "BOOT"), true);
        expect_eq(sliceEq(raw[m.msg_start..(m.msg_start + m.msg_len)], "ready to serve"), true);
    });

    test("6. @cap/syslog — rejects invalid PRI (>191, missing '>') & tiny buffers", () => {
        // PRI > 191
        expect_eq(parseRfc5424("<192>1 - - - - - - hi").valid, false);
        // Missing '>'
        expect_eq(parseRfc5424("<34 1 - - - - - - hi").valid, false);
        // Empty PRI
        expect_eq(parseRfc5424("<>1 - - - - - - hi").valid, false);
        // Invalid facility/severity in format
        let buf: [64]u8 = undefined;
        expect_eq(formatRfc5424(24, 0, "", "", "", "", "", "hi", buf[0..64]), -1 as i64);
        expect_eq(formatRfc5424(0, 8, "", "", "", "", "", "hi", buf[0..64]), -1 as i64);
        // Buffer overflow
        let tiny: [8]u8 = undefined;
        expect_eq(formatRfc5424(FAC_LOCAL0, SEV_INFO, "2026", "h", "a", "1", "m", "hello", tiny[0..8]), -1 as i64);
    });
}

// Source: cp/cap/test/test_leef_production_deep.cp
// Deep Production Test Suite for cap:leef
// Covers:
//   1. LEEF 1.0 Tab-delimited event with Syslog prefix parsing
//   2. LEEF 2.0 custom delimiters: single-char '^', hex '0x7C' ('|'), hex 'x09' ('\t') & empty fallback
//   3. LeefAttrIter & findLeefAttr with embedded '=' inside values
//   4. formatLeefHeader 1.0 & 2.0 roundtrip encode/decode (including automatic 0x7C for '|')
//   5. Invalid version, missing pipe fields, invalid hex delimiter & small output buffer guards

import { test, expect_eq } from "cap:test";
import {
    LEEF_PREFIX,
    LEEF_DEFAULT_DELIM,
    LeefEvent,
    LeefAttr,
    LeefAttrIter,
    isLeef,
    parseLeefLine,
    findLeefAttr,
    formatLeefHeader,
} from "cap:leef";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("leef: LEEF 1.0 Tab-delimited event with Syslog prefix") {
    const line = "<134>Nov 21 20:00:00 qradar LEEF:1.0|IBM|Guardium|11.5|LOGIN_OK|src=10.1.2.3\tdst=10.1.2.4\tsrcPort=51422\tdstPort=443\tusrName=alice\tsev=5\tcat=auth\r\n";

    expect_eq(strEq(LEEF_PREFIX, "LEEF:"), true);
    expect_eq(LEEF_DEFAULT_DELIM, '\t');
    expect_eq(isLeef(line), true);

    if (ev := parseLeefLine(line)) {
        expect_eq(strEq(ev.syslog_prefix, "<134>Nov 21 20:00:00 qradar"), true);
        expect_eq(ev.version_major, 1);
        expect_eq(ev.version_minor, 0);
        expect_eq(strEq(ev.vendor, "IBM"), true);
        expect_eq(strEq(ev.product, "Guardium"), true);
        expect_eq(strEq(ev.product_version, "11.5"), true);
        expect_eq(strEq(ev.event_id, "LOGIN_OK"), true);
        expect_eq(ev.delimiter, '\t');

        if (src := findLeefAttr(ev, "src")) {
            expect_eq(strEq(src, "10.1.2.3"), true);
        } else {
            expect_eq(true, false);
        }
        if (usr := ev.findAttr("usrName")) {
            expect_eq(strEq(usr, "alice"), true);
        } else {
            expect_eq(true, false);
        }
        if (sev := findLeefAttr(ev, "sev")) {
            expect_eq(strEq(sev, "5"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("leef: LEEF 2.0 custom delimiters ('^', '0x7C', 'x09' & empty default fallback)") {
    // 1. Single-char '^'
    const l_caret = "LEEF:2.0|Vendor|Prod|2.1|EVT1|^|src=1.2.3.4^cat=fw^sev=7";
    if (ev := parseLeefLine(l_caret)) {
        expect_eq(ev.version_major, 2);
        expect_eq(ev.delimiter, '^');
        if (cat := findLeefAttr(ev, "cat")) {
            expect_eq(strEq(cat, "fw"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 2. Hex 0x7C ('|')
    const l_pipe = "LEEF:2.0|Vendor|Prod|2.1|EVT2|0x7C|src=9.9.9.9|dstPort=53|usrName=bob";
    if (ev := parseLeefLine(l_pipe)) {
        expect_eq(ev.delimiter, '|');
        if (dp := findLeefAttr(ev, "dstPort")) {
            expect_eq(strEq(dp, "53"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 3. Short hex x09 ('\t')
    const l_x09 = "LEEF:2.0|V|P|1|E|x09|devTime=2026-11-21\tsev=3";
    if (ev := parseLeefLine(l_x09)) {
        expect_eq(ev.delimiter, '\t');
        if (dt := findLeefAttr(ev, "devTime")) {
            expect_eq(strEq(dt, "2026-11-21"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4. Empty delimiter field falls back to '\t'
    const l_empty = "LEEF:2.0|V|P|1|E||k1=v1\tk2=v2";
    if (ev := parseLeefLine(l_empty)) {
        expect_eq(ev.delimiter, '\t');
        if (v2 := findLeefAttr(ev, "k2")) {
            expect_eq(strEq(v2, "v2"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("leef: LeefAttrIter & findLeefAttr preserve embedded '=' inside values") {
    const line = "LEEF:2.0|Acme|WAF|3.0|REQ|^|url=https://ex.com?a=1&b=2^dn=CN=admin,DC=corp^sev=4";
    if (ev := parseLeefLine(line)) {
        let it = ev.attrs();
        let count: u32 = 0;
        while (attr := it.next()) {
            count += 1;
            if (count == 1) {
                expect_eq(strEq(attr.key, "url"), true);
                expect_eq(strEq(attr.value, "https://ex.com?a=1&b=2"), true);
            } else if (count == 2) {
                expect_eq(strEq(attr.key, "dn"), true);
                expect_eq(strEq(attr.value, "CN=admin,DC=corp"), true);
            } else if (count == 3) {
                expect_eq(strEq(attr.key, "sev"), true);
                expect_eq(strEq(attr.value, "4"), true);
            }
        }
        expect_eq(count, 3);
    } else {
        expect_eq(true, false);
    }
}

test("leef: formatLeefHeader 1.0 & 2.0 roundtrip encode/decode") {
    let buf: [128]u8 = [0; 128];

    // LEEF 1.0
    if (n := formatLeefHeader(1, "IBM", "QRadar", "7.5", "EVT10", '\t', buf[0..128])) {
        expect_eq(strEq(buf[0..n], "LEEF:1.0|IBM|QRadar|7.5|EVT10|"), true);
        if (ev := parseLeefLine(buf[0..n])) {
            expect_eq(ev.version_major, 1);
            expect_eq(ev.delimiter, '\t');
            expect_eq(strEq(ev.event_id, "EVT10"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // LEEF 2.0 with printable '^'
    if (n := formatLeefHeader(2, "Acme", "IDS", "4.0", "ALERT", '^', buf[0..128])) {
        expect_eq(strEq(buf[0..n], "LEEF:2.0|Acme|IDS|4.0|ALERT|^|"), true);
        if (ev := parseLeefLine(buf[0..n])) {
            expect_eq(ev.version_major, 2);
            expect_eq(ev.delimiter, '^');
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // LEEF 2.0 with '|' -> automatically formatted as 0x7C
    if (n := formatLeefHeader(2, "Acme", "IDS", "4.0", "ALERT", '|', buf[0..128])) {
        expect_eq(strEq(buf[0..n], "LEEF:2.0|Acme|IDS|4.0|ALERT|0x7C|"), true);
        if (ev := parseLeefLine(buf[0..n])) {
            expect_eq(ev.delimiter, '|');
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("leef: invalid version, missing pipe fields, bad hex delimiter & buffer guards") {
    // 1. Unsupported LEEF version
    expect_eq(isLeef("LEEF:3.0|V|P|1|E|a=1"), false);
    if (ev := parseLeefLine("LEEF:3.0|V|P|1|E|a=1")) {
        * = ev;
        expect_eq(true, false);
    }

    // 2. Missing pipe fields in LEEF 1.0 (only 4 pipes)
    if (ev := parseLeefLine("LEEF:1.0|V|P|1|E")) {
        * = ev;
        expect_eq(true, false);
    }

    // 3. Missing 6th pipe in LEEF 2.0
    if (ev := parseLeefLine("LEEF:2.0|V|P|1|E|src=1.1.1.1")) {
        * = ev;
        expect_eq(true, false);
    }

    // 4. Invalid hex delimiter in LEEF 2.0
    if (ev := parseLeefLine("LEEF:2.0|V|P|1|E|0xZZ|src=1.1.1.1")) {
        * = ev;
        expect_eq(true, false);
    }
    if (ev := parseLeefLine("LEEF:2.0|V|P|1|E|abc|src=1.1.1.1")) {
        * = ev;
        expect_eq(true, false);
    }

    // 5. Unsupported version / tiny buffer in formatLeefHeader
    let buf: [8]u8 = [0; 8];
    if (n := formatLeefHeader(3, "V", "P", "1", "E", '\t', buf[0..8])) {
        * = n;
        expect_eq(true, false);
    }
    if (n := formatLeefHeader(1, "Vendor", "Product", "1.0", "EVT", '\t', buf[0..8])) {
        * = n;
        expect_eq(true, false);
    }
}

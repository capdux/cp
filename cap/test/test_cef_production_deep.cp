// Source: cp/cap/test/test_cef_production_deep.cp
// Deep Production Test Suite for cap:cef
// Covers:
//   1. Complete CEF:0|... event with Syslog prefix parsing
//   2. Escaped pipes (\|) & escaped backslashes (\\ and \\|) in header fields + unescapeCefHeader
//   3. Extension key=value iteration with multi-space values, URLs & \= / \\ / \n / \r escapes
//   4. severityLevel() numeric (0..10) & textual (Low/Medium/High/Very-High) mapping
//   5. formatCefHeader roundtrip encode/decode & missing fields / invalid version guards

import { test, expect_eq } from "cap:test";
import {
    CEF_PREFIX,
    CefEvent,
    CefExtPair,
    CefExtIter,
    isCef,
    parseCefLine,
    findCefExt,
    unescapeCefHeader,
    unescapeCefExtValue,
    formatCefHeader,
} from "cap:cef";

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

test("cef: complete CEF:0 event with Syslog prefix") {
    const line = "Sep 19 08:26:10 host CEF:0|Security|threatmanager|1.0|100|worm successfully stopped|10|src=10.0.0.1 dst=2.1.2.2 spt=1232\r\n";

    expect_eq(strEq(CEF_PREFIX, "CEF:"), true);
    expect_eq(isCef(line), true);

    if (ev := parseCefLine(line)) {
        expect_eq(strEq(ev.syslog_prefix, "Sep 19 08:26:10 host"), true);
        expect_eq(ev.version, 0);
        expect_eq(strEq(ev.device_vendor, "Security"), true);
        expect_eq(strEq(ev.device_product, "threatmanager"), true);
        expect_eq(strEq(ev.device_version, "1.0"), true);
        expect_eq(strEq(ev.signature_id, "100"), true);
        expect_eq(strEq(ev.name, "worm successfully stopped"), true);
        expect_eq(strEq(ev.severity_raw, "10"), true);
        expect_eq(ev.severityLevel(), 10);

        if (src := ev.findExt("src")) {
            expect_eq(strEq(src, "10.0.0.1"), true);
        } else {
            expect_eq(true, false);
        }
        if (spt := ev.findExt("spt")) {
            expect_eq(strEq(spt, "1232"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("cef: escaped pipes (\\|) and escaped backslashes (\\\\ and \\\\|) in header fields") {
    // Vendor ends with escaped backslash `\\`, followed by real delimiter `|`!
    // Name contains escaped pipe `\|` and escaped backslash `\\`.
    const line = "CEF:1|Acme\\\\|Guard\\|Pro|2.5|SIG-9|Path C:\\\\bin\\|Alert|High|act=blocked";

    if (ev := parseCefLine(line)) {
        expect_eq(ev.version, 1);
        expect_eq(strEq(ev.syslog_prefix, ""), true);
        expect_eq(strEq(ev.device_vendor, "Acme\\\\"), true);
        expect_eq(strEq(ev.device_product, "Guard\\|Pro"), true);
        expect_eq(strEq(ev.name, "Path C:\\\\bin\\|Alert"), true);
        expect_eq(ev.severityLevel(), 8);

        let out: [128]u8 = [0; 128];
        if (n := unescapeCefHeader(ev.device_vendor, out[0..128])) {
            expect_eq(strEq(out[0..n], "Acme\\"), true);
        } else {
            expect_eq(true, false);
        }
        if (n := unescapeCefHeader(ev.name, out[0..128])) {
            expect_eq(strEq(out[0..n], "Path C:\\bin|Alert"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("cef: Extension multi-space values, embedded URL '=', and escape sequences") {
    const ext = "msg=User login failed from office src=10.0.0.1 cs1=a\\=b\\nline2\\r\\\\end request=https://ex.com?k=1";

    let it = CefExtIter.init(ext);
    let count: u32 = 0;
    while (pair := it.next()) {
        count += 1;
        if (count == 1) {
            expect_eq(strEq(pair.key, "msg"), true);
            expect_eq(strEq(pair.raw_value, "User login failed from office"), true);
        } else if (count == 2) {
            expect_eq(strEq(pair.key, "src"), true);
            expect_eq(strEq(pair.raw_value, "10.0.0.1"), true);
        } else if (count == 3) {
            expect_eq(strEq(pair.key, "cs1"), true);
            expect_eq(strEq(pair.raw_value, "a\\=b\\nline2\\r\\\\end"), true);
            let out: [64]u8 = [0; 64];
            if (n := unescapeCefExtValue(pair.raw_value, out[0..64])) {
                expect_eq(strEq(out[0..n], "a=b\nline2\r\\end"), true);
            } else {
                expect_eq(true, false);
            }
        } else if (count == 4) {
            expect_eq(strEq(pair.key, "request"), true);
            expect_eq(strEq(pair.raw_value, "https://ex.com?k=1"), true);
        }
    }
    expect_eq(count, 4);

    if (v := findCefExt(ext, "src")) {
        expect_eq(strEq(v, "10.0.0.1"), true);
    } else {
        expect_eq(true, false);
    }
}

test("cef: severityLevel() numeric and textual severity mapping") {
    const e0 = CefEvent { severity_raw: "0" };
    const e7 = CefEvent { severity_raw: "7" };
    const e10 = CefEvent { severity_raw: "10" };
    const e_low = CefEvent { severity_raw: "Low" };
    const e_med = CefEvent { severity_raw: "MEDIUM" };
    const e_high = CefEvent { severity_raw: "high" };
    const e_vh = CefEvent { severity_raw: "Very-High" };
    const e_unk = CefEvent { severity_raw: "Unknown" };

    expect_eq(e0.severityLevel(), 0);
    expect_eq(e7.severityLevel(), 7);
    expect_eq(e10.severityLevel(), 10);
    expect_eq(e_low.severityLevel(), 3);
    expect_eq(e_med.severityLevel(), 5);
    expect_eq(e_high.severityLevel(), 8);
    expect_eq(e_vh.severityLevel(), 10);
    expect_eq(e_unk.severityLevel(), 0);
}

test("cef: formatCefHeader roundtrip & malformed line / invalid version guards") {
    let buf: [256]u8 = [0; 256];
    if (n := formatCefHeader(0, "Acme|Corp", "Net\\Guard", "3.1", "404", "Rule | Check", "Very-High", buf[0..256])) {
        const encoded = buf[0..n];
        if (ev := parseCefLine(encoded)) {
            expect_eq(ev.version, 0);
            expect_eq(ev.severityLevel(), 10);

            let tmp: [64]u8 = [0; 64];
            if (vn := unescapeCefHeader(ev.device_vendor, tmp[0..64])) {
                expect_eq(strEq(tmp[0..vn], "Acme|Corp"), true);
            } else {
                expect_eq(true, false);
            }
            if (pn := unescapeCefHeader(ev.device_product, tmp[0..64])) {
                expect_eq(strEq(tmp[0..pn], "Net\\Guard"), true);
            } else {
                expect_eq(true, false);
            }
            if (nn := unescapeCefHeader(ev.name, tmp[0..64])) {
                expect_eq(strEq(tmp[0..nn], "Rule | Check"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Buffer too small for formatCefHeader
    let tiny: [8]u8 = [0; 8];
    if (n := formatCefHeader(0, "V", "P", "1", "1", "N", "1", tiny[0..8])) {
        * = n;
        expect_eq(true, false);
    }

    // Missing CEF: marker
    expect_eq(isCef("Sep 19 host plain syslog line"), false);

    // Fewer than 7 unescaped pipes
    expect_eq(isCef("CEF:0|Vendor|Product|1.0|100|Name"), false);

    // Invalid non-numeric CEF version
    expect_eq(isCef("CEF:X|Vendor|Product|1.0|100|Name|5|src=1.1.1.1"), false);
}

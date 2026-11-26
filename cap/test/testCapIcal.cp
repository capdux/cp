import { test, expect_eq } from "cap:test";
import { IcalEvent, IcalWriter, parseFirstEvent, unescapeText } from "../../packages/@cap/ical/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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

fn containsSlice(haystack: []const u8, needle: []const u8) -> bool {
    if (needle.len == 0 || needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (testEq(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/ical — Single & multi-event .ics generation with CRLF endings", () => {
        let buf: [1024]u8 = undefined;
        let w = IcalWriter.init(buf[0..1024], "-//CP Lang//CapCal//EN");
        expect_eq(w.addEvent(IcalEvent {
            uid: "ev-1@cp.dev",
            dtstart: "20261126T090000Z",
            dtend: "20261126T100000Z",
            summary: "Architecture Review",
            location: "Room A",
            description: "Zero-alloc review",
        }), true);
        expect_eq(w.addEvent(IcalEvent {
            uid: "ev-2@cp.dev",
            dtstart: "20261126T140000Z",
            dtend: "20261126T150000Z",
            summary: "Release Sync",
            location: "",
            description: "",
        }), true);
        expect_eq(w.eventCount(), 2 as usize);

        const total = w.finish();
        expect_eq(total > 0, true);
        const out = w.written();
        expect_eq(containsSlice(out, "BEGIN:VCALENDAR\r\nVERSION:2.0\r\n"), true);
        expect_eq(containsSlice(out, "UID:ev-1@cp.dev\r\n"), true);
        expect_eq(containsSlice(out, "UID:ev-2@cp.dev\r\n"), true);
        expect_eq(containsSlice(out, "END:VCALENDAR\r\n"), true);
    });

    test("2. @cap/ical — Text escaping & unescapeText roundtrip", () => {
        const escaped = "Line1\\nLine2\\NComma\\,Semi\\;Slash\\\\End";
        let dst: [128]u8 = undefined;
        const n = unescapeText(escaped, dst[0..128]);
        const expected = "Line1\nLine2\nComma,Semi;Slash\\End";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(dst[0..n as usize], expected), true);

        let tiny: [4]u8 = undefined;
        expect_eq(unescapeText(escaped, tiny[0..4]), -1 as i64);
    });

    test("3. @cap/ical — parseFirstEvent handles parameterized DTSTART;TZID=UTC", () => {
        const ics = "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nBEGIN:VEVENT\r\nUID:tz-42@cap.dev\r\nDTSTART;TZID=UTC:20261126T120000Z\r\nDTEND;TZID=UTC:20261126T133000Z\r\nSUMMARY:Launch\\, v1.0\r\nLOCATION:HQ\\; Floor 3\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n";
        const ev = parseFirstEvent(ics);
        expect_eq(ev.valid, true);
        expect_eq(testEq(ics[ev.uid_start..ev.uid_start + ev.uid_len], "tz-42@cap.dev"), true);
        expect_eq(testEq(ics[ev.dtstart_start..ev.dtstart_start + ev.dtstart_len], "20261126T120000Z"), true);
        expect_eq(testEq(ics[ev.dtend_start..ev.dtend_start + ev.dtend_len], "20261126T133000Z"), true);
        expect_eq(testEq(ics[ev.summary_start..ev.summary_start + ev.summary_len], "Launch\\, v1.0"), true);
        expect_eq(testEq(ics[ev.location_start..ev.location_start + ev.location_len], "HQ\\; Floor 3"), true);
    });

    test("4. @cap/ical — IcalWriter -> parseFirstEvent + unescapeText full roundtrip", () => {
        let buf: [512]u8 = undefined;
        let w = IcalWriter.init(buf[0..512], "-//CP//Test//EN");
        * = w.addEvent(IcalEvent {
            uid: "roundtrip-1",
            dtstart: "20261201T080000Z",
            dtend: "20261201T090000Z",
            summary: "Sprint, Demo; Q4",
            location: "Building A\\B",
            description: "Agenda:\n1. Build\n2. Ship",
        });
        const total = w.finish();
        expect_eq(total > 0, true);

        const ics = w.written();
        const parsed = parseFirstEvent(ics);
        expect_eq(parsed.valid, true);
        expect_eq(testEq(ics[parsed.uid_start..parsed.uid_start + parsed.uid_len], "roundtrip-1"), true);

        let sum_buf: [64]u8 = undefined;
        const sn = unescapeText(ics[parsed.summary_start..parsed.summary_start + parsed.summary_len], sum_buf[0..64]);
        expect_eq(testEq(sum_buf[0..sn as usize], "Sprint, Demo; Q4"), true);

        let loc_buf: [64]u8 = undefined;
        const ln = unescapeText(ics[parsed.location_start..parsed.location_start + parsed.location_len], loc_buf[0..64]);
        expect_eq(testEq(loc_buf[0..ln as usize], "Building A\\B"), true);
    });

    test("5. @cap/ical — Missing BEGIN:VEVENT or END:VEVENT returns valid == false", () => {
        expect_eq(parseFirstEvent("").valid, false);
        expect_eq(parseFirstEvent("BEGIN:VCALENDAR\r\nEND:VCALENDAR\r\n").valid, false);
        expect_eq(parseFirstEvent("BEGIN:VEVENT\r\nUID:orphan\r\n").valid, false);
    });

    test("6. @cap/ical — Undersized writer buffer returns -1 on finish", () => {
        let tiny: [32]u8 = undefined;
        let w = IcalWriter.init(tiny[0..32], "-//CP//Tiny//EN");
        expect_eq(w.finish(), -1 as i64);

        let med: [64]u8 = undefined;
        let w2 = IcalWriter.init(med[0..64], "-//CP//Med//EN");
        expect_eq(w2.addEvent(IcalEvent {
            uid: "too-long-event-id-1234567890",
            dtstart: "20261126T000000Z",
            dtend: "20261126T010000Z",
            summary: "Overflow test",
            location: "",
            description: "",
        }), false);
        expect_eq(w2.finish(), -1 as i64);
    });
}

// Source: cp/cap/test/test_syslog_production_deep.cp
// Deep Production Test Suite for cap:syslog & cap.syslog
// Covers:
//   1. RFC 5424 full fields, multiple SD-ELEMENTs, and escape/unescape round-trip
//   2. NILVALUE '-', UTF-8 BOM stripping, and empty message boundaries
//   3. RFC 3164 BSD Syslog auto-detection, tag[pid]: msg extraction, and formatting
//   4. Malformed packet interception (invalid PRI > 191 / non-digit, unsupported version, unclosed SD-ELEMENT)
//   5. Dynamic namespace mounting (`import cap from "cap"; cap.syslog`) and std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SyslogError,
    Facility,
    Severity,
    SdParam,
    SdElement,
    SyslogMessage,
    parse,
    parseRfc5424,
    parseRfc3164,
    formatRfc5424,
    formatRfc3164,
} from "cap:syslog";
import cap from "cap";

fn expectParseError(alloc: std.mem.Allocator, raw: []const u8, expected: SyslogError) {
    switch (parse(alloc, raw)) {
        msg => {
            let mut_msg = msg;
            mut_msg.deinit();
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == expected, true);
        },
    }
}

fn testRfc5424FullRoundTrip()! {
    const alloc = std.testing.allocator;
    const raw =
        "<165>1 2026-11-20T17:58:42.123456+08:00 edge-gw-01.prod envoy 4219 ID47 " ++
        "[exampleSDID@32473 iut=\"3\" eventSource=\"App \\\"Core\\\"\" eventID=\"1011\\]\\\\x\"]" ++
        "[meta@12345 trace=\"abc-999\"] High latency detected on upstream";

    let parsed = parseRfc5424(alloc, raw)?;
    defer parsed.deinit();

    expect_eq(parsed.is_rfc5424, true);
    expect_eq(parsed.priority, 165 as u8);
    expect_eq(parsed.version, 1 as u8);
    expect_eq(parsed.facility == .Local4, true);
    expect_eq(parsed.severity == .Notice, true);
    expect_eq(std.mem.eql(u8, parsed.facility.toString(), "local4"), true);
    expect_eq(std.mem.eql(u8, parsed.severity.toString(), "notice"), true);

    expect_eq(std.mem.eql(u8, parsed.timestamp.?, "2026-11-20T17:58:42.123456+08:00"), true);
    expect_eq(std.mem.eql(u8, parsed.hostname.?, "edge-gw-01.prod"), true);
    expect_eq(std.mem.eql(u8, parsed.app_name.?, "envoy"), true);
    expect_eq(std.mem.eql(u8, parsed.proc_id.?, "4219"), true);
    expect_eq(std.mem.eql(u8, parsed.msg_id.?, "ID47"), true);
    expect_eq(std.mem.eql(u8, parsed.message, "High latency detected on upstream"), true);

    expect_eq(parsed.structured_data.len, 2 as usize);
    let sd0 = parsed.getSdElement("exampleSDID@32473").?;
    expect_eq(std.mem.eql(u8, sd0.getParam("iut").?, "3"), true);
    expect_eq(std.mem.eql(u8, sd0.getParam("eventSource").?, "App \"Core\""), true);
    expect_eq(std.mem.eql(u8, sd0.getParam("eventID").?, "1011]\\x"), true);

    let sd1 = parsed.getSdElement("meta@12345").?;
    expect_eq(std.mem.eql(u8, sd1.getParam("trace").?, "abc-999"), true);

    // Format back to RFC 5424 and verify lossless round-trip
    let formatted = formatRfc5424(alloc, parsed)?;
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, raw), true);
}

fn testRfc5424NilValuesBomAndEmptyMsg()! {
    const alloc = std.testing.allocator;

    // 1. NILVALUEs with UTF-8 BOM in message
    const raw_bom = "<14>1 - - - - - - \xEF\xBB\xBFBOM payload";
    let msg_bom = parse(alloc, raw_bom)?;
    defer msg_bom.deinit();

    expect_eq(msg_bom.is_rfc5424, true);
    expect_eq(msg_bom.facility == .User, true);
    expect_eq(msg_bom.severity == .Info, true);
    expect_eq(msg_bom.timestamp == null, true);
    expect_eq(msg_bom.hostname == null, true);
    expect_eq(msg_bom.app_name == null, true);
    expect_eq(msg_bom.proc_id == null, true);
    expect_eq(msg_bom.msg_id == null, true);
    expect_eq(msg_bom.structured_data.len, 0 as usize);
    expect_eq(std.mem.eql(u8, msg_bom.message, "BOM payload"), true);

    // 2. Empty message (no trailing SP MSG)
    const raw_empty = "<0>1 2026-11-20T00:00:00Z host1 - - - -";
    let msg_empty = parse(alloc, raw_empty)?;
    defer msg_empty.deinit();

    expect_eq(msg_empty.facility == .Kern, true);
    expect_eq(msg_empty.severity == .Emerg, true);
    expect_eq(std.mem.eql(u8, msg_empty.hostname.?, "host1"), true);
    expect_eq(msg_empty.message.len, 0 as usize);

    let re_encoded = formatRfc5424(alloc, msg_empty)?;
    defer alloc.free(re_encoded);
    expect_eq(std.mem.eql(u8, re_encoded, raw_empty), true);
}

fn testRfc3164AutoDetectAndFormat()! {
    const alloc = std.testing.allocator;
    const raw_bsd = "<34>Oct  5 22:14:15 mymachine sshd[12345]: Failed password for root";

    let parsed = parse(alloc, raw_bsd)?;
    defer parsed.deinit();

    expect_eq(parsed.is_rfc5424, false);
    expect_eq(parsed.priority, 34 as u8);
    expect_eq(parsed.facility == .Auth, true);
    expect_eq(parsed.severity == .Crit, true);
    expect_eq(std.mem.eql(u8, parsed.timestamp.?, "Oct  5 22:14:15"), true);
    expect_eq(std.mem.eql(u8, parsed.hostname.?, "mymachine"), true);
    expect_eq(std.mem.eql(u8, parsed.app_name.?, "sshd"), true);
    expect_eq(std.mem.eql(u8, parsed.proc_id.?, "12345"), true);
    expect_eq(std.mem.eql(u8, parsed.message, "Failed password for root"), true);

    let formatted = formatRfc3164(alloc, parsed)?;
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, raw_bsd), true);

    // Also test parseRfc3164 without PID
    const raw_nopid = "<13>Nov 20 09:00:01 web01 kernel: link up";
    let nopid = parseRfc3164(alloc, raw_nopid)?;
    defer nopid.deinit();
    expect_eq(std.mem.eql(u8, nopid.app_name.?, "kernel"), true);
    expect_eq(nopid.proc_id == null, true);
    expect_eq(std.mem.eql(u8, nopid.message, "link up"), true);
}

fn testMalformedPacketGuards()! {
    const alloc = std.testing.allocator;

    // Invalid PRI > 191
    expectParseError(alloc, "<192>1 - - - - - - msg", SyslogError.InvalidPriority);
    // Invalid PRI non-digit / empty
    expectParseError(alloc, "<abc>1 - - - - - - msg", SyslogError.InvalidPriority);
    expectParseError(alloc, "<>1 - - - - - - msg", SyslogError.InvalidPriority);

    // Unsupported version
    expectParseError(alloc, "<34>2 - - - - - - msg", SyslogError.InvalidVersion);
    expectParseError(alloc, "<34>0 - - - - - - msg", SyslogError.InvalidVersion);

    // Unclosed SD-ELEMENT / malformed SD
    expectParseError(alloc, "<34>1 - - - - - [id@1 k=\"v\" msg", SyslogError.MalformedStructuredData);
    expectParseError(alloc, "<34>1 - - - - - [id@1 k=\"bad]val\"] msg", SyslogError.MalformedStructuredData);

    // Invalid timestamp
    expectParseError(alloc, "<34>1 2026-13-40T99:00:00Z - - - - - msg", SyslogError.InvalidTimestamp);
}

fn testDynamicCapMountAndZeroLeak()! {
    const alloc = std.testing.allocator;

    let p_code = cap.syslog.Facility.Local7.toCode();
    expect_eq(p_code, 23 as u8);
    expect_eq(cap.syslog.Severity.fromCode(4).? == .Warning, true);

    let msg = cap.syslog.SyslogMessage{
        facility: cap.syslog.Facility.Daemon,
        severity: cap.syslog.Severity.Warning,
        timestamp: "2026-11-20T18:00:00Z",
        hostname: "node-a",
        app_name: "capd",
        proc_id: "77",
        msg_id: "BOOT",
        message: "service ready",
    };

    let wire = cap.syslog.formatRfc5424(alloc, msg)?;
    defer alloc.free(wire);
    expect_eq(
        std.mem.eql(u8, wire, "<28>1 2026-11-20T18:00:00Z node-a capd 77 BOOT - service ready"),
        true,
    );

    let roundtrip = cap.syslog.parse(alloc, wire)?;
    defer roundtrip.deinit();
    expect_eq(roundtrip.priority, 28 as u8);
    expect_eq(std.mem.eql(u8, roundtrip.message, "service ready"), true);
}

test("syslog: 1. RFC 5424 full fields and multi SD-ELEMENT escape round-trip", () => {
    testRfc5424FullRoundTrip() catch unreachable;
});

test("syslog: 2. NILVALUE '-', UTF-8 BOM, and empty message boundaries", () => {
    testRfc5424NilValuesBomAndEmptyMsg() catch unreachable;
});

test("syslog: 3. RFC 3164 BSD Syslog auto-detection and formatting", () => {
    testRfc3164AutoDetectAndFormat() catch unreachable;
});

test("syslog: 4. Malformed packet guards (invalid PRI, version, SD-ELEMENT, timestamp)", () => {
    testMalformedPacketGuards() catch unreachable;
});

test("syslog: 5. Dynamic cap.syslog mount and std.testing.allocator zero-leak", () => {
    testDynamicCapMountAndZeroLeak() catch unreachable;
});

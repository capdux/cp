// Source: cp/cap/test/test_igc_production_deep.cp (Production deep tests for cap:igc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IgcHeader,
    IgcFix,
    IgcStats,
    isIgc,
    parseIgcHeader,
    parseIgcBRecord,
    IgcFixIter,
    summarizeIgc,
} from "cap:text";

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.0001;
}

const SAMPLE_IGC_NEW_DATE: []const u8 =
    "AFLX123FLIGHT:1\r\n" ++
    "HFDTEDATE:211126,01\r\n" ++
    "HFPLTPILOTINCHARGE:Ada Lovelace\r\n" ++
    "HFGTYGLIDERTYPE:ASW-27B\r\n" ++
    "HFGIDGLIDERID:D-8827\r\n" ++
    "HFFTYFRTYPE:LXNAV,LX9000\r\n" ++
    "HFDTM100GPSDATUM:WGS-1984\r\n" ++
    "I023638FXA3940SIU\r\n" ++
    "B1200004730000N00830000EA0050000540\r\n" ++
    "B1201004730500N00831000EA0062000665\r\n" ++
    "B1202304731000N00832000EV0060000000\r\n";

test("1. isIgc & parseIgcHeader parse A/H records and modern HFDTEDATE:DDMMYY,NN format", () => {
    expect_eq(isIgc(SAMPLE_IGC_NEW_DATE), true);
    const hdr = parseIgcHeader(SAMPLE_IGC_NEW_DATE) ?? unreachable;

    expect_eq(std.mem.eql(u8, hdr.manufacturer, "FLX"), true);
    expect_eq(std.mem.eql(u8, hdr.logger_id, "123"), true);
    expect_eq(std.mem.eql(u8, hdr.date_ddmmyy, "211126"), true);
    expect_eq(std.mem.eql(u8, hdr.pilot, "Ada Lovelace"), true);
    expect_eq(std.mem.eql(u8, hdr.glider_type, "ASW-27B"), true);
    expect_eq(std.mem.eql(u8, hdr.glider_id, "D-8827"), true);
    expect_eq(std.mem.eql(u8, hdr.fr_type, "LXNAV,LX9000"), true);
    expect_eq(std.mem.eql(u8, hdr.gps_datum, "WGS-1984"), true);
});

test("2. Legacy HFDTEDDMMYY date header compatibility", () => {
    const legacy =
        "AXCS001\n" ++
        "HFDTE160725\n" ++
        "HFPLTPILOT:Grace Hopper\n" ++
        "B1015305100000N00000000EA0010000120\n";
    const hdr = parseIgcHeader(legacy) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.manufacturer, "XCS"), true);
    expect_eq(std.mem.eql(u8, hdr.logger_id, "001"), true);
    expect_eq(std.mem.eql(u8, hdr.date_ddmmyy, "160725"), true);
    expect_eq(std.mem.eql(u8, hdr.pilot, "Grace Hopper"), true);
});

test("3. parseIgcBRecord converts DDMMmmm South/West coordinates and negative pressure altitude", () => {
    // 33 deg 51.600 min S = -(33 + 51.6/60) = -33.86
    // 151 deg 12.600 min W = -(151 + 12.6/60) = -151.21
    // Pressure alt = -0015 m, GNSS alt = 00042 m
    const line = "B2359503351600S15112600WA-001500042EXTRA123";
    const fix = parseIgcBRecord(line) ?? unreachable;
    expect_eq(fix.hour, 23);
    expect_eq(fix.minute, 59);
    expect_eq(fix.second, 50);
    expect_eq(fix.timeSeconds(), 86390);
    expect_eq(approxEq(fix.lat_deg, -33.86), true);
    expect_eq(approxEq(fix.lon_deg, -151.21), true);
    expect_eq(fix.validity, 'A');
    expect_eq(fix.pressure_alt_m, -15);
    expect_eq(fix.gnss_alt_m, 42);
});

test("4. IgcFixIter & summarizeIgc track 3D fixes, bounds, altitudes, and UTC midnight rollover", () => {
    const midnight_flight =
        "ALXN999\n" ++
        "HFDTE211126\n" ++
        "B2358004500000N01000000EA0100001050\n" ++
        "B2359304530000N01030000EA0120001250\n" ++
        "B0001004600000N01100000EA0090000950\n" ++
        "B0002004600000N01100000EV0080000000\n";

    let it = IgcFixIter.init(midnight_flight);
    let count: u32 = 0;
    while (f := it.next()) {
        * = f;
        count += 1;
    }
    expect_eq(count, 4);

    const st = summarizeIgc(midnight_flight);
    expect_eq(st.fix_count, 4);
    expect_eq(st.valid_3d_count, 3);
    // 23:58:00 -> 23:59:30 (+90s) -> 00:01:00 (+90s) -> 00:02:00 (+60s) = 240s
    expect_eq(st.duration_sec, 240);
    expect_eq(st.min_alt_m, 800);
    expect_eq(st.max_alt_m, 1250);
    expect_eq(approxEq(st.min_lat, 45.0), true);
    expect_eq(approxEq(st.max_lat, 46.0), true);
    expect_eq(approxEq(st.min_lon, 10.0), true);
    expect_eq(approxEq(st.max_lon, 11.0), true);
});

test("5. Malformed B records, out-of-range coordinates, and non-IGC inputs fail safely", () => {
    // Non-IGC header
    expect_eq(isIgc(""), false);
    expect_eq(isIgc("B1200004730000N00830000EA0050000540\n"), false);
    expect_eq(isIgc("A12345\n"), false); // < 7 bytes
    expect_eq(parseIgcHeader("NOT IGC DATA") == null, true);

    // Too short B record (< 35 bytes)
    expect_eq(parseIgcBRecord("B1200004730000N00830000EA00500") == null, true);

    // Invalid time (hour 24, minute 60)
    expect_eq(parseIgcBRecord("B2400004730000N00830000EA0050000540") == null, true);
    expect_eq(parseIgcBRecord("B1260004730000N00830000EA0050000540") == null, true);

    // Out-of-range latitude (> 90 deg) and longitude (> 180 deg)
    expect_eq(parseIgcBRecord("B1200009001000N00830000EA0050000540") == null, true);
    expect_eq(parseIgcBRecord("B1200004730000N18001000EA0050000540") == null, true);

    // Invalid hemisphere / validity flag
    expect_eq(parseIgcBRecord("B1200004730000X00830000EA0050000540") == null, true);
    expect_eq(parseIgcBRecord("B1200004730000N00830000EZ0050000540") == null, true);
});

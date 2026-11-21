// Source: cp/cap/test/test_nmea_production_deep.cp
// Deep Production Test Suite for cap:nmea
// Covers:
//   1. Real $GPGGA, $GNRMC, $GPVTG XOR checksum verification, lat/lon/alt/speed/course decoding
//   2. Southern/Western negative coordinate conversion (`S`/`W`) & speedKmh/speedMs helpers
//   3. Empty-field-preserving `NmeaFieldIter` & no-fix GGA sentence handling
//   4. Multi-line serial buffer `NmeaLineIter` filtering noise, blank lines & bad checksums (incl. `!AIVDM`)
//   5. Bad checksum, missing `*HH`, stray delimiters & invalid hemisphere/coordinate range guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    NmeaSentence,
    NmeaGga,
    NmeaRmc,
    NmeaVtg,
    NmeaFieldIter,
    NmeaLineIter,
    nmeaChecksum,
    verifyNmeaSentence,
    parseNmeaSentence,
    parseNmeaCoord,
    parseNmeaGga,
    parseNmeaRmc,
    parseNmeaVtg,
} from "cap:nmea";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.0001;
}

fn testGgaRmcVtgRealSentences() {
    const gga_line = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
    expect_eq(verifyNmeaSentence(gga_line), true);
    expect_eq(nmeaChecksum("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"), 0x47);

    const gga = parseNmeaGga(gga_line).?;
    expect_eq(std.mem.eql(u8, gga.talker, "GP"), true);
    expect_eq(std.mem.eql(u8, gga.utc_time, "123519"), true);
    expect_eq(approxEq(gga.lat, 48.0 + 7.038 / 60.0), true);
    expect_eq(approxEq(gga.lon, 11.0 + 31.0 / 60.0), true);
    expect_eq(gga.has_fix, true);
    expect_eq(gga.fix_quality, 1);
    expect_eq(gga.satellites, 8);
    expect_eq(approxEq(gga.hdop, 0.9), true);
    expect_eq(approxEq(gga.altitude_m, 545.4), true);

    const rmc_line = "$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*74\n";
    const rmc = parseNmeaRmc(rmc_line).?;
    expect_eq(std.mem.eql(u8, rmc.talker, "GN"), true);
    expect_eq(std.mem.eql(u8, rmc.utc_time, "123519"), true);
    expect_eq(rmc.is_active, true);
    expect_eq(approxEq(rmc.lat, 48.0 + 7.038 / 60.0), true);
    expect_eq(approxEq(rmc.lon, 11.0 + 31.0 / 60.0), true);
    expect_eq(approxEq(rmc.speed_knots, 22.4), true);
    expect_eq(approxEq(rmc.course_deg, 84.4), true);
    expect_eq(std.mem.eql(u8, rmc.date_ddmmyy, "230394"), true);
    expect_eq(approxEq(rmc.speedKmh(), 22.4 * 1.852), true);
    expect_eq(approxEq(rmc.speedMs(), 22.4 * 0.5144444), true);

    const vtg_line = "$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48";
    const vtg = parseNmeaVtg(vtg_line).?;
    expect_eq(std.mem.eql(u8, vtg.talker, "GP"), true);
    expect_eq(approxEq(vtg.true_track_deg, 54.7), true);
    expect_eq(approxEq(vtg.mag_track_deg, 34.4), true);
    expect_eq(approxEq(vtg.speed_knots, 5.5), true);
    expect_eq(approxEq(vtg.speed_kmh, 10.2), true);
}

fn testSouthWestNegativeCoordinates() {
    // BeiDou RMC in Southern / Western hemisphere
    const bd_rmc = "$BDRMC,235959.50,A,3352.1280,S,15112.5580,W,12.50,270.25,211126,,,D*70";
    expect_eq(verifyNmeaSentence(bd_rmc), true);

    const rmc = parseNmeaRmc(bd_rmc).?;
    expect_eq(std.mem.eql(u8, rmc.talker, "BD"), true);
    expect_eq(rmc.is_active, true);
    expect_eq(approxEq(rmc.lat, -(33.0 + 52.1280 / 60.0)), true);
    expect_eq(approxEq(rmc.lon, -(151.0 + 12.5580 / 60.0)), true);
    expect_eq(approxEq(rmc.speed_knots, 12.50), true);
    expect_eq(approxEq(rmc.course_deg, 270.25), true);
    expect_eq(std.mem.eql(u8, rmc.date_ddmmyy, "211126"), true);

    // Direct parseNmeaCoord boundary checks
    expect_eq(approxEq(parseNmeaCoord("9000.0000", "S").?, -90.0), true);
    expect_eq(approxEq(parseNmeaCoord("18000.0000", "W").?, -180.0), true);
    expect_eq(approxEq(parseNmeaCoord("0000.0000", "S").?, 0.0), true);
}

fn testEmptyFieldsAndNoFixGga() {
    const no_fix = "$GNGGA,000000.00,,,,,0,00,99.99,,,,,,*78";
    const sent = parseNmeaSentence(no_fix).?;
    expect_eq(std.mem.eql(u8, sent.talker, "GN"), true);
    expect_eq(std.mem.eql(u8, sent.sentence_type, "GGA"), true);

    // Verify all 14 fields including consecutive empty fields and trailing empty field
    let fit = sent.fields();
    let count: usize = 0;
    let empty_count: usize = 0;
    while (f := fit.next()) {
        if (f.len == 0) {
            empty_count += 1;
        }
        count += 1;
    }
    expect_eq(count, 14);
    expect_eq(empty_count, 10);

    const gga = parseNmeaGga(no_fix).?;
    expect_eq(gga.has_fix, false);
    expect_eq(gga.fix_quality, 0);
    expect_eq(gga.satellites, 0);
    expect_eq(approxEq(gga.hdop, 99.99), true);
    expect_eq(approxEq(gga.lat, 0.0), true);
    expect_eq(approxEq(gga.lon, 0.0), true);
}

fn testMultiLineSerialStreamFiltering() {
    const stream =
        "BOOT: GNSS module v2.1 ready\r\n" ++
        "\r\n" ++
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n" ++
        "$GPGGA,CORRUPTED,BAD,CHECKSUM*FF\r\n" ++
        "partial garbage without newline or star\n" ++
        "$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*74\n" ++
        "!AIVDM,1,1,,B,177KQJ5000G?tO`K>RA1wUbN0TKH,0*5C\r\n";

    let it = NmeaLineIter.init(stream);

    const s0 = it.next().?;
    expect_eq(s0.start_char, '$');
    expect_eq(std.mem.eql(u8, s0.talker, "GP"), true);
    expect_eq(std.mem.eql(u8, s0.sentence_type, "GGA"), true);

    const s1 = it.next().?;
    expect_eq(s1.start_char, '$');
    expect_eq(std.mem.eql(u8, s1.talker, "GN"), true);
    expect_eq(std.mem.eql(u8, s1.sentence_type, "RMC"), true);

    const s2 = it.next().?;
    expect_eq(s2.start_char, '!');
    expect_eq(std.mem.eql(u8, s2.talker, "AI"), true);
    expect_eq(std.mem.eql(u8, s2.sentence_type, "VDM"), true);
    expect_eq(s2.checksum, 0x5C);

    expect_eq(it.next() == null, true);
}

fn testChecksumAndCoordinateGuards() {
    // Bad checksum
    expect_eq(verifyNmeaSentence("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48"), false);
    // Missing *HH
    expect_eq(verifyNmeaSentence("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"), false);
    // Non-hex checksum
    expect_eq(verifyNmeaSentence("$GPGGA,123*ZZ"), false);
    // Wrong start char
    expect_eq(verifyNmeaSentence("#GPGGA,123*47"), false);
    // Empty / too short
    expect_eq(verifyNmeaSentence(""), false);
    expect_eq(verifyNmeaSentence("$*00"), false);

    // Sentence type mismatch
    const gga_line = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
    expect_eq(parseNmeaRmc(gga_line) == null, true);
    expect_eq(parseNmeaVtg(gga_line) == null, true);

    // Invalid hemisphere & out-of-range coordinates
    expect_eq(parseNmeaCoord("4807.038", "X") == null, true);
    expect_eq(parseNmeaCoord("4807.038", "") == null, true);
    expect_eq(parseNmeaCoord("4807.038", "NN") == null, true);
    expect_eq(parseNmeaCoord("9100.0000", "N") == null, true);   // lat > 90
    expect_eq(parseNmeaCoord("18100.0000", "E") == null, true);  // lon > 180
    expect_eq(parseNmeaCoord("4860.0000", "N") == null, true);   // minutes >= 60
    expect_eq(parseNmeaCoord("-4807.038", "N") == null, true);   // negative sign invalid in NMEA
}

test("1. Real $GPGGA, $GNRMC & $GPVTG Checksum & Structured Decoding", () => {
    testGgaRmcVtgRealSentences();
});

test("2. Southern/Western Negative Coordinate Conversion & Speed Units", () => {
    testSouthWestNegativeCoordinates();
});

test("3. Empty-Field-Preserving NmeaFieldIter & No-Fix GGA Decoding", () => {
    testEmptyFieldsAndNoFixGga();
});

test("4. Multi-Line Serial Stream NmeaLineIter Noise & Bad-Checksum Filtering", () => {
    testMultiLineSerialStreamFiltering();
});

test("5. Checksum Error, Missing *HH & Invalid Hemisphere/Range Guards", () => {
    testChecksumAndCoordinateGuards();
});

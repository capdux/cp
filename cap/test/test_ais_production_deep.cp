// Source: cp/cap/test/test_ais_production_deep.cp (Production deep tests for cap:ais)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AIS_LON_UNAVAILABLE,
    AIS_LAT_UNAVAILABLE,
    AisSentence,
    AisPositionReport,
    AisClassBPosition,
    AisBitReader,
    aisUnarmorChar,
    aisMessageType,
    parseAisSentence,
    parseAisPositionReport,
    parseAisClassBPosition,
} from "cap:ais";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn armorSixbit(v: u8): u8 {
    const s = v & 0x3F;
    if (s < 40) return s + 48;
    return s + 56;
}

fn pushBits(bits_buf: []u8, bit_len: *usize, val: u32, count: u8) {
    let i: u8 = count;
    while (i > 0) {
        i -= 1;
        const shift: u5 = i as u5;
        const b: u8 = ((val >> shift) & 1) as u8;
        const p = bit_len.*;
        bits_buf[p] = b;
        bit_len.* = p + 1;
    }
}

fn packToArmor(bits_buf: []const u8, total_bits: usize, out: []u8): usize {
    const nchars = total_bits / 6;
    let c: usize = 0;
    while (c < nchars) {
        let v: u8 = 0;
        let k: usize = 0;
        while (k < 6) {
            v = (v << 1) | (bits_buf[c * 6 + k] & 1);
            k += 1;
        }
        out[c] = armorSixbit(v);
        c += 1;
    }
    return nchars;
}

test("1. Standard !AIVDM Class A Position Report vector full decoding", () => {
    // Classic ITU-R M.1371 / GPSD Type 1 vector (Seattle, WA)
    const line = "!AIVDM,1,1,,B,177KQJ5000G?tO`K>RA1wUbN0TKH,0*5C\r\n";
    const sent = parseAisSentence(line) ?? unreachable;
    expect_eq(sent.is_own_vessel, false);
    expect_eq(sent.frag_count, 1);
    expect_eq(sent.frag_num, 1);
    expect_eq(sent.seq_id.len, 0);
    expect_eq(sent.channel, 'B');
    expect_eq(sent.fill_bits, 0);
    expect_eq(aisMessageType(sent.payload) ?? 0, 1);

    const pos = parseAisPositionReport(sent.payload, sent.fill_bits) ?? unreachable;
    expect_eq(pos.msg_type, 1);
    expect_eq(pos.repeat, 0);
    expect_eq(pos.mmsi, 477553000);
    expect_eq(pos.nav_status, 5);
    expect_eq(pos.rot, 0);
    expect_eq(pos.sog_knots >= -0.01 && pos.sog_knots <= 0.01, true);
    expect_eq(pos.pos_accuracy, false);
    expect_eq(pos.lon_available, true);
    expect_eq(pos.lat_available, true);
    // lon = -122.345833..., lat = 47.582833...
    expect_eq(pos.lon_deg < -122.34 && pos.lon_deg > -122.35, true);
    expect_eq(pos.lat_deg > 47.58 && pos.lat_deg < 47.59, true);
    expect_eq(pos.cog_deg > 50.9 && pos.cog_deg < 51.1, true);
    expect_eq(pos.true_heading, 181);
    expect_eq(pos.timestamp_sec, 15);
});

test("2. Negative lon/lat 2's complement sign extension, ROT=-128, and unavailable sentinels", () => {
    // Build a 168-bit Type 3 report with:
    // rot = -128 (0x80), lon = -150.25 deg (-90150000), lat = -33.8688 deg (-20321280)
    let bits: [168]u8 = std.mem.zeroes([168]u8);
    let blen: usize = 0;
    pushBits(bits[0..], *blen, 3, 6);           // msg_type = 3
    pushBits(bits[0..], *blen, 1, 2);           // repeat = 1
    pushBits(bits[0..], *blen, 503123456, 30);  // mmsi
    pushBits(bits[0..], *blen, 0, 4);           // nav_status = under way
    pushBits(bits[0..], *blen, 0x80, 8);        // rot = -128 (not available)
    pushBits(bits[0..], *blen, 145, 10);        // sog = 14.5 knots
    pushBits(bits[0..], *blen, 1, 1);           // pos_accuracy = true
    // 28-bit 2's complement of -90150000: (1<<28) - 90150000 = 178285456
    pushBits(bits[0..], *blen, 178285456, 28);  // lon = -150.25 deg
    // 27-bit 2's complement of -20321280: (1<<27) - 20321280 = 113896448
    pushBits(bits[0..], *blen, 113896448, 27);  // lat = -33.8688 deg
    pushBits(bits[0..], *blen, 2753, 12);       // cog = 275.3 deg
    pushBits(bits[0..], *blen, 275, 9);         // heading = 275
    pushBits(bits[0..], *blen, 42, 6);          // timestamp = 42
    pushBits(bits[0..], *blen, 0, 25);          // spare + radio

    let payload: [28]u8 = std.mem.zeroes([28]u8);
    * = packToArmor(bits[0..], 168, payload[0..]);

    const pos = parseAisPositionReport(payload[0..], 0) ?? unreachable;
    expect_eq(pos.msg_type, 3);
    expect_eq(pos.mmsi, 503123456);
    expect_eq(pos.rot, -128);
    expect_eq(pos.sog_knots > 14.49 && pos.sog_knots < 14.51, true);
    expect_eq(pos.pos_accuracy, true);
    expect_eq(pos.lon_deg < -150.24 && pos.lon_deg > -150.26, true);
    expect_eq(pos.lat_deg < -33.86 && pos.lat_deg > -33.87, true);

    // Unavailable sentinels (181*600000 and 91*600000)
    blen = 0;
    pushBits(bits[0..], *blen, 1, 6);
    pushBits(bits[0..], *blen, 0, 2);
    pushBits(bits[0..], *blen, 123456789, 30);
    pushBits(bits[0..], *blen, 15, 4);
    pushBits(bits[0..], *blen, 0, 8);
    pushBits(bits[0..], *blen, 1023, 10);
    pushBits(bits[0..], *blen, 0, 1);
    pushBits(bits[0..], *blen, AIS_LON_UNAVAILABLE as u32, 28);
    pushBits(bits[0..], *blen, AIS_LAT_UNAVAILABLE as u32, 27);
    pushBits(bits[0..], *blen, 3600, 12);
    pushBits(bits[0..], *blen, 511, 9);
    pushBits(bits[0..], *blen, 60, 6);
    pushBits(bits[0..], *blen, 0, 25);
    * = packToArmor(bits[0..], 168, payload[0..]);

    const unavail = parseAisPositionReport(payload[0..], 0) ?? unreachable;
    expect_eq(unavail.lon_available, false);
    expect_eq(unavail.lat_available, false);
});

test("3. Type 18 Class B Position Report decoding and !AIVDO own-vessel flag", () => {
    let bits: [168]u8 = std.mem.zeroes([168]u8);
    let blen: usize = 0;
    pushBits(bits[0..], *blen, 18, 6);          // msg_type = 18
    pushBits(bits[0..], *blen, 0, 2);           // repeat
    pushBits(bits[0..], *blen, 338087471, 30);  // mmsi
    pushBits(bits[0..], *blen, 0, 8);           // regional reserved
    pushBits(bits[0..], *blen, 125, 10);        // sog = 12.5 knots
    pushBits(bits[0..], *blen, 1, 1);           // pos_accuracy = true
    // lon = -70.5 deg -> -42300000 -> (1<<28) - 42300000 = 226135456
    pushBits(bits[0..], *blen, 226135456, 28);
    // lat = +41.25 deg -> +24750000
    pushBits(bits[0..], *blen, 24750000, 27);
    pushBits(bits[0..], *blen, 2155, 12);       // cog = 215.5 deg
    pushBits(bits[0..], *blen, 214, 9);         // true_heading = 214
    pushBits(bits[0..], *blen, 32, 6);          // timestamp_sec = 32
    pushBits(bits[0..], *blen, 0, 29);          // remaining flags/radio

    let payload: [28]u8 = std.mem.zeroes([28]u8);
    * = packToArmor(bits[0..], 168, payload[0..]);

    expect_eq(aisMessageType(payload[0..]) ?? 0, 18);
    const cb = parseAisClassBPosition(payload[0..], 0) ?? unreachable;
    expect_eq(cb.msg_type, 18);
    expect_eq(cb.mmsi, 338087471);
    expect_eq(cb.sog_knots > 12.49 && cb.sog_knots < 12.51, true);
    expect_eq(cb.pos_accuracy, true);
    expect_eq(cb.lon_deg < -70.49 && cb.lon_deg > -70.51, true);
    expect_eq(cb.lat_deg > 41.24 && cb.lat_deg < 41.26, true);
    expect_eq(cb.cog_deg > 215.4 && cb.cog_deg < 215.6, true);
    expect_eq(cb.true_heading, 214);
    expect_eq(cb.timestamp_sec, 32);

    // !AIVDO own-vessel sentence verification (XOR of "AIVDO,1,1,,A,177KQJ5000G?tO`K>RA1wUbN0TKH,0" = 0x5D)
    // Wait: 'M'(0x4D)^'O'(0x4F)=0x02, 'B'(0x42)^'A'(0x41)=0x03 -> 0x5C ^ 0x02 ^ 0x03 = 0x5D!
    const aivdo = "!AIVDO,1,1,,A,177KQJ5000G?tO`K>RA1wUbN0TKH,0*5D";
    const own = parseAisSentence(aivdo) ?? unreachable;
    expect_eq(own.is_own_vessel, true);
    expect_eq(own.channel, 'A');
});

test("4. 6-bit AIS vessel name string decoding with trailing '@' and space trimming", () => {
    // Encode "EVER GIVEN 1  @@@@@@" (20 chars) into 20 6-bit armored characters:
    // For ASCII 'A'..'Z' (65..90) -> 6-bit = ch - 64; '@' (64) -> 0; ' '..'?' (32..63) -> ch
    const raw_name = "EVER GIVEN 1  @@@@@@";
    let armored: [20]u8 = std.mem.zeroes([20]u8);
    let i: usize = 0;
    while (i < 20) {
        const ch = raw_name[i];
        const six: u8 = if (ch >= 64) ch - 64 else ch;
        armored[i] = armorSixbit(six);
        i += 1;
    }

    let br = AisBitReader.init(armored[0..], 0) ?? unreachable;
    let out: [20]u8 = std.mem.zeroes([20]u8);
    const n = br.readSixbitString(20, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..n], "EVER GIVEN 1"), true);
});

test("5. Fault tolerance: bad checksum, illegal armor chars, and truncated bitstreams", () => {
    // Bad checksum (*5D instead of *5C)
    expect_eq(parseAisSentence("!AIVDM,1,1,,B,177KQJ5000G?tO`K>RA1wUbN0TKH,0*5D") == null, true);

    // Non-AIS talker
    expect_eq(parseAisSentence("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47") == null, true);

    // Illegal armored characters: 47 ('/'), 88 ('X'), 95 ('_'), 120 ('x')
    expect_eq(aisUnarmorChar(47) == null, true);
    expect_eq(aisUnarmorChar(48) ?? 255, 0);
    expect_eq(aisUnarmorChar(87) ?? 255, 39);
    expect_eq(aisUnarmorChar(88) == null, true);
    expect_eq(aisUnarmorChar(95) == null, true);
    expect_eq(aisUnarmorChar(96) ?? 255, 40);
    expect_eq(aisUnarmorChar(119) ?? 255, 63);
    expect_eq(aisUnarmorChar(120) == null, true);

    // Truncated payload (< 168 bits)
    expect_eq(parseAisPositionReport("177KQJ5000", 0) == null, true);
    expect_eq(parseAisClassBPosition("B5NJ;PP005", 0) == null, true);

    // Invalid fill_bits > 5
    expect_eq(AisBitReader.init("177KQJ5", 6) == null, true);

    // Type mismatch: Type 1 payload passed to Class B Type 18 parser -> null
    expect_eq(parseAisClassBPosition("177KQJ5000G?tO`K>RA1wUbN0TKH", 0) == null, true);
});

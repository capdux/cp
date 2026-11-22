// Source: cp/cap/test/test_ntp_production_deep.cp (Production deep tests for cap:ntp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NTP_UDP_PORT,
    NTP_PACKET_SIZE,
    NTP_V3,
    NTP_V4,
    NTP_UNIX_EPOCH_DELTA,
    LI_NO_WARNING,
    LI_61_SEC,
    LI_59_SEC,
    LI_ALARM,
    MODE_RESERVED,
    MODE_SYMMETRIC_ACTIVE,
    MODE_SYMMETRIC_PASSIVE,
    MODE_CLIENT,
    MODE_SERVER,
    MODE_BROADCAST,
    MODE_CONTROL,
    MODE_PRIVATE,
    NtpTimestamp,
    NtpPacket,
    isNtp,
    parseNtp,
    encodeNtp,
    makeClientRequest,
    calcOffsetNs,
    calcDelayNs,
} from "cap:net";

test("1. 48B NTPv4 client request and server response encode/decode roundtrip", () => {
    expect_eq(NTP_UDP_PORT, 123 as u16);
    expect_eq(NTP_PACKET_SIZE, 48);
    expect_eq(NTP_UNIX_EPOCH_DELTA, 2_208_988_800 as u64);

    // Client request
    const tx = NtpTimestamp.fromUnixMs(1_700_000_000_250);
    const req = makeClientRequest(tx);
    let buf: [48]u8 = std.mem.zeroes([48]u8);
    const n = encodeNtp(req, buf[0..]) ?? unreachable;
    expect_eq(n, 48);
    expect_eq(isNtp(buf[0..]), true);

    const dec_req = parseNtp(buf[0..]) ?? unreachable;
    expect_eq(dec_req.li, LI_NO_WARNING);
    expect_eq(dec_req.version, NTP_V4);
    expect_eq(dec_req.mode, MODE_CLIENT);
    expect_eq(dec_req.poll, 6 as i8);
    expect_eq(dec_req.precision, -20 as i8);
    expect_eq(dec_req.tx_ts.toUnixMs(), 1_700_000_000_250 as i64);
    expect_eq(dec_req.ref_ts.isZero(), true);

    // Server response with 16.16 fixed-point root_delay (0.5s = 32768) & root_dispersion (0.25s = 16384)
    const srv = NtpPacket{
        .li = LI_61_SEC,
        .version = NTP_V4,
        .mode = MODE_SERVER,
        .stratum = 2,
        .poll = 6,
        .precision = -24,
        .root_delay = 32768,
        .root_dispersion = 16384,
        .ref_id = [192, 168, 1, 1],
        .ref_ts = NtpTimestamp.fromUnixMs(1_700_000_000_000),
        .orig_ts = tx,
        .rx_ts = NtpTimestamp.fromUnixMs(1_700_000_000_300),
        .tx_ts = NtpTimestamp.fromUnixMs(1_700_000_000_310),
    };
    let sbuf: [48]u8 = std.mem.zeroes([48]u8);
    expect_eq(encodeNtp(srv, sbuf[0..]) ?? 0, 48);
    const dec_srv = parseNtp(sbuf[0..]) ?? unreachable;
    expect_eq(dec_srv.li, LI_61_SEC);
    expect_eq(dec_srv.mode, MODE_SERVER);
    expect_eq(dec_srv.stratum, 2 as u8);
    expect_eq(dec_srv.precision, -24 as i8);
    expect_eq(dec_srv.isKissOfDeath(), false);
    expect_eq(dec_srv.isPrimary(), false);
    expect_eq(dec_srv.rootDelayMs() > 499.9 && dec_srv.rootDelayMs() < 500.1, true);
    expect_eq(dec_srv.rootDispersionMs() > 249.9 && dec_srv.rootDispersionMs() < 250.1, true);
    expect_eq(dec_srv.ref_id[0], 192 as u8);
    expect_eq(dec_srv.ref_id[3], 1 as u8);
});

test("2. Bidirectional Unix <-> NTP epoch timestamp conversion precision", () => {
    // Unix epoch 0 -> NTP seconds = 2208988800, fraction = 0
    const ts0 = NtpTimestamp.fromUnixMs(0);
    expect_eq(ts0.seconds, 2_208_988_800 as u32);
    expect_eq(ts0.fraction, 0 as u32);
    expect_eq(ts0.toUnixMs(), 0 as i64);
    expect_eq(ts0.toUnixNs(), 0 as i128);

    // Half second (500ms) -> fraction = 0x80000000
    const ts_half = NtpTimestamp.fromUnixMs(1_700_000_000_500);
    expect_eq(ts_half.fraction, 0x8000_0000 as u32);
    expect_eq(ts_half.toUnixMs(), 1_700_000_000_500 as i64);
    expect_eq(ts_half.toUnixNs(), 1_700_000_000_500_000_000 as i128);

    // Millisecond roundtrip across fractional values
    const samples: [4]i64 = [
        1_600_000_000_001,
        1_600_000_000_125,
        1_600_000_000_750,
        1_600_000_000_999,
    ];
    let i: usize = 0;
    while (i < 4) {
        const t = NtpTimestamp.fromUnixMs(samples[i]);
        expect_eq(t.toUnixMs(), samples[i]);
        i += 1;
    }
});

test("3. Kiss-o'-Death (Stratum 0) and Primary Reference (Stratum 1) RefID parsing", () => {
    // Kiss-o'-Death RATE
    const kod = NtpPacket{
        .li = LI_ALARM,
        .version = NTP_V4,
        .mode = MODE_SERVER,
        .stratum = 0,
        .poll = 4,
        .precision = 0,
        .root_delay = 0,
        .root_dispersion = 0,
        .ref_id = ['R', 'A', 'T', 'E'],
        .ref_ts = NtpTimestamp.zero(),
        .orig_ts = NtpTimestamp.zero(),
        .rx_ts = NtpTimestamp.zero(),
        .tx_ts = NtpTimestamp.zero(),
    };
    let buf: [48]u8 = std.mem.zeroes([48]u8);
    * = encodeNtp(kod, buf[0..]) ?? unreachable;
    const p_kod = parseNtp(buf[0..]) ?? unreachable;
    expect_eq(p_kod.isKissOfDeath(), true);
    expect_eq(p_kod.isPrimary(), false);
    expect_eq(std.mem.eql(u8, p_kod.ref_id[0..], "RATE"), true);

    // Stratum 1 GPS primary server
    const gps = NtpPacket{
        .li = LI_NO_WARNING,
        .version = NTP_V3,
        .mode = MODE_SERVER,
        .stratum = 1,
        .poll = 4,
        .precision = -20,
        .root_delay = 0,
        .root_dispersion = 0,
        .ref_id = ['G', 'P', 'S', 0],
        .ref_ts = NtpTimestamp.zero(),
        .orig_ts = NtpTimestamp.zero(),
        .rx_ts = NtpTimestamp.zero(),
        .tx_ts = NtpTimestamp.zero(),
    };
    * = encodeNtp(gps, buf[0..]) ?? unreachable;
    const p_gps = parseNtp(buf[0..]) ?? unreachable;
    expect_eq(p_gps.version, NTP_V3);
    expect_eq(p_gps.isPrimary(), true);
    expect_eq(p_gps.isKissOfDeath(), false);
    expect_eq(p_gps.ref_id[0], 'G' as u8);
    expect_eq(p_gps.ref_id[1], 'P' as u8);
    expect_eq(p_gps.ref_id[2], 'S' as u8);
});

test("4. RFC 5905 clock offset and round-trip delay calculation", () => {
    // Suppose client clock is 100ms behind server, network delay is 20ms each way, server processing is 10ms:
    // T1 (client send) = 1000ms
    // T2 (server recv) = 1000 + 100 (offset) + 20 (fwd) = 1120ms
    // T3 (server send) = 1120 + 10 (proc) = 1130ms
    // T4 (client recv) = 1000 + 20 (fwd) + 10 (proc) + 20 (rev) = 1050ms
    // Expected offset = ((1120 - 1000) + (1130 - 1050)) / 2 = (120 + 80) / 2 = +100ms
    // Expected delay  = (1050 - 1000) - (1130 - 1120) = 50 - 10 = 40ms
    const base: i64 = 1_700_000_000_000;
    const t1 = NtpTimestamp.fromUnixMs(base + 1000);
    const t2 = NtpTimestamp.fromUnixMs(base + 1120);
    const t3 = NtpTimestamp.fromUnixMs(base + 1130);
    const t4 = NtpTimestamp.fromUnixMs(base + 1050);

    const offset_ns = calcOffsetNs(t1, t2, t3, t4);
    const delay_ns = calcDelayNs(t1, t2, t3, t4);

    // Allow sub-microsecond quantization tolerance from 32-bit binary fraction
    const expected_offset: i128 = 100_000_000;
    const expected_delay: i128 = 40_000_000;
    expect_eq(offset_ns > expected_offset - 1_000 && offset_ns < expected_offset + 1_000, true);
    expect_eq(delay_ns > expected_delay - 1_000 && delay_ns < expected_delay + 1_000, true);

    // Negative offset case: client clock is 50ms ahead of server
    const nt1 = NtpTimestamp.fromUnixMs(base + 2000);
    const nt2 = NtpTimestamp.fromUnixMs(base + 1960); // -50 + 10
    const nt3 = NtpTimestamp.fromUnixMs(base + 1965); // +5 proc
    const nt4 = NtpTimestamp.fromUnixMs(base + 2025); // +10 + 5 + 10
    const neg_offset = calcOffsetNs(nt1, nt2, nt3, nt4);
    const expected_neg: i128 = -50_000_000;
    expect_eq(neg_offset > expected_neg - 1_000 && neg_offset < expected_neg + 1_000, true);
});

test("5. Truncated packet, invalid version (0 or 5), mode=0, and short encode buffer rejection", () => {
    // Truncated (< 48B)
    const short47: [47]u8 = [0; 47];
    expect_eq(isNtp(short47[0..]), false);
    expect_eq(parseNtp(short47[0..]) == null, true);

    // Version 0 (invalid)
    let bad_v0: [48]u8 = std.mem.zeroes([48]u8);
    bad_v0[0] = ((0 << 3) as u8) | MODE_CLIENT;
    expect_eq(isNtp(bad_v0[0..]), false);
    expect_eq(parseNtp(bad_v0[0..]) == null, true);

    // Version 5 (invalid)
    let bad_v5: [48]u8 = std.mem.zeroes([48]u8);
    bad_v5[0] = ((5 << 3) as u8) | MODE_CLIENT;
    expect_eq(isNtp(bad_v5[0..]), false);
    expect_eq(parseNtp(bad_v5[0..]) == null, true);

    // Mode 0 (reserved / invalid)
    let bad_m0: [48]u8 = std.mem.zeroes([48]u8);
    bad_m0[0] = (NTP_V4 << 3) | MODE_RESERVED;
    expect_eq(isNtp(bad_m0[0..]), false);
    expect_eq(parseNtp(bad_m0[0..]) == null, true);

    // Encode into short buffer (< 48B)
    let tiny: [47]u8 = std.mem.zeroes([47]u8);
    const req = makeClientRequest(NtpTimestamp.zero());
    expect_eq(encodeNtp(req, tiny[0..]) == null, true);
});

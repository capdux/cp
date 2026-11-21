// Source: cp/cap/test/test_mseed_production_deep.cp
// Deep Production Test Suite for cap:mseed
// Covers:
//   1. Big-Endian 512B MiniSEED record auto-detection + Blockette 1000 (steim2) & Blockette 1001 chain
//   2. Little-Endian 4096B MiniSEED record auto-detection + Blockette 1000 (int32)
//   3. sampleRateHz() exact SEED v2.4 calculation across all 4 positive/negative factor/mult sign combos
//   4. Multi-record concatenated stream via MseedIter & summarizeMseed (mixed 512B/4096B/256B records)
//   5. Invalid quality code, out-of-bounds data_offset, invalid BTIME year & truncated buffer guards

import { test, expect_eq } from "cap:test";
import {
    MSEED_FIXED_HEADER_SIZE,
    MSEED_MIN_RECORD_SIZE,
    MSEED_BLK_1000,
    MSEED_BLK_1001,
    MseedEncoding,
    mseedEncodingFromU8,
    MseedBtime,
    MseedRecord,
    MseedStats,
    isMseedRecord,
    parseMseedRecord,
    MseedIter,
    summarizeMseed,
} from "cap:mseed";

fn putU16(buf: []u8, off: usize, v: u16, is_be: bool) {
    if (is_be) {
        buf[off] = ((v >> 8) & 0xFF) as u8;
        buf[off + 1] = (v & 0xFF) as u8;
    } else {
        buf[off] = (v & 0xFF) as u8;
        buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    }
}

fn putI16(buf: []u8, off: usize, v: i16, is_be: bool) {
    const bits: u16 = @bitCast(v);
    putU16(buf, off, bits, is_be);
}

fn putU32(buf: []u8, off: usize, v: u32, is_be: bool) {
    if (is_be) {
        buf[off] = ((v >> 24) & 0xFF) as u8;
        buf[off + 1] = ((v >> 16) & 0xFF) as u8;
        buf[off + 2] = ((v >> 8) & 0xFF) as u8;
        buf[off + 3] = (v & 0xFF) as u8;
    } else {
        buf[off] = (v & 0xFF) as u8;
        buf[off + 1] = ((v >> 8) & 0xFF) as u8;
        buf[off + 2] = ((v >> 16) & 0xFF) as u8;
        buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    }
}

fn copyFixedPadded(buf: []u8, off: usize, width: usize, s: []const u8) {
    let i: usize = 0;
    while (i < width) {
        if (i < s.len) {
            buf[off + i] = s[i];
        } else {
            buf[off + i] = ' ';
        }
        i += 1;
    }
}

fn buildMseedRecord(
    buf: []u8,
    base: usize,
    seq: []const u8,
    qual: u8,
    sta: []const u8,
    loc: []const u8,
    cha: []const u8,
    net: []const u8,
    year: u16,
    doy: u16,
    num_samples: u16,
    sr_factor: i16,
    sr_mult: i16,
    is_be: bool,
    enc_code: u8,
    rec_exp: u8,
    include_blk1001: bool,
    timing_q: u8,
    usec: i8,
    frames: u8,
) {
    const rec_len = (1 as usize) << (rec_exp as u6);
    let i: usize = 0;
    while (i < rec_len) {
        buf[base + i] = 0;
        i += 1;
    }

    copyFixedPadded(buf, base + 0, 6, seq);
    buf[base + 6] = qual;
    buf[base + 7] = ' ';
    copyFixedPadded(buf, base + 8, 5, sta);
    copyFixedPadded(buf, base + 13, 2, loc);
    copyFixedPadded(buf, base + 15, 3, cha);
    copyFixedPadded(buf, base + 18, 2, net);

    // BTIME (offset 20..30)
    putU16(buf, base + 20, year, is_be);
    putU16(buf, base + 22, doy, is_be);
    buf[base + 24] = 12; // hour
    buf[base + 25] = 34; // minute
    buf[base + 26] = 56; // second
    buf[base + 27] = 0;
    putU16(buf, base + 28, 7890, is_be); // tenths_ms

    putU16(buf, base + 30, num_samples, is_be);
    putI16(buf, base + 32, sr_factor, is_be);
    putI16(buf, base + 34, sr_mult, is_be);
    buf[base + 36] = 0;
    buf[base + 37] = 0;
    buf[base + 38] = 0;
    buf[base + 39] = if (include_blk1001) 2 else 1;
    putU32(buf, base + 40, 0, is_be);
    putU16(buf, base + 44, 64, is_be); // data_offset = 64
    putU16(buf, base + 46, 48, is_be); // first_blockette_offset = 48

    // Blockette 1000 at offset 48 (8 bytes)
    putU16(buf, base + 48, MSEED_BLK_1000, is_be);
    putU16(buf, base + 50, if (include_blk1001) 56 else 0, is_be);
    buf[base + 52] = enc_code;
    buf[base + 53] = if (is_be) 1 else 0;
    buf[base + 54] = rec_exp;
    buf[base + 55] = 0;

    if (include_blk1001) {
        // Blockette 1001 at offset 56 (8 bytes)
        putU16(buf, base + 56, MSEED_BLK_1001, is_be);
        putU16(buf, base + 58, 0, is_be);
        buf[base + 60] = timing_q;
        buf[base + 61] = @bitCast(usec);
        buf[base + 62] = 0;
        buf[base + 63] = frames;
    }

    // Sentinel payload bytes
    buf[base + 64] = 0xDE;
    buf[base + 65] = 0xAD;
}

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

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) a - b else b - a;
    return diff < 0.000000001;
}

test("mseed: Big-Endian 512B record auto-detection with Blockette 1000 (steim2) + 1001") {
    let buf: [512]u8 = [0; 512];
    buildMseedRecord(
        buf[0..512],
        0,
        "000001",
        'D',
        "ANMO",
        "00",
        "BHZ",
        "IU",
        2026,
        142,
        420,
        40,
        1,
        true,
        11, // steim2
        9,  // 2^9 = 512
        true,
        98,
        -25,
        7,
    );

    const data = buf[0..512];
    expect_eq(isMseedRecord(data), true);
    expect_eq(MSEED_FIXED_HEADER_SIZE, 48);
    expect_eq(MSEED_MIN_RECORD_SIZE, 256);

    if (rec := parseMseedRecord(data, 0)) {
        expect_eq(rec.is_big_endian, true);
        expect_eq(strEq(rec.seq_num, "000001"), true);
        expect_eq(rec.quality_indicator, 'D');
        expect_eq(strEq(rec.station, "ANMO"), true);
        expect_eq(strEq(rec.location, "00"), true);
        expect_eq(strEq(rec.channel, "BHZ"), true);
        expect_eq(strEq(rec.network, "IU"), true);
        expect_eq(rec.start_time.year, 2026);
        expect_eq(rec.start_time.day_of_year, 142);
        expect_eq(rec.start_time.hour, 12);
        expect_eq(rec.start_time.minute, 34);
        expect_eq(rec.start_time.second, 56);
        expect_eq(rec.start_time.tenths_ms, 7890);
        expect_eq(rec.num_samples, 420);
        expect_eq(approxEq(rec.sampleRateHz(), 40.0), true);
        expect_eq(rec.has_blk_1000, true);
        expect_eq(rec.encoding == MseedEncoding.steim2, true);
        expect_eq(rec.word_order, 1);
        expect_eq(rec.data_rec_len_exp, 9);
        expect_eq(rec.record_bytes, 512);
        expect_eq(rec.has_blk_1001, true);
        expect_eq(rec.timing_quality, 98);
        expect_eq(rec.microsecond, -25);
        expect_eq(rec.frame_count, 7);
        expect_eq(rec.payload.len, 512 - 64);
        expect_eq(rec.payload[0], 0xDE);
        expect_eq(rec.payload[1], 0xAD);
    } else {
        expect_eq(true, false);
    }
}

test("mseed: Little-Endian 4096B record auto-detection with Blockette 1000 (int32)") {
    let buf: [4096]u8 = [0; 4096];
    buildMseedRecord(
        buf[0..4096],
        0,
        "000042",
        'Q',
        "KIP",
        "",
        "HHN",
        "II",
        2024,
        366,
        1000,
        100,
        1,
        false,
        3,  // int32
        12, // 2^12 = 4096
        false,
        0,
        0,
        0,
    );

    const data = buf[0..4096];
    if (rec := parseMseedRecord(data, 0)) {
        expect_eq(rec.is_big_endian, false);
        expect_eq(rec.quality_indicator, 'Q');
        expect_eq(strEq(rec.station, "KIP"), true);
        expect_eq(strEq(rec.location, ""), true);
        expect_eq(strEq(rec.channel, "HHN"), true);
        expect_eq(strEq(rec.network, "II"), true);
        expect_eq(rec.start_time.year, 2024);
        expect_eq(rec.start_time.day_of_year, 366);
        expect_eq(rec.encoding == MseedEncoding.int32, true);
        expect_eq(rec.word_order, 0);
        expect_eq(rec.record_bytes, 4096);
        expect_eq(rec.has_blk_1001, false);
        expect_eq(rec.payload.len, 4096 - 64);
    } else {
        expect_eq(true, false);
    }
}

test("mseed: sampleRateHz() SEED v2.4 positive/negative factor & multiplier formulas") {
    let r1 = MseedRecord { sample_rate_factor: 100, sample_rate_mult: 2 };
    expect_eq(approxEq(r1.sampleRateHz(), 200.0), true);

    let r2 = MseedRecord { sample_rate_factor: 40, sample_rate_mult: -2 };
    expect_eq(approxEq(r2.sampleRateHz(), 20.0), true);

    let r3 = MseedRecord { sample_rate_factor: -10, sample_rate_mult: 1 };
    expect_eq(approxEq(r3.sampleRateHz(), 0.1), true);

    let r4 = MseedRecord { sample_rate_factor: -10, sample_rate_mult: -2 };
    expect_eq(approxEq(r4.sampleRateHz(), 0.05), true);

    let r5 = MseedRecord { sample_rate_factor: 0, sample_rate_mult: 1 };
    expect_eq(approxEq(r5.sampleRateHz(), 0.0), true);

    expect_eq(mseedEncodingFromU8(10) == MseedEncoding.steim1, true);
    expect_eq(mseedEncodingFromU8(99) == MseedEncoding.unknown, true);
}

test("mseed: multi-record MseedIter and summarizeMseed with mixed record sizes") {
    // 512 (BE) + 4096 (LE) + 256 (BE) = 4864 bytes
    let buf: [4864]u8 = [0; 4864];
    buildMseedRecord(buf[0..4864], 0, "000001", 'D', "STA1", "00", "BHZ", "IU", 2025, 10, 300, 20, 1, true, 11, 9, false, 0, 0, 0);
    buildMseedRecord(buf[0..4864], 512, "000002", 'R', "STA2", "01", "BHN", "IU", 2025, 10, 1200, 100, 1, false, 3, 12, false, 0, 0, 0);
    buildMseedRecord(buf[0..4864], 4608, "000003", 'M', "STA3", "02", "BHE", "IU", 2025, 10, 150, 10, 1, true, 10, 8, false, 0, 0, 0);

    const stream = buf[0..4864];
    if (stats := summarizeMseed(stream)) {
        expect_eq(stats.record_count, 3);
        expect_eq(stats.total_samples, 1650);
        expect_eq(stats.min_record_bytes, 256);
        expect_eq(stats.max_record_bytes, 4096);
    } else {
        expect_eq(true, false);
    }

    let it = MseedIter.init(stream);
    let seen: u32 = 0;
    while (rec := it.next()) {
        seen += 1;
        if (seen == 1) {
            expect_eq(strEq(rec.station, "STA1"), true);
            expect_eq(rec.record_bytes, 512);
        } else if (seen == 2) {
            expect_eq(strEq(rec.station, "STA2"), true);
            expect_eq(rec.record_bytes, 4096);
        } else if (seen == 3) {
            expect_eq(strEq(rec.station, "STA3"), true);
            expect_eq(rec.record_bytes, 256);
        }
    }
    expect_eq(seen, 3);
    expect_eq(it.hasError(), false);
}

test("mseed: invalid quality code, bad BTIME year, out-of-bounds data_offset & truncation guards") {
    let buf: [512]u8 = [0; 512];
    buildMseedRecord(buf[0..512], 0, "000001", 'D', "ANMO", "00", "BHZ", "IU", 2026, 100, 100, 20, 1, true, 11, 9, false, 0, 0, 0);

    // 1. Invalid quality indicator ('X')
    buf[6] = 'X';
    expect_eq(isMseedRecord(buf[0..512]), false);
    if (r := parseMseedRecord(buf[0..512], 0)) {
        * = r;
        expect_eq(true, false);
    }
    buf[6] = 'D';

    // 2. Invalid BTIME year (1800 in BE; LE would be 0x0807 = 2055, so also set invalid doy=0)
    putU16(buf[0..512], 20, 1800, true);
    putU16(buf[0..512], 22, 0, true);
    if (r := parseMseedRecord(buf[0..512], 0)) {
        * = r;
        expect_eq(true, false);
    }
    putU16(buf[0..512], 20, 2026, true);
    putU16(buf[0..512], 22, 100, true);

    // 3. Out-of-bounds data_offset (600 > 512)
    putU16(buf[0..512], 44, 600, true);
    if (r := parseMseedRecord(buf[0..512], 0)) {
        * = r;
        expect_eq(true, false);
    }
    putU16(buf[0..512], 44, 64, true);

    // 4. Truncated buffer (only 200 bytes for a 512-byte record)
    if (r := parseMseedRecord(buf[0..200], 0)) {
        * = r;
        expect_eq(true, false);
    }
    if (s := summarizeMseed(buf[0..200])) {
        * = s;
        expect_eq(true, false);
    }
}

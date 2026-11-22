// Source: cp/cap/test/test_xm_production_deep.cp (Production deep tests for cap:xm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    XM_MAGIC,
    XM_SUB_BYTE,
    XM_MIN_HEADER_SIZE,
    XM_MAX_CHANNELS,
    XmHeader,
    XmPattern,
    XmInstrument,
    XmPatternIter,
    XmInstrumentIter,
    isXm,
    parseXmHeader,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn wU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn wU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn wCopy(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn buildSampleXm(out: []u8): usize {
    // 1. Standard 336-byte header (60 + header_size=276)
    wCopy(out, 0, XM_MAGIC);
    wCopy(out, 17, "CP Chiptune Demo   "); // 20B with trailing spaces
    out[37] = XM_SUB_BYTE;
    wCopy(out, 38, "FastTracker v2.00   ");
    wU16Le(out, 58, 0x0104); // v1.04
    wU32Le(out, 60, 276);    // header_size = 20 + 256
    wU16Le(out, 64, 4);      // song_length
    wU16Le(out, 66, 1);      // restart_position
    wU16Le(out, 68, 8);      // num_channels
    wU16Le(out, 70, 2);      // num_patterns
    wU16Le(out, 72, 3);      // num_instruments
    wU16Le(out, 74, 1);      // flags: bit 0 = Linear freq table
    wU16Le(out, 76, 6);      // default_tempo
    wU16Le(out, 78, 138);    // default_bpm
    out[80] = 0;
    out[81] = 1;
    out[82] = 0;
    out[83] = 1;

    let pos: usize = 336;

    // 2. Pattern 0: empty compressed pattern (packed_data_size = 0, 64 rows)
    wU32Le(out, pos, 9);
    out[pos + 4] = 0;
    wU16Le(out, pos + 5, 64);
    wU16Le(out, pos + 7, 0);
    pos += 9;

    // 3. Pattern 1: 32 rows, 12 bytes packed payload
    wU32Le(out, pos, 9);
    out[pos + 4] = 0;
    wU16Le(out, pos + 5, 32);
    wU16Le(out, pos + 7, 12);
    let k: usize = 0;
    while (k < 12) {
        out[pos + 9 + k] = (0x80 + k) as u8;
        k += 1;
    }
    pos += 9 + 12;

    // 4. Instrument 0: 0 samples (inst_header_size = 29)
    wU32Le(out, pos, 29);
    wCopy(out, pos + 4, "Empty Lead");
    out[pos + 26] = 0;
    wU16Le(out, pos + 27, 0);
    pos += 29;

    // 5. Instrument 1: 2 samples (inst_header_size = 263, sample_header_size = 40, PCM = 16 + 24 = 40B)
    wU32Le(out, pos, 263);
    wCopy(out, pos + 4, "Square Bass  ");
    out[pos + 26] = 0;
    wU16Le(out, pos + 27, 2);
    wU32Le(out, pos + 29, 40);
    pos += 263;
    // Sample 0 header (40B, length=16)
    wU32Le(out, pos, 16);
    pos += 40;
    // Sample 1 header (40B, length=24)
    wU32Le(out, pos, 24);
    pos += 40;
    // Sample 0 + 1 PCM data (40B)
    pos += 40;

    // 6. Instrument 2: 0 samples (verifies exact alignment after Instrument 1!)
    wU32Le(out, pos, 29);
    wCopy(out, pos + 4, "Noise Snare");
    out[pos + 26] = 0;
    wU16Le(out, pos + 27, 0);
    pos += 29;

    return pos;
}

test("1. XM v1.04 header, 256B pattern order table, and Linear/Amiga frequency flags", () => {
    expect_eq(XM_MIN_HEADER_SIZE, 80);
    expect_eq(XM_MAX_CHANNELS, 32);

    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleXm(buf[0..]);
    expect_eq(isXm(buf[0..flen]), true);

    const hdr = parseXmHeader(buf[0..flen]) ?? unreachable;
    expect_eq(bytesEq(hdr.module_name, "CP Chiptune Demo"), true);
    expect_eq(bytesEq(hdr.tracker_name, "FastTracker v2.00"), true);
    expect_eq(hdr.version, 0x0104);
    expect_eq(hdr.versionMajor(), 1);
    expect_eq(hdr.versionMinor(), 4);
    expect_eq(hdr.header_size, 276);
    expect_eq(hdr.song_length, 4);
    expect_eq(hdr.restart_position, 1);
    expect_eq(hdr.num_channels, 8);
    expect_eq(hdr.num_patterns, 2);
    expect_eq(hdr.num_instruments, 3);
    expect_eq(hdr.isLinearFreqTable(), true);
    expect_eq(hdr.default_tempo, 6);
    expect_eq(hdr.default_bpm, 138);
    expect_eq(hdr.pattern_order_table.len, 256);
    expect_eq(hdr.pattern_order_table[1], 1);
    expect_eq(hdr.patterns_offset, 336);

    // Amiga frequency table when bit 0 == 0
    wU16Le(buf[0..], 74, 0);
    const hdr_amiga = parseXmHeader(buf[0..flen]) ?? unreachable;
    expect_eq(hdr_amiga.isLinearFreqTable(), false);
});

test("2. Multi-pattern iteration (empty + packed) and instruments_offset calculation", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleXm(buf[0..]);
    const hdr = parseXmHeader(buf[0..flen]) ?? unreachable;

    let pit = XmPatternIter.init(buf[0..flen], hdr);

    const p0 = pit.next() ?? unreachable;
    expect_eq(p0.index, 0);
    expect_eq(p0.num_rows, 64);
    expect_eq(p0.packed_data_size, 0);
    expect_eq(p0.packed_data.len, 0);

    const p1 = pit.next() ?? unreachable;
    expect_eq(p1.index, 1);
    expect_eq(p1.num_rows, 32);
    expect_eq(p1.packed_data_size, 12);
    expect_eq(p1.packed_data.len, 12);
    expect_eq(p1.packed_data[0], 0x80);
    expect_eq(p1.packed_data[11], 0x8B);

    expect_eq(pit.next() == null, true);
    expect_eq(pit.had_error, false);
    // 336 + 9 + (9 + 12) = 366
    expect_eq(pit.instruments_offset, 366);
});

test("3. Multi-instrument iteration with 0-sample and multi-sample PCM skipping", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleXm(buf[0..]);
    const hdr = parseXmHeader(buf[0..flen]) ?? unreachable;

    let pit = XmPatternIter.init(buf[0..flen], hdr);
    while (p := pit.next()) { * = p; }

    let iit = XmInstrumentIter.init(buf[0..flen], pit.instruments_offset, hdr.num_instruments);

    const i0 = iit.next() ?? unreachable;
    expect_eq(i0.index, 0);
    expect_eq(bytesEq(i0.name, "Empty Lead"), true);
    expect_eq(i0.num_samples, 0);
    expect_eq(i0.total_sample_bytes, 0);

    const i1 = iit.next() ?? unreachable;
    expect_eq(i1.index, 1);
    expect_eq(bytesEq(i1.name, "Square Bass"), true);
    expect_eq(i1.num_samples, 2);
    expect_eq(i1.sample_header_size, 40);
    expect_eq(i1.total_sample_bytes, 40);

    const i2 = iit.next() ?? unreachable;
    expect_eq(i2.index, 2);
    expect_eq(bytesEq(i2.name, "Noise Snare"), true);
    expect_eq(i2.num_samples, 0);

    expect_eq(iit.next() == null, true);
    expect_eq(iit.had_error, false);
});

test("4. Header fault tolerance: bad magic, bad 0x1A, invalid channels, out-of-bounds header_size", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleXm(buf[0..]);

    // Wrong 0x1A byte at offset 37
    buf[37] = 0x00;
    expect_eq(isXm(buf[0..flen]), false);
    expect_eq(parseXmHeader(buf[0..flen]) == null, true);
    buf[37] = XM_SUB_BYTE;

    // Wrong magic at offset 0
    buf[0] = 'X';
    expect_eq(isXm(buf[0..flen]), false);
    buf[0] = 'E';

    // num_channels == 0 -> rejected
    wU16Le(buf[0..], 68, 0);
    expect_eq(parseXmHeader(buf[0..flen]) == null, true);
    // num_channels == 33 (> 32) -> rejected
    wU16Le(buf[0..], 68, 33);
    expect_eq(parseXmHeader(buf[0..flen]) == null, true);
    wU16Le(buf[0..], 68, 8);

    // header_size < 20 -> rejected
    wU32Le(buf[0..], 60, 12);
    expect_eq(parseXmHeader(buf[0..flen]) == null, true);

    // 60 + header_size > data.len -> rejected
    wU32Le(buf[0..], 60, 5000);
    expect_eq(parseXmHeader(buf[0..flen]) == null, true);
});

test("5. Pattern & Instrument truncation / invalid packing_type fault tolerance", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    const flen = buildSampleXm(buf[0..]);
    const hdr = parseXmHeader(buf[0..flen]) ?? unreachable;

    // 1. Corrupt packing_type of Pattern 0 to 1 -> had_error=true
    buf[hdr.patterns_offset + 4] = 1;
    let pit_bad = XmPatternIter.init(buf[0..flen], hdr);
    expect_eq(pit_bad.next() == null, true);
    expect_eq(pit_bad.had_error, true);
    buf[hdr.patterns_offset + 4] = 0;

    // 2. Truncated pattern data slice -> had_error=true
    let pit_trunc = XmPatternIter.init(buf[0..hdr.patterns_offset + 15], hdr);
    * = pit_trunc.next() ?? unreachable; // Pattern 0 ok (9B)
    expect_eq(pit_trunc.next() == null, true); // Pattern 1 truncated
    expect_eq(pit_trunc.had_error, true);

    // 3. Truncated instrument sample PCM -> had_error=true
    let iit_trunc = XmInstrumentIter.init(buf[0..flen - 35], 366, hdr.num_instruments);
    * = iit_trunc.next() ?? unreachable; // Inst 0 ok
    expect_eq(iit_trunc.next() == null, true); // Inst 1 truncated PCM
    expect_eq(iit_trunc.had_error, true);
});

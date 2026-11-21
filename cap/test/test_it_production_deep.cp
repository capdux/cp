// Source: cp/cap/test/test_it_production_deep.cp (Production deep tests for cap:it)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IT_MAGIC,
    IT_INST_MAGIC,
    IT_SAMPLE_MAGIC,
    IT_HEADER_SIZE,
    IT_SAMPLE_HEADER_SIZE,
    IT_FLAG_STEREO,
    IT_FLAG_USE_INSTRUMENTS,
    IT_FLAG_LINEAR_SLIDES,
    IT_SPECIAL_HAS_MESSAGE,
    ItHeader,
    ItSample,
    ItInstrument,
    ItPattern,
    isIt,
    parseItHeader,
    parseItSample,
    parseItInstrument,
    parseItPattern,
} from "cap:it";

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

fn buildSampleIt(out: []u8): usize {
    // 1. 192B IMPM header
    wCopy(out, 0, IT_MAGIC);
    wCopy(out, 4, "Impulse Demo Track");
    wU16Le(out, 32, 4); // order_count = 4
    wU16Le(out, 34, 1); // instrument_count = 1
    wU16Le(out, 36, 1); // sample_count = 1
    wU16Le(out, 38, 2); // pattern_count = 2
    wU16Le(out, 40, 0x0214); // created_with_version = 2.14
    wU16Le(out, 42, 0x0200); // compatible_version = 2.00
    wU16Le(out, 44, IT_FLAG_STEREO | IT_FLAG_USE_INSTRUMENTS | IT_FLAG_LINEAR_SLIDES);
    wU16Le(out, 46, IT_SPECIAL_HAS_MESSAGE);
    out[48] = 128; // global_volume
    out[49] = 48;  // mix_volume
    out[50] = 6;   // initial_speed
    out[51] = 132; // initial_tempo
    out[52] = 128; // pan_separation

    // Tables layout:
    // orders:      192..196 (4 bytes)
    // inst table:  196..200 (1 * 4 = 4 bytes)
    // smp table:   200..204 (1 * 4 = 4 bytes)
    // pat table:   204..212 (2 * 4 = 8 bytes)
    out[192] = 0;
    out[193] = 1;
    out[194] = 255; // end marker
    out[195] = 255;

    // Song message at offset 216
    const msg = "Composed with Impulse Tracker 2.14";
    const msg_off: usize = 216;
    wU16Le(out, 54, (msg.len + 1) as u16);
    wU32Le(out, 56, msg_off as u32);
    wCopy(out, msg_off, msg);
    out[msg_off + msg.len] = 0;

    // Instrument 0 ("IMPI") at offset 260 (64 bytes)
    const inst_off: usize = 260;
    wU32Le(out, 196, inst_off as u32);
    wCopy(out, inst_off, IT_INST_MAGIC);
    wCopy(out, inst_off + 4, "LEAD01.ITI");
    out[inst_off + 24] = 100; // global_volume
    out[inst_off + 25] = 32;  // default_pan
    wCopy(out, inst_off + 32, "Sawtooth Lead");

    // Sample 0 ("IMPS", 80B) at offset 324
    const smp_off: usize = 324;
    wU32Le(out, 200, smp_off as u32);
    wCopy(out, smp_off, IT_SAMPLE_MAGIC);
    wCopy(out, smp_off + 4, "SAW44K.WAV");
    out[smp_off + 17] = 64;             // global_volume
    out[smp_off + 18] = 0x01 | 0x02 | 0x10; // has_data | 16-bit | loop
    out[smp_off + 19] = 60;             // default_volume
    wCopy(out, smp_off + 20, "Saw Wave 16bit");
    wU32Le(out, smp_off + 48, 2048);    // length
    wU32Le(out, smp_off + 52, 256);     // loop_begin
    wU32Le(out, smp_off + 56, 1024);    // loop_end
    wU32Le(out, smp_off + 60, 44100);   // c5_speed
    wU32Le(out, smp_off + 72, 512);     // sample_pointer

    // Pattern 0: offset 0 (empty 64-row pattern)
    wU32Le(out, 204, 0);

    // Pattern 1 at offset 410 (8B header + 10B packed payload)
    const pat1_off: usize = 410;
    wU32Le(out, 208, pat1_off as u32);
    wU16Le(out, pat1_off, 10); // packed_len = 10
    wU16Le(out, pat1_off + 2, 32); // rows = 32
    let i: usize = 0;
    while (i < 10) {
        out[pat1_off + 8 + i] = (0xA0 + i) as u8;
        i += 1;
    }

    return pat1_off + 18;
}

test("1. 192B IMPM header, flags, and zero-copy songMessage extraction", () => {
    expect_eq(IT_HEADER_SIZE, 192);
    expect_eq(IT_SAMPLE_HEADER_SIZE, 80);

    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const flen = buildSampleIt(buf[0..]);
    expect_eq(isIt(buf[0..flen]), true);

    const hdr = parseItHeader(buf[0..flen]) ?? unreachable;
    expect_eq(bytesEq(hdr.song_name, "Impulse Demo Track"), true);
    expect_eq(hdr.order_count, 4);
    expect_eq(hdr.instrument_count, 1);
    expect_eq(hdr.sample_count, 1);
    expect_eq(hdr.pattern_count, 2);
    expect_eq(hdr.created_with_version, 0x0214);
    expect_eq(hdr.compatible_version, 0x0200);
    expect_eq(hdr.global_volume, 128);
    expect_eq(hdr.mix_volume, 48);
    expect_eq(hdr.initial_speed, 6);
    expect_eq(hdr.initial_tempo, 132);
    expect_eq(hdr.pan_separation, 128);

    expect_eq(hdr.isStereo(), true);
    expect_eq(hdr.usesInstruments(), true);
    expect_eq(hdr.usesLinearSlides(), true);
    expect_eq(hdr.hasMessage(), true);

    const msg = hdr.songMessage(buf[0..flen]) ?? unreachable;
    expect_eq(bytesEq(msg, "Composed with Impulse Tracker 2.14"), true);
});

test("2. Order, Instrument, Sample, and Pattern 32-bit offset table addressing", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const flen = buildSampleIt(buf[0..]);
    const hdr = parseItHeader(buf[0..flen]) ?? unreachable;

    expect_eq(hdr.orders_offset, 192);
    expect_eq(hdr.instruments_table_offset, 196);
    expect_eq(hdr.samples_table_offset, 200);
    expect_eq(hdr.patterns_table_offset, 204);

    const orders = hdr.ordersSlice(buf[0..flen]);
    expect_eq(orders.len, 4);
    expect_eq(orders[0], 0);
    expect_eq(orders[1], 1);
    expect_eq(orders[2], 255);

    expect_eq(hdr.instrumentOffset(buf[0..flen], 0) ?? 0, 260);
    expect_eq(hdr.instrumentOffset(buf[0..flen], 1) == null, true);

    expect_eq(hdr.sampleOffset(buf[0..flen], 0) ?? 0, 324);
    expect_eq(hdr.sampleOffset(buf[0..flen], 1) == null, true);

    expect_eq(hdr.patternOffset(buf[0..flen], 0) ?? 999, 0);
    expect_eq(hdr.patternOffset(buf[0..flen], 1) ?? 0, 410);
    expect_eq(hdr.patternOffset(buf[0..flen], 2) == null, true);
});

test("3. IMPI instrument and 80B IMPS sample header (16-bit, Loop, C5Speed) parsing", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const flen = buildSampleIt(buf[0..]);
    const hdr = parseItHeader(buf[0..flen]) ?? unreachable;

    const inst = parseItInstrument(buf[0..flen], hdr, 0) ?? unreachable;
    expect_eq(bytesEq(inst.dos_filename, "LEAD01.ITI"), true);
    expect_eq(inst.global_volume, 100);
    expect_eq(inst.default_pan, 32);
    expect_eq(bytesEq(inst.name, "Sawtooth Lead"), true);

    const smp = parseItSample(buf[0..flen], hdr, 0) ?? unreachable;
    expect_eq(bytesEq(smp.dos_filename, "SAW44K.WAV"), true);
    expect_eq(bytesEq(smp.sample_name, "Saw Wave 16bit"), true);
    expect_eq(smp.global_volume, 64);
    expect_eq(smp.default_volume, 60);
    expect_eq(smp.hasData(), true);
    expect_eq(smp.is16Bit(), true);
    expect_eq(smp.isStereo(), false);
    expect_eq(smp.isCompressed(), false);
    expect_eq(smp.hasLoop(), true);
    expect_eq(smp.length, 2048);
    expect_eq(smp.loop_begin, 256);
    expect_eq(smp.loop_end, 1024);
    expect_eq(smp.c5_speed, 44100);
    expect_eq(smp.sample_pointer, 512);
});

test("4. Empty Pattern (offset 0 -> 64 rows) and packed Pattern parsing", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const flen = buildSampleIt(buf[0..]);
    const hdr = parseItHeader(buf[0..flen]) ?? unreachable;

    const p0 = parseItPattern(buf[0..flen], hdr, 0) ?? unreachable;
    expect_eq(p0.is_empty, true);
    expect_eq(p0.rows, 64);
    expect_eq(p0.packed_len, 0);
    expect_eq(p0.packed_data.len, 0);

    const p1 = parseItPattern(buf[0..flen], hdr, 1) ?? unreachable;
    expect_eq(p1.is_empty, false);
    expect_eq(p1.rows, 32);
    expect_eq(p1.packed_len, 10);
    expect_eq(p1.packed_data.len, 10);
    expect_eq(p1.packed_data[0], 0xA0);
    expect_eq(p1.packed_data[9], 0xA9);
});

test("5. Fault tolerance: bad IMPM/IMPI/IMPS magic, out-of-bounds offsets, and invalid pattern rows", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const flen = buildSampleIt(buf[0..]);
    const hdr = parseItHeader(buf[0..flen]) ?? unreachable;

    // 1. Corrupt IMPI magic -> parseItInstrument returns null
    buf[260] = 'X';
    expect_eq(parseItInstrument(buf[0..flen], hdr, 0) == null, true);
    buf[260] = 'I';

    // 2. Corrupt IMPS magic -> parseItSample returns null
    buf[324] = 'X';
    expect_eq(parseItSample(buf[0..flen], hdr, 0) == null, true);
    buf[324] = 'I';

    // 3. Invalid pattern rows (201 > 200) -> parseItPattern returns null
    wU16Le(buf[0..], 412, 201);
    expect_eq(parseItPattern(buf[0..flen], hdr, 1) == null, true);
    wU16Le(buf[0..], 412, 32);

    // 4. Truncated pattern payload -> parseItPattern returns null
    expect_eq(parseItPattern(buf[0..420], hdr, 1) == null, true);

    // 5. Out-of-bounds offset table count -> parseItHeader returns null
    wU16Le(buf[0..], 38, 999);
    expect_eq(parseItHeader(buf[0..flen]) == null, true);

    // 6. Wrong IMPM magic -> isIt / parseItHeader return false / null
    buf[0] = 'X';
    expect_eq(isIt(buf[0..flen]), false);
    expect_eq(parseItHeader(buf[0..flen]) == null, true);
});

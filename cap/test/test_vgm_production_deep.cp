// Source: cp/cap/test/test_vgm_production_deep.cp (Production deep tests for cap:vgm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VGM_MAGIC,
    VGM_GD3_MAGIC,
    VGM_MIN_HEADER_SIZE,
    VGM_SAMPLE_RATE,
    CMD_WAIT_N,
    CMD_WAIT_60HZ,
    CMD_WAIT_50HZ,
    CMD_END,
    CMD_DATA_BLOCK,
    CMD_WAIT_SHORT_START,
    VgmHeader,
    VgmGd3,
    VgmCmdStats,
    isVgm,
    parseVgmHeader,
    parseVgmGd3,
    decodeUtf16LeToUtf8,
    scanVgmCommands,
} from "cap:media";

fn writeU16LE(buf: []u8, offset: usize, val: u16) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

fn copyBytes(dst: []u8, offset: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
}

fn appendAsciiAsUtf16LeZ(buf: []u8, pos: *usize, s: []const u8) {
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.*] = s[i];
        buf[pos.* + 1] = 0;
        pos.* += 2;
        i += 1;
    }
    buf[pos.*] = 0;
    buf[pos.* + 1] = 0;
    pos.* += 2;
}

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.01;
}

test("1. VGM 1.71 header BCD version, sound clocks, durationMs, and relative offsets", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    copyBytes(buf[0..], 0, VGM_MAGIC);
    writeU32LE(buf[0..], 0x04, 252); // eof_offset = 256 - 4
    writeU32LE(buf[0..], 0x08, 0x00000171); // v1.71
    writeU32LE(buf[0..], 0x0C, 3579545); // SN76489
    writeU32LE(buf[0..], 0x10, 3579545); // YM2413
    writeU32LE(buf[0..], 0x18, 44100 * 2); // 2.0 seconds = 2000ms
    writeU32LE(buf[0..], 0x1C, 0x80 - 0x1C); // loop at 0x80
    writeU32LE(buf[0..], 0x20, 44100);
    writeU32LE(buf[0..], 0x24, 60); // 60 Hz
    writeU32LE(buf[0..], 0x2C, 7670454); // YM2612
    writeU32LE(buf[0..], 0x30, 3579545); // YM2151
    writeU32LE(buf[0..], 0x34, 0x80 - 0x34); // data_offset = 0x80

    expect_eq(isVgm(buf[0..]), true);
    const hdr = parseVgmHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.versionMajor(), 1);
    expect_eq(hdr.versionMinor(), 71);
    expect_eq(hdr.sn76489_clock, 3579545);
    expect_eq(hdr.ym2612_clock, 7670454);
    expect_eq(hdr.ym2151_clock, 3579545);
    expect_eq(hdr.rate_hz, 60);
    expect_eq(hdr.data_offset, 0x80);
    expect_eq(hdr.loop_abs_offset ?? 0, 0x80);
    expect_eq(hdr.gd3_abs_offset == null, true);
    expect_eq(approxEq(hdr.durationMs(), 2000.0), true);
});

test("2. GD3 UTF-16LE tag block parsing and zero-alloc UTF-8 decoding", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    copyBytes(buf[0..], 0, VGM_MAGIC);
    writeU32LE(buf[0..], 0x08, 0x00000150); // v1.50
    writeU32LE(buf[0..], 0x34, 0x40 - 0x34); // data_offset = 0x40
    buf[0x40] = CMD_END;

    const gd3_off: usize = 0x50;
    writeU32LE(buf[0..], 0x14, (gd3_off - 0x14) as u32);
    copyBytes(buf[0..], gd3_off, VGM_GD3_MAGIC);
    writeU32LE(buf[0..], gd3_off + 4, 0x00000100); // GD3 v1.00

    let p: usize = gd3_off + 12;
    appendAsciiAsUtf16LeZ(buf[0..], &p, "Green Hill Zone");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "Sonic the Hedgehog");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "Sega Mega Drive");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "Masato Nakamura");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "1991/06/23");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "CP VGM Tool");
    appendAsciiAsUtf16LeZ(buf[0..], &p, "Zero-alloc test");

    const gd3_body_len = p - (gd3_off + 12);
    writeU32LE(buf[0..], gd3_off + 8, gd3_body_len as u32);

    const hdr = parseVgmHeader(buf[0..p]) ?? unreachable;
    expect_eq(hdr.gd3_abs_offset ?? 0, gd3_off);

    const gd3 = parseVgmGd3(buf[0..p], hdr) ?? unreachable;
    expect_eq(gd3.version, 0x00000100);

    let tmp: [64]u8 = std.mem.zeroes([64]u8);
    const n1 = decodeUtf16LeToUtf8(gd3.track_name_en, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n1], "Green Hill Zone"), true);

    const n2 = decodeUtf16LeToUtf8(gd3.game_name_en, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n2], "Sonic the Hedgehog"), true);

    const n3 = decodeUtf16LeToUtf8(gd3.system_name_en, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n3], "Sega Mega Drive"), true);

    const n4 = decodeUtf16LeToUtf8(gd3.author_en, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n4], "Masato Nakamura"), true);

    const n5 = decodeUtf16LeToUtf8(gd3.release_date, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n5], "1991/06/23"), true);

    const n6 = decodeUtf16LeToUtf8(gd3.vgm_creator, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n6], "CP VGM Tool"), true);

    const n7 = decodeUtf16LeToUtf8(gd3.notes, tmp[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, tmp[0..n7], "Zero-alloc test"), true);
});

test("3. scanVgmCommands handles wait opcodes, YM/PSG writes, 0x67 data block, and 0x66 end", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    copyBytes(buf[0..], 0, VGM_MAGIC);
    writeU32LE(buf[0..], 0x08, 0x00000150);
    // data_offset defaults to 0x40 when 0x34 == 0

    let p: usize = 0x40;
    // 1. 0x50 SN76489 write (2B)
    buf[p] = 0x50; buf[p + 1] = 0x9F; p += 2;
    // 2. 0x52 YM2612 port 0 write (3B)
    buf[p] = 0x52; buf[p + 1] = 0x28; buf[p + 2] = 0xF0; p += 3;
    // 3. 0x67 Data Block: 0x67 0x66 <type=0x00> <size=4> <4B payload>
    buf[p] = CMD_DATA_BLOCK;
    buf[p + 1] = 0x66;
    buf[p + 2] = 0x00;
    writeU32LE(buf[0..], p + 3, 4);
    p += 7 + 4;
    // 4. 0x61 Wait 1000 samples (3B)
    buf[p] = CMD_WAIT_N; writeU16LE(buf[0..], p + 1, 1000); p += 3;
    // 5. 0x62 Wait 735 samples (1B)
    buf[p] = CMD_WAIT_60HZ; p += 1;
    // 6. 0x63 Wait 882 samples (1B)
    buf[p] = CMD_WAIT_50HZ; p += 1;
    // 7. 0x75 Wait (5 + 1) = 6 samples (1B)
    buf[p] = CMD_WAIT_SHORT_START + 5; p += 1;
    // 8. 0x66 End of sound data (1B)
    buf[p] = CMD_END; p += 1;

    const hdr = parseVgmHeader(buf[0..p]) ?? unreachable;
    expect_eq(hdr.data_offset, 0x40);

    const st = scanVgmCommands(buf[0..p], hdr);
    expect_eq(st.has_end, true);
    expect_eq(st.command_count, 8);
    expect_eq(st.data_block_count, 1);
    // 1000 + 735 + 882 + 6 = 2623
    expect_eq(st.computed_wait_samples, 2623);
});

test("4. VGM 1.00 legacy header always uses 0x40 data_offset regardless of 0x34 bytes", () => {
    let buf: [80]u8 = std.mem.zeroes([80]u8);
    copyBytes(buf[0..], 0, VGM_MAGIC);
    writeU32LE(buf[0..], 0x08, 0x00000100); // v1.00
    writeU32LE(buf[0..], 0x34, 0x12345678); // ignored for v < 1.50
    buf[0x40] = CMD_WAIT_60HZ;
    buf[0x41] = CMD_END;

    const hdr = parseVgmHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.versionMajor(), 1);
    expect_eq(hdr.versionMinor(), 0);
    expect_eq(hdr.data_offset, 0x40);

    const st = scanVgmCommands(buf[0..], hdr);
    expect_eq(st.has_end, true);
    expect_eq(st.computed_wait_samples, 735);
});

test("5. Malformed VGM headers (bad magic, short buffer, out-of-bounds gd3/data offsets) fail safely", () => {
    expect_eq(isVgm(""), false);
    expect_eq(isVgm("Vgm short"), false);
    expect_eq(parseVgmHeader("NOT VGM DATA") == null, true);

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    copyBytes(buf[0..], 0, VGM_MAGIC);
    writeU32LE(buf[0..], 0x08, 0x00000150);

    // Out-of-bounds gd3_offset
    let bad_gd3: [64]u8 = buf;
    writeU32LE(bad_gd3[0..], 0x14, 500);
    expect_eq(parseVgmHeader(bad_gd3[0..]) == null, true);

    // Out-of-bounds data_offset (> buf.len)
    let bad_data: [64]u8 = buf;
    writeU32LE(bad_data[0..], 0x34, 200);
    expect_eq(parseVgmHeader(bad_data[0..]) == null, true);

    // Under-minimum data_offset (< 0x40)
    let small_data: [64]u8 = buf;
    writeU32LE(small_data[0..], 0x34, 4); // 0x34 + 4 = 0x38 < 0x40
    expect_eq(parseVgmHeader(small_data[0..]) == null, true);

    // Output buffer overflow protection in decodeUtf16LeToUtf8
    let tiny: [2]u8 = std.mem.zeroes([2]u8);
    const u16_abc: [6]u8 = ['A', 0, 'B', 0, 'C', 0];
    expect_eq(decodeUtf16LeToUtf8(u16_abc[0..], tiny[0..]) == null, true);
});

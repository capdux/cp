// Source: cp/cap/test/test_nsf_production_deep.cp (Production deep tests for cap:nsf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NSF_MAGIC,
    NSF_HEADER_SIZE,
    NSFE_MAGIC,
    NSF_CHIP_VRC6,
    NSF_CHIP_VRC7,
    NSF_CHIP_FDS,
    NSF_CHIP_MMC5,
    NsfRegion,
    NsfFormat,
    NsfHeader,
    NsfeChunk,
    NsfeInfo,
    detectNsfFormat,
    isNsf,
    isNsfe,
    parseNsfHeader,
    NsfeChunkIter,
    findNsfeChunk,
    parseNsfeInfo,
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

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.1;
}

fn appendNsfeChunk(buf: []u8, pos: *usize, id: []const u8, payload: []const u8) {
    writeU32LE(buf, pos.*, payload.len as u32);
    pos.* += 4;
    copyBytes(buf, pos.*, id);
    pos.* += 4;
    copyBytes(buf, pos.*, payload);
    pos.* += payload.len;
}

test("1. Standard 128B NSF1 header with VRC6 chip, bankswitching, and NTSC/PAL frame rates", () => {
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    copyBytes(buf[0..], 0, NSF_MAGIC);
    buf[0x05] = 1; // NSF v1
    buf[0x06] = 12; // total_songs
    buf[0x07] = 1; // starting_song
    writeU16LE(buf[0..], 0x08, 0x8000); // load_addr
    writeU16LE(buf[0..], 0x0A, 0x8003); // init_addr
    writeU16LE(buf[0..], 0x0C, 0x8006); // play_addr
    copyBytes(buf[0..], 0x0E, "Castlevania III");
    copyBytes(buf[0..], 0x2E, "Konami Kukeiha Club");
    copyBytes(buf[0..], 0x4E, "1989 Konami");
    writeU16LE(buf[0..], 0x6E, 16639); // ~60.1 Hz NTSC
    buf[0x70] = 0;
    buf[0x71] = 1; // non-zero bankswitch_init
    writeU16LE(buf[0..], 0x78, 19997); // ~50.0 Hz PAL
    buf[0x7A] = 0x02; // Dual NTSC/PAL
    buf[0x7B] = NSF_CHIP_VRC6 | NSF_CHIP_MMC5;
    copyBytes(buf[0..], 128, "\xEA\xEA\x60"); // 6502 NOP NOP RTS

    expect_eq(isNsf(buf[0..]), true);
    expect_eq(isNsfe(buf[0..]), false);
    expect_eq(detectNsfFormat(buf[0..]), NsfFormat.nsf);

    const hdr = parseNsfHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.version, 1);
    expect_eq(hdr.total_songs, 12);
    expect_eq(hdr.starting_song, 1);
    expect_eq(hdr.load_addr, 0x8000);
    expect_eq(hdr.init_addr, 0x8003);
    expect_eq(hdr.play_addr, 0x8006);
    expect_eq(std.mem.eql(u8, hdr.song_name, "Castlevania III"), true);
    expect_eq(std.mem.eql(u8, hdr.artist, "Konami Kukeiha Club"), true);
    expect_eq(std.mem.eql(u8, hdr.copyright, "1989 Konami"), true);
    expect_eq(hdr.region, NsfRegion.dual);
    expect_eq(hdr.usesBankswitching(), true);
    expect_eq(hdr.hasChip(NSF_CHIP_VRC6), true);
    expect_eq(hdr.hasChip(NSF_CHIP_MMC5), true);
    expect_eq(hdr.hasChip(NSF_CHIP_VRC7), false);
    expect_eq(approxEq(hdr.ntscFps(), 60.1), true);
    expect_eq(approxEq(hdr.palFps(), 50.0), true);
    expect_eq(hdr.program_data.len, 32);
    expect_eq(hdr.program_data[0], 0xEA);
});

test("2. NSF2 FDS load address >= 0x6000 allowed and non-bankswitched check", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    copyBytes(buf[0..], 0, NSF_MAGIC);
    buf[0x05] = 2; // NSF v2
    buf[0x06] = 3;
    buf[0x07] = 2;
    writeU16LE(buf[0..], 0x08, 0x6000); // valid for FDS
    buf[0x7B] = NSF_CHIP_FDS;

    const hdr = parseNsfHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.version, 2);
    expect_eq(hdr.load_addr, 0x6000);
    expect_eq(hdr.usesBankswitching(), false);
    expect_eq(hdr.hasChip(NSF_CHIP_FDS), true);

    // Without FDS chip, 0x6000 load_addr must be rejected
    buf[0x7B] = 0;
    expect_eq(parseNsfHeader(buf[0..]) == null, true);
});

test("3. NSFE multi-chunk container iteration (INFO/DATA/AUTH/NEND) and findNsfeChunk", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    copyBytes(buf[0..], 0, NSFE_MAGIC);
    let pos: usize = 4;

    let info_payload: [10]u8 = std.mem.zeroes([10]u8);
    writeU16LE(info_payload[0..], 0, 0x8000);
    writeU16LE(info_payload[0..], 2, 0x8010);
    writeU16LE(info_payload[0..], 4, 0x8020);
    info_payload[6] = 0x01; // PAL
    info_payload[7] = NSF_CHIP_VRC6;
    info_payload[8] = 8; // total_songs
    info_payload[9] = 2; // starting_song (0-based)

    appendNsfeChunk(buf[0..], &pos, "INFO", info_payload[0..]);
    appendNsfeChunk(buf[0..], &pos, "DATA", "\xA9\x00\x60");
    appendNsfeChunk(buf[0..], &pos, "AUTH", "My Game\x00Composer\x00");
    appendNsfeChunk(buf[0..], &pos, "NEND", "");

    const nsfe = buf[0..pos];
    expect_eq(isNsfe(nsfe), true);
    expect_eq(detectNsfFormat(nsfe), NsfFormat.nsfe);

    let it = NsfeChunkIter.init(nsfe);
    let count: u32 = 0;
    while (ch := it.next()) {
        * = ch;
        count += 1;
    }
    expect_eq(count, 4);
    expect_eq(it.malformed, false);

    const found_info = findNsfeChunk(nsfe, "INFO") ?? unreachable;
    const info = parseNsfeInfo(found_info) ?? unreachable;
    expect_eq(info.load_addr, 0x8000);
    expect_eq(info.init_addr, 0x8010);
    expect_eq(info.play_addr, 0x8020);
    expect_eq(info.region, NsfRegion.pal);
    expect_eq(info.expansion_chips, NSF_CHIP_VRC6);
    expect_eq(info.total_songs, 8);
    expect_eq(info.starting_song, 2);

    const found_data = findNsfeChunk(nsfe, "DATA") ?? unreachable;
    expect_eq(found_data.len, 3);
    expect_eq(found_data[0], 0xA9);
    expect_eq(findNsfeChunk(nsfe, "NONE") == null, true);
});

test("4. parseNsfeInfo supports minimal 8-byte INFO block with default song counts", () => {
    let min_info: [8]u8 = std.mem.zeroes([8]u8);
    writeU16LE(min_info[0..], 0, 0x9000);
    writeU16LE(min_info[0..], 2, 0x9000);
    writeU16LE(min_info[0..], 4, 0x9003);
    min_info[6] = 0x00; // NTSC
    min_info[7] = 0x00;

    const info = parseNsfeInfo(min_info[0..]) ?? unreachable;
    expect_eq(info.load_addr, 0x9000);
    expect_eq(info.region, NsfRegion.ntsc);
    expect_eq(info.total_songs, 1);
    expect_eq(info.starting_song, 0);

    // < 8 bytes rejected
    expect_eq(parseNsfeInfo(min_info[0..7]) == null, true);
});

test("5. Malformed NSF/NSFe inputs (bad magic, total_songs==0, starting_song > total_songs, OOB NSFe chunk) fail safely", () => {
    expect_eq(isNsf(""), false);
    expect_eq(isNsfe("BAD!"), false);
    expect_eq(parseNsfHeader("short") == null, true);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    copyBytes(buf[0..], 0, NSF_MAGIC);
    buf[0x05] = 1;
    buf[0x06] = 5;
    buf[0x07] = 1;
    writeU16LE(buf[0..], 0x08, 0x8000);

    // total_songs == 0
    let zero_songs: [128]u8 = buf;
    zero_songs[0x06] = 0;
    expect_eq(parseNsfHeader(zero_songs[0..]) == null, true);

    // starting_song > total_songs
    let bad_start: [128]u8 = buf;
    bad_start[0x07] = 6;
    expect_eq(parseNsfHeader(bad_start[0..]) == null, true);

    // Out-of-bounds NSFe chunk size
    let bad_nsfe: [20]u8 = std.mem.zeroes([20]u8);
    copyBytes(bad_nsfe[0..], 0, NSFE_MAGIC);
    writeU32LE(bad_nsfe[0..], 4, 99999);
    copyBytes(bad_nsfe[0..], 8, "INFO");
    let it = NsfeChunkIter.init(bad_nsfe[0..]);
    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, true);
});

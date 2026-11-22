// Source: cp/cap/test/test_mp4_production_deep.cp (Production deep tests for cap:mp4)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MP4_BOX_HEADER_SIZE,
    MP4_LARGE_HEADER_SIZE,
    MP4_UUID_SIZE,
    Mp4Box,
    FtypBox,
    MvhdBox,
    MdhdBox,
    TkhdBox,
    BoxCursor,
    fourccEquals,
    findBox,
    findBoxPath,
    parseFtyp,
    isMp4,
    parseMvhd,
    parseMdhd,
    parseTkhd,
} from "cap:media";

fn writeTag(buf: []u8, off: usize, tag: []const u8) {
    buf[off] = tag[0];
    buf[off + 1] = tag[1];
    buf[off + 2] = tag[2];
    buf[off + 3] = tag[3];
}

fn writeU16BE(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeU64BE(buf: []u8, off: usize, v: u64) {
    writeU32BE(buf, off, ((v >> 32) & 0xFFFF_FFFF) as u32);
    writeU32BE(buf, off + 4, (v & 0xFFFF_FFFF) as u32);
}

fn writeBoxHeader(buf: []u8, off: usize, size: u32, fourcc: []const u8) {
    writeU32BE(buf, off, size);
    writeTag(buf, off + 4, fourcc);
}

test("1. Full ftyp + moov(mvhd/trak/tkhd/mdia/mdhd) + mdat container parsing", () => {
    // ftyp: 8 header + 16 payload (isom + minor 512 + iso2 + mp41) = 24
    // mvhd v0: 8 header + 100 payload = 108
    // tkhd v0: 8 header + 84 payload = 92
    // mdhd v0: 8 header + 24 payload = 32
    // mdia: 8 header + 32 = 40
    // trak: 8 header + 92 + 40 = 140
    // moov: 8 header + 108 + 140 = 256
    // mdat: 8 header + 4 payload = 12
    // total = 24 + 256 + 12 = 292
    let buf: [292]u8 = std.mem.zeroes([292]u8);

    // ftyp at 0..24
    writeBoxHeader(buf[0..], 0, 24, "ftyp");
    writeTag(buf[0..], 8, "isom");
    writeU32BE(buf[0..], 12, 512);
    writeTag(buf[0..], 16, "iso2");
    writeTag(buf[0..], 20, "mp41");

    // moov at 24..280
    writeBoxHeader(buf[0..], 24, 256, "moov");

    // mvhd v0 at 32..140 (payload 40..140)
    writeBoxHeader(buf[0..], 32, 108, "mvhd");
    buf[40] = 0; // version 0
    writeU32BE(buf[0..], 52, 1000); // timescale = 1000
    writeU32BE(buf[0..], 56, 12500); // duration = 12500 (12.5s)
    writeU32BE(buf[0..], 60, 0x0001_0000); // rate = 1.0
    writeU16BE(buf[0..], 64, 0x0100); // volume = 1.0
    writeU32BE(buf[0..], 136, 2); // next_track_id = 2

    // trak at 140..280
    writeBoxHeader(buf[0..], 140, 140, "trak");

    // tkhd v0 at 148..240 (payload 156..240)
    writeBoxHeader(buf[0..], 148, 92, "tkhd");
    buf[156] = 0; // version 0
    buf[159] = 3; // flags = 3
    writeU32BE(buf[0..], 168, 1); // track_id = 1
    writeU32BE(buf[0..], 176, 12500); // duration
    writeU16BE(buf[0..], 232, 1920); // width integer part
    writeU16BE(buf[0..], 236, 1080); // height integer part

    // mdia at 240..280
    writeBoxHeader(buf[0..], 240, 40, "mdia");

    // mdhd v0 at 248..280 (payload 256..280)
    writeBoxHeader(buf[0..], 248, 32, "mdhd");
    buf[256] = 0; // version 0
    writeU32BE(buf[0..], 268, 48000); // timescale
    writeU32BE(buf[0..], 272, 96000); // duration (2000 ms)
    // "eng" packed: ('e'-0x60)=5, ('n'-0x60)=14, ('g'-0x60)=7 -> (5<<10)|(14<<5)|7 = 0x15C7
    writeU16BE(buf[0..], 276, 0x15C7);

    // mdat at 280..292
    writeBoxHeader(buf[0..], 280, 12, "mdat");
    writeTag(buf[0..], 288, "DATA");

    expect_eq(isMp4(buf[0..]), true);

    const ftyp_box = findBox(buf[0..], "ftyp") ?? unreachable;
    const ftyp = parseFtyp(ftyp_box.payload) ?? unreachable;
    expect_eq(fourccEquals(ftyp.major_brand, "isom"), true);
    expect_eq(ftyp.minor_version, 512);
    expect_eq(ftyp.brandCount(), 2);
    expect_eq(ftyp.hasBrand("isom"), true);
    expect_eq(ftyp.hasBrand("mp41"), true);
    expect_eq(ftyp.hasBrand("qt  "), false);

    const mvhd_box = findBoxPath(buf[0..], "moov/mvhd") ?? unreachable;
    const mvhd = parseMvhd(mvhd_box.payload) ?? unreachable;
    expect_eq(mvhd.version, 0);
    expect_eq(mvhd.timescale, 1000);
    expect_eq(mvhd.durationMs(), 12500);
    expect_eq(mvhd.next_track_id, 2);

    const tkhd_box = findBoxPath(buf[0..], "moov/trak/tkhd") ?? unreachable;
    const tkhd = parseTkhd(tkhd_box.payload) ?? unreachable;
    expect_eq(tkhd.track_id, 1);
    expect_eq(tkhd.width, 1920);
    expect_eq(tkhd.height, 1080);

    const mdhd_box = findBoxPath(buf[0..], "moov/trak/mdia/mdhd") ?? unreachable;
    const mdhd = parseMdhd(mdhd_box.payload) ?? unreachable;
    expect_eq(mdhd.durationMs(), 2000);
    expect_eq(std.mem.eql(u8, mdhd.language[0..], "eng"), true);

    const mdat = findBox(buf[0..], "mdat") ?? unreachable;
    expect_eq(std.mem.eql(u8, mdat.payload, "DATA"), true);
});

test("2. 64-bit largesize (size==1), uuid 16-byte extended type, and size==0 to-EOF boxes", () => {
    // Box 0: largesize (size32=1), type="free", largesize=20 (16B header + 4B payload)
    // Box 1: uuid box, size32=28 (8B header + 16B uuid + 4B payload)
    // Box 2: size32=0 ("mdat"), extends to end of stream (8B header + 6B payload = 14B)
    // total = 20 + 28 + 14 = 62
    let buf: [62]u8 = std.mem.zeroes([62]u8);

    // Box 0 at 0..20
    writeBoxHeader(buf[0..], 0, 1, "free");
    writeU64BE(buf[0..], 8, 20);
    writeTag(buf[0..], 16, "WIDE");

    // Box 1 at 20..48
    writeBoxHeader(buf[0..], 20, 28, "uuid");
    let i: usize = 0;
    while (i < 16) {
        buf[28 + i] = (0x10 + i) as u8;
        i += 1;
    }
    writeTag(buf[0..], 44, "XMP!");

    // Box 2 at 48..62 (size=0 -> extends to EOF)
    writeBoxHeader(buf[0..], 48, 0, "mdat");
    buf[56] = 'a'; buf[57] = 'b'; buf[58] = 'c'; buf[59] = 'd'; buf[60] = 'e'; buf[61] = 'f';

    let cur = BoxCursor.init(buf[0..]);

    const b0 = cur.next() ?? unreachable;
    expect_eq(b0.isType("free"), true);
    expect_eq(b0.header_size, 16);
    expect_eq(b0.size, 20);
    expect_eq(std.mem.eql(u8, b0.payload, "WIDE"), true);

    const b1 = cur.next() ?? unreachable;
    expect_eq(b1.isType("uuid"), true);
    expect_eq(b1.has_uuid, true);
    expect_eq(b1.header_size, 24);
    expect_eq(b1.uuid[0], 0x10);
    expect_eq(b1.uuid[15], 0x1F);
    expect_eq(std.mem.eql(u8, b1.payload, "XMP!"), true);

    const b2 = cur.next() ?? unreachable;
    expect_eq(b2.isType("mdat"), true);
    expect_eq(b2.size, 14);
    expect_eq(std.mem.eql(u8, b2.payload, "abcdef"), true);
    expect_eq(cur.remaining(), 0);
    expect_eq(cur.next() == null, true);
});

test("3. mvhd v1 64-bit duration calculation and zero-timescale guard", () => {
    let payload: [112]u8 = std.mem.zeroes([112]u8);
    payload[0] = 1; // version 1
    writeU64BE(payload[0..], 4, 1700000000); // creation_time
    writeU64BE(payload[0..], 12, 1700000100); // modification_time
    writeU32BE(payload[0..], 20, 90000); // timescale = 90kHz
    // 64-bit duration > 32-bit max: 5,400,000,000 ticks = 60,000 seconds = 60,000,000 ms
    writeU64BE(payload[0..], 24, 5_400_000_000);
    writeU32BE(payload[0..], 108, 3); // next_track_id

    const mvhd = parseMvhd(payload[0..]) ?? unreachable;
    expect_eq(mvhd.version, 1);
    expect_eq(mvhd.timescale, 90000);
    expect_eq(mvhd.duration, 5_400_000_000);
    expect_eq(mvhd.durationMs(), 60_000_000);
    expect_eq(mvhd.durationSecs() > 59999.9 && mvhd.durationSecs() < 60000.1, true);
    expect_eq(mvhd.next_track_id, 3);

    // Zero timescale returns 0 ms and 0.0 secs cleanly
    writeU32BE(payload[0..], 20, 0);
    const zero_ts = parseMvhd(payload[0..]) ?? unreachable;
    expect_eq(zero_ts.durationMs(), 0);
    expect_eq(zero_ts.durationSecs() == 0.0, true);
});

test("4. Nested container traversal through udta/meta FullBox to ilst child box", () => {
    // ilst: 8 header + 4 payload = 12
    // meta: 8 header + 4 FullBox ver/flags + 12 ilst = 24
    // udta: 8 header + 24 meta = 32
    // moov: 8 header + 32 udta = 40
    let buf: [40]u8 = std.mem.zeroes([40]u8);
    writeBoxHeader(buf[0..], 0, 40, "moov");
    writeBoxHeader(buf[0..], 8, 32, "udta");
    writeBoxHeader(buf[0..], 16, 24, "meta");
    // bytes 24..28 are meta's 4-byte FullBox version/flags (0)
    writeBoxHeader(buf[0..], 28, 12, "ilst");
    writeTag(buf[0..], 36, "META");

    const moov = findBox(buf[0..], "moov") ?? unreachable;
    expect_eq(moov.isContainer(), true);

    const ilst = findBoxPath(buf[0..], "moov/udta/meta/ilst") ?? unreachable;
    expect_eq(std.mem.eql(u8, ilst.payload, "META"), true);

    const meta = findBoxPath(buf[0..], "moov/udta/meta") ?? unreachable;
    let meta_kids = meta.children();
    const kid = meta_kids.next() ?? unreachable;
    expect_eq(kid.isType("ilst"), true);
    expect_eq(meta_kids.next() == null, true);
});

test("5. Truncated, overflowing, and malformed boxes fail cleanly without out-of-bounds reads", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);

    // Invalid size 5 (< 8)
    writeBoxHeader(buf[0..], 0, 5, "ftyp");
    let c1 = BoxCursor.init(buf[0..]);
    expect_eq(c1.next() == null, true);

    // Declared size > buffer
    writeBoxHeader(buf[0..], 0, 1024, "moov");
    let c2 = BoxCursor.init(buf[0..]);
    expect_eq(c2.next() == null, true);

    // largesize < 16 (invalid)
    writeBoxHeader(buf[0..], 0, 1, "mdat");
    writeU64BE(buf[0..], 8, 12);
    let c3 = BoxCursor.init(buf[0..]);
    expect_eq(c3.next() == null, true);

    // largesize > buffer (overflowing 64-bit size)
    writeBoxHeader(buf[0..], 0, 1, "mdat");
    writeU64BE(buf[0..], 8, 0xFFFF_FFFF_FFFF_FFF0);
    let c4 = BoxCursor.init(buf[0..]);
    expect_eq(c4.next() == null, true);

    // uuid box too small to contain 16-byte extended type (size = 16 < 24)
    writeBoxHeader(buf[0..], 0, 16, "uuid");
    let c5 = BoxCursor.init(buf[0..]);
    expect_eq(c5.next() == null, true);

    // Misaligned ftyp payload (10 bytes: 8 + 2 trailing bytes)
    expect_eq(parseFtyp(buf[0..10]) == null, true);

    // Unsupported mvhd version 2
    let bad_mvhd: [112]u8 = std.mem.zeroes([112]u8);
    bad_mvhd[0] = 2;
    expect_eq(parseMvhd(bad_mvhd[0..]) == null, true);
});

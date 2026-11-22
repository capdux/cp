// Source: cp/cap/test/test_id3_production_deep.cp (Production deep tests for cap:id3)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Id3v2Header,
    Id3v2Frame,
    Id3v1Tag,
    decodeSyncsafeU32,
    parseId3v2Header,
    Id3v2FrameIter,
    parseId3v1Tail,
} from "cap:media";

fn putSyncsafe(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 21) & 0x7F) as u8;
    buf[off + 1] = ((v >> 14) & 0x7F) as u8;
    buf[off + 2] = ((v >> 7) & 0x7F) as u8;
    buf[off + 3] = (v & 0x7F) as u8;
}

fn putU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn copyBytes(dst: *[]u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

test("1. decodeSyncsafeU32 decodes 28-bit values and rejects high-bit or short inputs", () => {
    const b0: [4]u8 = [0x00, 0x00, 0x02, 0x01]; // 2*128 + 1 = 257
    expect_eq(decodeSyncsafeU32(b0[0..4]) ?? unreachable, 257);

    const b_max: [4]u8 = [0x7F, 0x7F, 0x7F, 0x7F]; // 2^28 - 1 = 268435455
    expect_eq(decodeSyncsafeU32(b_max[0..4]) ?? unreachable, 268435455);

    const b_bad: [4]u8 = [0x00, 0x80, 0x00, 0x01]; // bit 7 set -> invalid
    expect_eq(decodeSyncsafeU32(b_bad[0..4]) == null, true);

    const b_short: [3]u8 = [0x00, 0x00, 0x01];
    expect_eq(decodeSyncsafeU32(b_short[0..3]) == null, true);
});

test("2. parseId3v2Header decodes v2.3 and v2.4 headers and computes totalBytes with/without footer", () => {
    let hbuf: [10]u8 = undefined;
    let s = hbuf[0..10];
    s[0] = 'I';
    s[1] = 'D';
    s[2] = '3';
    s[3] = 3;
    s[4] = 0;
    s[5] = 0;
    putSyncsafe(*s, 6, 512);

    const h3 = parseId3v2Header(s) ?? unreachable;
    expect_eq(h3.version_major, 3);
    expect_eq(h3.version_minor, 0);
    expect_eq(h3.tag_size, 512);
    expect_eq(h3.totalBytes(), 522);

    // v2.4 with footer flag (0x10) adds 10 bytes
    s[3] = 4;
    s[5] = 0x10;
    const h4 = parseId3v2Header(s) ?? unreachable;
    expect_eq(h4.version_major, 4);
    expect_eq(h4.totalBytes(), 532);

    // Non-ID3 magic rejected
    s[0] = 'X';
    expect_eq(parseId3v2Header(s) == null, true);
});

test("3. Id3v2FrameIter iterates v2.3 frames with big-endian sizes, extracts textContent, and stops on zero padding", () => {
    // 10 header + TIT2 (10 + 7) + TPE1 (10 + 7) + 6 zero padding = 50 bytes
    let buf: [50]u8 = undefined;
    let i: usize = 0;
    while (i < 50) {
        buf[i] = 0;
        i += 1;
    }
    let s = buf[0..50];
    copyBytes(*s, 0, "ID3");
    s[3] = 3; // v2.3
    s[4] = 0;
    s[5] = 0;
    putSyncsafe(*s, 6, 40);

    // Frame 0: TIT2 (encoding 0x00 + "CP Song" -> len 8? let's use "Hello\0" = 1 + 6 = 7)
    copyBytes(*s, 10, "TIT2");
    putU32BE(*s, 14, 7);
    s[20] = 0x00; // ISO-8859-1
    copyBytes(*s, 21, "Hello");
    s[26] = 0x00; // trailing NUL

    // Frame 1: TPE1 (encoding 0x03 UTF-8 + "Artist")
    copyBytes(*s, 27, "TPE1");
    putU32BE(*s, 31, 7);
    s[37] = 0x03; // UTF-8
    copyBytes(*s, 38, "Artist");
    // bytes 44..50 are 0x00 padding

    let it = Id3v2FrameIter.init(s);

    const f0 = it.next() ?? unreachable;
    expect_eq(f0.idEquals("TIT2"), true);
    expect_eq(f0.size, 7);
    const txt0 = f0.textContent() ?? unreachable;
    expect_eq(std.mem.eql(u8, txt0, "Hello"), true);

    const f1 = it.next() ?? unreachable;
    expect_eq(f1.idEquals("TPE1"), true);
    const txt1 = f1.textContent() ?? unreachable;
    expect_eq(std.mem.eql(u8, txt1, "Artist"), true);

    // Zero padding terminates iteration cleanly
    expect_eq(it.next() == null, true);
});

test("4. Id3v2FrameIter distinguishes v2.4 syncsafe frame sizes and guards against out-of-bounds frames", () => {
    // v2.4 frame with syncsafe size = 130 (0x00, 0x00, 0x01, 0x02)
    let buf: [150]u8 = undefined;
    let i: usize = 0;
    while (i < 150) {
        buf[i] = 0;
        i += 1;
    }
    let s = buf[0..150];
    copyBytes(*s, 0, "ID3");
    s[3] = 4; // v2.4
    putSyncsafe(*s, 6, 140);

    copyBytes(*s, 10, "TALB");
    putSyncsafe(*s, 14, 130); // syncsafe 130 = 0x00000102
    s[20] = 0x03;
    copyBytes(*s, 21, "AlbumV24");

    let it = Id3v2FrameIter.init(s);
    const f0 = it.next() ?? unreachable;
    expect_eq(f0.idEquals("TALB"), true);
    expect_eq(f0.size, 130);
    expect_eq(f0.payload.len, 130);
    const alb = f0.textContent() ?? unreachable;
    expect_eq(std.mem.eql(u8, alb, "AlbumV24"), true);
    expect_eq(it.next() == null, true);

    // Malformed v2.4 frame claiming size larger than buffer -> safely yields null
    putSyncsafe(*s, 14, 9999);
    let it_bad = Id3v2FrameIter.init(s);
    expect_eq(it_bad.next() == null, true);
});

test("5. parseId3v1Tail parses ID3v1.1 fields with track number and rejects missing TAG", () => {
    // 16 bytes fake audio + 128 bytes ID3v1.1 tail = 144 bytes
    let mp3: [144]u8 = undefined;
    let i: usize = 0;
    while (i < 144) {
        mp3[i] = 0;
        i += 1;
    }
    let s = mp3[0..144];
    const base: usize = 16;
    copyBytes(*s, base + 0, "TAG");
    copyBytes(*s, base + 3, "Bohemian Rhapsody   ");
    copyBytes(*s, base + 33, "Queen");
    copyBytes(*s, base + 63, "A Night at the Opera");
    copyBytes(*s, base + 93, "1975");
    s[base + 125] = 0x00; // ID3v1.1 marker
    s[base + 126] = 11;   // Track 11
    s[base + 127] = 17;   // Rock

    const tag = parseId3v1Tail(s) ?? unreachable;
    expect_eq(std.mem.eql(u8, tag.title, "Bohemian Rhapsody"), true);
    expect_eq(std.mem.eql(u8, tag.artist, "Queen"), true);
    expect_eq(std.mem.eql(u8, tag.album, "A Night at the Opera"), true);
    expect_eq(std.mem.eql(u8, tag.year, "1975"), true);
    expect_eq(tag.genre, 17);
    expect_eq(tag.track ?? 0, 11);

    // Without ID3v1.1 zero marker -> track is null
    s[base + 125] = '!';
    const tag_v10 = parseId3v1Tail(s) ?? unreachable;
    expect_eq(tag_v10.track == null, true);

    // Short buffer (< 128 bytes) -> null
    expect_eq(parseId3v1Tail(s[0..64]) == null, true);
});

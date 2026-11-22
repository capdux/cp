// Source: cp/cap/test/test_jxl_production_deep.cp (Production deep tests for cap:jxl)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    JxlContainerKind,
    JxlSizeHeader,
    JxlBox,
    JxlInfo,
    isJxlCodestream,
    isJxlContainer,
    detectJxlKind,
    isJxl,
    parseJxlSizeHeader,
    JxlBoxIter,
    parseJxlInfo,
} from "cap:image";

const JXL_SIG_BOX: [12]u8 = [
    0x00, 0x00, 0x00, 0x0C,
    0x4A, 0x58, 0x4C, 0x20,
    0x0D, 0x0A, 0x87, 0x0A,
];

fn writeU32Be(buf: []u8, pos: usize, v: u32) {
    buf[pos] = u8((v >> 24) & 0xFF);
    buf[pos + 1] = u8((v >> 16) & 0xFF);
    buf[pos + 2] = u8((v >> 8) & 0xFF);
    buf[pos + 3] = u8(v & 0xFF);
}

struct BitWriter {
    buf: [16]u8,
    bit_pos: usize = 0,

    static fn init(): BitWriter {
        return BitWriter {
            buf: std.mem.zeroes([16]u8),
            bit_pos: 0,
        };
    }

    fn writeBits(*_, val: u32, n: u32) {
        let i: u32 = 0;
        while (i < n) {
            const bit = u8((val >> u5(i)) & 1);
            const byte_idx = _.bit_pos / 8;
            const shift = u3(_.bit_pos % 8);
            _.buf[byte_idx] |= bit << shift;
            _.bit_pos += 1;
            i += 1;
        }
    }
}

fn appendBox(out: []u8, pos: usize, tag: []const u8, payload: []const u8): usize {
    const total = 8 + payload.len;
    writeU32Be(out, pos, u32(total));
    out[pos + 4] = tag[0];
    out[pos + 5] = tag[1];
    out[pos + 6] = tag[2];
    out[pos + 7] = tag[3];
    let i: usize = 0;
    while (i < payload.len) {
        out[pos + 8 + i] = payload[i];
        i += 1;
    }
    return pos + total;
}

test("1. Magic detection distinguishes JXL codestream, ISOBMFF container, and unknown", () => {
    const cs: [4]u8 = [0xFF, 0x0A, 0x00, 0x00];
    expect_eq(isJxlCodestream(cs[0..]), true);
    expect_eq(isJxlContainer(cs[0..]), false);
    expect_eq(isJxl(cs[0..]), true);
    expect_eq(detectJxlKind(cs[0..]) == JxlContainerKind.codestream, true);

    expect_eq(isJxlContainer(JXL_SIG_BOX[0..]), true);
    expect_eq(isJxlCodestream(JXL_SIG_BOX[0..]), false);
    expect_eq(isJxl(JXL_SIG_BOX[0..]), true);
    expect_eq(detectJxlKind(JXL_SIG_BOX[0..]) == JxlContainerKind.container, true);

    expect_eq(isJxl("not-jxl"), false);
    expect_eq(detectJxlKind("not-jxl") == JxlContainerKind.unknown, true);
});

test("2. parseJxlSizeHeader decodes small=1 codestreams with explicit ratio=0 and 16:9 ratio=5", () => {
    // small=1, ysize=(5+1)*8=48, ratio=0 (explicit), xsize=(7+1)*8=64 -> 1+5+3+5 = 14 bits
    let bw = BitWriter.init();
    bw.writeBits(1, 1); // small = 1
    bw.writeBits(5, 5); // height = 48
    bw.writeBits(0, 3); // ratio = 0
    bw.writeBits(7, 5); // width = 64

    let cs: [6]u8 = [0xFF, 0x0A, bw.buf[0], bw.buf[1], bw.buf[2], 0];
    const sz = parseJxlSizeHeader(cs[0..]) ?? unreachable;
    expect_eq(sz.width, 64);
    expect_eq(sz.height, 48);
    expect_eq(sz.is_small, true);
    expect_eq(sz.ratio, 0);
    expect_eq(sz.bits_read, 14);

    // small=1, ysize=(8+1)*8=72, ratio=5 (16:9 -> width = 72*16/9 = 128) -> 1+5+3 = 9 bits
    let bw2 = BitWriter.init();
    bw2.writeBits(1, 1);
    bw2.writeBits(8, 5); // height = 72
    bw2.writeBits(5, 3); // ratio = 5 (16:9)
    let cs2: [5]u8 = [0xFF, 0x0A, bw2.buf[0], bw2.buf[1], 0];
    const sz2 = parseJxlSizeHeader(cs2[0..]) ?? unreachable;
    expect_eq(sz2.width, 128);
    expect_eq(sz2.height, 72);
    expect_eq(sz2.ratio, 5);
    expect_eq(sz2.bits_read, 9);
});

test("3. parseJxlSizeHeader decodes small=0 large dimensions (1920x1080 explicit and 3840x2160 16:9)", () => {
    // 1920x1080: small=0, height sel=1 (13-bit, 1079), ratio=0, width sel=1 (13-bit, 1919)
    let bw = BitWriter.init();
    bw.writeBits(0, 1);     // small = 0
    bw.writeBits(1, 2);     // sel = 1 -> 13-bit
    bw.writeBits(1079, 13); // height = 1080
    bw.writeBits(0, 3);     // ratio = 0
    bw.writeBits(1, 2);     // sel = 1 -> 13-bit
    bw.writeBits(1919, 13); // width = 1920

    let cs: [10]u8 = [0xFF, 0x0A, bw.buf[0], bw.buf[1], bw.buf[2], bw.buf[3], bw.buf[4], bw.buf[5], 0, 0];
    const sz = parseJxlSizeHeader(cs[0..]) ?? unreachable;
    expect_eq(sz.width, 1920);
    expect_eq(sz.height, 1080);
    expect_eq(sz.is_small, false);
    expect_eq(sz.bits_read, 34);

    // 3840x2160 via ratio=5 (16:9): small=0, sel=1 (13-bit, 2159), ratio=5 -> 1+2+13+3 = 19 bits
    let bw2 = BitWriter.init();
    bw2.writeBits(0, 1);
    bw2.writeBits(1, 2);
    bw2.writeBits(2159, 13); // height = 2160
    bw2.writeBits(5, 3);     // 16:9 -> 3840
    let cs2: [6]u8 = [0xFF, 0x0A, bw2.buf[0], bw2.buf[1], bw2.buf[2], 0];
    const info = parseJxlInfo(cs2[0..]) ?? unreachable;
    expect_eq(info.kind == JxlContainerKind.codestream, true);
    expect_eq(info.width, 3840);
    expect_eq(info.height, 2160);
    expect_eq(info.is_partial, false);
});

test("4. ISOBMFF container parses jxlc/Exif/xml and partial jxlp + brob metadata", () => {
    // Build a 64x64 small=1 ratio=1 (1:1) codestream
    let bw = BitWriter.init();
    bw.writeBits(1, 1);
    bw.writeBits(7, 5); // height = 64
    bw.writeBits(1, 3); // 1:1 -> width = 64
    const cs: [4]u8 = [0xFF, 0x0A, bw.buf[0], bw.buf[1]];

    // Container A: JXL  + ftyp + jxlc + Exif + xml
    let c1: [128]u8 = std.mem.zeroes([128]u8);
    let k: usize = 0;
    while (k < 12) {
        c1[k] = JXL_SIG_BOX[k];
        k += 1;
    }
    let p: usize = 12;
    p = appendBox(c1[0..], p, "ftyp", "jxl \x00\x00\x00\x00jxl ");
    p = appendBox(c1[0..], p, "jxlc", cs[0..]);
    p = appendBox(c1[0..], p, "Exif", "\x00\x00\x00\x00MM\x00*");
    p = appendBox(c1[0..], p, "xml ", "<xmp/>");

    // Verify JxlBoxIter counts 5 boxes
    let bit = JxlBoxIter.init(c1[0..p]);
    let box_cnt: u32 = 0;
    while (b := bit.next()) {
        * = b;
        box_cnt += 1;
    }
    expect_eq(box_cnt, 5);
    expect_eq(bit.failed, false);

    const info1 = parseJxlInfo(c1[0..p]) ?? unreachable;
    expect_eq(info1.kind == JxlContainerKind.container, true);
    expect_eq(info1.width, 64);
    expect_eq(info1.height, 64);
    expect_eq(info1.has_exif, true);
    expect_eq(info1.has_xmp, true);
    expect_eq(info1.is_partial, false);

    // Container B: partial jxlp (chunk 0 + chunk 1) + brob(xml )
    let c2: [128]u8 = std.mem.zeroes([128]u8);
    k = 0;
    while (k < 12) {
        c2[k] = JXL_SIG_BOX[k];
        k += 1;
    }
    let p2: usize = 12;
    const jxlp0: [8]u8 = [0, 0, 0, 0, cs[0], cs[1], cs[2], cs[3]];
    const jxlp1: [6]u8 = [0x80, 0, 0, 1, 0x00, 0x00]; // last chunk flag set
    p2 = appendBox(c2[0..], p2, "jxlp", jxlp0[0..]);
    p2 = appendBox(c2[0..], p2, "jxlp", jxlp1[0..]);
    p2 = appendBox(c2[0..], p2, "brob", "xml compressed");

    const info2 = parseJxlInfo(c2[0..p2]) ?? unreachable;
    expect_eq(info2.kind == JxlContainerKind.container, true);
    expect_eq(info2.width, 64);
    expect_eq(info2.height, 64);
    expect_eq(info2.is_partial, true);
    expect_eq(info2.has_exif, false);
    expect_eq(info2.has_xmp, true);
});

test("5. Malformed and truncated input protection for codestream and ISOBMFF boxes", () => {
    // Truncated codestream (only 1 byte after 0xFF 0x0A -> cannot read 9 bits)
    const trunc_cs: [3]u8 = [0xFF, 0x0A, 0xFF];
    expect_eq(parseJxlSizeHeader(trunc_cs[0..]) == null, true);
    expect_eq(parseJxlInfo(trunc_cs[0..]) == null, true);

    // Container missing jxlc/jxlp
    expect_eq(parseJxlInfo(JXL_SIG_BOX[0..]) == null, true);

    // Container with out-of-bounds box size
    let bad_box: [24]u8 = std.mem.zeroes([24]u8);
    let i: usize = 0;
    while (i < 12) {
        bad_box[i] = JXL_SIG_BOX[i];
        i += 1;
    }
    writeU32Be(bad_box[0..], 12, 9999); // exceeds 24 bytes
    bad_box[16] = 'j'; bad_box[17] = 'x'; bad_box[18] = 'l'; bad_box[19] = 'c';
    expect_eq(parseJxlInfo(bad_box[0..]) == null, true);

    // Box with illegal size32 == 4 (< 8)
    writeU32Be(bad_box[0..], 12, 4);
    let it = JxlBoxIter.init(bad_box[0..]);
    * = it.next(); // signature box
    expect_eq(it.next() == null, true);
    expect_eq(it.failed, true);
});

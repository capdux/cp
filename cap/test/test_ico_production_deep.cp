// Source: cp/cap/test/test_ico_production_deep.cp (Production deep tests for cap:ico)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IcoKind,
    IcoHeader,
    IcoEntry,
    isIcoOrCur,
    parseIcoHeader,
    IcoEntryIter,
    findBestIcoEntry,
} from "cap:ico";

fn writeU16Le(buf: []u8, pos: usize, v: u16) {
    buf[pos] = u8(v & 0xFF);
    buf[pos + 1] = u8((v >> 8) & 0xFF);
}

fn writeU32Le(buf: []u8, pos: usize, v: u32) {
    buf[pos] = u8(v & 0xFF);
    buf[pos + 1] = u8((v >> 8) & 0xFF);
    buf[pos + 2] = u8((v >> 16) & 0xFF);
    buf[pos + 3] = u8((v >> 24) & 0xFF);
}

fn writeDirEntry(
    buf: []u8,
    idx: usize,
    w: u8,
    h: u8,
    colors: u8,
    planes_or_hx: u16,
    bpp_or_hy: u16,
    size: u32,
    offset: u32,
) {
    const p = 6 + idx * 16;
    buf[p] = w;
    buf[p + 1] = h;
    buf[p + 2] = colors;
    buf[p + 3] = 0;
    writeU16Le(buf, p + 4, planes_or_hx);
    writeU16Le(buf, p + 6, bpp_or_hy);
    writeU32Le(buf, p + 8, size);
    writeU32Le(buf, p + 12, offset);
}

test("1. parseIcoHeader and isIcoOrCur validate ICONDIR magic for ICO and CUR", () => {
    let ico_hdr: [6]u8 = [0, 0, 1, 0, 3, 0];
    expect_eq(isIcoOrCur(ico_hdr[0..]), true);
    const h1 = parseIcoHeader(ico_hdr[0..]) ?? unreachable;
    expect_eq(h1.kind == IcoKind.icon, true);
    expect_eq(h1.count, 3);

    let cur_hdr: [6]u8 = [0, 0, 2, 0, 1, 0];
    expect_eq(isIcoOrCur(cur_hdr[0..]), true);
    const h2 = parseIcoHeader(cur_hdr[0..]) ?? unreachable;
    expect_eq(h2.kind == IcoKind.cursor, true);
    expect_eq(h2.count, 1);

    // Invalid reserved != 0, invalid type = 3, truncated < 6 bytes
    let bad_res: [6]u8 = [1, 0, 1, 0, 1, 0];
    expect_eq(isIcoOrCur(bad_res[0..]), false);
    let bad_type: [6]u8 = [0, 0, 3, 0, 1, 0];
    expect_eq(isIcoOrCur(bad_type[0..]), false);
    let short_buf: [4]u8 = [0, 0, 1, 0];
    expect_eq(parseIcoHeader(short_buf[0..]) == null, true);
});

test("2. IcoEntryIter decodes 0->256 dimensions, PNG vs BMP signatures, and extracts exact payload slices", () => {
    // 6 header + 3*16 dir (54 bytes) + 12 BMP + 12 BMP + 16 PNG = 94 bytes
    let buf: [94]u8 = std.mem.zeroes([94]u8);
    writeU16Le(buf[0..], 0, 0);
    writeU16Le(buf[0..], 2, 1);
    writeU16Le(buf[0..], 4, 3);

    // Entry 0: 16x16 8bpp BMP at offset 54, len 12
    writeDirEntry(buf[0..], 0, 16, 16, 0, 1, 8, 12, 54);
    buf[54] = 40; // BITMAPINFOHEADER size = 40
    buf[55] = 0xAA;

    // Entry 1: 32x32 32bpp BMP at offset 66, len 12
    writeDirEntry(buf[0..], 1, 32, 32, 0, 1, 32, 12, 66);
    buf[66] = 40;
    buf[67] = 0xBB;

    // Entry 2: 256x256 (0x0) PNG at offset 78, len 16
    writeDirEntry(buf[0..], 2, 0, 0, 0, 1, 32, 16, 78);
    const png_sig: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    let k: usize = 0;
    while (k < 8) {
        buf[78 + k] = png_sig[k];
        k += 1;
    }
    buf[86] = 0xCC;

    let it = IcoEntryIter.init(buf[0..]);

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.width, 16);
    expect_eq(e0.height, 16);
    expect_eq(e0.bpp_or_hotspot_y, 8);
    expect_eq(e0.is_png, false);
    const p0 = e0.payload(buf[0..]) ?? unreachable;
    expect_eq(p0.len, 12);
    expect_eq(p0[1], 0xAA);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.width, 32);
    expect_eq(e1.height, 32);
    expect_eq(e1.bpp_or_hotspot_y, 32);
    expect_eq(e1.is_png, false);
    const p1 = e1.payload(buf[0..]) ?? unreachable;
    expect_eq(p1.len, 12);
    expect_eq(p1[1], 0xBB);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.width, 256);
    expect_eq(e2.height, 256);
    expect_eq(e2.is_png, true);
    const p2 = e2.payload(buf[0..]) ?? unreachable;
    expect_eq(p2.len, 16);
    expect_eq(p2[8], 0xCC);

    expect_eq(it.next() == null, true);
});

test("3. CUR cursor container preserves hotspot_x and hotspot_y coordinates", () => {
    // 6 header + 16 dir + 8 payload = 30 bytes
    let buf: [30]u8 = std.mem.zeroes([30]u8);
    writeU16Le(buf[0..], 0, 0);
    writeU16Le(buf[0..], 2, 2); // CUR
    writeU16Le(buf[0..], 4, 1);
    writeDirEntry(buf[0..], 0, 32, 32, 0, 7, 13, 8, 22);
    buf[22] = 0x42;

    const hdr = parseIcoHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.kind == IcoKind.cursor, true);

    let it = IcoEntryIter.init(buf[0..]);
    const cur = it.next() ?? unreachable;
    expect_eq(cur.planes_or_hotspot_x, 7);
    expect_eq(cur.bpp_or_hotspot_y, 13);
    const pay = cur.payload(buf[0..]) ?? unreachable;
    expect_eq(pay.len, 8);
    expect_eq(pay[0], 0x42);
});

test("4. findBestIcoEntry selects closest size and prefers PNG / higher bpp on equal size", () => {
    // 4 entries: 16x16 BMP, 32x32 8bpp BMP, 32x32 32bpp BMP, 32x32 PNG
    // 6 + 4*16 = 70 bytes header+dir, 4*12 = 48 bytes payloads => 118 bytes
    let buf: [118]u8 = std.mem.zeroes([118]u8);
    writeU16Le(buf[0..], 0, 0);
    writeU16Le(buf[0..], 2, 1);
    writeU16Le(buf[0..], 4, 4);

    writeDirEntry(buf[0..], 0, 16, 16, 0, 1, 32, 12, 70);
    writeDirEntry(buf[0..], 1, 32, 32, 0, 1, 8, 12, 82);
    writeDirEntry(buf[0..], 2, 32, 32, 0, 1, 32, 12, 94);
    writeDirEntry(buf[0..], 3, 32, 32, 0, 1, 32, 12, 106);

    // Mark entry 3 payload as PNG
    const png_sig: [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A];
    let k: usize = 0;
    while (k < 8) {
        buf[106 + k] = png_sig[k];
        k += 1;
    }

    // Target 16 -> exact 16x16
    const b16 = findBestIcoEntry(buf[0..], 16) ?? unreachable;
    expect_eq(b16.width, 16);
    expect_eq(b16.image_offset, 70);

    // Target 32 -> among three 32x32 entries, selects PNG (offset 106)
    const b32 = findBestIcoEntry(buf[0..], 32) ?? unreachable;
    expect_eq(b32.width, 32);
    expect_eq(b32.is_png, true);
    expect_eq(b32.image_offset, 106);

    // Target 24 -> equidistant from 16 and 32 -> prefers larger (32) for downsampling
    const b24 = findBestIcoEntry(buf[0..], 24) ?? unreachable;
    expect_eq(b24.width, 32);
    expect_eq(b24.is_png, true);
});

test("5. Malformed and overflow boundary protection: truncated dir, out-of-bounds offset, u32 overflow", () => {
    // Empty buffer
    expect_eq(findBestIcoEntry("", 32) == null, true);

    // Truncated directory (claims 2 entries = 38 bytes, only 22 bytes given)
    let trunc_dir: [22]u8 = std.mem.zeroes([22]u8);
    writeU16Le(trunc_dir[0..], 2, 1);
    writeU16Le(trunc_dir[0..], 4, 2);
    let it_trunc = IcoEntryIter.init(trunc_dir[0..]);
    expect_eq(it_trunc.next() == null, true);

    // Payload u32 addition overflow: offset = 0xFFFFFFF0, size = 0x20
    let ovf_buf: [22]u8 = std.mem.zeroes([22]u8);
    writeU16Le(ovf_buf[0..], 2, 1);
    writeU16Le(ovf_buf[0..], 4, 1);
    writeDirEntry(ovf_buf[0..], 0, 16, 16, 0, 1, 32, 0x20, 0xFFFFFFF0);
    let it_ovf = IcoEntryIter.init(ovf_buf[0..]);
    expect_eq(it_ovf.next() == null, true);

    // Direct IcoEntry.payload overflow protection
    const forged = IcoEntry {
        width: 16,
        height: 16,
        color_count: 0,
        planes_or_hotspot_x: 1,
        bpp_or_hotspot_y: 32,
        bytes_in_res: 0x20,
        image_offset: 0xFFFFFFF0,
        is_png: false,
    };
    expect_eq(forged.payload(ovf_buf[0..]) == null, true);
});

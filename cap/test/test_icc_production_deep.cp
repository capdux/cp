// Source: cp/cap/test/test_icc_production_deep.cp (Production deep tests for cap:icc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ICC_HEADER_SIZE,
    ICC_TAG_TABLE_OFFSET,
    ICC_TAG_ENTRY_SIZE,
    IccHeader,
    IccTagEntry,
    iccTagEquals,
    isIccProfile,
    parseIccHeader,
    IccTagIter,
    findIccTag,
} from "cap:gfx";

fn writeTag(buf: []u8, off: usize, tag: []const u8) {
    buf[off] = tag[0];
    buf[off + 1] = tag[1];
    buf[off + 2] = tag[2];
    buf[off + 3] = tag[3];
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn makeValidIccHeader(
    buf: []u8,
    profile_size: u32,
    dev_class: []const u8,
    color_space: []const u8,
    pcs: []const u8,
) {
    writeU32BE(buf, 0, profile_size);
    writeTag(buf, 4, "lcms");
    buf[8] = 4; // version_major = 4
    buf[9] = 0x32; // version_minor = 3, version_bugfix = 2
    writeTag(buf, 12, dev_class);
    writeTag(buf, 16, color_space);
    writeTag(buf, 20, pcs);
    writeTag(buf, 36, "acsp");
    writeU32BE(buf, 64, 1); // Relative Colorimetric
}

test("1. isIccProfile and parseIccHeader decode Display RGB and Printer CMYK profiles", () => {
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    makeValidIccHeader(buf[0..], 160, "mntr", "RGB ", "XYZ ");

    expect_eq(isIccProfile(buf[0..]), true);
    const h1 = parseIccHeader(buf[0..]) ?? unreachable;
    expect_eq(h1.profile_size, 160);
    expect_eq(iccTagEquals(h1.cmm_type, "lcms"), true);
    expect_eq(h1.version_major, 4);
    expect_eq(h1.version_minor, 3);
    expect_eq(h1.version_bugfix, 2);
    expect_eq(h1.isRgb(), true);
    expect_eq(h1.isCmyk(), false);
    expect_eq(h1.isDisplay(), true);
    expect_eq(iccTagEquals(h1.pcs, "XYZ "), true);
    expect_eq(h1.rendering_intent, 1);
    expect_eq(iccTagEquals(h1.signature, "acsp"), true);

    makeValidIccHeader(buf[0..], 128, "prtr", "CMYK", "Lab ");
    const h2 = parseIccHeader(buf[0..]) ?? unreachable;
    expect_eq(h2.isRgb(), false);
    expect_eq(h2.isCmyk(), true);
    expect_eq(h2.isDisplay(), false);
    expect_eq(iccTagEquals(h2.pcs, "Lab "), true);
});

test("2. IccTagIter and IccTagEntry.slice decode tag table entries with zero-copy payloads", () => {
    // 128 header + 4 tag_count + 3*12 entries (= 168) + payload data (32 bytes) = 200 bytes
    let buf: [200]u8 = std.mem.zeroes([200]u8);
    makeValidIccHeader(buf[0..], 200, "mntr", "RGB ", "XYZ ");
    writeU32BE(buf[0..], 128, 3);

    // Entry 0: "desc" at 168, size 4 ("sRGB")
    writeTag(buf[0..], 132, "desc");
    writeU32BE(buf[0..], 136, 168);
    writeU32BE(buf[0..], 140, 4);
    writeTag(buf[0..], 168, "sRGB");

    // Entry 1: "cprt" at 172, size 4 ("MIT ")
    writeTag(buf[0..], 144, "cprt");
    writeU32BE(buf[0..], 148, 172);
    writeU32BE(buf[0..], 152, 4);
    writeTag(buf[0..], 172, "MIT ");

    // Entry 2: "wtpt" at 176, size 8
    writeTag(buf[0..], 156, "wtpt");
    writeU32BE(buf[0..], 160, 176);
    writeU32BE(buf[0..], 164, 8);
    writeTag(buf[0..], 176, "XYZ ");

    let it = IccTagIter.init(buf[0..]);

    const t0 = it.next() ?? unreachable;
    expect_eq(iccTagEquals(t0.sig, "desc"), true);
    expect_eq(std.mem.eql(u8, t0.slice(buf[0..]) ?? "", "sRGB"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(iccTagEquals(t1.sig, "cprt"), true);
    expect_eq(std.mem.eql(u8, t1.slice(buf[0..]) ?? "", "MIT "), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(iccTagEquals(t2.sig, "wtpt"), true);
    expect_eq((t2.slice(buf[0..]) ?? "").len, 8);

    expect_eq(it.next() == null, true);
});

test("3. findIccTag locates matching tags and returns null for missing or malformed signatures", () => {
    let buf: [180]u8 = std.mem.zeroes([180]u8);
    makeValidIccHeader(buf[0..], 180, "mntr", "RGB ", "XYZ ");
    writeU32BE(buf[0..], 128, 2);

    writeTag(buf[0..], 132, "rXYZ");
    writeU32BE(buf[0..], 136, 156);
    writeU32BE(buf[0..], 140, 12);

    writeTag(buf[0..], 144, "gXYZ");
    writeU32BE(buf[0..], 148, 168);
    writeU32BE(buf[0..], 152, 12);

    const found = findIccTag(buf[0..], "gXYZ") ?? unreachable;
    expect_eq(found.offset, 168);
    expect_eq(found.size, 12);

    expect_eq(findIccTag(buf[0..], "bXYZ") == null, true);
    expect_eq(findIccTag(buf[0..], "BAD") == null, true);
});

test("4. parseIccHeader rejects short buffers, bad acsp signature, and invalid profile_size", () => {
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    expect_eq(isIccProfile(buf[0..127]), false);
    expect_eq(parseIccHeader(buf[0..127]) == null, true);

    // Wrong signature at offset 36
    makeValidIccHeader(buf[0..], 160, "mntr", "RGB ", "XYZ ");
    buf[39] = 'x';
    expect_eq(isIccProfile(buf[0..]), false);
    expect_eq(parseIccHeader(buf[0..]) == null, true);

    // profile_size < 128
    makeValidIccHeader(buf[0..], 100, "mntr", "RGB ", "XYZ ");
    expect_eq(parseIccHeader(buf[0..]) == null, true);

    // profile_size > data.len
    makeValidIccHeader(buf[0..], 512, "mntr", "RGB ", "XYZ ");
    expect_eq(parseIccHeader(buf[0..]) == null, true);
});

test("5. IccTagIter and IccTagEntry.slice guard against excessive tag_count and u32 offset+size overflow", () => {
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    makeValidIccHeader(buf[0..], 160, "mntr", "RGB ", "XYZ ");

    // Huge tag_count (0xFFFF_FFFF) exceeds available bytes -> rejected safely
    writeU32BE(buf[0..], 128, 0xFFFF_FFFF);
    let it_huge = IccTagIter.init(buf[0..]);
    expect_eq(it_huge.next() == null, true);

    // Single tag with u32 overflow in offset + size (0xFFFF_FFF0 + 32)
    writeU32BE(buf[0..], 128, 1);
    writeTag(buf[0..], 132, "desc");
    writeU32BE(buf[0..], 136, 0xFFFF_FFF0);
    writeU32BE(buf[0..], 140, 32);

    let it_ovf = IccTagIter.init(buf[0..]);
    const entry = it_ovf.next() ?? unreachable;
    expect_eq(entry.slice(buf[0..]) == null, true);

    // Tag whose offset + size exceeds buffer length
    writeU32BE(buf[0..], 136, 150);
    writeU32BE(buf[0..], 140, 20);
    let it_oob = IccTagIter.init(buf[0..]);
    const entry2 = it_oob.next() ?? unreachable;
    expect_eq(entry2.slice(buf[0..]) == null, true);
});

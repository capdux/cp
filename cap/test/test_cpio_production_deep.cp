// Source: cp/cap/test/test_cpio_production_deep.cp (Production deep tests for cap:cpio)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CPIO_MAGIC_NEWC,
    CPIO_MAGIC_CRC,
    CPIO_MAGIC_ODC,
    CPIO_NEWC_HEADER_SIZE,
    CPIO_ODC_HEADER_SIZE,
    CPIO_TRAILER,
    CpioFormat,
    CpioEntry,
    detectCpioFormat,
    isCpio,
    CpioIter,
    findCpioEntry,
    countCpioEntries,
} from "cap:cpio";

const HEX_CHARS: []const u8 = "0123456789ABCDEF";

fn writeHex8(out: []u8, pos: usize, v: u32) {
    let i: usize = 0;
    while (i < 8) {
        const shift = u5((7 - i) * 4);
        out[pos + i] = HEX_CHARS[usize((v >> shift) & 0xF)];
        i += 1;
    }
}

fn writeOctal(out: []u8, pos: usize, width: usize, val: u64) {
    let v = val;
    let i: usize = 0;
    while (i < width) {
        out[pos + width - 1 - i] = u8('0' + (v & 7));
        v = v >> 3;
        i += 1;
    }
}

fn copySlice(out: []u8, pos: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendNewcEntry(
    out: []u8,
    start: usize,
    magic: []const u8,
    ino: u32,
    mode: u32,
    uid: u32,
    gid: u32,
    mtime: u32,
    checksum: u32,
    name: []const u8,
    data: []const u8,
): usize {
    let p = copySlice(out, start, magic);
    writeHex8(out, p, ino); p += 8;
    writeHex8(out, p, mode); p += 8;
    writeHex8(out, p, uid); p += 8;
    writeHex8(out, p, gid); p += 8;
    writeHex8(out, p, 1); p += 8; // nlink
    writeHex8(out, p, mtime); p += 8;
    writeHex8(out, p, u32(data.len)); p += 8;
    writeHex8(out, p, 0); p += 8; // devmajor
    writeHex8(out, p, 0); p += 8; // devminor
    writeHex8(out, p, 0); p += 8; // rdevmajor
    writeHex8(out, p, 0); p += 8; // rdevminor
    writeHex8(out, p, u32(name.len + 1)); p += 8; // namesize incl NUL
    writeHex8(out, p, checksum); p += 8;

    p = copySlice(out, p, name);
    out[p] = 0;
    p += 1;
    while ((p - start) % 4 != 0) {
        out[p] = 0;
        p += 1;
    }
    p = copySlice(out, p, data);
    while ((p - start) % 4 != 0) {
        out[p] = 0;
        p += 1;
    }
    return p;
}

fn appendOdcEntry(
    out: []u8,
    start: usize,
    ino: u64,
    mode: u64,
    mtime: u64,
    name: []const u8,
    data: []const u8,
): usize {
    let p = copySlice(out, start, CPIO_MAGIC_ODC);
    writeOctal(out, p, 6, 0); p += 6; // dev
    writeOctal(out, p, 6, ino); p += 6;
    writeOctal(out, p, 6, mode); p += 6;
    writeOctal(out, p, 6, 1000); p += 6; // uid
    writeOctal(out, p, 6, 1000); p += 6; // gid
    writeOctal(out, p, 6, 1); p += 6; // nlink
    writeOctal(out, p, 6, 0); p += 6; // rdev
    writeOctal(out, p, 11, mtime); p += 11;
    writeOctal(out, p, 6, u64(name.len + 1)); p += 6;
    writeOctal(out, p, 11, u64(data.len)); p += 11;

    p = copySlice(out, p, name);
    out[p] = 0;
    p += 1;
    p = copySlice(out, p, data);
    return p;
}

test("1. newc multi-entry archive with dir, regular file, symlink, 4B alignment, and TRAILER!!!", () => {
    expect_eq(CPIO_NEWC_HEADER_SIZE, 110);
    expect_eq(CPIO_ODC_HEADER_SIZE, 76);

    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    let p: usize = 0;
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 1, 0x41ED, 0, 0, 1700000000, 0, "etc", "");
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 2, 0x81A4, 1000, 1000, 1700000001, 0, "etc/motd", "hello cpio!");
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 3, 0xA1FF, 0, 0, 1700000002, 0, "bin/sh", "busybox");
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 0, 0, 0, 0, 0, 0, CPIO_TRAILER, "");

    expect_eq(isCpio(buf[0..p]), true);
    expect_eq(detectCpioFormat(buf[0..p]) == CpioFormat.newc, true);

    let it = CpioIter.init(buf[0..p]);

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.format == CpioFormat.newc, true);
    expect_eq(e0.isDir(), true);
    expect_eq(e0.isRegular(), false);
    expect_eq(std.mem.eql(u8, e0.name, "etc"), true);
    expect_eq(e0.data.len, 0);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.isRegular(), true);
    expect_eq(e1.uid, 1000);
    expect_eq(e1.gid, 1000);
    expect_eq(e1.mtime, 1700000001);
    expect_eq(std.mem.eql(u8, e1.name, "etc/motd"), true);
    expect_eq(std.mem.eql(u8, e1.data, "hello cpio!"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.isSymlink(), true);
    expect_eq(std.mem.eql(u8, e2.name, "bin/sh"), true);
    expect_eq(std.mem.eql(u8, e2.data, "busybox"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.failed, false);
});

test("2. crc (070702) format parses checksum field accurately", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    let p: usize = 0;
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_CRC, 42, 0x81A4, 0, 0, 123456, 0x000001A4, "init", "ABCD");
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_CRC, 0, 0, 0, 0, 0, 0, CPIO_TRAILER, "");

    expect_eq(detectCpioFormat(buf[0..p]) == CpioFormat.crc, true);
    let it = CpioIter.init(buf[0..p]);
    const e = it.next() ?? unreachable;
    expect_eq(e.format == CpioFormat.crc, true);
    expect_eq(e.ino, 42);
    expect_eq(e.checksum, 0x000001A4);
    expect_eq(std.mem.eql(u8, e.data, "ABCD"), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.failed, false);
});

test("3. odc (070707) POSIX octal format parses without 4B padding", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    let p: usize = 0;
    p = appendOdcEntry(buf[0..], p, 7, 0x81A4, 1700000123, "a.txt", "12345");
    p = appendOdcEntry(buf[0..], p, 0, 0, 0, CPIO_TRAILER, "");

    expect_eq(detectCpioFormat(buf[0..p]) == CpioFormat.odc, true);
    let it = CpioIter.init(buf[0..p]);
    const e = it.next() ?? unreachable;
    expect_eq(e.format == CpioFormat.odc, true);
    expect_eq(e.ino, 7);
    expect_eq(e.isRegular(), true);
    expect_eq(e.mtime, 1700000123);
    expect_eq(std.mem.eql(u8, e.name, "a.txt"), true);
    expect_eq(std.mem.eql(u8, e.data, "12345"), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.failed, false);
});

test("4. findCpioEntry and countCpioEntries locate targets and exclude TRAILER!!!", () => {
    let buf: [1024]u8 = std.mem.zeroes([1024]u8);
    let p: usize = 0;
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 1, 0x81A4, 0, 0, 1, 0, "usr/a", "alpha");
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 2, 0x81A4, 0, 0, 2, 0, "usr/b", "beta");
    p = appendNewcEntry(buf[0..], p, CPIO_MAGIC_NEWC, 0, 0, 0, 0, 0, 0, CPIO_TRAILER, "");

    expect_eq(countCpioEntries(buf[0..p]), 2);
    const hit = findCpioEntry(buf[0..p], "usr/b") ?? unreachable;
    expect_eq(std.mem.eql(u8, hit.data, "beta"), true);
    expect_eq(findCpioEntry(buf[0..p], "usr/missing") == null, true);
    expect_eq(findCpioEntry(buf[0..p], CPIO_TRAILER) == null, true);
});

test("5. Malformed CPIO protection: bad magic, truncated header, invalid hex, out-of-bounds filesize", () => {
    expect_eq(isCpio("TAR\x00\x00\x00"), false);
    expect_eq(countCpioEntries(""), 0);

    // Truncated newc header (< 110 bytes)
    let trunc: [60]u8 = std.mem.zeroes([60]u8);
    * = copySlice(trunc[0..], 0, CPIO_MAGIC_NEWC);
    let it_t = CpioIter.init(trunc[0..]);
    expect_eq(it_t.next() == null, true);
    expect_eq(it_t.failed, true);

    // Invalid hex character in header
    let bad_hex: [128]u8 = std.mem.zeroes([128]u8);
    * = appendNewcEntry(bad_hex[0..], 0, CPIO_MAGIC_NEWC, 1, 0x81A4, 0, 0, 0, 0, "x", "y");
    bad_hex[10] = 'Z'; // corrupt hex digit in c_ino
    let it_h = CpioIter.init(bad_hex[0..]);
    expect_eq(it_h.next() == null, true);
    expect_eq(it_h.failed, true);

    // Out-of-bounds filesize (claims 0x00010000 bytes in 128-byte buffer)
    let oob: [128]u8 = std.mem.zeroes([128]u8);
    * = appendNewcEntry(oob[0..], 0, CPIO_MAGIC_NEWC, 1, 0x81A4, 0, 0, 0, 0, "x", "y");
    writeHex8(oob[0..], 54, 0x00010000);
    let it_o = CpioIter.init(oob[0..]);
    expect_eq(it_o.next() == null, true);
    expect_eq(it_o.failed, true);
});

// Source: cp/cap/test/test_rar_production_deep.cp (Production deep tests for cap:rar)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RAR4_SIGNATURE,
    RAR5_SIGNATURE,
    RAR_VER_4,
    RAR_VER_5,
    RAR5_HEAD_MAIN,
    RAR5_HEAD_FILE,
    RAR5_HEAD_SERVICE,
    RAR5_HEAD_CRYPT,
    RAR5_HEAD_ENDARC,
    HFL_EXTRA,
    HFL_DATA,
    HFL_SKIP_IF_UNKNOWN,
    HFL_SPLIT_BEFORE,
    HFL_SPLIT_AFTER,
    FILE_FLAG_DIR,
    FILE_FLAG_UTIME,
    FILE_FLAG_CRC32,
    FILE_FLAG_UNP_UNKNOWN,
    RAR4_TYPE_MAIN,
    RAR4_TYPE_FILE,
    RAR4_TYPE_ENDARC,
    RAR4_FLAG_LONG_BLOCK,
    detectRarVersion,
    isRar,
    rarCrc32,
    Rar5Vint,
    decodeRar5Vint,
    encodeRar5Vint,
    Rar5Block,
    Rar5BlockIter,
    iterRar5Blocks,
    Rar5FileEntry,
    parseRar5FileEntry,
    Rar4Block,
    Rar4BlockIter,
    iterRar4Blocks,
} from "cap:rar";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

// Builds a complete RAR5 block at `buf[off..]` and returns total bytes written.
fn writeRar5Block(
    buf: []u8,
    off: usize,
    htype: u64,
    flags: u64,
    body: []const u8,
    extra: []const u8,
    data: []const u8,
): usize {
    let hdr_tmp: [256]u8 = std.mem.zeroes([256]u8);
    let hpos: usize = 0;
    hpos += encodeRar5Vint(htype, hdr_tmp[hpos..]) ?? unreachable;
    hpos += encodeRar5Vint(flags, hdr_tmp[hpos..]) ?? unreachable;
    if ((flags & HFL_EXTRA) != 0) {
        hpos += encodeRar5Vint(extra.len as u64, hdr_tmp[hpos..]) ?? unreachable;
    }
    if ((flags & HFL_DATA) != 0) {
        hpos += encodeRar5Vint(data.len as u64, hdr_tmp[hpos..]) ?? unreachable;
    }
    copyBytes(hdr_tmp[0..], hpos, body);
    hpos += body.len;
    if ((flags & HFL_EXTRA) != 0) {
        copyBytes(hdr_tmp[0..], hpos, extra);
        hpos += extra.len;
    }

    let pos = off + 4; // reserve 4B for CRC32
    pos += encodeRar5Vint(hpos as u64, buf[pos..]) ?? unreachable;
    copyBytes(buf, pos, hdr_tmp[0..hpos]);
    pos += hpos;

    const crc = rarCrc32(buf[(off + 4)..pos]);
    putU32Le(buf, off, crc);

    if ((flags & HFL_DATA) != 0) {
        copyBytes(buf, pos, data);
        pos += data.len;
    }
    return pos - off;
}

// Builds a complete RAR4 block at `buf[off..]` and returns total bytes written.
fn writeRar4Block(
    buf: []u8,
    off: usize,
    htype: u8,
    flags: u16,
    body: []const u8,
    data: []const u8,
): usize {
    const has_add = (flags & RAR4_FLAG_LONG_BLOCK) != 0 || htype == RAR4_TYPE_FILE;
    let hdr_len: usize = (7 as usize) + body.len;
    if (has_add) {
        hdr_len = (11 as usize) + body.len;
    }
    buf[off + 2] = htype;
    putU16Le(buf, off + 3, flags);
    putU16Le(buf, off + 5, hdr_len as u16);
    if (has_add) {
        putU32Le(buf, off + 7, data.len as u32);
        copyBytes(buf, off + 11, body);
    } else {
        copyBytes(buf, off + 7, body);
    }
    const crc16 = (rarCrc32(buf[(off + 2)..(off + hdr_len)]) & 0xFFFF) as u16;
    putU16Le(buf, off, crc16);

    if (has_add && data.len > 0) {
        copyBytes(buf, off + hdr_len, data);
    }
    if (has_add) {
        return hdr_len + data.len;
    }
    return hdr_len;
}

test("1. RAR4 / RAR5 signature detection, IEEE CRC32 & 7-bit Vint 1..10B roundtrip", () => {
    expect_eq(RAR5_HEAD_SERVICE, 3 as u64);
    expect_eq(RAR5_HEAD_CRYPT, 4 as u64);
    expect_eq(HFL_SKIP_IF_UNKNOWN, 0x0004 as u64);
    expect_eq(HFL_SPLIT_BEFORE, 0x0008 as u64);
    expect_eq(HFL_SPLIT_AFTER, 0x0010 as u64);
    expect_eq(FILE_FLAG_UNP_UNKNOWN, 0x0008 as u64);

    // Signatures
    expect_eq(isRar(RAR4_SIGNATURE[0..]), true);
    expect_eq(detectRarVersion(RAR4_SIGNATURE[0..]) ?? unreachable, RAR_VER_4);
    expect_eq(isRar(RAR5_SIGNATURE[0..]), true);
    expect_eq(detectRarVersion(RAR5_SIGNATURE[0..]) ?? unreachable, RAR_VER_5);
    expect_eq(isRar("PK\x03\x04\x14\x00\x00\x00"), false);

    // Standard IEEE CRC32 test vector: "123456789" -> 0xCBF43926
    expect_eq(rarCrc32("123456789"), 0xCBF43926 as u32);

    // Vint roundtrip across 1B, 2B, 5B, 10B (u64::MAX)
    let vbuf: [16]u8 = std.mem.zeroes([16]u8);
    const n1 = encodeRar5Vint(0, vbuf[0..]) ?? unreachable;
    expect_eq(n1, 1 as usize);
    const d1 = decodeRar5Vint(vbuf[0..n1]) ?? unreachable;
    expect_eq(d1.value, 0 as u64);
    expect_eq(d1.consumed, 1 as usize);

    const n2 = encodeRar5Vint(300, vbuf[0..]) ?? unreachable;
    expect_eq(n2, 2 as usize);
    const d2 = decodeRar5Vint(vbuf[0..n2]) ?? unreachable;
    expect_eq(d2.value, 300 as u64);

    const max_u64: u64 = 0xFFFFFFFFFFFFFFFF;
    const n10 = encodeRar5Vint(max_u64, vbuf[0..]) ?? unreachable;
    expect_eq(n10, 10 as usize);
    const d10 = decodeRar5Vint(vbuf[0..n10]) ?? unreachable;
    expect_eq(d10.value, max_u64);
    expect_eq(d10.consumed, 10 as usize);

    // Overflowing 10th byte (0x02) & truncated varint rejected
    vbuf[9] = 0x02;
    expect_eq(decodeRar5Vint(vbuf[0..10]) == null, true);
    expect_eq(decodeRar5Vint(vbuf[0..2]) == null, true); // continuation bit set at end
});

test("2. Full RAR5 archive (Main + File[Store+CRC32+mtime+extra] + Dir + EndArc) CRC32 & zero-copy parse", () => {
    let arc: [512]u8 = std.mem.zeroes([512]u8);
    let pos: usize = 0;
    copyBytes(arc[0..], 0, RAR5_SIGNATURE[0..]);
    pos += 8;

    // Block 0: Main Header
    let main_body: [1]u8 = [0]; // archive flags = 0
    pos += writeRar5Block(arc[0..], pos, RAR5_HEAD_MAIN, 0, main_body[0..], "", "");

    // Block 1: File Header ("hello.txt", Store method=0, solid=false, mtime + crc32 + extra_area)
    const file_payload = "Hello RAR5 World!";
    const payload_crc = rarCrc32(file_payload);
    let fbody: [64]u8 = std.mem.zeroes([64]u8);
    let fp: usize = 0;
    fp += encodeRar5Vint(FILE_FLAG_UTIME | FILE_FLAG_CRC32, fbody[fp..]) ?? unreachable;
    fp += encodeRar5Vint(file_payload.len as u64, fbody[fp..]) ?? unreachable; // unpacked_size
    fp += encodeRar5Vint(0o644, fbody[fp..]) ?? unreachable; // attributes
    putU32Le(fbody[0..], fp, 1700000000); // mtime
    fp += 4;
    putU32Le(fbody[0..], fp, payload_crc); // data_crc32
    fp += 4;
    fp += encodeRar5Vint(0, fbody[fp..]) ?? unreachable; // comp_info: version=0, solid=0, method=0 (Store)
    fp += encodeRar5Vint(1, fbody[fp..]) ?? unreachable; // host_os = Unix (1)
    fp += encodeRar5Vint(9, fbody[fp..]) ?? unreachable; // name_len = 9
    copyBytes(fbody[0..], fp, "hello.txt");
    fp += 9;

    const extra_bytes = "\x01\x02\x03\x04";
    pos += writeRar5Block(arc[0..], pos, RAR5_HEAD_FILE, HFL_EXTRA | HFL_DATA, fbody[0..fp], extra_bytes, file_payload);

    // Block 2: Directory entry ("docs", solid flag set in comp_info for flag coverage)
    let dbody: [32]u8 = std.mem.zeroes([32]u8);
    let dp: usize = 0;
    dp += encodeRar5Vint(FILE_FLAG_DIR, dbody[dp..]) ?? unreachable;
    dp += encodeRar5Vint(0, dbody[dp..]) ?? unreachable;
    dp += encodeRar5Vint(0o755, dbody[dp..]) ?? unreachable;
    dp += encodeRar5Vint(0x40 | (3 << 7), dbody[dp..]) ?? unreachable; // solid=1, method=3 (Normal)
    dp += encodeRar5Vint(1, dbody[dp..]) ?? unreachable;
    dp += encodeRar5Vint(4, dbody[dp..]) ?? unreachable;
    copyBytes(dbody[0..], dp, "docs");
    dp += 4;
    pos += writeRar5Block(arc[0..], pos, RAR5_HEAD_FILE, 0, dbody[0..dp], "", "");

    // Block 3: EndArc
    let end_body: [1]u8 = [0];
    pos += writeRar5Block(arc[0..], pos, RAR5_HEAD_ENDARC, 0, end_body[0..], "", "");

    let it = iterRar5Blocks(arc[0..pos]);

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.header_type, RAR5_HEAD_MAIN);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.header_type, RAR5_HEAD_FILE);
    expect_eq(b1.extra_area.len, 4 as usize);
    expect_eq(eqStr(b1.data_area, file_payload), true);

    const fe = parseRar5FileEntry(b1) ?? unreachable;
    expect_eq(fe.isDir(), false);
    expect_eq(fe.hasMtime(), true);
    expect_eq(fe.hasCrc32(), true);
    expect_eq(fe.mtime, 1700000000 as u32);
    expect_eq(fe.data_crc32, payload_crc);
    expect_eq(fe.unpacked_size, file_payload.len as u64);
    expect_eq(fe.compMethod(), 0 as u8);
    expect_eq(fe.isSolid(), false);
    expect_eq(eqStr(fe.name, "hello.txt"), true);
    expect_eq(eqStr(fe.data, file_payload), true);

    const b2 = it.next() ?? unreachable;
    const de = parseRar5FileEntry(b2) ?? unreachable;
    expect_eq(de.isDir(), true);
    expect_eq(de.hasMtime(), false);
    expect_eq(de.hasCrc32(), false);
    expect_eq(de.isSolid(), true);
    expect_eq(de.compMethod(), 3 as u8);
    expect_eq(eqStr(de.name, "docs"), true);

    const b3 = it.next() ?? unreachable;
    expect_eq(b3.header_type, RAR5_HEAD_ENDARC);

    // Stops cleanly after EndArc
    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("3. RAR4 archive (Main + File + EndArc) low-16-bit CRC32 verification & iteration", () => {
    let arc: [256]u8 = std.mem.zeroes([256]u8);
    let pos: usize = 0;
    copyBytes(arc[0..], 0, RAR4_SIGNATURE[0..]);
    pos += 7;

    let main_body: [6]u8 = [0; 6];
    pos += writeRar4Block(arc[0..], pos, RAR4_TYPE_MAIN, 0, main_body[0..], "");

    const file_meta = "META";
    const file_data = "RAR4_PAYLOAD";
    pos += writeRar4Block(arc[0..], pos, RAR4_TYPE_FILE, RAR4_FLAG_LONG_BLOCK, file_meta, file_data);

    pos += writeRar4Block(arc[0..], pos, RAR4_TYPE_ENDARC, 0, "", "");

    let it = iterRar4Blocks(arc[0..pos]);

    const r0 = it.next() ?? unreachable;
    expect_eq(r0.head_type, RAR4_TYPE_MAIN);
    expect_eq(r0.header_body.len, 6 as usize);
    expect_eq(r0.data_area.len, 0 as usize);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.head_type, RAR4_TYPE_FILE);
    expect_eq(r1.add_size, file_data.len as u32);
    expect_eq(eqStr(r1.header_body, file_meta), true);
    expect_eq(eqStr(r1.data_area, file_data), true);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.head_type, RAR4_TYPE_ENDARC);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("4. Malformed RAR5 rejection (bad signature, bad CRC32, out-of-bounds data_area, truncated)", () => {
    // 1. Bad signature
    let bad_it = iterRar5Blocks("NOT_RAR5_ARCHIVE");
    expect_eq(bad_it.hasError(), true);
    expect_eq(bad_it.next() == null, true);

    // 2. Corrupted block CRC32
    let arc: [128]u8 = std.mem.zeroes([128]u8);
    copyBytes(arc[0..], 0, RAR5_SIGNATURE[0..]);
    let main_body: [1]u8 = [0];
    const blen = writeRar5Block(arc[0..], 8, RAR5_HEAD_MAIN, 0, main_body[0..], "", "");
    arc[8] = arc[8] ^ 0xFF; // corrupt CRC32
    let it_crc = iterRar5Blocks(arc[0..(8 + blen)]);
    expect_eq(it_crc.next() == null, true);
    expect_eq(it_crc.hasError(), true);

    // 3. Truncated data_area (header claims 10 bytes of data, archive cut short)
    let arc2: [128]u8 = std.mem.zeroes([128]u8);
    copyBytes(arc2[0..], 0, RAR5_SIGNATURE[0..]);
    const blen2 = writeRar5Block(arc2[0..], 8, RAR5_HEAD_FILE, HFL_DATA, "\x00", "", "0123456789");
    let it_trunc = iterRar5Blocks(arc2[0..(8 + blen2 - 3)]);
    expect_eq(it_trunc.next() == null, true);
    expect_eq(it_trunc.hasError(), true);
});

test("5. Malformed RAR4 rejection (bad signature, bad CRC16, truncated data_area, non-file to parseRar5FileEntry)", () => {
    let bad4 = iterRar4Blocks("NOT_RAR4");
    expect_eq(bad4.hasError(), true);
    expect_eq(bad4.next() == null, true);

    let arc: [128]u8 = std.mem.zeroes([128]u8);
    copyBytes(arc[0..], 0, RAR4_SIGNATURE[0..]);
    const blen = writeRar4Block(arc[0..], 7, RAR4_TYPE_FILE, RAR4_FLAG_LONG_BLOCK, "AB", "123456");

    // Corrupt CRC16
    arc[7] = arc[7] ^ 0x01;
    let it_crc = iterRar4Blocks(arc[0..(7 + blen)]);
    expect_eq(it_crc.next() == null, true);
    expect_eq(it_crc.hasError(), true);

    // Restore CRC16, truncate data_area
    arc[7] = arc[7] ^ 0x01;
    let it_trunc = iterRar4Blocks(arc[0..(7 + blen - 2)]);
    expect_eq(it_trunc.next() == null, true);
    expect_eq(it_trunc.hasError(), true);

    // parseRar5FileEntry rejects non-file block
    const non_file = Rar5Block{
        .crc32 = 0,
        .header_type = RAR5_HEAD_MAIN,
        .flags = 0,
        .body = "",
        .extra_area = "",
        .data_area = "",
        .consumed = 0,
    };
    expect_eq(parseRar5FileEntry(non_file) == null, true);
});

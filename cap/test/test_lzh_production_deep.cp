// Source: cp/cap/test/test_lzh_production_deep.cp (Production deep tests for cap:lzh)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LZH_METHOD_LH0,
    LZH_METHOD_LH4,
    LZH_METHOD_LH5,
    LZH_METHOD_LH6,
    LZH_METHOD_LH7,
    LZH_METHOD_LHD,
    LEVEL_0,
    LEVEL_1,
    LEVEL_2,
    EXT_COMMON_CRC,
    EXT_FILENAME,
    EXT_DIRECTORY,
    EXT_UNIX_PERM,
    EXT_UNIX_GID_UID,
    EXT_UNIX_MTIME,
    lzhHeaderSum,
    lzhCrc16,
    isLzhMethod,
    isLzh,
    LzhExtHeader,
    LzhExtIter,
    LzhEntry,
    parseLzhEntry,
    LzhEntryIter,
    iterLzhEntries,
} from "cap:lzh";

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

// Writes a Level 0 entry at buf[off..] and returns total bytes written.
fn writeLevel0(buf: []u8, off: usize, method: []const u8, name: []const u8, payload: []const u8): usize {
    const hdr_size: usize = (22 as usize) + name.len; // 20 + 1(name_len) + name + 2(crc16) - 2 = 22 + name.len? Wait:
    // Bytes 2..(2+hdr_size): method(5)+comp(4)+orig(4)+mtime(4)+attr(1)+level(1)+nlen(1)+name(N)+crc(2) = 22 + N
    buf[off] = hdr_size as u8;
    copyBytes(buf, off + 2, method);
    putU32Le(buf, off + 7, payload.len as u32);
    putU32Le(buf, off + 11, payload.len as u32);
    putU32Le(buf, off + 15, 0x58210000);
    buf[off + 19] = 0x20;
    buf[off + 20] = LEVEL_0;
    buf[off + 21] = name.len as u8;
    copyBytes(buf, off + 22, name);
    putU16Le(buf, off + 22 + name.len, lzhCrc16(payload));
    buf[off + 1] = lzhHeaderSum(buf[(off + 2)..(off + 2 + hdr_size)]);
    const data_off = off + 2 + hdr_size;
    copyBytes(buf, data_off, payload);
    return 2 + hdr_size + payload.len;
}

// Writes a Level 1 entry with EXT_DIRECTORY + EXT_UNIX_PERM extended headers.
fn writeLevel1WithExts(
    buf: []u8,
    off: usize,
    method: []const u8,
    name: []const u8,
    dir: []const u8,
    perm: u16,
    comp_data: []const u8,
    orig_size: u32,
): usize {
    // Ext 1: EXT_DIRECTORY (1B type + dir.len + 2B next_size)
    const ext1_sz: usize = (3 as usize) + dir.len;
    // Ext 2: EXT_UNIX_PERM (1B type + 2B perm + 2B next_size = 5)
    const ext2_sz: usize = 5;
    const ext_total = ext1_sz + ext2_sz;

    // Base header bytes 2..(2+hdr_size): 20(up to nlen) + name.len + 2(crc) + 1(os_id) + 2(first_ext_size) = 25 + name.len
    const hdr_size: usize = (25 as usize) + name.len;
    buf[off] = hdr_size as u8;
    copyBytes(buf, off + 2, method);
    // Level 1 compressed_size includes ext_total!
    putU32Le(buf, off + 7, (comp_data.len + ext_total) as u32);
    putU32Le(buf, off + 11, orig_size);
    putU32Le(buf, off + 15, 0x58211111);
    buf[off + 19] = 0x20;
    buf[off + 20] = LEVEL_1;
    buf[off + 21] = name.len as u8;
    copyBytes(buf, off + 22, name);
    putU16Le(buf, off + 22 + name.len, 0xBEEF); // data crc16
    buf[off + 24 + name.len] = 0x55; // 'U' Unix
    putU16Le(buf, off + 25 + name.len, ext1_sz as u16); // first_ext_size
    buf[off + 1] = lzhHeaderSum(buf[(off + 2)..(off + 2 + hdr_size)]);

    let pos = off + 2 + hdr_size;
    // Write Ext 1: EXT_DIRECTORY
    buf[pos] = EXT_DIRECTORY;
    copyBytes(buf, pos + 1, dir);
    putU16Le(buf, pos + 1 + dir.len, ext2_sz as u16);
    pos += ext1_sz;

    // Write Ext 2: EXT_UNIX_PERM
    buf[pos] = EXT_UNIX_PERM;
    putU16Le(buf, pos + 1, perm);
    putU16Le(buf, pos + 3, 0); // end of ext chain
    pos += ext2_sz;

    copyBytes(buf, pos, comp_data);
    pos += comp_data.len;
    return pos - off;
}

// Writes a Level 2 entry with EXT_COMMON_CRC + EXT_FILENAME.
fn writeLevel2(
    buf: []u8,
    off: usize,
    method: []const u8,
    name: []const u8,
    comp_data: []const u8,
    orig_size: u32,
): usize {
    const ext_crc_sz: usize = 5; // 1B type + 2B crc + 2B next_size
    const ext_name_sz: usize = (3 as usize) + name.len;
    const total_hdr_size: usize = (26 as usize) + ext_crc_sz + ext_name_sz;

    putU16Le(buf, off, total_hdr_size as u16);
    copyBytes(buf, off + 2, method);
    putU32Le(buf, off + 7, comp_data.len as u32);
    putU32Le(buf, off + 11, orig_size);
    putU32Le(buf, off + 15, 1700000123); // Unix timestamp
    buf[off + 19] = 0;
    buf[off + 20] = LEVEL_2;
    putU16Le(buf, off + 21, lzhCrc16(comp_data));
    buf[off + 23] = 0x55; // 'U'
    putU16Le(buf, off + 24, ext_crc_sz as u16); // first_ext_size

    let pos = off + 26;
    // Ext 0: EXT_COMMON_CRC (placeholder 0x0000 at pos+1..pos+3)
    const crc_field_off = pos + 1;
    buf[pos] = EXT_COMMON_CRC;
    putU16Le(buf, crc_field_off, 0);
    putU16Le(buf, pos + 3, ext_name_sz as u16);
    pos += ext_crc_sz;

    // Ext 1: EXT_FILENAME
    buf[pos] = EXT_FILENAME;
    copyBytes(buf, pos + 1, name);
    putU16Le(buf, pos + 1 + name.len, 0);
    pos += ext_name_sz;

    // Compute and patch Level 2 header CRC-16/ARC
    const hcrc = lzhCrc16(buf[off..(off + total_hdr_size)]);
    putU16Le(buf, crc_field_off, hcrc);

    copyBytes(buf, off + total_hdr_size, comp_data);
    return total_hdr_size + comp_data.len;
}

test("1. CRC-16/ARC standard vector (0xBB3D) & Level 0 (-lh0-) header sum + data CRC verification", () => {
    expect_eq(isLzhMethod(LZH_METHOD_LH0), true);
    expect_eq(isLzhMethod(LZH_METHOD_LH4), true);
    expect_eq(isLzhMethod(LZH_METHOD_LH5), true);
    expect_eq(isLzhMethod(LZH_METHOD_LH6), true);
    expect_eq(isLzhMethod(LZH_METHOD_LH7), true);
    expect_eq(isLzhMethod(LZH_METHOD_LHD), true);
    expect_eq(EXT_UNIX_GID_UID, 0x51 as u8);
    expect_eq(EXT_UNIX_MTIME, 0x54 as u8);

    // Standard CRC-16/ARC check vector: "123456789" -> 0xBB3D
    expect_eq(lzhCrc16("123456789"), 0xBB3D as u16);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const payload = "123456789";
    const n = writeLevel0(buf[0..], 0, LZH_METHOD_LH0, "hello.txt", payload);

    expect_eq(isLzh(buf[0..n]), true);
    const ent = parseLzhEntry(buf[0..n]) ?? unreachable;
    expect_eq(ent.level, LEVEL_0);
    expect_eq(ent.isStored(), true);
    expect_eq(ent.isDir(), false);
    expect_eq(ent.compressed_size, 9 as u32);
    expect_eq(ent.original_size, 9 as u32);
    expect_eq(ent.crc16, 0xBB3D as u16);
    expect_eq(eqStr(ent.name, "hello.txt"), true);
    expect_eq(eqStr(ent.data, payload), true);
    expect_eq(ent.verifyDataCrc(), true);
    expect_eq(ent.consumed, n);
});

test("2. Level 1 (-lh5-) with EXT_DIRECTORY & EXT_UNIX_PERM extended header chain", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const comp = "\xAA\xBB\xCC\xDD";
    const n = writeLevel1WithExts(buf[0..], 0, LZH_METHOD_LH5, "main.cp", "src/app/", 0o755, comp, 1024);

    const ent = parseLzhEntry(buf[0..n]) ?? unreachable;
    expect_eq(ent.level, LEVEL_1);
    expect_eq(ent.isStored(), false);
    expect_eq(ent.compressed_size, 4 as u32); // ext header bytes stripped from skip_size
    expect_eq(ent.original_size, 1024 as u32);
    expect_eq(ent.os_id, 0x55 as u8);
    expect_eq(eqStr(ent.name, "main.cp"), true);
    expect_eq(eqStr(ent.dir_name, "src/app/"), true);
    expect_eq(eqStr(ent.data, comp), true);
    expect_eq(ent.verifyDataCrc(), false); // not stored (-lh5-)

    // Walk extended header chain directly
    let eit = ent.extHeaders();
    const e0 = eit.next() ?? unreachable;
    expect_eq(e0.ext_type, EXT_DIRECTORY);
    expect_eq(eqStr(e0.data, "src/app/"), true);

    const e1 = eit.next() ?? unreachable;
    expect_eq(e1.ext_type, EXT_UNIX_PERM);
    expect_eq(e1.data.len, 2 as usize);
    const perm_val = (e1.data[0] as u16) | ((e1.data[1] as u16) << 8);
    expect_eq(perm_val, 0o755 as u16);

    expect_eq(eit.next() == null, true);
});

test("3. Level 2 (-lh7- & -lhd-) with EXT_COMMON_CRC header CRC verification & EXT_FILENAME", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const n1 = writeLevel2(buf[0..], 0, LZH_METHOD_LH7, "archive_v2.bin", "\x11\x22\x33", 4096);
    const e1 = parseLzhEntry(buf[0..n1]) ?? unreachable;
    expect_eq(e1.level, LEVEL_2);
    expect_eq(e1.isDir(), false);
    expect_eq(e1.mtime_raw, 1700000123 as u32);
    expect_eq(eqStr(e1.name, "archive_v2.bin"), true);
    expect_eq(e1.data.len, 3 as usize);

    // Directory entry (-lhd-)
    const n2 = writeLevel2(buf[0..], 0, LZH_METHOD_LHD, "subdir", "", 0);
    const e2 = parseLzhEntry(buf[0..n2]) ?? unreachable;
    expect_eq(e2.level, LEVEL_2);
    expect_eq(e2.isDir(), true);
    expect_eq(eqStr(e2.name, "subdir"), true);
    expect_eq(e2.data.len, 0 as usize);
});

test("4. LzhEntryIter multi-entry archive (Level 0 + Level 1 + Level 2) terminating at 0x00 sentinel", () => {
    let arc: [512]u8 = std.mem.zeroes([512]u8);
    let pos: usize = 0;
    pos += writeLevel0(arc[0..], pos, LZH_METHOD_LH0, "a.txt", "AAA");
    pos += writeLevel1WithExts(arc[0..], pos, LZH_METHOD_LH5, "b.bin", "dir/", 0o644, "\xDE\xAD", 100);
    pos += writeLevel2(arc[0..], pos, LZH_METHOD_LHD, "empty_dir", "", 0);
    arc[pos] = 0x00; // end-of-archive sentinel
    pos += 1;

    let it = iterLzhEntries(arc[0..pos]);
    const x0 = it.next() ?? unreachable;
    expect_eq(x0.level, LEVEL_0);
    expect_eq(eqStr(x0.name, "a.txt"), true);
    expect_eq(x0.verifyDataCrc(), true);

    const x1 = it.next() ?? unreachable;
    expect_eq(x1.level, LEVEL_1);
    expect_eq(eqStr(x1.name, "b.bin"), true);
    expect_eq(eqStr(x1.dir_name, "dir/"), true);

    const x2 = it.next() ?? unreachable;
    expect_eq(x2.level, LEVEL_2);
    expect_eq(x2.isDir(), true);
    expect_eq(eqStr(x2.name, "empty_dir"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("5. Malformed LZH rejection (bad header_sum, bad Level 2 common CRC, invalid method/level, truncated)", () => {
    // 1. Invalid method & level > 2
    expect_eq(isLzhMethod("lh5--"), false);
    expect_eq(isLzhMethod("-lh5"), false);
    expect_eq(isLzhMethod("-l!5-"), false);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const n0 = writeLevel0(buf[0..], 0, LZH_METHOD_LH0, "t.txt", "DATA");

    // Corrupt header_sum
    buf[1] = buf[1] ^ 0x01;
    expect_eq(parseLzhEntry(buf[0..n0]) == null, true);
    let it_err = iterLzhEntries(buf[0..n0]);
    expect_eq(it_err.next() == null, true);
    expect_eq(it_err.hasError(), true);

    // Restore header_sum, corrupt level to 3
    buf[1] = buf[1] ^ 0x01;
    buf[20] = 3;
    expect_eq(isLzh(buf[0..n0]), false);
    expect_eq(parseLzhEntry(buf[0..n0]) == null, true);

    // Truncated Level 0 payload
    const n0b = writeLevel0(buf[0..], 0, LZH_METHOD_LH0, "t.txt", "DATA");
    expect_eq(parseLzhEntry(buf[0..(n0b - 2)]) == null, true);

    // Corrupted Level 2 EXT_COMMON_CRC
    const n2 = writeLevel2(buf[0..], 0, LZH_METHOD_LH7, "bad.bin", "\x01\x02", 10);
    buf[15] = buf[15] ^ 0xFF; // mutate mtime without updating EXT_COMMON_CRC
    expect_eq(parseLzhEntry(buf[0..n2]) == null, true);
});

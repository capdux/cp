// Source: cp/cap/test/test_jffs2_production_deep.cp (Production deep tests for cap:jffs2)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    JFFS2_MAGIC_BITMASK,
    JFFS2_OLD_MAGIC_BITMASK,
    JFFS2_EMPTY_BITMASK,
    JFFS2_COMMON_HEADER_SIZE,
    JFFS2_INODE_HEADER_SIZE,
    JFFS2_DIRENT_HEADER_SIZE,
    NODETYPE_DIRENT,
    NODETYPE_INODE,
    NODETYPE_CLEANMARKER,
    NODETYPE_PADDING,
    COMPR_NONE,
    COMPR_ZLIB,
    DT_REG,
    DT_DIR,
    jffs2Crc32,
    isJffs2,
    parseJffs2NodeHeader,
    parseJffs2Inode,
    parseJffs2Dirent,
    iterJffs2Nodes,
} from "cap:devops";

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn align4(n: usize): usize {
    const rem = n % 4;
    if (rem == 0) {
        return n;
    }
    return n + (4 - rem);
}

fn emitCleanmarker(buf: []u8, pos: usize): usize {
    writeU16Le(buf, pos, JFFS2_MAGIC_BITMASK);
    writeU16Le(buf, pos + 2, NODETYPE_CLEANMARKER);
    writeU32Le(buf, pos + 4, 12);
    writeU32Le(buf, pos + 8, jffs2Crc32(buf[pos .. pos + 8]));
    return pos + 12;
}

fn emitPadding(buf: []u8, pos: usize, totlen: usize): usize {
    writeU16Le(buf, pos, JFFS2_MAGIC_BITMASK);
    writeU16Le(buf, pos + 2, NODETYPE_PADDING);
    writeU32Le(buf, pos + 4, totlen as u32);
    writeU32Le(buf, pos + 8, jffs2Crc32(buf[pos .. pos + 8]));
    return pos + align4(totlen);
}

fn emitDirent(buf: []u8, pos: usize, pino: u32, ver: u32, ino: u32, dtype: u8, name: []const u8): usize {
    const totlen: usize = JFFS2_DIRENT_HEADER_SIZE + name.len;
    writeU16Le(buf, pos, JFFS2_MAGIC_BITMASK);
    writeU16Le(buf, pos + 2, NODETYPE_DIRENT);
    writeU32Le(buf, pos + 4, totlen as u32);
    writeU32Le(buf, pos + 8, jffs2Crc32(buf[pos .. pos + 8]));

    writeU32Le(buf, pos + 12, pino);
    writeU32Le(buf, pos + 16, ver);
    writeU32Le(buf, pos + 20, ino);
    writeU32Le(buf, pos + 24, 1700000000);
    buf[pos + 28] = name.len as u8;
    buf[pos + 29] = dtype;
    buf[pos + 30] = 0;
    buf[pos + 31] = 0;
    writeU32Le(buf, pos + 32, jffs2Crc32(buf[pos .. pos + 32]));
    copyInto(buf, pos + 40, name);
    writeU32Le(buf, pos + 36, jffs2Crc32(buf[pos + 40 .. pos + 40 + name.len]));
    return pos + align4(totlen);
}

fn emitInode(buf: []u8, pos: usize, ino: u32, ver: u32, compr: u8, payload: []const u8): usize {
    const totlen: usize = JFFS2_INODE_HEADER_SIZE + payload.len;
    writeU16Le(buf, pos, JFFS2_MAGIC_BITMASK);
    writeU16Le(buf, pos + 2, NODETYPE_INODE);
    writeU32Le(buf, pos + 4, totlen as u32);
    writeU32Le(buf, pos + 8, jffs2Crc32(buf[pos .. pos + 8]));

    writeU32Le(buf, pos + 12, ino);
    writeU32Le(buf, pos + 16, ver);
    writeU32Le(buf, pos + 20, 0x81A4); // regular 0644
    writeU16Le(buf, pos + 24, 0);
    writeU16Le(buf, pos + 26, 0);
    writeU32Le(buf, pos + 28, payload.len as u32); // isize
    writeU32Le(buf, pos + 32, 1700000001);
    writeU32Le(buf, pos + 36, 1700000002);
    writeU32Le(buf, pos + 40, 1700000003);
    writeU32Le(buf, pos + 44, 0);                  // offset
    writeU32Le(buf, pos + 48, payload.len as u32); // csize
    writeU32Le(buf, pos + 52, payload.len as u32); // dsize
    buf[pos + 56] = compr;
    buf[pos + 57] = 0;
    writeU16Le(buf, pos + 58, 0);
    copyInto(buf, pos + 68, payload);
    writeU32Le(buf, pos + 60, jffs2Crc32(buf[pos + 68 .. pos + 68 + payload.len]));
    writeU32Le(buf, pos + 64, jffs2Crc32(buf[pos .. pos + 60]));
    return pos + align4(totlen);
}

test("1. JFFS2 un-inverted CRC32 verification and protocol constants", () => {
    expect_eq(JFFS2_MAGIC_BITMASK, 0x1985 as u16);
    expect_eq(JFFS2_OLD_MAGIC_BITMASK, 0x1984 as u16);
    expect_eq(JFFS2_EMPTY_BITMASK, 0xFFFF as u16);
    expect_eq(JFFS2_COMMON_HEADER_SIZE, 12);
    expect_eq(JFFS2_INODE_HEADER_SIZE, 68);
    expect_eq(JFFS2_DIRENT_HEADER_SIZE, 40);

    // Empty input -> 0 (initial value = 0, no final XOR)
    expect_eq(jffs2Crc32(""), 0 as u32);

    // Deterministic non-zero CRC32 and bit-flip sensitivity
    const c1 = jffs2Crc32("hello-jffs2");
    const c2 = jffs2Crc32("hello-jffs3");
    expect_eq(c1 != 0, true);
    expect_eq(c1 != c2, true);
});

test("2. Full flash eraseblock streaming: Cleanmarker + Dirent + Tombstone + Inodes + Padding + 0xFFFF skip", () => {
    let flash: [512]u8 = [0xFF; 512]; // prefill with erased flash 0xFF
    let p: usize = 0;

    // 1. Cleanmarker at offset 0 (12B)
    p = emitCleanmarker(flash[0..], p);

    // 2. Dirent "config.ini" (ino=2, DT_REG) at offset 12 (40 + 10 = 50 -> padded to 52, ends at 64)
    p = emitDirent(flash[0..], p, 1, 1, 2, DT_REG, "config.ini");

    // 3. Deletion tombstone Dirent "old.log" (ino=0, DT_REG) at offset 64 (40 + 7 = 47 -> padded to 48, ends at 112)
    p = emitDirent(flash[0..], p, 1, 2, 0, DT_REG, "old.log");

    // 4. Padding node (16B) at offset 112..128
    p = emitPadding(flash[0..], p, 16);

    // Leave 16 bytes of 0xFF erased gap (128..144) to test automatic 0xFFFF skip!
    p += 16;

    // 5. Inode (ino=2, COMPR_NONE, "mode=fast") at offset 144 (68 + 9 = 77 -> padded to 80, ends at 224)
    p = emitInode(flash[0..], p, 2, 1, COMPR_NONE, "mode=fast");

    // 6. Inode (ino=3, COMPR_ZLIB, 5 bytes) at offset 224
    p = emitInode(flash[0..], p, 3, 1, COMPR_ZLIB, "zlib!");
    * = p;

    expect_eq(isJffs2(flash[0..]), true);

    let iter = iterJffs2Nodes(flash[0..]);

    const n0 = iter.next() ?? unreachable;
    expect_eq(n0.nodetype, NODETYPE_CLEANMARKER);
    expect_eq(n0.offset, 0);

    const n1 = iter.next() ?? unreachable;
    expect_eq(n1.nodetype, NODETYPE_DIRENT);
    const d1 = parseJffs2Dirent(n1) ?? unreachable;
    expect_eq(d1.ino, 2 as u32);
    expect_eq(d1.dirent_type, DT_REG);
    expect_eq(d1.isDeletion(), false);
    expect_eq(std.mem.eql(u8, d1.name, "config.ini"), true);

    const n2 = iter.next() ?? unreachable;
    const d2 = parseJffs2Dirent(n2) ?? unreachable;
    expect_eq(d2.isDeletion(), true);
    expect_eq(std.mem.eql(u8, d2.name, "old.log"), true);

    const n3 = iter.next() ?? unreachable;
    expect_eq(n3.nodetype, NODETYPE_PADDING);

    // Erased 0xFF gap at 128..144 was skipped automatically!
    const n4 = iter.next() ?? unreachable;
    expect_eq(n4.nodetype, NODETYPE_INODE);
    expect_eq(n4.offset, 144);
    const ino4 = parseJffs2Inode(n4) ?? unreachable;
    expect_eq(ino4.ino, 2 as u32);
    expect_eq(ino4.compr, COMPR_NONE);
    expect_eq(std.mem.eql(u8, ino4.data, "mode=fast"), true);

    const n5 = iter.next() ?? unreachable;
    const ino5 = parseJffs2Inode(n5) ?? unreachable;
    expect_eq(ino5.ino, 3 as u32);
    expect_eq(ino5.compr, COMPR_ZLIB);
    expect_eq(std.mem.eql(u8, ino5.data, "zlib!"), true);

    // Trailing 0xFF flash ends iteration cleanly without error
    expect_eq(iter.next() == null, true);
    expect_eq(iter.hasError(), false);
});

test("3. Dirent deletion tombstone and directory type validation", () => {
    let buf: [64]u8 = [0; 64];
    * = emitDirent(buf[0..], 0, 1, 7, 42, DT_DIR, "etc");
    const hdr = parseJffs2NodeHeader(buf[0..]) ?? unreachable;
    const de = parseJffs2Dirent(hdr) ?? unreachable;
    expect_eq(de.pino, 1 as u32);
    expect_eq(de.version, 7 as u32);
    expect_eq(de.ino, 42 as u32);
    expect_eq(de.dirent_type, DT_DIR);
    expect_eq(de.isDeletion(), false);
    expect_eq(std.mem.eql(u8, de.name, "etc"), true);
    // Parsing a dirent as inode must fail
    expect_eq(parseJffs2Inode(hdr) == null, true);
});

test("4. Corrupted CRC rejection: bad hdr_crc, bad node_crc, bad data_crc, bad name_crc", () => {
    // Inode: bad node_crc and bad data_crc
    let ibuf: [96]u8 = [0; 96];
    * = emitInode(ibuf[0..], 0, 5, 1, COMPR_NONE, "payload");

    // Flip payload byte -> bad data_crc
    ibuf[70] ^= 0x01;
    const ihdr1 = parseJffs2NodeHeader(ibuf[0..]) ?? unreachable;
    expect_eq(parseJffs2Inode(ihdr1) == null, true);
    ibuf[70] ^= 0x01; // restore

    // Flip inode metadata byte -> bad node_crc
    ibuf[20] ^= 0x01;
    const ihdr2 = parseJffs2NodeHeader(ibuf[0..]) ?? unreachable;
    expect_eq(parseJffs2Inode(ihdr2) == null, true);

    // Dirent: bad name_crc and bad node_crc
    let dbuf: [64]u8 = [0; 64];
    * = emitDirent(dbuf[0..], 0, 1, 1, 9, DT_REG, "file.bin");

    // Flip name byte -> bad name_crc
    dbuf[42] ^= 0x02;
    const dhdr1 = parseJffs2NodeHeader(dbuf[0..]) ?? unreachable;
    expect_eq(parseJffs2Dirent(dhdr1) == null, true);
    dbuf[42] ^= 0x02; // restore

    // Flip dirent metadata byte -> bad node_crc
    dbuf[20] ^= 0x01;
    const dhdr2 = parseJffs2NodeHeader(dbuf[0..]) ?? unreachable;
    expect_eq(parseJffs2Dirent(dhdr2) == null, true);
});

test("5. Malformed header & stream error detection: bad magic, bad hdr_crc, truncated totlen", () => {
    expect_eq(isJffs2("x"), false);
    expect_eq(parseJffs2NodeHeader("short") == null, true);

    // Bad magic
    let bad: [32]u8 = [0; 32];
    writeU16Le(bad[0..], 0, 0x1234);
    expect_eq(isJffs2(bad[0..]), false);
    expect_eq(parseJffs2NodeHeader(bad[0..]) == null, true);

    // Bad hdr_crc sets iter.hasError()
    * = emitCleanmarker(bad[0..], 0);
    bad[10] ^= 0xFF;
    expect_eq(parseJffs2NodeHeader(bad[0..]) == null, true);
    let it_bad = iterJffs2Nodes(bad[0..]);
    expect_eq(it_bad.next() == null, true);
    expect_eq(it_bad.hasError(), true);

    // totlen > available buffer
    writeU16Le(bad[0..], 0, JFFS2_MAGIC_BITMASK);
    writeU16Le(bad[0..], 2, NODETYPE_CLEANMARKER);
    writeU32Le(bad[0..], 4, 256);
    writeU32Le(bad[0..], 8, jffs2Crc32(bad[0..8]));
    expect_eq(parseJffs2NodeHeader(bad[0..]) == null, true);
});

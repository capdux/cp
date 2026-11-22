// Source: cp/cap/test/test_cramfs_production_deep.cp (Production deep tests for cap:cramfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CRAMFS_MAGIC,
    CRAMFS_SIGNATURE,
    CRAMFS_SUPER_SIZE,
    CRAMFS_INODE_SIZE,
    CRAMFS_CRC_OFFSET,
    FLAG_FSID_VERSION_2,
    FLAG_SORTED_DIRS,
    FLAG_HOLES,
    S_IFDIR,
    S_IFREG,
    S_IFLNK,
    cramfsCrc32,
    isCramfs,
    parseCramfsInode,
    parseCramfsSuperblock,
    iterCramfsDir,
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

fn encodeInode(buf: []u8, off: usize, mode: u16, uid: u16, size24: u32, gid: u8, namelen_words: u8, offset_words: u32) {
    writeU16Le(buf, off, mode);
    writeU16Le(buf, off + 2, uid);
    const w1: u32 = (size24 & 0x00FFFFFF) | ((gid as u32) << 24);
    const w2: u32 = ((namelen_words as u32) & 0x3F) | (offset_words << 6);
    writeU32Le(buf, off + 4, w1);
    writeU32Le(buf, off + 8, w2);
}

// Builds a synthetic CramFS image with 3 root entries:
//   1. "bin" (dir, 0755)
//   2. "hello.txt" (reg, 0644, size=13)
//   3. "sh" (symlink, 0777, size=7)
fn buildSampleCramfs(buf: []u8, flags: u32): usize {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }

    // Root directory entries start at offset 76 (offset_words = 19)
    // Entry 0: 12B inode + 4B "bin\0" = 16B (76..92)
    encodeInode(buf, 76, S_IFDIR | 0x1ED, 0, 0, 0, 1, 0);
    copyInto(buf, 88, "bin");

    // Entry 1: 12B inode + 12B "hello.txt\0\0\0" = 24B (92..116)
    encodeInode(buf, 92, S_IFREG | 0x1A4, 1000, 13, 100, 3, 40);
    copyInto(buf, 104, "hello.txt");

    // Entry 2: 12B inode + 4B "sh\0\0" = 16B (116..132)
    encodeInode(buf, 116, S_IFLNK | 0x1FF, 0, 7, 0, 1, 48);
    copyInto(buf, 128, "sh");

    const dir_size: u32 = 16 + 24 + 16; // 56 bytes
    const total_size: usize = 160; // padded image size

    // Superblock (0..76)
    writeU32Le(buf, 0, CRAMFS_MAGIC);
    writeU32Le(buf, 4, total_size as u32);
    writeU32Le(buf, 8, flags);
    writeU32Le(buf, 12, 0);
    copyInto(buf, 16, CRAMFS_SIGNATURE);
    writeU32Le(buf, 36, 1);  // edition
    writeU32Le(buf, 40, 2);  // blocks
    writeU32Le(buf, 44, 4);  // files (root + 3 entries)
    copyInto(buf, 48, "cp-rootfs");

    // Root inode at 64..76: dir, mode 0755, size = 56, offset = 76 (19 words)
    encodeInode(buf, 64, S_IFDIR | 0x1ED, 0, dir_size, 0, 0, 19);

    if ((flags & FLAG_FSID_VERSION_2) != 0) {
        const crc = cramfsCrc32(buf[0..total_size]);
        writeU32Le(buf, CRAMFS_CRC_OFFSET, crc);
    }
    return total_size;
}

test("1. CramFS v2 76B Superblock parsing, signature, flags, and IEEE CRC-32 verification", () => {
    expect_eq(CRAMFS_MAGIC, 0x28CD3D45 as u32);
    expect_eq(CRAMFS_SUPER_SIZE, 76);
    expect_eq(CRAMFS_INODE_SIZE, 12);
    expect_eq(CRAMFS_CRC_OFFSET, 32);

    let img: [256]u8 = [0; 256];
    const total = buildSampleCramfs(img[0..], FLAG_FSID_VERSION_2 | FLAG_SORTED_DIRS | FLAG_HOLES);

    expect_eq(isCramfs(img[0..total]), true);
    const sb = parseCramfsSuperblock(img[0..total]) ?? unreachable;
    expect_eq(sb.size, total as u32);
    expect_eq(sb.isVersion2(), true);
    expect_eq(sb.hasSortedDirs(), true);
    expect_eq(sb.hasHoles(), true);
    expect_eq(sb.edition, 1 as u32);
    expect_eq(sb.blocks, 2 as u32);
    expect_eq(sb.files, 4 as u32);
    expect_eq(std.mem.eql(u8, sb.nameSlice(), "cp-rootfs"), true);
    expect_eq(sb.root.isDir(), true);
    expect_eq(sb.root.dataOffset(), 76);
    expect_eq(sb.root.size, 56 as u32);
    expect_eq(sb.root.permissions(), 0x1ED as u16); // 0755
});

test("2. 12-byte bit-packed CramfsInode decoding (24-bit size, 6-bit namelen, 26-bit offset)", () => {
    let raw: [12]u8 = [0; 12];
    // Max 24-bit size = 0xFFFFFF (16MB - 1), gid = 250, namelen_words = 63 (252B), offset_words = 0x01234567
    encodeInode(raw[0..], 0, S_IFREG | 0x1A4, 501, 0x00ABCDEF, 250, 63, 0x01234567);

    const ino = parseCramfsInode(raw[0..]) ?? unreachable;
    expect_eq(ino.isRegular(), true);
    expect_eq(ino.isDir(), false);
    expect_eq(ino.isSymlink(), false);
    expect_eq(ino.permissions(), 0x1A4 as u16); // 0644
    expect_eq(ino.uid, 501 as u16);
    expect_eq(ino.gid, 250 as u8);
    expect_eq(ino.size, 0x00ABCDEF as u32);
    expect_eq(ino.namelen_words, 63 as u8);
    expect_eq(ino.nameBytesLen(), 252);
    expect_eq(ino.offset_words, 0x01234567 as u32);
    expect_eq(ino.dataOffset(), (0x01234567 as usize) * 4);
});

test("3. Zero-copy CramfsDirIter traversal and trailing NUL stripping", () => {
    let img: [256]u8 = [0; 256];
    const total = buildSampleCramfs(img[0..], FLAG_FSID_VERSION_2);
    const sb = parseCramfsSuperblock(img[0..total]) ?? unreachable;

    let iter = iterCramfsDir(img[0..total], sb.root) ?? unreachable;

    // Entry 0: "bin" (dir)
    const e0 = iter.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.name, "bin"), true);
    expect_eq(e0.inode.isDir(), true);

    // Entry 1: "hello.txt" (regular file, uid=1000, gid=100, size=13, dataOffset=160)
    const e1 = iter.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.name, "hello.txt"), true);
    expect_eq(e1.inode.isRegular(), true);
    expect_eq(e1.inode.uid, 1000 as u16);
    expect_eq(e1.inode.gid, 100 as u8);
    expect_eq(e1.inode.size, 13 as u32);
    expect_eq(e1.inode.dataOffset(), 160);

    // Entry 2: "sh" (symlink, size=7, dataOffset=192)
    const e2 = iter.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.name, "sh"), true);
    expect_eq(e2.inode.isSymlink(), true);
    expect_eq(e2.inode.size, 7 as u32);
    expect_eq(e2.inode.dataOffset(), 192);

    expect_eq(iter.next() == null, true);
});

test("4. CramFS v1 compatibility (no FLAG_FSID_VERSION_2 skips CRC check)", () => {
    let img: [256]u8 = [0; 256];
    const total = buildSampleCramfs(img[0..], 0); // v1: flags == 0
    writeU32Le(img[0..], CRAMFS_CRC_OFFSET, 0xDEADBEEF); // arbitrary CRC ignored in v1

    const sb = parseCramfsSuperblock(img[0..total]) ?? unreachable;
    expect_eq(sb.isVersion2(), false);
    expect_eq(sb.crc, 0xDEADBEEF as u32);
});

test("5. Malformed CramFS rejection: bad magic, bad signature, bad CRC32, out-of-bounds dir, truncation", () => {
    expect_eq(isCramfs("short"), false);
    expect_eq(parseCramfsInode("short") == null, true);
    expect_eq(parseCramfsSuperblock("short") == null, true);

    let img: [256]u8 = [0; 256];
    const total = buildSampleCramfs(img[0..], FLAG_FSID_VERSION_2);

    // Bad magic
    writeU32Le(img[0..], 0, 0x11223344);
    expect_eq(isCramfs(img[0..total]), false);
    expect_eq(parseCramfsSuperblock(img[0..total]) == null, true);

    // Bad signature
    * = buildSampleCramfs(img[0..], FLAG_FSID_VERSION_2);
    img[16] = 0x58; // 'X' instead of 'C'
    expect_eq(isCramfs(img[0..total]), false);
    expect_eq(parseCramfsSuperblock(img[0..total]) == null, true);

    // Corrupted CRC32 in v2
    * = buildSampleCramfs(img[0..], FLAG_FSID_VERSION_2);
    img[100] ^= 0x01;
    expect_eq(parseCramfsSuperblock(img[0..total]) == null, true);

    // Declared size > data.len
    * = buildSampleCramfs(img[0..], 0);
    writeU32Le(img[0..], 4, (total + 64) as u32);
    expect_eq(parseCramfsSuperblock(img[0..total]) == null, true);

    // iterCramfsDir rejects non-directory inode and out-of-bounds directory range
    * = buildSampleCramfs(img[0..], FLAG_FSID_VERSION_2);
    let reg_raw: [12]u8 = [0; 12];
    encodeInode(reg_raw[0..], 0, S_IFREG | 0x1A4, 0, 10, 0, 0, 19);
    const reg_ino = parseCramfsInode(reg_raw[0..]) ?? unreachable;
    expect_eq(iterCramfsDir(img[0..total], reg_ino) == null, true);

    let oob_dir_raw: [12]u8 = [0; 12];
    encodeInode(oob_dir_raw[0..], 0, S_IFDIR | 0x1ED, 0, 500, 0, 0, 19);
    const oob_dir = parseCramfsInode(oob_dir_raw[0..]) ?? unreachable;
    expect_eq(iterCramfsDir(img[0..total], oob_dir) == null, true);
});

// Source: cp/cap/test/test_minix_production_deep.cp (Production deep tests for cap:minix)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MINIX_SUPER_OFFSET,
    MINIX_BLOCK_SIZE,
    MINIX_V1_INODE_SIZE,
    MINIX_V2_INODE_SIZE,
    MINIX_V1_MAGIC,
    MINIX_V1_MAGIC2,
    MINIX_V2_MAGIC,
    MINIX_V2_MAGIC2,
    MINIX_V3_MAGIC,
    MINIX_VALID_FS,
    MINIX_ERROR_FS,
    S_IFMT,
    S_IFDIR,
    S_IFREG,
    S_IFLNK,
    MinixSuperblock,
    isMinixSuperblock,
    parseMinixSuperblock,
    MinixInode,
    parseMinixV1Inode,
    parseMinixV2Inode,
    MinixDirEntry,
    MinixDirIter,
    iterMinixDir,
} from "cap:devops";

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

fn initV1V2Sb(buf: []u8, magic: u16, ninodes: u16, nzones_v1: u16, nzones_v2: u32, state: u16) {
    putU16Le(buf, 0, ninodes);
    putU16Le(buf, 2, nzones_v1);
    putU16Le(buf, 4, 1); // imap_blocks
    putU16Le(buf, 6, 2); // zmap_blocks
    putU16Le(buf, 8, 16); // firstdatazone
    putU16Le(buf, 10, 0); // log_zone_size = 0 (zone == block)
    putU32Le(buf, 12, 268966912); // max_size
    putU16Le(buf, 16, magic);
    putU16Le(buf, 18, state);
    putU32Le(buf, 20, nzones_v2);
}

fn initV3Sb(buf: []u8, ninodes: u32, nzones: u32, block_size: u16, log_zone_size: u16) {
    putU32Le(buf, 0, ninodes);
    putU16Le(buf, 6, 2); // imap_blocks
    putU16Le(buf, 8, 4); // zmap_blocks
    putU16Le(buf, 10, 40); // firstdatazone
    putU16Le(buf, 12, log_zone_size);
    putU32Le(buf, 16, 2147483647);
    putU32Le(buf, 20, nzones);
    putU16Le(buf, 24, MINIX_V3_MAGIC);
    putU16Le(buf, 28, block_size);
}

test("1. MINIX v1 (14B/30B), v2 (30B) & v3 (60B + 4KB block) superblock parsing & offset math", () => {
    expect_eq(MINIX_SUPER_OFFSET, 1024 as usize);
    expect_eq(MINIX_BLOCK_SIZE, 1024 as u16);
    expect_eq(MINIX_V1_INODE_SIZE, 32 as usize);
    expect_eq(MINIX_V2_INODE_SIZE, 64 as usize);
    expect_eq(S_IFMT, 0xF000 as u16);

    // v1 (14B names)
    let sb1_raw: [32]u8 = std.mem.zeroes([32]u8);
    initV1V2Sb(sb1_raw[0..], MINIX_V1_MAGIC, 512, 1440, 0, MINIX_VALID_FS);
    expect_eq(isMinixSuperblock(sb1_raw[0..]), true);
    const sb1 = parseMinixSuperblock(sb1_raw[0..]) ?? unreachable;
    expect_eq(sb1.version, 1 as u8);
    expect_eq(sb1.ninodes, 512 as u32);
    expect_eq(sb1.nzones, 1440 as u32);
    expect_eq(sb1.name_len, 14 as u8);
    expect_eq(sb1.dir_entry_size, 16 as u8);
    expect_eq(sb1.zoneSizeBytes(), 1024 as u32);
    expect_eq(sb1.inodeSizeBytes(), 32 as usize);
    // (2 + 1 + 2) * 1024 = 5120
    expect_eq(sb1.inodeTableByteOffset(), 5120 as u64);
    expect_eq(sb1.zoneToByteOffset(16), 16384 as u64);
    expect_eq(sb1.isClean(), true);

    // v1 (30B names) with MINIX_ERROR_FS -> not clean
    initV1V2Sb(sb1_raw[0..], MINIX_V1_MAGIC2, 512, 1440, 0, MINIX_VALID_FS | MINIX_ERROR_FS);
    const sb1b = parseMinixSuperblock(sb1_raw[0..]) ?? unreachable;
    expect_eq(sb1b.name_len, 30 as u8);
    expect_eq(sb1b.dir_entry_size, 32 as u8);
    expect_eq(sb1b.isClean(), false);

    // v2 (30B names) uses 32-bit s_zones
    let sb2_raw: [32]u8 = std.mem.zeroes([32]u8);
    initV1V2Sb(sb2_raw[0..], MINIX_V2_MAGIC2, 2048, 0, 100000, MINIX_VALID_FS);
    const sb2 = parseMinixSuperblock(sb2_raw[0..]) ?? unreachable;
    expect_eq(sb2.version, 2 as u8);
    expect_eq(sb2.nzones, 100000 as u32);
    expect_eq(sb2.inodeSizeBytes(), 64 as usize);
    expect_eq(sb2.dir_entry_size, 32 as u8);

    // v3 (60B names, 4096B block_size, log_zone_size=1 -> 8192B zone)
    let sb3_raw: [32]u8 = std.mem.zeroes([32]u8);
    initV3Sb(sb3_raw[0..], 65536, 262144, 4096, 1);
    expect_eq(isMinixSuperblock(sb3_raw[0..]), true);
    const sb3 = parseMinixSuperblock(sb3_raw[0..]) ?? unreachable;
    expect_eq(sb3.version, 3 as u8);
    expect_eq(sb3.ninodes, 65536 as u32);
    expect_eq(sb3.nzones, 262144 as u32);
    expect_eq(sb3.block_size, 4096 as u16);
    expect_eq(sb3.name_len, 60 as u8);
    expect_eq(sb3.dir_entry_size, 64 as u8);
    expect_eq(sb3.zoneSizeBytes(), 8192 as u32);
    expect_eq(sb3.inodeSizeBytes(), 64 as usize);
    // (2 + 2 + 4) * 4096 = 32768
    expect_eq(sb3.inodeTableByteOffset(), 32768 as u64);
    expect_eq(sb3.zoneToByteOffset(40), 327680 as u64);
    expect_eq(sb3.isClean(), true);
});

test("2. 32B MINIX v1 & 64B MINIX v2/v3 unified Inode parsing", () => {
    // v1 32B regular file inode
    let raw1: [32]u8 = std.mem.zeroes([32]u8);
    putU16Le(raw1[0..], 0, S_IFREG | 0o644);
    putU16Le(raw1[0..], 2, 1000); // uid
    putU32Le(raw1[0..], 4, 4096); // size
    putU32Le(raw1[0..], 8, 1700000000); // mtime
    raw1[12] = 100; // gid
    raw1[13] = 2; // nlinks
    putU16Le(raw1[0..], 14, 20); // direct[0]
    putU16Le(raw1[0..], 26, 26); // direct[6]
    putU16Le(raw1[0..], 28, 30); // indirect
    putU16Le(raw1[0..], 30, 31); // double_indirect

    const ino1 = parseMinixV1Inode(raw1[0..]) ?? unreachable;
    expect_eq(ino1.isRegular(), true);
    expect_eq(ino1.isDir(), false);
    expect_eq(ino1.isSymlink(), false);
    expect_eq(ino1.permissions(), 0o644 as u16);
    expect_eq(ino1.uid, 1000 as u16);
    expect_eq(ino1.gid, 100 as u16);
    expect_eq(ino1.nlinks, 2 as u16);
    expect_eq(ino1.size, 4096 as u32);
    expect_eq(ino1.mtime, 1700000000 as u32);
    expect_eq(ino1.direct_zones[0], 20 as u32);
    expect_eq(ino1.direct_zones[6], 26 as u32);
    expect_eq(ino1.indirect_zone, 30 as u32);
    expect_eq(ino1.double_indirect, 31 as u32);
    expect_eq(ino1.triple_indirect, 0 as u32);

    // v2/v3 64B symlink inode
    let raw2: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(raw2[0..], 0, S_IFLNK | 0o777);
    putU16Le(raw2[0..], 2, 1); // nlinks
    putU16Le(raw2[0..], 4, 0); // uid
    putU16Le(raw2[0..], 6, 0); // gid
    putU32Le(raw2[0..], 8, 12); // size
    putU32Le(raw2[0..], 12, 1700000010); // atime
    putU32Le(raw2[0..], 16, 1700000020); // mtime
    putU32Le(raw2[0..], 20, 1700000030); // ctime
    putU32Le(raw2[0..], 24, 50000); // direct[0]
    putU32Le(raw2[0..], 60, 99999); // triple_indirect

    const ino2 = parseMinixV2Inode(raw2[0..]) ?? unreachable;
    expect_eq(ino2.isSymlink(), true);
    expect_eq(ino2.permissions(), 0o777 as u16);
    expect_eq(ino2.atime, 1700000010 as u32);
    expect_eq(ino2.mtime, 1700000020 as u32);
    expect_eq(ino2.ctime, 1700000030 as u32);
    expect_eq(ino2.direct_zones[0], 50000 as u32);
    expect_eq(ino2.triple_indirect, 99999 as u32);

    // Directory mode check
    putU16Le(raw2[0..], 0, S_IFDIR | 0o755);
    const ino_dir = parseMinixV2Inode(raw2[0..]) ?? unreachable;
    expect_eq(ino_dir.isDir(), true);
    expect_eq(ino_dir.permissions(), 0o755 as u16);
});

test("3. MINIX v1/v2 16B & 32B directory iteration (u16 ino) with inode == 0 slot skipping", () => {
    let sb_raw: [32]u8 = std.mem.zeroes([32]u8);
    initV1V2Sb(sb_raw[0..], MINIX_V1_MAGIC, 128, 512, 0, MINIX_VALID_FS);
    const sb = parseMinixSuperblock(sb_raw[0..]) ?? unreachable;

    // 4 slots * 16B = 64B (slot 1 is deleted: inode == 0)
    let dir: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(dir[0..], 0, 1);
    copyBytes(dir[0..], 2, ".");
    // slot 1 at offset 16: inode == 0 (deleted) -> skipped
    putU16Le(dir[0..], 16, 0);
    copyBytes(dir[0..], 18, "deleted");
    // slot 2 at offset 32: ".."
    putU16Le(dir[0..], 32, 1);
    copyBytes(dir[0..], 34, "..");
    // slot 3 at offset 48: full 14-char name without trailing NUL
    putU16Le(dir[0..], 48, 42);
    copyBytes(dir[0..], 50, "0123456789abcd");

    let it = iterMinixDir(sb, dir[0..]);
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.inode, 1 as u32);
    expect_eq(eqStr(e0.name, "."), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.inode, 1 as u32);
    expect_eq(eqStr(e1.name, ".."), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.inode, 42 as u32);
    expect_eq(eqStr(e2.name, "0123456789abcd"), true);

    expect_eq(it.next() == null, true);
});

test("4. MINIX v3 64B directory iteration (u32 ino > 65535) with empty slot skipping", () => {
    let sb_raw: [32]u8 = std.mem.zeroes([32]u8);
    initV3Sb(sb_raw[0..], 200000, 500000, 4096, 0);
    const sb = parseMinixSuperblock(sb_raw[0..]) ?? unreachable;

    // 3 slots * 64B = 192B
    let dir: [192]u8 = std.mem.zeroes([192]u8);
    putU32Le(dir[0..], 0, 100000);
    copyBytes(dir[0..], 4, "long_v3_filename_example.tar.gz");
    // slot 1: empty
    putU32Le(dir[0..], 64, 0);
    // slot 2: active
    putU32Le(dir[0..], 128, 150000);
    copyBytes(dir[0..], 132, "kernel.elf");

    let it = iterMinixDir(sb, dir[0..]);
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.inode, 100000 as u32);
    expect_eq(eqStr(e0.name, "long_v3_filename_example.tar.gz"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.inode, 150000 as u32);
    expect_eq(eqStr(e1.name, "kernel.elf"), true);

    expect_eq(it.next() == null, true);
});

test("5. Malformed superblock & inode rejection (bad magic, ninodes==0, non-pow2 block_size, truncated)", () => {
    // 1. Truncated superblock
    let short_sb: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(isMinixSuperblock(short_sb[0..]), false);
    expect_eq(parseMinixSuperblock(short_sb[0..]) == null, true);

    // 2. Bad magic
    let bad: [32]u8 = std.mem.zeroes([32]u8);
    initV1V2Sb(bad[0..], 0xDEAD, 128, 512, 0, MINIX_VALID_FS);
    expect_eq(isMinixSuperblock(bad[0..]), false);
    expect_eq(parseMinixSuperblock(bad[0..]) == null, true);

    // 3. ninodes == 0 or nzones == 0 or log_zone_size > 16
    initV1V2Sb(bad[0..], MINIX_V2_MAGIC, 0, 0, 1000, MINIX_VALID_FS);
    expect_eq(parseMinixSuperblock(bad[0..]) == null, true);

    initV1V2Sb(bad[0..], MINIX_V2_MAGIC, 100, 0, 0, MINIX_VALID_FS);
    expect_eq(parseMinixSuperblock(bad[0..]) == null, true);

    initV1V2Sb(bad[0..], MINIX_V1_MAGIC, 100, 1000, 0, MINIX_VALID_FS);
    putU16Le(bad[0..], 10, 17); // log_zone_size > 16
    expect_eq(parseMinixSuperblock(bad[0..]) == null, true);

    // 4. v3 invalid block_size (< 1024 or non-power-of-2)
    let bad3: [32]u8 = std.mem.zeroes([32]u8);
    initV3Sb(bad3[0..], 1000, 10000, 512, 0);
    expect_eq(parseMinixSuperblock(bad3[0..]) == null, true);
    initV3Sb(bad3[0..], 1000, 10000, 3072, 0); // non-pow2
    expect_eq(parseMinixSuperblock(bad3[0..]) == null, true);

    // 5. Truncated inodes
    expect_eq(parseMinixV1Inode(bad[0..31]) == null, true);
    expect_eq(parseMinixV2Inode(bad[0..32]) == null, true);
});

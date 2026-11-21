// Source: cp/cap/test/test_ext4_production_deep.cp (Production deep tests for cap:ext4)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EXT4_SUPER_OFFSET,
    EXT4_SUPER_SIZE,
    EXT4_SUPER_MAGIC,
    EXT4_EXTENT_MAGIC,
    EXT4_ROOT_INO,
    COMPAT_HAS_JOURNAL,
    INCOMPAT_FILETYPE,
    INCOMPAT_EXTENTS,
    INCOMPAT_64BIT,
    RO_COMPAT_SPARSE_SUPER,
    RO_COMPAT_LARGE_FILE,
    RO_COMPAT_METADATA_CSUM,
    EXT4_EXTENTS_FL,
    S_IFMT,
    S_IFREG,
    S_IFDIR,
    S_IFLNK,
    FT_UNKNOWN,
    FT_REG_FILE,
    FT_DIR,
    FT_CHRDEV,
    FT_BLKDEV,
    FT_FIFO,
    FT_SOCK,
    FT_SYMLINK,
    Ext4Superblock,
    isExt4Superblock,
    parseExt4Superblock,
    Ext4Inode,
    parseExt4Inode,
    Ext4ExtentHeader,
    parseExt4ExtentHeader,
    Ext4Extent,
    Ext4ExtentIter,
    iterExt4Extents,
    Ext4DirEntry,
    Ext4DirIter,
} from "cap:ext4";

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

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

test("1. 1024B ext4 64-bit Superblock parsing, block_size, groupCount, and volumeNameSlice", () => {
    expect_eq(EXT4_SUPER_OFFSET, 1024 as usize);
    expect_eq(EXT4_SUPER_SIZE, 1024 as usize);
    expect_eq(EXT4_SUPER_MAGIC, 0xEF53 as u16);
    expect_eq(EXT4_ROOT_INO, 2 as u32);

    let sb_raw: [1024]u8 = std.mem.zeroes([1024]u8);
    putU32Le(sb_raw[0..], 0x00, 65536);       // inodes_count
    putU32Le(sb_raw[0..], 0x04, 0x00020000);  // blocks_count_lo
    putU32Le(sb_raw[0..], 0x0C, 0x00010000);  // free_blocks_count_lo
    putU32Le(sb_raw[0..], 0x10, 60000);       // free_inodes_count
    putU32Le(sb_raw[0..], 0x14, 0);           // first_data_block (0 for 4KB blocks)
    putU32Le(sb_raw[0..], 0x18, 2);           // log_block_size = 2 -> 4096B
    putU32Le(sb_raw[0..], 0x20, 32768);       // blocks_per_group
    putU32Le(sb_raw[0..], 0x28, 8192);        // inodes_per_group
    putU16Le(sb_raw[0..], 0x38, EXT4_SUPER_MAGIC);
    putU32Le(sb_raw[0..], 0x4C, 1);           // rev_level = 1
    putU16Le(sb_raw[0..], 0x58, 256);         // inode_size = 256
    putU32Le(sb_raw[0..], 0x5C, COMPAT_HAS_JOURNAL);
    putU32Le(sb_raw[0..], 0x60, INCOMPAT_FILETYPE | INCOMPAT_EXTENTS | INCOMPAT_64BIT);
    putU32Le(sb_raw[0..], 0x64, RO_COMPAT_SPARSE_SUPER | RO_COMPAT_LARGE_FILE | RO_COMPAT_METADATA_CSUM);
    putBytes(sb_raw[0..], 0x78, "cp-rootfs");
    putU16Le(sb_raw[0..], 0xFE, 64);          // desc_size = 64
    putU32Le(sb_raw[0..], 0x150, 1);          // blocks_count_hi = 1 -> 0x1_00020000
    putU32Le(sb_raw[0..], 0x158, 0);          // free_blocks_count_hi = 0

    expect_eq(isExt4Superblock(sb_raw[0..]), true);
    const sb = parseExt4Superblock(sb_raw[0..]) ?? unreachable;
    expect_eq(sb.block_size, 4096 as u32);
    expect_eq(sb.inode_size, 256 as u16);
    expect_eq(sb.desc_size, 64 as u16);
    expect_eq(sb.hasJournal(), true);
    expect_eq(sb.hasExtents(), true);
    expect_eq(sb.is64Bit(), true);
    expect_eq(sb.blocks_count, 0x00000001_00020000 as u64);
    expect_eq(sb.groupCount(), (0x00000001_00020000 / 32768) as u64);
    expect_eq(std.mem.eql(u8, sb.volumeNameSlice(), "cp-rootfs"), true);
});

test("2. Ext4Inode 64-bit file size, 32-bit UID/GID, mode predicates, and permissions", () => {
    let raw: [128]u8 = std.mem.zeroes([128]u8);
    // Regular file 0644, uid=0x0001_03E8 (66536), gid=1000, size=5GB (0x1_40000000)
    putU16Le(raw[0..], 0x00, S_IFREG | 0x01A4); // 0644 = 0x1A4
    putU16Le(raw[0..], 0x02, 0x03E8);           // uid_lo
    putU32Le(raw[0..], 0x04, 0x40000000);       // size_lo
    putU32Le(raw[0..], 0x08, 1700000001);       // atime
    putU32Le(raw[0..], 0x0C, 1700000002);       // ctime
    putU32Le(raw[0..], 0x10, 1700000003);       // mtime
    putU16Le(raw[0..], 0x18, 1000);             // gid_lo
    putU16Le(raw[0..], 0x1A, 1);                // links_count
    putU32Le(raw[0..], 0x20, EXT4_EXTENTS_FL);
    putU32Le(raw[0..], 0x6C, 0x00000001);       // size_high = 1
    putU16Le(raw[0..], 0x78, 0x0001);           // uid_high = 1

    const ino = parseExt4Inode(raw[0..]) ?? unreachable;
    expect_eq(ino.isRegular(), true);
    expect_eq(ino.isDirectory(), false);
    expect_eq(ino.isSymlink(), false);
    expect_eq(ino.usesExtents(), true);
    expect_eq(ino.permissions(), 0x01A4 as u16);
    expect_eq(ino.uid, 0x000103E8 as u32);
    expect_eq(ino.gid, 1000 as u32);
    expect_eq(ino.size, 0x00000001_40000000 as u64);
    expect_eq(ino.links_count, 1 as u16);

    // Directory mode does not merge size_high
    putU16Le(raw[0..], 0x00, S_IFDIR | 0x01ED); // 0755
    const dir_ino = parseExt4Inode(raw[0..]) ?? unreachable;
    expect_eq(dir_ino.isDirectory(), true);
    expect_eq(dir_ino.size, 0x40000000 as u64);
});

test("3. Extent Header (0xF30A) and 48-bit physical block + Uninitialized Extent iteration", () => {
    let area: [60]u8 = std.mem.zeroes([60]u8);
    // Header: magic=0xF30A, entries=2, max=4, depth=0, gen=7
    putU16Le(area[0..], 0, EXT4_EXTENT_MAGIC);
    putU16Le(area[0..], 2, 2);
    putU16Le(area[0..], 4, 4);
    putU16Le(area[0..], 6, 0);
    putU32Le(area[0..], 8, 7);

    // Extent 0 (at 12): logical=0, len=16 (initialized), phys = 0x0002_00001000
    putU32Le(area[0..], 12, 0);
    putU16Le(area[0..], 16, 16);
    putU16Le(area[0..], 18, 0x0002);
    putU32Le(area[0..], 20, 0x00001000);

    // Extent 1 (at 24): logical=16, raw_len=32768 + 64 (uninitialized!), phys = 0x0000_00002000
    putU32Le(area[0..], 24, 16);
    putU16Le(area[0..], 28, 32768 + 64);
    putU16Le(area[0..], 30, 0x0000);
    putU32Le(area[0..], 32, 0x00002000);

    const hdr = parseExt4ExtentHeader(area[0..]) ?? unreachable;
    expect_eq(hdr.entries, 2 as u16);
    expect_eq(hdr.max_entries, 4 as u16);
    expect_eq(hdr.depth, 0 as u16);
    expect_eq(hdr.generation, 7 as u32);

    let it = iterExt4Extents(area[0..]) ?? unreachable;

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.logical_block, 0 as u32);
    expect_eq(e0.isUninitialized(), false);
    expect_eq(e0.blockLen(), 16 as u16);
    expect_eq(e0.physical_block, 0x00000002_00001000 as u64);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.logical_block, 16 as u32);
    expect_eq(e1.isUninitialized(), true);
    expect_eq(e1.blockLen(), 64 as u16);
    expect_eq(e1.physical_block, 0x00002000 as u64);

    expect_eq(it.next() == null, true);
});

test("4. Linear directory block Ext4DirIter parsing and skipping inode == 0 holes", () => {
    let dir_blk: [64]u8 = std.mem.zeroes([64]u8);
    // Entry 0 (offset 0, rec_len=12): inode=2, name=".", FT_DIR
    putU32Le(dir_blk[0..], 0, 2);
    putU16Le(dir_blk[0..], 4, 12);
    dir_blk[6] = 1;
    dir_blk[7] = FT_DIR;
    dir_blk[8] = '.';

    // Entry 1 (offset 12, rec_len=16): inode=0 (deleted hole!)
    putU32Le(dir_blk[0..], 12, 0);
    putU16Le(dir_blk[0..], 16, 16);
    dir_blk[18] = 4;
    dir_blk[19] = FT_REG_FILE;

    // Entry 2 (offset 28, rec_len=16): inode=12, name="init", FT_REG_FILE
    putU32Le(dir_blk[0..], 28, 12);
    putU16Le(dir_blk[0..], 32, 16);
    dir_blk[34] = 4;
    dir_blk[35] = FT_REG_FILE;
    putBytes(dir_blk[0..], 36, "init");

    // Entry 3 (offset 44, rec_len=20): inode=15, name="bin", FT_SYMLINK
    putU32Le(dir_blk[0..], 44, 15);
    putU16Le(dir_blk[0..], 48, 20);
    dir_blk[50] = 3;
    dir_blk[51] = FT_SYMLINK;
    putBytes(dir_blk[0..], 52, "bin");

    let it = Ext4DirIter.init(dir_blk[0..]);

    const d0 = it.next() ?? unreachable;
    expect_eq(d0.inode, 2 as u32);
    expect_eq(d0.file_type, FT_DIR);
    expect_eq(std.mem.eql(u8, d0.name, "."), true);

    // Hole at offset 12 skipped automatically
    const d1 = it.next() ?? unreachable;
    expect_eq(d1.inode, 12 as u32);
    expect_eq(d1.file_type, FT_REG_FILE);
    expect_eq(std.mem.eql(u8, d1.name, "init"), true);

    const d2 = it.next() ?? unreachable;
    expect_eq(d2.inode, 15 as u32);
    expect_eq(d2.file_type, FT_SYMLINK);
    expect_eq(std.mem.eql(u8, d2.name, "bin"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("5. Malformed superblock magic, huge log_block_size, bad extent magic, and invalid rec_len < 8 rejection", () => {
    let sb_bad: [1024]u8 = std.mem.zeroes([1024]u8);
    // Bad magic
    putU16Le(sb_bad[0..], 0x38, 0x1234);
    putU32Le(sb_bad[0..], 0x20, 32768);
    putU32Le(sb_bad[0..], 0x28, 8192);
    expect_eq(isExt4Superblock(sb_bad[0..]), false);
    expect_eq(parseExt4Superblock(sb_bad[0..]) == null, true);

    // Excessive log_block_size (> 6)
    putU16Le(sb_bad[0..], 0x38, EXT4_SUPER_MAGIC);
    putU32Le(sb_bad[0..], 0x18, 7);
    expect_eq(parseExt4Superblock(sb_bad[0..]) == null, true);

    // Zero blocks_per_group
    putU32Le(sb_bad[0..], 0x18, 2);
    putU32Le(sb_bad[0..], 0x20, 0);
    expect_eq(parseExt4Superblock(sb_bad[0..]) == null, true);

    // Truncated inode (< 128B)
    let short_ino: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(parseExt4Inode(short_ino[0..]) == null, true);

    // Bad extent magic & non-leaf depth != 0
    let bad_ext: [60]u8 = std.mem.zeroes([60]u8);
    expect_eq(parseExt4ExtentHeader(bad_ext[0..]) == null, true);
    putU16Le(bad_ext[0..], 0, EXT4_EXTENT_MAGIC);
    putU16Le(bad_ext[0..], 2, 1);
    putU16Le(bad_ext[0..], 4, 4);
    putU16Le(bad_ext[0..], 6, 1); // depth = 1 (internal index node)
    expect_eq(iterExt4Extents(bad_ext[0..]) == null, true);

    // Malformed directory entry: rec_len < 8 and unaligned rec_len (10)
    let bad_dir1: [16]u8 = std.mem.zeroes([16]u8);
    putU32Le(bad_dir1[0..], 0, 2);
    putU16Le(bad_dir1[0..], 4, 4); // rec_len = 4 < 8
    let it1 = Ext4DirIter.init(bad_dir1[0..]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.hasError(), true);

    let bad_dir2: [16]u8 = std.mem.zeroes([16]u8);
    putU32Le(bad_dir2[0..], 0, 2);
    putU16Le(bad_dir2[0..], 4, 10); // not 4-byte aligned
    let it2 = Ext4DirIter.init(bad_dir2[0..]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.hasError(), true);
});

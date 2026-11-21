// Source: cp/cap/test/test_erofs_production_deep.cp (Production deep tests for cap:erofs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EROFS_SUPER_OFFSET,
    EROFS_SUPER_SIZE,
    EROFS_SUPER_MAGIC,
    EROFS_INODE_COMPACT_SIZE,
    EROFS_INODE_EXTENDED_SIZE,
    EROFS_DIRENT_SIZE,
    FEATURE_COMPAT_SB_CHKSUM,
    FEATURE_INCOMPAT_ZERO_PADDING,
    FEATURE_INCOMPAT_COMPR_CFGS,
    FEATURE_INCOMPAT_BIG_PCLUSTER,
    FEATURE_INCOMPAT_CHUNKED_FILE,
    FEATURE_INCOMPAT_DEVICE_TABLE,
    FEATURE_INCOMPAT_ZTAILPACKING,
    FEATURE_INCOMPAT_FRAGMENTS,
    LAYOUT_FLAT_PLAIN,
    LAYOUT_FLAT_COMPRESSION_LEGACY,
    LAYOUT_FLAT_INLINE,
    LAYOUT_FLAT_COMPRESSION,
    LAYOUT_CHUNK_BASED,
    FT_UNKNOWN,
    FT_REG_FILE,
    FT_DIR,
    FT_CHRDEV,
    FT_BLKDEV,
    FT_FIFO,
    FT_SOCK,
    FT_SYMLINK,
    S_IFMT,
    S_IFREG,
    S_IFDIR,
    erofsCrc32c,
    ErofsSuperblock,
    ErofsInode,
    ErofsDirent,
    ErofsDirIter,
    isErofsSuperblock,
    parseErofsSuperblock,
    parseErofsInode,
    iterErofsDirBlock,
} from "cap:erofs";

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

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn slicesEq(a: []const u8, b: []const u8): bool {
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

fn buildSuperblock(with_csum: bool): [128]u8 {
    let sb: [128]u8 = std.mem.zeroes([128]u8);
    putU32Le(sb[0..], 0, EROFS_SUPER_MAGIC);
    let compat: u32 = 0;
    if (with_csum) {
        compat = FEATURE_COMPAT_SB_CHKSUM;
    }
    putU32Le(sb[0..], 8, compat);               // feature_compat
    sb[12] = 12;                                // blkszbits = 12 -> 4096B
    sb[13] = 0;                                 // sb_extslots
    putU16Le(sb[0..], 14, 36);                  // root_nid
    putU64Le(sb[0..], 16, 1024);                // inos
    putU64Le(sb[0..], 24, 1700000000);          // build_time
    putU32Le(sb[0..], 32, 123456);              // build_time_nsec
    putU32Le(sb[0..], 36, 4096);                // blocks
    putU32Le(sb[0..], 40, 2);                   // meta_blkaddr
    putU32Le(sb[0..], 44, 0);                   // xattr_blkaddr
    let i: usize = 0;
    while (i < 16) {
        sb[48 + i] = (0xE0 + i) as u8;          // uuid
        i += 1;
    }
    putBytes(sb[0..], 64, "cp-erofs-img");      // volume_name
    putU32Le(
        sb[0..],
        80,
        FEATURE_INCOMPAT_ZERO_PADDING | FEATURE_INCOMPAT_CHUNKED_FILE | FEATURE_INCOMPAT_FRAGMENTS,
    );

    if (with_csum) {
        const crc = erofsCrc32c(sb[0..]);
        putU32Le(sb[0..], 4, crc);
    }
    return sb;
}

test("1. 128B EROFS Superblock parsing, blockSize, nidToOffset, feature flags, and volumeNameSlice", () => {
    expect_eq(EROFS_SUPER_OFFSET, 1024 as usize);
    expect_eq(EROFS_SUPER_SIZE, 128 as usize);
    expect_eq(EROFS_SUPER_MAGIC, 0xE0F5E1E2 as u32);
    expect_eq(EROFS_INODE_COMPACT_SIZE, 32 as usize);
    expect_eq(EROFS_INODE_EXTENDED_SIZE, 64 as usize);
    expect_eq(EROFS_DIRENT_SIZE, 12 as usize);

    const sb_raw = buildSuperblock(false);
    expect_eq(isErofsSuperblock(sb_raw[0..]), true);

    if (sb := parseErofsSuperblock(sb_raw[0..])) {
        expect_eq(sb.blkszbits, 12 as u8);
        expect_eq(sb.blockSize(), 4096 as u32);
        expect_eq(sb.root_nid, 36 as u16);
        expect_eq(sb.inos, 1024 as u64);
        expect_eq(sb.build_time, 1700000000 as u64);
        expect_eq(sb.build_time_nsec, 123456 as u32);
        expect_eq(sb.blocks, 4096 as u32);
        expect_eq(sb.meta_blkaddr, 2 as u32);
        expect_eq(sb.hasChecksum(), false);
        expect_eq(sb.hasChunkedFile(), true);
        expect_eq(sb.hasFragments(), true);
        expect_eq(sb.uuid[0], 0xE0 as u8);
        expect_eq(sb.uuid[15], 0xEF as u8);
        expect_eq(slicesEq(sb.volumeNameSlice(), "cp-erofs-img"), true);
        // meta_blkaddr(2) * 4096 + root_nid(36) * 32 = 8192 + 1152 = 9344
        expect_eq(sb.nidToOffset(sb.root_nid as u64), 9344 as u64);
    } else {
        expect_eq(true, false);
    }
});

test("2. FEATURE_COMPAT_SB_CHKSUM CRC32C verification (valid and corrupted)", () => {
    let sb_csum = buildSuperblock(true);
    if (sb := parseErofsSuperblock(sb_csum[0..])) {
        expect_eq(sb.hasChecksum(), true);
        expect_eq(sb.checksum, erofsCrc32c(sb_csum[0..]));
    } else {
        expect_eq(true, false);
    }

    // Flip 1 bit in volume_name -> CRC32C mismatch must reject
    sb_csum[64] ^= 0x01;
    if (_ := parseErofsSuperblock(sb_csum[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});

test("3. Unified 32B Compact and 64B Extended Inode parsing and data_layout predicates", () => {
    // 3a. 32B Compact Inode (version=0, LAYOUT_FLAT_INLINE, regular file)
    let compact: [32]u8 = std.mem.zeroes([32]u8);
    const fmt_compact: u16 = (0 as u16) | ((LAYOUT_FLAT_INLINE as u16) << 1);
    putU16Le(compact[0..], 0, fmt_compact);
    putU16Le(compact[0..], 2, 2);                 // xattr_icount
    putU16Le(compact[0..], 4, S_IFREG | 0x01A4);  // 0100644
    putU16Le(compact[0..], 6, 1);                 // nlink
    putU32Le(compact[0..], 8, 512);               // size
    putU32Le(compact[0..], 16, 10);               // raw_blkaddr
    putU32Le(compact[0..], 20, 42);               // ino
    putU16Le(compact[0..], 24, 1000);             // uid
    putU16Le(compact[0..], 26, 1000);             // gid

    if (ino := parseErofsInode(compact[0..])) {
        expect_eq(ino.is_extended, false);
        expect_eq(ino.data_layout, LAYOUT_FLAT_INLINE);
        expect_eq(ino.isInline(), true);
        expect_eq(ino.isCompressed(), false);
        expect_eq(ino.isRegular(), true);
        expect_eq(ino.isDirectory(), false);
        expect_eq(ino.xattr_icount, 2 as u16);
        expect_eq(ino.nlink, 1 as u32);
        expect_eq(ino.size, 512 as u64);
        expect_eq(ino.raw_blkaddr, 10 as u32);
        expect_eq(ino.ino, 42 as u32);
        expect_eq(ino.uid, 1000 as u32);
        expect_eq(ino.gid, 1000 as u32);
        expect_eq(ino.mtime, 0 as u64);
    } else {
        expect_eq(true, false);
    }

    // 3b. 64B Extended Inode (version=1, LAYOUT_FLAT_COMPRESSION, directory)
    let ext: [64]u8 = std.mem.zeroes([64]u8);
    const fmt_ext: u16 = (1 as u16) | ((LAYOUT_FLAT_COMPRESSION as u16) << 1);
    putU16Le(ext[0..], 0, fmt_ext);
    putU16Le(ext[0..], 2, 0);                     // xattr_icount
    putU16Le(ext[0..], 4, S_IFDIR | 0x01ED);      // 0040755
    putU64Le(ext[0..], 8, 0x0000000100001000);    // 64-bit size
    putU32Le(ext[0..], 16, 200);                  // raw_blkaddr
    putU32Le(ext[0..], 20, 777);                  // ino
    putU32Le(ext[0..], 24, 65534);                // uid
    putU32Le(ext[0..], 28, 65534);                // gid
    putU64Le(ext[0..], 32, 1712345678);           // mtime
    putU32Le(ext[0..], 44, 12);                   // nlink

    if (ino := parseErofsInode(ext[0..])) {
        expect_eq(ino.is_extended, true);
        expect_eq(ino.data_layout, LAYOUT_FLAT_COMPRESSION);
        expect_eq(ino.isCompressed(), true);
        expect_eq(ino.isInline(), false);
        expect_eq(ino.isRegular(), false);
        expect_eq(ino.isDirectory(), true);
        expect_eq(ino.size, 0x0000000100001000 as u64);
        expect_eq(ino.uid, 65534 as u32);
        expect_eq(ino.gid, 65534 as u32);
        expect_eq(ino.mtime, 1712345678 as u64);
        expect_eq(ino.nlink, 12 as u32);
    } else {
        expect_eq(true, false);
    }

    // 3c. Extended inode truncated to 32B must return null
    if (_ := parseErofsInode(ext[0..32])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});

test("4. EROFS directory block zero-copy iteration ('.', '..', and regular/symlink entries)", () => {
    // 4 entries -> dirent table occupies 4 * 12 = 48 bytes (nameoff0 = 48)
    let blk: [128]u8 = std.mem.zeroes([128]u8);

    // Entry 0: "." at offset 48 (len 1)
    putU64Le(blk[0..], 0, 36);
    putU16Le(blk[0..], 8, 48);
    blk[10] = FT_DIR;

    // Entry 1: ".." at offset 49 (len 2)
    putU64Le(blk[0..], 12, 36);
    putU16Le(blk[0..], 20, 49);
    blk[22] = FT_DIR;

    // Entry 2: "hello.txt" at offset 51 (len 9)
    putU64Le(blk[0..], 24, 100);
    putU16Le(blk[0..], 32, 51);
    blk[34] = FT_REG_FILE;

    // Entry 3: "link.so" at offset 60 (len 7, followed by trailing NULs to end of block)
    putU64Le(blk[0..], 36, 200);
    putU16Le(blk[0..], 44, 60);
    blk[46] = FT_SYMLINK;

    putBytes(blk[0..], 48, ".");
    putBytes(blk[0..], 49, "..");
    putBytes(blk[0..], 51, "hello.txt");
    putBytes(blk[0..], 60, "link.so");

    if (iter_init := iterErofsDirBlock(blk[0..])) {
        let iter = iter_init;
        if (e0 := iter.next()) {
            expect_eq(e0.nid, 36 as u64);
            expect_eq(e0.file_type, FT_DIR);
            expect_eq(slicesEq(e0.name, "."), true);
        } else {
            expect_eq(true, false);
        }

        if (e1 := iter.next()) {
            expect_eq(e1.nid, 36 as u64);
            expect_eq(e1.file_type, FT_DIR);
            expect_eq(slicesEq(e1.name, ".."), true);
        } else {
            expect_eq(true, false);
        }

        if (e2 := iter.next()) {
            expect_eq(e2.nid, 100 as u64);
            expect_eq(e2.file_type, FT_REG_FILE);
            expect_eq(slicesEq(e2.name, "hello.txt"), true);
        } else {
            expect_eq(true, false);
        }

        if (e3 := iter.next()) {
            expect_eq(e3.nid, 200 as u64);
            expect_eq(e3.file_type, FT_SYMLINK);
            expect_eq(slicesEq(e3.name, "link.so"), true);
        } else {
            expect_eq(true, false);
        }

        // Exhausted
        if (_ := iter.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }

    // Sanity check remaining constants
    expect_eq(FEATURE_INCOMPAT_COMPR_CFGS, 0x0002 as u32);
    expect_eq(FEATURE_INCOMPAT_BIG_PCLUSTER, 0x0002 as u32);
    expect_eq(FEATURE_INCOMPAT_DEVICE_TABLE, 0x0008 as u32);
    expect_eq(FEATURE_INCOMPAT_ZTAILPACKING, 0x0010 as u32);
    expect_eq(LAYOUT_FLAT_PLAIN, 0 as u8);
    expect_eq(LAYOUT_FLAT_COMPRESSION_LEGACY, 1 as u8);
    expect_eq(LAYOUT_CHUNK_BASED, 4 as u8);
    expect_eq(FT_UNKNOWN, 0 as u8);
    expect_eq(FT_CHRDEV, 3 as u8);
    expect_eq(FT_BLKDEV, 4 as u8);
    expect_eq(FT_FIFO, 5 as u8);
    expect_eq(FT_SOCK, 6 as u8);
    expect_eq(S_IFMT, 0xF000 as u16);
});

test("5. Malformed input rejection: bad magic, invalid blkszbits, non-multiple-of-12 or out-of-bounds dir nameoff", () => {
    // 5a. Wrong magic
    let bad_magic = buildSuperblock(false);
    bad_magic[0] = 0x00;
    expect_eq(isErofsSuperblock(bad_magic[0..]), false);
    if (_ := parseErofsSuperblock(bad_magic[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Invalid blkszbits (< 9 or > 16)
    let bad_blk_lo = buildSuperblock(false);
    bad_blk_lo[12] = 8;
    if (_ := parseErofsSuperblock(bad_blk_lo[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let bad_blk_hi = buildSuperblock(false);
    bad_blk_hi[12] = 17;
    if (_ := parseErofsSuperblock(bad_blk_hi[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Truncated superblock and inode
    const ok_sb = buildSuperblock(false);
    expect_eq(isErofsSuperblock(ok_sb[0..127]), false);
    if (_ := parseErofsSuperblock(ok_sb[0..64])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let short_ino: [31]u8 = std.mem.zeroes([31]u8);
    if (_ := parseErofsInode(short_ino[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Directory block with nameoff not a multiple of 12, or zero, or out of bounds
    let bad_dir: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(bad_dir[0..], 8, 13); // not multiple of 12
    if (_ := iterErofsDirBlock(bad_dir[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    putU16Le(bad_dir[0..], 8, 0); // zero
    if (_ := iterErofsDirBlock(bad_dir[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    putU16Le(bad_dir[0..], 8, 72); // > block.len (64)
    if (_ := iterErofsDirBlock(bad_dir[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5e. Directory block with decreasing nameoff on second entry terminates safely
    let bad_order: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(bad_order[0..], 8, 24);  // entry 0: nameoff = 24
    putU16Le(bad_order[0..], 20, 20); // entry 1: nameoff = 20 (< 24)
    if (it0 := iterErofsDirBlock(bad_order[0..])) {
        let it = it0;
        if (_ := it.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }
});
